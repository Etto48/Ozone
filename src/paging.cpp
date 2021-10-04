#include "include/paging.h"

namespace paging
{
    void page_table_t::set_entry(uint16_t index, void *child, uint16_t flags)
    {
        auto old_data = data[index];
        flags = 0xfff & flags;
        uint64_t final_data = (uint64_t)child & ((0x0000fffffffff000));
        final_data |= flags;
        if (index < 512)
        {
            auto frame_index = get_frame_index(this);
            if ((final_data & flags::PRESENT) && !(old_data & flags::PRESENT))
            {
                frame_descriptors[frame_index].present_entries++;
            }
            else if (!(final_data & flags::PRESENT) && (old_data & flags::PRESENT))
            {
                frame_descriptors[frame_index].present_entries--;
            }
            this->data[index] = final_data;
        }
    }
    void page_table_t::copy_from(const page_table_t &source_table, uint16_t from, uint16_t count)
    {
        for (uint32_t i = from; i < from + count; i++)
        {
            set_entry(i, (void *)source_table.data[i], (uint16_t)source_table.data[i]);
        }
    }
    bool page_table_t::is_present(uint16_t index)
    {
        if (index < 512)
        {
            return data[index] & flags::PRESENT;
        }
        return false;
    }
    page_table_t *page_table_t::operator[](uint16_t index)
    {
        if (index < 512)
            return (page_table_t *)(data[index] & (~0xfff));
        else
            return nullptr;
    }

    frame_descriptor_t frame_descriptors[FRAME_COUNT];
    uint64_t first_free_frame_index = FRAME_COUNT;
    uint64_t last_memory_address = 0xffffffffffffffff;
    uint64_t system_memory = FRAME_COUNT * 0x1000;
    uint64_t kernel_frames = 0;
    uint64_t secondary_frames = 0;
    volatile uint64_t free_frames = 0;

    void init_frame_mapping(multiboot_info_t *mbi)
    {
        /* _____________
        * |KERNEL FRAMES|
        * |             |
        * |_____________|
        * |SECON. FRAMES|
        * |             |
        * |             |
        * |_ _ _ _ _ _ _|
        */
        void *first_frame_after_modules = nullptr;
        if (mbi->flags & MULTIBOOT_INFO_MODS && mbi->mods_count >= 1)
            first_frame_after_modules = memory::align((void *)(uint64_t)(((multiboot_module_t *)(uint64_t)mbi->mods_addr)[mbi->mods_count - 1].mod_end), 0x1000);
        void *first_frame_after_kernel = memory::align((void *)&memory::_end, 0x1000);

        void *first_secondary_frame = (void *)(max((uint64_t)first_frame_after_kernel, (uint64_t)first_frame_after_modules));

        kernel_frames = get_frame_index(first_secondary_frame); //frames occupied by the kernel module
        secondary_frames = FRAME_COUNT - kernel_frames;         //frames free at the start of the system
        free_frames = secondary_frames;

        first_free_frame_index = kernel_frames; //it's the index of the first secondary_frame
        for (uint64_t i = first_free_frame_index + 1; i <= FRAME_COUNT; i++)
        {
            frame_descriptors[i - 1].next_free_frame_index = i;
        }
        //find filled frames and page tables
        uint64_t identity_l4_table_frame_index = get_frame_index(&identity_l4_table);
        for (uint64_t i = 0; i < first_free_frame_index; i++)
        {
            if (i == identity_l4_table_frame_index) //l4 table 1 entry
            {
                frame_descriptors[i].present_entries = 1;
            }
            else if (i == identity_l4_table_frame_index + 1) //l3 table 1 entry
            {
                frame_descriptors[i].present_entries = 1;
            }
            else if (i == identity_l4_table_frame_index + 2) //l2 table 512 entries
            {
                frame_descriptors[i].present_entries = 512;
            }
            else
            {
                frame_descriptors[i].present_entries = 0;
            }
        }
        if (mbi->flags & MULTIBOOT_INFO_MEM_MAP)
        {
            free_frames = 0;
            uint64_t true_first_frame_index = FRAME_COUNT;
            uint64_t last_free_frame = FRAME_COUNT;
            system_memory = 0;
            for (uint64_t i = 0; i < mbi->mmap_length; i += sizeof(multiboot_memory_map_t))
            {
                auto &mmap = *((multiboot_memory_map_t *)(mbi->mmap_addr + i));
                for (uint64_t f = get_frame_index((void *)mmap.addr); f < get_frame_index((void *)(mmap.addr + mmap.len)); f++)
                {
                    if (mmap.type == 1 & f >= first_free_frame_index)
                    {
                        if (f < true_first_frame_index)
                            true_first_frame_index = f;
                        if (last_free_frame != FRAME_COUNT) //link the list
                            frame_descriptors[last_free_frame].next_free_frame_index = f;
                        frame_descriptors[f].is_available == true;
                        last_free_frame = f;
                        free_frames++;
                    }
                }
                if(mmap.type==1)
                    system_memory+=mmap.len;
            }
            last_memory_address = (uint64_t)get_frame_address(last_free_frame) + 0x1000 - 1;
            frame_descriptors[last_free_frame].next_free_frame_index = FRAME_COUNT;
        }
        /*auto fb_size = (mbi->framebuffer_palette_num_colors==0?2:4)*mbi->framebuffer_height*mbi->framebuffer_width;
        auto end_addr = memory::align((void*)(uint64_t)(mbi->framebuffer_addr + fb_size),0x1000);
        frame_descriptors[get_frame_index((void*)(uint64_t)mbi->framebuffer_addr)-1].next_free_frame_index = get_frame_index(end_addr);*/
    }

    void *get_frame_address(uint64_t frame_descriptor_index)
    {
        return (void *)(frame_descriptor_index * 0x1000);
    }
    uint64_t get_frame_index(void *frame_phisical_address)
    {
        return ((uint64_t)frame_phisical_address / 0x1000);
    }

    void *frame_alloc()
    {
        auto fff_index = first_free_frame_index;
        if (fff_index >= FRAME_COUNT)
            return nullptr; //ERROR no more frames
        first_free_frame_index = frame_descriptors[fff_index].next_free_frame_index;
        frame_descriptors[fff_index].next_free_frame_index = 0;
        free_frames--;
        return get_frame_address(fff_index);
    }
    void free_frame(void *address)
    {
        auto new_frame_index = get_frame_index(address);
        if (new_frame_index < kernel_frames)
            return; //ERROR you cannot free a kernel frame
        frame_descriptors[new_frame_index].next_free_frame_index = first_free_frame_index;
        first_free_frame_index = new_frame_index;
        free_frames++;
    }

    page_table_t *table_alloc()
    {
        auto new_table_address = frame_alloc();
        if (new_table_address)
        {
            auto frame_index = get_frame_index(new_table_address);
            memory::memset(new_table_address, 0, sizeof(page_table_t));
            frame_descriptors[frame_index].present_entries = 0;
        }
        return (page_table_t *)new_table_address;
    }
    void free_table(page_table_t *address)
    {
        auto frame_index = get_frame_index(address);
        if (frame_descriptors[frame_index].present_entries != 0)
            return; //ERROR you must empty the table first
        free_frame(address);
    }

    page_table_t *create_paging_trie()
    {
        auto root = table_alloc();
        root->copy_from(identity_l4_table, 0, 1);
        return root;
    }
    void destroy_paging_trie(page_table_t *trie_root)
    {
        for (uint64_t i4 = 1; i4 < 512; i4++) //skip the shared part;
        {
            if (trie_root->is_present(i4))
            {
                auto l3 = trie_root->operator[](i4);
                for (uint64_t i3 = 0; i3 < 512; i3++)
                {
                    if (l3->is_present(i3))
                    {
                        auto l2 = l3->operator[](i3);
                        for (uint64_t i2 = 0; i2 < 512; i2++)
                        {
                            if (l2->is_present(i2))
                            {
                                if (!(l2->data[i2] & flags::BIG))
                                {
                                    auto l1 = l2->operator[](i2);
                                    for (uint64_t i1 = 0; i1 < 512; i1++)
                                    {
                                        if (l1->is_present(i1))
                                            debug::log(debug::level::wrn, "Filled page found during paging trie destruction");
                                        l1->set_entry(i1, nullptr, 0);
                                    }
                                    free_table(l1);
                                }
                            }
                        }
                        free_table(l2);
                    }
                }
                free_table(l3);
            }
        }
        trie_root->set_entry(0, nullptr, 0);
        free_table(trie_root);
    }

    uint16_t get_index_of_level(void *virtual_address, uint8_t level) //level must be 0,1,2,3,4 (0 is offset)
    {
        switch (level)
        {
        case 0:
            return ((uint64_t)virtual_address & (0x0000000000000fffUL));
            break;
        case 1:
            return ((uint64_t)virtual_address & (0x00000000001ff000UL << 0)) >> (12);
            break;
        case 2:
            return ((uint64_t)virtual_address & (0x00000000001ff000UL << 9)) >> (12 + 9);
            break;
        case 3:
            return ((uint64_t)virtual_address & (0x00000000001ff000UL << 18)) >> (12 + 18);
            break;
        case 4:
            return ((uint64_t)virtual_address & (0x00000000001ff000UL << 27)) >> (12 + 27);
            break;
        default:
            return 0;
            break;
        }
    }

    interrupt::privilege_level_t get_level(void *virtual_address, page_table_t *trie_root)
    {
        uint16_t l4_index = ((uint64_t)virtual_address & (0x00000000001ff000UL << 27)) >> (12 + 27);
        uint16_t l3_index = ((uint64_t)virtual_address & (0x00000000001ff000UL << 18)) >> (12 + 18);
        uint16_t l2_index = ((uint64_t)virtual_address & (0x00000000001ff000UL << 9)) >> (12 + 9);
        uint16_t l1_index = ((uint64_t)virtual_address & (0x00000000001ff000UL << 0)) >> (12);
        uint16_t offset = ((uint64_t)virtual_address & (0x0000000000000fffUL));

        if (trie_root)
        {
            if (trie_root->is_present(l4_index) && trie_root->data[l4_index] & paging::flags::USER)
            {
                auto l3 = trie_root->operator[](l4_index);
                if (l3->is_present(l3_index) && l3->data[l3_index] & paging::flags::USER)
                {
                    auto l2 = l3->operator[](l3_index);
                    if (l2->is_present(l2_index) && l2->data[l2_index] & paging::flags::USER)
                    {
                        if (l2->data[l2_index] & paging::flags::BIG)
                        {
                            return interrupt::privilege_level_t::user;
                        }
                        else
                        {
                            auto l1 = l2->operator[](l2_index);
                            if (l1->is_present(l1_index) && l1->data[l1_index] & paging::flags::USER)
                            {
                                return interrupt::privilege_level_t::user;
                            }
                        }
                    }
                }
            }
        }
        return interrupt::privilege_level_t::system;
    }

    void *virtual_to_phisical(void *virtual_address, page_table_t *trie_root)
    {
        uint16_t l4_index = ((uint64_t)virtual_address & (0x00000000001ff000UL << 27)) >> (12 + 27);
        uint16_t l3_index = ((uint64_t)virtual_address & (0x00000000001ff000UL << 18)) >> (12 + 18);
        uint16_t l2_index = ((uint64_t)virtual_address & (0x00000000001ff000UL << 9)) >> (12 + 9);
        uint16_t l1_index = ((uint64_t)virtual_address & (0x00000000001ff000UL << 0)) >> (12);
        uint16_t offset = ((uint64_t)virtual_address & (0x0000000000000fffUL));

        if (trie_root)
        {
            if (trie_root->is_present(l4_index))
            {
                auto l3 = trie_root->operator[](l4_index);
                if (l3->is_present(l3_index))
                {
                    auto l2 = l3->operator[](l3_index);
                    if (l2->is_present(l2_index))
                    {
                        if (l2->data[l2_index] & paging::flags::BIG)
                        {
                            auto address = (uint64_t)l2->operator[](l1_index);
                            return (void *)(address + offset + (l1_index << 12));
                        }
                        else
                        {
                            auto l1 = l2->operator[](l2_index);
                            if (l1->is_present(l1_index))
                            {
                                auto address = (uint64_t)l1->operator[](l1_index);
                                return (void *)(address + offset);
                            }
                        }
                    }
                }
            }
        }
        return nullptr;
    }

    void *extend_identity_mapping(multiboot_info_t *mbi)
    {
        auto ret = map((void *)0x40000000, ((uint64_t)memory::align((void *)(last_memory_address + 1), 0x1000)) / 0x1000, flags::RW, &identity_l4_table, [](void *v_address, bool big_pages)
                       { return v_address; },
                       false);
        if (mbi->flags & MULTIBOOT_INFO_VBE_INFO)
        {
            auto ret2 = unmap((void *)(uint64_t)(mbi->framebuffer_addr), (uint64_t)memory::align((void *)(uint64_t)(mbi->framebuffer_addr + ((mbi->framebuffer_palette_num_colors == 0 ? 2 : 4) * mbi->framebuffer_height * mbi->framebuffer_width)), 0x1000) / 0x1000, &identity_l4_table, [](void *, bool) {}, false);
            auto ret3 = map((void *)(uint64_t)mbi->framebuffer_addr, (uint64_t)memory::align((void *)(uint64_t)(mbi->framebuffer_addr + ((mbi->framebuffer_palette_num_colors == 0 ? 2 : 4) * mbi->framebuffer_height * mbi->framebuffer_width)), 0x1000) / 0x1000, flags::RW | flags::WRITE_THROUGH, &identity_l4_table, [](void *va, bool)
                            { return va; },
                            false);
        }
        return ret;
    }
};