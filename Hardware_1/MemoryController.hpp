#ifndef MEMORYCONTROLLER_HPP
#define MEMORYCONTROLLER_HPP

#include "Common.hpp"
#include "Bus.hpp"

class Bus;

class MemoryController
{
private:
    Bus *bus;                          // pristup magistrali
    const static int MEMORY_SIZE = 32; // broj mem. lokacija unutar memorije
    Data memory[MEMORY_SIZE];          // 32 * 32 memorije
    bool busy;                         // da li je u toku čitanje / pisanje podatka?
    uint64_t readyAtCycle;             // u koliko se dešava čitanje ili pisanje, kada će sa njim biti gotovo?

public:
    MemoryController();    // konstruktor
    void setBus(Bus *bus); // povezivanje sa magistralom

    Data read(Address address); // čitanje iz memorije

    void write(Address address, Data data); // pisanje u memoriju

    void snoop(const BusSignal &signal); // osluškivanje magistrale

    void tick(); // jedan otkucaj u simulaciji

    void stateOfMem();

private:
    void loadData(std::vector<Data> initData); // učitavanje podataka (32-bitnih) u memoriju
};

#endif
