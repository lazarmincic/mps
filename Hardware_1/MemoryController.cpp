#include "MemoryController.hpp"

MemoryController::MemoryController() : bus(nullptr)
{
    /*
    Konstruktor
    */
    busy = false;
    readyAtCycle = 0;
    // inicijalizacija memorije na sve nule, da bi asemblerski kod iz primera radio kako treba
    std::vector<Data> niz(MEMORY_SIZE, {0});
    loadData(niz);
    std::cout << "Sys: MMC na mrezi.\n";
}

void MemoryController::setBus(Bus *bus)
{
    /*
    Postavljanje magistrale u main-u
    */
    this->bus = bus;
}

Data MemoryController::read(Address address)
{
    /*
    Čitanje sa memorijske adrese
    */
    return memory[address.val];
}

void MemoryController::write(Address address, Data data)
{
    /*
    Pisanje podatka na memorijsku adresu
    */
    memory[address.val] = data;
    stateOfMem(); // ispis kada se promeni
}

void MemoryController::loadData(std::vector<Data> initData)
{
    /*
    Učitavanje prvobitnih podataka u memoriju, lakši debug
    */
    for (size_t i = 0; i < initData.size(); i++)
    {
        memory[i] = initData[i];
    }
}

void MemoryController::snoop(const BusSignal &signal)
{
    /*
    Poziva se unutar Bus, nakon što je signal postavljen na magistralu
    */

    // mem c je slave, samo odgovara
    if (signal.type == BusTransaction::READ)
    {
        // na osnovu copiesExist zna da li će tuđi keš dostaviti podatak ili treba on da ga dostavi
        // copiesExist = false - podatak se vuče iz memorije
        // podatak se sigurno nalazi u memoriji, zato nema provere adrese
        if (bus->sharedLineActive())
        {

            // memorija tiha, cache to cache transfer i arbitracija
            std::cout << Simulation::cycleCount << " " << "MEMC: Presretnut signal sa magistrale. C linija visoka, ne radim nista.\n";
        }
        else
        {
            if (!busy)
            {
                // podatak se nalazi isključivo u memoriji i ona ga mora dostaviti
                // ovo traje 80 ciklusa
                busy = true;
                readyAtCycle = Simulation::cycleCount + Latency::MEMORY;
                std::cout << Simulation::cycleCount << " MEMC: Presretnut signal sa magistrale. C linija niska. Pokrenuto citanje sa adrese " << bus->getCurrentSignal().address.val << ".\n";
            }
        }
    }

    if (signal.type == BusTransaction::FLUSH) // onaj koji ima M flushovao
    {
        if (!busy)
        {

            std::cout << Simulation::cycleCount << " MEMC: Pokrenut upis podatka " << bus->getCurrentSignal().data.val << " na adresu " << bus->getCurrentSignal().address.val << ".\n";
            busy = true;
            readyAtCycle = Simulation::cycleCount + Latency::MEMORY;
        }
    }
}

void MemoryController::tick()
{
    /*
    Jedan otkucaj u simulaciji
    */
    // 1. Ako nismo zauzeti, gledamo šta se dešava na magistrali
    if (!busy)
    {
        // sva logika je unutar snoop metode
    }
    else
    {
        // 2. Ako smo zauzeti, čekamo da prođe 80 ciklusa
        if (Simulation::cycleCount >= readyAtCycle)
        {

            // VREME JE ISTEKLO! Dostavljamo podatak
            BusSignal memSignal = bus->getCurrentSignal();
            if (memSignal.type == BusTransaction::READ)
            {
                memSignal.data = read(memSignal.address);
                bus->putDataOnBus(memSignal); // ovo treba da dobije onaj ko je podatak trazio
                busy = false;
                std::cout << Simulation::cycleCount << " MEMC: Podatak " << memSignal.data.val << " procitan sa adrese "
                          << memSignal.address.val << " nakon " << (unsigned int)Latency::MEMORY << " ciklusa čekanja.\n";
            }
            else if (memSignal.type == BusTransaction::FLUSH)
            {
                // gotovo sa pisanjem

                busy = false;
                bus->acknowledgeResponse();
                Address addr = {bus->getCurrentSignal().address};
                Data data = {bus->getCurrentSignal().data};
                std::cout << Simulation::cycleCount << " MEMC: Podatak " << data.val << " upisan na adresu " << addr.val << " nakon " << (unsigned int)Latency::MEMORY << " ciklusa čekanja.\n";
                write(addr, data);
            }
        }
    }
}

void MemoryController::stateOfMem()
{
    std::cout << Simulation::cycleCount << " MEMC: Stanje memorije - Address: 0, Data: " << read({0}).val << "; Address: 1, Data: " << read({1}).val << "\n";
}
