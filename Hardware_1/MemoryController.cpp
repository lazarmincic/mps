#include "MemoryController.hpp"

MemoryController::MemoryController() : bus(nullptr)
{
}

void MemoryController::setBus(Bus *bus)
{
    this->bus = bus;
}

int32_t MemoryController::read(uint32_t address)
{
    return memory[address];
}

void MemoryController::write(uint32_t address, int32_t data)
{
    memory[address] = data;
}
