#ifndef MEMORYCONTROLLER_HPP
#define MEMORYCONTROLLER_HPP

#include "Bus.hpp"

class Bus ;

class MemoryController {
public:
    MemoryController();
    void setBus(Bus* b);
private:
    Bus* bus;
    int32_t memory[128]; // 128 reči od po 32 bita
};

#endif