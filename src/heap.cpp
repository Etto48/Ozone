#include "include/heap.h"

namespace interrupt
{
	struct context_t
    {
        uint64_t rax, rbx, rcx, rdx,
            rsi, rdi,
            rbp,
            r8, r9, r10, r11, r12, r13, r14, r15,
            fs, gs,
            int_num, int_info,
            rip, cs, rflags, rsp, ss;
    } __attribute__((packed));
};

namespace multitasking
{
	extern void abort(const char *msg, interrupt::context_t* context= nullptr);
};

// Lo heap è composto da zone di memoria libere e occupate. Ogni zona di memoria
// è identificata dal suo indirizzo di partenza e dalla sua size.
// Ogni zona di memoria contiene, nei primi byte, un descrittore di zona di
// memoria (di tipo memory_descriptor_t) con un campo "size" (size in byte
// della zona, escluso il descrittore stesso) e un campo "next", significativo
// solo se la zona è libera, nel qual caso il campo punta alla prossima zona
// libera. Si viene quindi a creare una lista delle zone libere, la cui testa
// è puntata dalla variabile "freemem" (allocata staticamente). La lista è
// mantenuta ordinata in base agli indirizzi di partenza delle zone libere.

heap system_heap((void *)&memory::stack_bottom, 1024 * 1024 * 2); //2MiB half of available space

// rende libera la zona di memoria puntata da "indirizzo" e grande "quanti"
// byte, preoccupandosi di creare il descrittore della zona e, se possibile, di
// unificare la zona con eventuali zone libere contigue in memoria.  La
// riunificazione è importante per evitare il problema della "falsa
// frammentazione", in cui si vengono a creare tante zone libere, contigue, ma
// non utilizzabili per allocazioni più grandi della size di ciascuna di
// esse.
// "internal_free" può essere usata anche in fase di inizializzazione, per
// definire le zone di memoria fisica che verranno utilizzate dall'allocatore
void heap::internal_free(void *indirizzo, size_t quanti)
{

	// non è un errore invocare internal_free con "quanti" uguale a 0
	if (quanti == 0)
		return;

	// il caso "indirizzo == 0", invece, va escluso, in quanto l'indirizzo
	// 0 viene usato per codificare il puntatore nullo
	if (indirizzo == nullptr)
		multitasking::abort("wrong heap initialization"); //panic("internal_free(0)");

	// la zona va inserita nella lista delle zone libere, ordinata per
	// indirizzo di partenza (tale ordinamento serve a semplificare la
	// successiva operazione di riunificazione)
	memory_descriptor_t *prec = nullptr, *scorri = freemem;
	while (scorri != nullptr && scorri < indirizzo)
	{
		prec = scorri;
		scorri = scorri->next;
	}
	// assert(scorri == 0 || scorri >= indirizzo)

	// in alcuni casi siamo in grado di riconoscere l'errore di doppia
	// free: "indirizzo" non deve essere l'indirizzo di partenza di una
	// zona gia' libera
	if (scorri == indirizzo)
	{
		//flog(LOG_ERR, "indirizzo = 0x%x", indirizzo);
		//panic("double free");
		multitasking::abort("double free");
	}
	// assert(scorri == 0 || scorri > indirizzo)

	// verifichiamo che la zona possa essere unificata alla zona che la
	// precede.  Ciò è possibile se tale zona esiste e il suo ultimo byte
	// è contiguo al primo byte della nuova zona
	if (prec != nullptr && (uint8_t *)(prec + 1) + prec->size == indirizzo)
	{

		// controlliamo se la zona è unificabile anche alla eventuale
		// zona che la segue
		if (scorri != nullptr && (uint8_t *)indirizzo + quanti == (uint8_t *)scorri)
		{

			// in questo caso le tre zone diventano una sola, di
			// size pari alla somma delle tre, più il
			// descrittore della zona puntata da scorri (che ormai
			// non serve più)
			prec->size += quanti + sizeof(memory_descriptor_t) + scorri->size;
			prec->next = scorri->next;
		}
		else
		{

			// unificazione con la zona precedente: non creiamo una
			// nuova zona, ma ci limitiamo ad aumentare la
			// size di quella precedente
			prec->size += quanti;
		}
	}
	else if (scorri != nullptr && (uint8_t *)indirizzo + quanti == (uint8_t *)scorri)
	{

		// la zona non è unificabile con quella che la precede, ma è
		// unificabile con quella che la segue. L'unificazione deve
		// essere eseguita con cautela, per evitare di sovrascrivere il
		// descrittore della zona che segue prima di averne letto il
		// contenuto

		// salviamo il descrittore della zona seguente in un posto
		// sicuro
		memory_descriptor_t salva = *scorri;

		// allochiamo il nuovo descrittore all'inizio della nuova zona
		// (se quanti < sizeof(memory_descriptor_t), tale descrittore va a
		// sovrapporsi parzialmente al descrittore puntato da scorri)
		memory_descriptor_t *nuovo = reinterpret_cast<memory_descriptor_t *>(indirizzo);

		// quindi copiamo il descrittore della zona seguente nel nuovo
		// descrittore. Il campo next del nuovo descrittore è
		// automaticamente corretto, mentre il campo size va
		// aumentato di "quanti"
		*nuovo = salva;
		nuovo->size += quanti;

		// infine, inseriamo "nuovo" in lista al posto di "scorri"
		if (prec != nullptr)
			prec->next = nuovo;
		else
			freemem = nuovo;
	}
	else if (quanti >= sizeof(memory_descriptor_t))
	{

		// la zona non può essere unificata con nessun'altra.  Non
		// possiamo, però, inserirla nella lista se la sua size
		// non è tale da contenere il descrittore (nel qual caso, la
		// zona viene ignorata)

		memory_descriptor_t *nuovo = reinterpret_cast<memory_descriptor_t *>(indirizzo);
		nuovo->size = quanti - sizeof(memory_descriptor_t);

		// inseriamo "nuovo" in lista, tra "prec" e "scorri"
		nuovo->next = scorri;
		if (prec != nullptr)
			prec->next = nuovo;
		else
			freemem = nuovo;
	}
}

heap::heap(void *address, size_t size)
{
	internal_free(address, size);
}

// funzione di allocazione: cerca la prima zona di memoria libera di size
// almeno pari a "quanti" byte e ne restituisce l'indirizzo di partenza.
// Se la zona trovata è sufficientemente più grande di "quanti" byte divide la zona
// in due: una grande "quanti" byte, che viene occupata, e una contenente i rimanenti byte
// a formare una nuova zona libera, con un nuovo descrittore.
void *heap::malloc(size_t size)
{
	// per motivi di efficienza conviene allocare sempre multipli della
	// size della linea (4 byte su 32bit, 8 su 64 bit)
	// (in modo che le strutture dati restino memory::alignte alla linea)
	// Come ulteriore semplificazione, però, conviene memory::alignre alla
	// size del descrittore, che occupa due linee.
	size_t quanti = (size_t)memory::align((void *)size, sizeof(memory_descriptor_t));
	// allochiamo "quanti" byte invece dei "dim" richiesti

	// per prima cosa cerchiamo una zona di size sufficiente
	memory_descriptor_t *prec = nullptr, *scorri = freemem;
	while (scorri != nullptr && scorri->size < quanti)
	{
		prec = scorri;
		scorri = scorri->next;
	}
	// assert(scorri == 0 || scorri->size >= quanti);

	void *p = nullptr;
	if (scorri != nullptr)
	{
		p = scorri + 1; // puntatore al primo byte dopo il descrittore
						// (coincide con l'indirizzo iniziale delle zona
						// di memoria)

		// per poter dividere in due la zona trovata, la parte
		// rimanente, dopo aver occupato "quanti" byte, deve poter contenere
		// almeno il descrittore più sizeof(memory_descriptor_t) byte (minima size
		// allocabile)
		if (scorri->size - quanti >= 2 * sizeof(memory_descriptor_t))
		{

			// il nuovo descrittore verrà scritto nei primi byte
			// della zona da creare (quindi, "quanti" byte dopo "p")
			void *pnuovo = static_cast<uint8_t *>(p) + quanti;
			memory_descriptor_t *nuovo = static_cast<memory_descriptor_t *>(pnuovo);

			// aggiustiamo le dimensioni della vecchia e della nuova zona
			nuovo->size = scorri->size - quanti - sizeof(memory_descriptor_t);
			scorri->size = quanti;

			// infine, inseriamo "nuovo" nella lista delle zone libere,
			// al posto precedentemente occupato da "scorri"
			nuovo->next = scorri->next;
			if (prec != nullptr)
				prec->next = nuovo;
			else
				freemem = nuovo;
		}
		else
		{

			// se la zona non è sufficientemente grande per essere
			// divisa in due la occupiamo tutta, rimuovendola
			// dalla lista delle zone libere
			if (prec != nullptr)
				prec->next = scorri->next;
			else
				freemem = scorri->next;
		}

		// a scopo di debug, inseriamo un valore particolare nel campo
		// next (altrimenti inutilizzato) del descrittore. Se tutto
		// funziona correttamente ci aspettiamo di ritrovare lo stesso
		// valore quando quando la zona verrà successivamente
		// freeta. (Notare che il valore non e' memory::alignto a
		// sizeof(long) byte, quindi un valido indirizzo di descrittore
		// non può assumere per caso questo valore).
		scorri->next = reinterpret_cast<memory_descriptor_t *>(0xdeadbeef);
	}

	// restituiamo l'indirizzo della zona allocata (nullptr se non è stato
	// possibile allocare).  NOTA: il descrittore della zona si trova nei
	// byte immediatamente precedenti l'indirizzo "p".  Questo è
	// importante, perché ci permette di recuperare il descrittore dato
	// "p" e, tramite il descrittore, la size della zona occupata
	// (tale size può essere diversa da quella richiesta, quindi
	// è ignota al chiamante della funzione).
	memory::memset(p, 0, size);
	return p;
}

void *heap::malloc(size_t dim, size_t a)
{
	uint64_t alignment = *reinterpret_cast<uint64_t *>(&a);

	if (!alignment || (alignment & (alignment - 1)))
		return nullptr;

	dim = (size_t)memory::align((void *)dim, sizeof(memory_descriptor_t));

	size_t newdim = dim + alignment + sizeof(memory_descriptor_t);
	if (newdim < dim) // overflow
		return nullptr;

	void *p = malloc(newdim);
	if (p == nullptr)
		return p;

	// calcoliamo il primo indirizzo successivo a p e memory::alignto ad
	// 'alignment'
	uint64_t m1 = reinterpret_cast<uint64_t>(p);
	uint64_t m2 = (m1 + alignment - 1) & ~(alignment - 1);
	memory_descriptor_t *d = reinterpret_cast<memory_descriptor_t *>(p) - 1;
	if (m2 != m1)
	{
		// dobbiamo freere la prima parte e spostare p
		size_t allocdim = d->size,
			   skipdim = m2 - m1 - sizeof(memory_descriptor_t);
		d->size = skipdim;
		free(p);
		p = reinterpret_cast<void *>(m2);
		d = reinterpret_cast<memory_descriptor_t *>(p) - 1;
		d->size = allocdim - skipdim - sizeof(memory_descriptor_t);
		d->next = reinterpret_cast<memory_descriptor_t *>(0xdeadbeef);
	}
	if (d->size - dim >= 2 * sizeof(memory_descriptor_t))
	{
		internal_free(reinterpret_cast<void *>(m2 + dim), d->size - dim);
		d->size = dim;
	}
	return p;
}

size_t heap::available()
{
	size_t tot = 0;
	for (memory_descriptor_t *scan = freemem; scan; scan = scan->next)
		tot += scan->size;
	return tot;
}

// "free" deve essere usata solo con puntatori restituiti da "alloca", e rende
// nuovamente libera la zona di memoria di indirizzo iniziale "p".
void heap::free(void *address)
{

	// e' normalmente ammesso invocare "free" su un puntatore nullo.
	// In questo caso, la funzione non deve fare niente.
	if (address == 0)
		return;

	// recuperiamo l'indirizzo del descrittore della zona
	memory_descriptor_t *des = reinterpret_cast<memory_descriptor_t *>(address) - 1;

	// se non troviamo questo valore, vuol dire che un qualche errore grave
	// è stato commesso (free su un puntatore non restituito da malloc,
	// doppio free di un puntatore, sovrascrittura del valore per accesso
	// al di fuori di un array, ...)
	if (des->next != reinterpret_cast<memory_descriptor_t *>(0xdeadbeef))
		//panic("free() errata");
		multitasking::abort("wrong delete");

	// la zona viene liberata tramite la funzione "internal_free", che ha
	// bisogno dell'indirizzo di partenza e della size della zona
	// comprensiva del suo descrittore
	internal_free(des, des->size + sizeof(memory_descriptor_t));
}