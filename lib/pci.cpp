#include "pci.h"

namespace pci
{
    namespace config
    {
        generic_header::generic_header(location_t loc):loc(loc){}
        uint16_t generic_header::get_device_id()
        {
            return read::w(loc,2);
        }
        uint16_t generic_header::get_vendor_id()
        {
            return read::w(loc,0);
        }
        uint16_t generic_header::get_status()
        {
            return read::w(loc,6);
        }
        uint16_t generic_header::get_command()
        {
            return read::w(loc,4);
        }
        uint8_t generic_header::get_class()
        {
            return read::w(loc,11);
        }
        uint8_t generic_header::get_subclass()
        {
            return read::w(loc,10);
        }
        uint8_t generic_header::get_prog_if()
        {
            return read::w(loc,9);
        }
        uint8_t generic_header::get_revision_id()
        {
            return read::w(loc,8);
        }
        uint8_t generic_header::get_BIST()
        {
            return read::w(loc,15);
        }
        uint8_t generic_header::get_header_type()
        {
            return read::w(loc,14);
        }
        uint8_t generic_header::get_latency_timer()
        {
            return read::w(loc,13);
        }
        uint8_t generic_header::get_cache_line_size()
        {
            return read::w(loc,12);
        }
            
        void generic_header::set_command(uint16_t data)
        {

        }
        void generic_header::set_BIST(uint8_t data)
        {

        }
        void generic_header::set_latency_timer(uint8_t data)
        {

        }
        void generic_header::set_cache_line_size(uint8_t data)
        {

        }

        location_t find(uint8_t class_id, uint8_t subclass, uint8_t prog_if, uint8_t revision_id)
        {
            for(uint64_t b = 0; b<256; b++)
            {
                for(uint64_t d = 0; d<256; d++)
                {
                    if(is_present({(uint8_t)b,(uint8_t)d,0}))
                    {
                        for(uint64_t f = 0; f<256; f++)
                        {
                            location_t loc{(uint8_t)b,(uint8_t)d,(uint8_t)f};
                            if(is_present(loc))
                            {
                                auto h = generic_header(loc);
                                if(h.get_class()==class_id && (h.get_subclass()==subclass || subclass==0xff) && (h.get_prog_if()==prog_if || prog_if==0xff) && (h.get_revision_id()==revision_id || revision_id==0xff))
                                    return loc;
                            }
                        }
                    }
                }
            }
            return {0xff,0xff,0xff};
        }

        bool is_present(location_t loc)
        {
            if(loc.function==0)
                return generic_header(loc).get_vendor_id()!=0xffff;
            else if(generic_header({loc.bus,loc.device,0}).get_vendor_id()!=0xffff && generic_header(loc).get_header_type()&0x80)
                return generic_header(loc).get_vendor_id() != 0xffff;
            else return false;
        }
        uint32_t build_CAP(location_t loc, uint8_t offset)
        {
            return 0x80000000 | (uint32_t(loc.bus) << 16) | (uint32_t(loc.device) << 11) | (uint32_t(loc.function) << 8) | (offset&0xfc);
        }
        namespace read
        {
            uint8_t b(location_t loc, uint8_t offset)
            {
                io::outl(CONFIG_ADDRESS,build_CAP(loc,offset));
                return io::inb(CONFIG_DATA+(offset&0x3));
            }
            uint16_t w(location_t loc, uint8_t offset)
            {
                io::outl(CONFIG_ADDRESS,build_CAP(loc,offset));
                return io::inw(CONFIG_DATA+(offset&0x2));
            }
            uint32_t l(location_t loc, uint8_t offset)
            {
                io::outl(CONFIG_ADDRESS,build_CAP(loc,offset));
                return io::inl(CONFIG_DATA);
            }
        }; 
        namespace write
        {
            void b(location_t loc, uint8_t offset, uint8_t data)
            {
                io::outl(CONFIG_ADDRESS,build_CAP(loc,offset));
                io::outb(CONFIG_DATA+(offset&0x3),data);
            }
            void w(location_t loc, uint8_t offset, uint16_t data)
            {
                io::outl(CONFIG_ADDRESS,build_CAP(loc,offset));
                io::outw(CONFIG_DATA+(offset&0x2),data);
            }
            void l(location_t loc, uint8_t offset, uint32_t data)
            {
                io::outl(CONFIG_ADDRESS,build_CAP(loc,offset));
                io::outl(CONFIG_DATA,data);
            }
        };
    };
};