#pragma once
#include <stdint.h>
#include "multiboot.h"
#include "memory.h"
#include "string_tools.h"
#include "printing.h"
#include "math.h"

namespace paging
{
    namespace flags
    {
        constexpr uint16_t PRESENT =        0b000000000001;
        constexpr uint16_t RW =             0b000000000010;
        constexpr uint16_t USER =           0b000000000100;
        constexpr uint16_t WRITE_THROUGH =  0b000000001000;
        constexpr uint16_t DISABLE_CACHE =  0b000000010000;
        constexpr uint16_t ACCESSED =       0b000000100000;
        constexpr uint16_t BIG =            0b000001000000;
    };
    struct page_table_t
    {
        uint64_t data[512];
        void set_entry(uint16_t index,void* child, uint16_t flags);
        void copy_from(const page_table_t& source_table,uint16_t from, uint16_t count);
        bool is_present(uint16_t index);
        page_table_t* operator[](uint16_t index);
    };

    extern "C" page_table_t identity_l4_table;

    struct frame_descriptor_t
    {
        bool is_available = false;
        union
        {
            uint64_t present_entries;//NOT FREE: number of present entries if the frame holds a page_table_t
            uint64_t next_free_frame_index;//FREE: index of the next free frame
        };
    };

    constexpr uint64_t FRAME_COUNT = 0x40000 * 32;//1GiB * GiB count
    extern frame_descriptor_t frame_descriptors[FRAME_COUNT];
    extern uint64_t first_free_frame_index;
    extern uint64_t last_memory_address;
    extern uint64_t system_memory;
    extern uint64_t kernel_frames;
    extern uint64_t secondary_frames;
    extern volatile uint64_t free_frames;

    void init_frame_mapping(multiboot_info_t* mbi);

    void* get_frame_address(uint64_t frame_descriptor_index);
    uint64_t get_frame_index(void* frame_phisical_address);

    //extract a free frame from the list of free frames
    void* frame_alloc();
    //inserts a frame into the list of free frames
    void free_frame(void* address);

    //creates an empty table taking a free frame
    page_table_t* table_alloc();
    //frees a table returning the frame
    void free_table(page_table_t* address);
    
    page_table_t* create_paging_trie();
    void destroy_paging_trie(page_table_t* trie_root);
    void* virtual_to_phisical(void* virtual_address,page_table_t* trie_root);
    
    uint16_t get_index_of_level(void*virtual_address,uint8_t level);//level must be 0,1,2,3,4 (0 is offset)

    extern "C" void set_current_trie(page_table_t* trie_root);
    extern "C" page_table_t* get_current_trie();
    extern "C" void* get_cr2();

    //should be 0xffffffffffffffff if it's all ok, if you're trying to map a region that's already mapped
    //the function returns the first address that generated the error
    template <typename T>
    void* map(void* virtual_address, uint64_t page_count, uint16_t flags, page_table_t* trie_root,T gen_addr, bool big_pages = false)
    {
        if(!memory::is_normalized(virtual_address))
            return nullptr;
        if((uint64_t)virtual_address&0x0000000000000fff)//check align 0x1000
            return nullptr;
        if((uint64_t)virtual_address&0x00000000001fffff && big_pages)
            return nullptr;
        virtual_address = (void*)((uint64_t)virtual_address &(0x0000fffffffff000));

        //0b0000 0000 0000 0000  1111 1111 1000 0000   0000 0000 0000 0000  0000 0000 0000 0000 l4
        //0b0000 0000 0000 0000  0000 0000 0111 1111   1100 0000 0000 0000  0000 0000 0000 0000 l3
        //0b0000 0000 0000 0000  0000 0000 0000 0000   0011 1111 1110 0000  0000 0000 0000 0000 l2
        //0b0000 0000 0000 0000  0000 0000 0000 0000   0000 0000 0001 1111  1111 0000 0000 0000 l1
        //0b1111 1111 1111 1111  1111 1111 1111 1111   1111 1111 1111 1111  1111 1111 1111 1111
        uint16_t l4_index = ((uint64_t)virtual_address & (0x00000000001ff000UL<<27))>>(12+27);
        uint16_t l3_index = ((uint64_t)virtual_address & (0x00000000001ff000UL<<18))>>(12+18);
        uint16_t l2_index = ((uint64_t)virtual_address & (0x00000000001ff000UL<<9))>>(12+9);
        uint16_t l1_index = ((uint64_t)virtual_address & (0x00000000001ff000UL<<0))>>(12);

        uint16_t high_level_flags = (flags & (flags::RW|flags::USER)) | flags::PRESENT;
        uint16_t low_level_flags = (flags & (flags::RW|flags::USER|flags::WRITE_THROUGH|flags::DISABLE_CACHE)) | flags::PRESENT;

        for(uint64_t i = 0; i<page_count; i++)
        {
            uint16_t l4_loop_index = l4_index+i/(512*512*512);
            uint16_t l3_loop_index = (l3_index+i/(512*512))%512;
            uint16_t l2_loop_index = (l2_index+i/(512))%512;
            uint16_t l1_loop_index = (l1_index+i)%512;
            
            if(big_pages)
            {
                l4_loop_index = l4_index+i/(512*512);
                l3_loop_index = (l3_index+i/(512))%512;
                l2_loop_index = (l2_index+i)%512;
                l1_loop_index = 0;
            }

            void* current_addr = memory::normalize((void*)((uint64_t)l4_loop_index<<(12+27)| (uint64_t)l3_loop_index<<(12+18) | (uint64_t)l2_loop_index<<(12+9) | (uint64_t)l1_loop_index<<12));

            void* phisical_address = gen_addr(current_addr,big_pages);
            if(!phisical_address)
                return current_addr;

            if(!trie_root->is_present(l4_loop_index))
            {//you need to allocate a l3 page
                auto new_l3 = table_alloc();
                trie_root->set_entry(l4_loop_index,new_l3,high_level_flags);
            }
            auto l3_table = (*trie_root)[l4_loop_index];
            if(!l3_table->is_present(l3_loop_index))
            {//you need to allocate a l2 page
                auto new_l2 = table_alloc();
                l3_table->set_entry(l3_loop_index,new_l2,high_level_flags);
            }
            auto l2_table = (*l3_table)[l3_loop_index];
            if(!l2_table->is_present(l2_loop_index))
            {
                if(!big_pages)//you need to allocate a l1 page
                {
                    auto new_l1 = table_alloc();
                    l2_table->set_entry(l2_loop_index,new_l1,high_level_flags);
                }
                else
                {
                    void* new_frame = phisical_address;
                    l2_table->set_entry(l2_loop_index,new_frame,low_level_flags|flags::BIG);
                    continue;
                }
            }
            else if(l2_table->data[l2_loop_index]&flags::BIG || big_pages)//if a big page is allocated error
            {
                return current_addr;
            }
            auto l1_table = (*l2_table)[l2_loop_index];
            if(l1_table->is_present(l1_loop_index))
                return current_addr;
            void* new_frame = phisical_address;
            l1_table->set_entry(l1_loop_index,new_frame,low_level_flags);
        }
        //debug::log(debug::level::inf,"MAP from: %p n: %uld, level: %s",virtual_address,page_count,flags&flags::USER?"user":"system");
        return (void*)0xffffffffffffffff;
    }
    template <typename T>
    void* unmap(void* virtual_address, uint64_t page_count, page_table_t* trie_root,T free_addr, bool big_pages = false)
    {
        if(!memory::is_normalized(virtual_address))
            return nullptr;
        if((uint64_t)virtual_address&0x0000000000000fff)//check align 0x1000
            return nullptr;
        if((uint64_t)virtual_address&0x00000000001fffff && big_pages)
            return nullptr;
        virtual_address = (void*)((uint64_t)virtual_address &(0x0000fffffffff000));

        //0b0000 0000 0000 0000  1111 1111 1000 0000   0000 0000 0000 0000  0000 0000 0000 0000 l4
        //0b0000 0000 0000 0000  0000 0000 0111 1111   1100 0000 0000 0000  0000 0000 0000 0000 l3
        //0b0000 0000 0000 0000  0000 0000 0000 0000   0011 1111 1110 0000  0000 0000 0000 0000 l2
        //0b0000 0000 0000 0000  0000 0000 0000 0000   0000 0000 0001 1111  1111 0000 0000 0000 l1
        //0b1111 1111 1111 1111  1111 1111 1111 1111   1111 1111 1111 1111  1111 1111 1111 1111
        uint16_t l4_index = ((uint64_t)virtual_address & (0x00000000001ff000UL<<27))>>(12+27);
        uint16_t l3_index = ((uint64_t)virtual_address & (0x00000000001ff000UL<<18))>>(12+18);
        uint16_t l2_index = ((uint64_t)virtual_address & (0x00000000001ff000UL<<9))>>(12+9);
        uint16_t l1_index = ((uint64_t)virtual_address & (0x00000000001ff000UL<<0))>>(12);

        for(uint64_t i = 0; i<page_count; i++)
        {
            uint16_t l4_loop_index = l4_index+i/(512*512*512);
            uint16_t l3_loop_index = (l3_index+i/(512*512))%512;
            uint16_t l2_loop_index = (l2_index+i/(512))%512;
            uint16_t l1_loop_index = (l1_index+i)%512;
            
            if(big_pages)
            {
                l4_loop_index = l4_index+i/(512*512);
                l3_loop_index = (l3_index+i/(512))%512;
                l2_loop_index = (l2_index+i)%512;
                l1_loop_index = 0;
            }

            void* current_addr = memory::normalize((void*)((uint64_t)l4_loop_index<<(12+27)| (uint64_t)l3_loop_index<<(12+18) | (uint64_t)l2_loop_index<<(12+9) | (uint64_t)l1_loop_index<<12));

            if(!trie_root->is_present(l4_loop_index))
            {
                return current_addr;
            }
            auto l3_table = (*trie_root)[l4_loop_index];
            if(!l3_table->is_present(l3_loop_index))
            {
                return current_addr;
            }
            auto l2_table = (*l3_table)[l3_loop_index];
            if(!l2_table->is_present(l2_loop_index))
            {
                return current_addr;
            }
            else if(l2_table->data[l2_loop_index]&flags::BIG)//if a big page is allocated error
            {
                if(big_pages)
                {
                    auto addr = l2_table->operator[](l2_loop_index);
                    free_addr((void*)addr,big_pages);
                    l2_table->set_entry(l2_loop_index,nullptr,0);
                    if(frame_descriptors[get_frame_index(l2_table)].present_entries==0)
                    {//we need to free the frame
                        free_table(l2_table);
                        l3_table->set_entry(l3_loop_index,nullptr,0);
                        if(frame_descriptors[get_frame_index(l3_table)].present_entries==0)
                        {
                            free_table(l3_table);
                            trie_root->set_entry(l4_loop_index,nullptr,0);
                        }
                    }
                    continue;
                }
                else
                {
                    return current_addr;
                }
            }
            auto l1_table = (*l2_table)[l2_loop_index];
            if(!l1_table->is_present(l1_loop_index))
                return current_addr;
            else
            {
                auto addr = l1_table->operator[](l1_loop_index);
                free_addr((void*)addr,big_pages);
                l1_table->set_entry(l1_loop_index,nullptr,0);
                if(frame_descriptors[get_frame_index(l1_table)].present_entries==0)
                {
                    free_table(l1_table);
                    l2_table->set_entry(l2_loop_index,nullptr,0);
                    if(frame_descriptors[get_frame_index(l2_table)].present_entries==0)
                    {//we need to free the frame
                        free_table(l2_table);
                        l3_table->set_entry(l3_loop_index,nullptr,0);
                        if(frame_descriptors[get_frame_index(l3_table)].present_entries==0)
                        {
                            free_table(l3_table);
                            trie_root->set_entry(l4_loop_index,nullptr,0);
                        }
                    }
                }
            }
        }
        return (void*)0xffffffffffffffff;
    }

    void* extend_identity_mapping(multiboot_info_t *mbi);
};

#include "interrupt.h"

namespace paging
{
    interrupt::privilege_level_t get_level(void* virtual_address,page_table_t* trie_root);  
};