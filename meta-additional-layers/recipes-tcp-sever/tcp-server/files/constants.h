
#pragma once


#include <string>

namespace demon_constant {

    const std::string device_name = "/dev/onewire_dev";

    class onewire_data {
        public:
        char cmd[2];
    };

    class onewire_read_address : public onewire_data {
        public:
            const char cmd = 0x33;
            char result[8];
    };
    

    class onewire_write : public onewire_data {
        public: 
            const char cmd[2] = { (char) 0xCC, (char) 0x4E};
            char result[8];
    };

    class onewire_read : public onewire_data {
        public:
            const char cmd[2] = { (char) 0xCC, (char) 0xBE};
            char result[8];
    };

};