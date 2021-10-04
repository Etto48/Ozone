#pragma once
#include <io.h>

namespace pci
{
    namespace config
    {
        constexpr uint16_t CONFIG_ADDRESS = 0xcf8;
        constexpr uint16_t CONFIG_DATA = 0xcfc;

        struct location_t
        {
            uint8_t bus, device, function;
            location_t(uint8_t bus, uint8_t device, uint8_t function) : bus(bus), device(device), function(function) {}
        };

        class generic_header
        {
        private:
            location_t loc;

        public:
            generic_header(location_t loc);
            uint16_t get_device_id();
            uint16_t get_vendor_id();
            uint16_t get_status();
            uint16_t get_command();
            uint8_t get_class();
            uint8_t get_subclass();
            uint8_t get_prog_if();
            uint8_t get_revision_id();
            uint8_t get_BIST();
            uint8_t get_header_type();
            uint8_t get_latency_timer();
            uint8_t get_cache_line_size();

            void set_command(uint16_t data);
            void set_BIST(uint8_t data);
            void set_latency_timer(uint8_t data);
            void set_cache_line_size(uint8_t data);
        };

        location_t find(uint8_t class_id, uint8_t subclass = 0xff, uint8_t prog_if = 0xff, uint8_t revision_id = 0xff);
        bool is_present(location_t loc);
        uint32_t build_CAP(location_t loc, uint8_t offset);
        namespace read
        {
            uint8_t b(location_t loc, uint8_t offset);
            uint16_t w(location_t loc, uint8_t offset);
            uint32_t l(location_t loc, uint8_t offset);
        };
        namespace write
        {
            void b(location_t loc, uint8_t offset, uint8_t data);
            void w(location_t loc, uint8_t offset, uint16_t data);
            void l(location_t loc, uint8_t offset, uint32_t data);
        };

    };
};