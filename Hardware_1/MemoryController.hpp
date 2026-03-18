#ifndef MEMORYCONTROLLER_HPP
#define MEMORYCONTROLLER_HPP

#include "Bus.hpp"
#include <map>

class Bus ;

class MemoryController {
private:
    Bus* bus;
    std::map<uint32_t, int32_t> memory; 

public:

    MemoryController();
    void setBus(Bus* b);

    int32_t read(uint32_t address);
    
    void write(uint32_t address, int32_t data);

};

#endif