#include "include/modules.h"

namespace modules
{
	// oggetto da usare con 'map' per il caricamento in memoria virtuale dei
	// segmenti ELF dei moduli utente e I/O
	struct copy_segment
	{
		// Il segmento si trova in memoria agli indirizzi (fisici) [beg, end)
		// e deve essere visibile in memoria virtuale a partire dall'indirizzo
		// virt_beg. Il segmento verrà copiato (una pagina alla volta) in
		// frame liberi di M2. La memoria precedentemente occupata dal modulo
		// sarà riutilizzata per lo heap di sistema.
		uint64_t mod_beg;
		uint64_t mod_end;
		uint64_t virt_beg;

		// funzione chiamata da map. Deve restituire l'indirizzo fisico
		// da far corrispondere all'indirizzo virtuale 'v'.
		void *operator()(void *, bool);
	};

	void *copy_segment::operator()(void *vv, bool big_pages)
	{
		uint64_t v = (uint64_t)vv;
		// allochiamo un frame libero in cui copiare la pagina
		uint64_t dst = (uint64_t)paging::frame_alloc();
		if (dst == 0)
			return 0;

		// offset della pagina all'interno del segmento
		uint64_t offset = v - virt_beg;
		// indirizzo della pagina all'interno del modulo
		uint64_t src = mod_beg + offset;

		// il segmento in memoria può essere più grande di quello nel modulo.
		// La parte eccedente deve essere azzerata.
		uint64_t tocopy = 0x1000;
		if (src > mod_end)
			tocopy = 0;
		else if (mod_end - src < 0x1000)
			tocopy = mod_end - src;
		if (tocopy > 0)
			memory::memcpy(reinterpret_cast<void *>(dst), reinterpret_cast<void *>(src), tocopy);
		if (tocopy < 0x1000)
			memory::memset(reinterpret_cast<void *>(dst + tocopy), 0, 0x1000 - tocopy);
		return (void *)dst;
	}

	// carica un modulo in M2 e lo mappa al suo indirizzo virtuale, aggiungendo
	// heap_size byte di heap dopo l'ultimo indirizzo virtuale usato.
	//
	// 'flags' dovrebbe essere BIT_US oppure zero.
	//uint64_t load_address = 0xffff800000000000;
	void (*load_module(multiboot_module_t *mod, paging::page_table_t *root_tab, interrupt::privilege_level_t privilege_level, multitasking::mapping_history_t *&mapping_history))()
	{
		mapping_history = nullptr;
		// puntatore all'intestazione ELF
		Elf64_Ehdr *elf_h = reinterpret_cast<Elf64_Ehdr *>(mod->mod_start);
		// indirizzo fisico della tabella dei segmenti
		uint64_t ph_addr = mod->mod_start + elf_h->e_phoff;
		// ultimo indirizzo virtuale usato
		uint64_t last_vaddr = 0;

		// esaminiamo tutta la tabella dei segmenti
		for (int i = 0; i < elf_h->e_phnum; i++)
		{
			Elf64_Phdr *elf_ph = reinterpret_cast<Elf64_Phdr *>(ph_addr);

			// ci interessano solo i segmenti di tipo PT_LOAD
			if (elf_ph->p_type != PT_LOAD)
				continue;

			// i byte che si trovano ora in memoria agli indirizzi (fisici)
			// [mod_beg, mod_end) devono diventare visibili nell'intervallo
			// di indirizzi virtuali [virt_beg, virt_end).
			uint64_t virt_beg = elf_ph->p_vaddr,
					 virt_end = virt_beg + elf_ph->p_memsz;
			uint64_t mod_beg = mod->mod_start + elf_ph->p_offset,
					 mod_end = mod_beg + elf_ph->p_filesz;

			// se necessario, allineiamo alla pagina gli indirizzi di
			// partenza e di fine
			uint64_t page_offset = virt_beg & (0x1000 - 1);
			virt_beg -= page_offset;
			mod_beg -= page_offset;
			virt_end = (uint64_t)memory::align((void *)virt_end, 0x1000);

			// aggiorniamo l'ultimo indirizzo virtuale usato
			if (virt_end > last_vaddr)
				last_vaddr = virt_end;

			// settiamo BIT_RW nella traduzione solo se il segmento è
			// scrivibile
			uint16_t flags = privilege_level == interrupt::privilege_level_t::user ? paging::flags::USER : 0;

			if (elf_ph->p_flags & PF_W)
				flags |= paging::flags::RW;

			// mappiamo il segmento
			//virt_beg += load_address;
			//virt_end += load_address;
			if (paging::map(
					(void *)(virt_beg),
					(virt_end - virt_beg) / 0x1000,
					flags,
					root_tab,
					copy_segment{mod_beg, mod_end, virt_beg}) != (void *)0xFFFFFFFFFFFFFFFF)
			{
				//printf("\nMAP ERROR %p",map_ret);
				return nullptr;
			};
			auto new_map = (multitasking::mapping_history_t *)system_heap.malloc(sizeof(multitasking::mapping_history_t));
			new_map->next = mapping_history;
			new_map->npages = (virt_end - virt_beg) / 0x1000;
			new_map->starting_address = (void *)virt_beg;
			new_map->flags = flags;

			mapping_history = new_map;

			//flog(LOG_INFO, " - segmento %s %s mappato a [%p, %p)",
			//		(flags & BIT_US) ? "utente " : "sistema",
			//		(flags & BIT_RW) ? "read/write" : "read-only ",
			//		virt_beg, virt_end);

			// passiamo alla prossima entrata della tabella dei segmenti
			ph_addr += elf_h->e_phentsize;
		}
		// dopo aver mappato tutti i segmenti, mappiamo lo spazio destinato
		// allo heap del modulo. I frame corrispondenti verranno allocati da
		// alloca_frame()
		//if (map(root_tab,
		//         last_vaddr,
		//	 last_vaddr + heap_size,
		//	 flags | BIT_RW,
		//	 [](vaddr) { return alloca_frame(); }) != last_vaddr + heap_size)
		//	return 0;
		//flog(LOG_INFO, " - heap:                                 [%p, %p)",
		//			last_vaddr, last_vaddr + heap_size);
		//flog(LOG_INFO, " - entry point: %p", elf_h->e_entry);
		//debug::log(debug::level::inf,"Entry point %p",(void (*)())(elf_h->e_entry+load_address));
		debug::log(debug::level::inf,"Entry point %p",(void (*)())(elf_h->e_entry));
		//return (void (*)())(elf_h->e_entry+load_address);
		return (void (*)())(elf_h->e_entry);
	}
};