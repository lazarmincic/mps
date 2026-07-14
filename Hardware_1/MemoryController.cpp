#include "MemoryController.hpp"

MemoryController::MemoryController(std::string name)
{
    /*
    Konstruktor
    */
    busy = false;
    readyAtCycle = 0;
    bus = nullptr;
    mcName = name;
    // inicijalizacija memorije na sve nule, da bi asemblerski kod iz primera radio kako treba
    std::vector<Data> niz(MEMORY_SIZE, {0});
    loadData(niz);

    msg << "Na mrezi.";
    mcLog(msg);
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
    if (signal.type == BusTransaction::READ || signal.type == BusTransaction::WRITE)
    {
        // na osnovu copiesExist zna da li će tuđi keš dostaviti podatak ili treba on da ga dostavi
        // copiesExist = false - podatak se vuče iz memorije
        // podatak se sigurno nalazi u memoriji, zato nema provere adrese
        if (bus->sharedLineActive())
        {

            // memorija tiha, cache to cache transfer i arbitracija
            msg << "Presretnut signal " << signal.type << " sa magistrale. C linija visoka, ne dostavljam ja podatak.";
            mcLog(msg);
        }
        else
        {
            if (!busy)
            {
                // podatak se nalazi isključivo u memoriji i ona ga mora dostaviti
                // ovo traje 80 ciklusa
                busy = true;
                readyAtCycle = Simulation::cycleCount + Latency::MEMORY;
                msg << "Presretnut signal " << signal.type << " sa magistrale. C linija niska. Pokrenuto citanje sa adrese "
                    << bus->getCurrentSignal().address << ".";
                mcLog(msg);
            }
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
            if (memSignal.type == BusTransaction::READ || memSignal.type == BusTransaction::WRITE)
            {
                memSignal.data = read(memSignal.address);
                msg << "Podatak " << memSignal.data << " procitan sa adrese "
                    << memSignal.address << " nakon " << Latency::MEMORY << " ciklusa čekanja.";
                mcLog(msg);
                bus->putDataOnBus(memSignal); // ovo treba da dobije onaj ko je podatak trazio
                busy = false;
            }
            else if (memSignal.type == BusTransaction::FLUSH)
            {
                // gotovo sa pisanjem

                busy = false;
                bus->acknowledgeResponse();
                Address addr = {bus->getCurrentSignal().address};
                Data data = {bus->getCurrentSignal().data};
                msg << "Podatak " << data << " upisan na adresu " << addr << " nakon " << Latency::MEMORY << " ciklusa čekanja.";
                mcLog(msg);
                write(addr, data);
            }
        }
    }
}

void MemoryController::stateOfMem()
{
    msg << "Stanje memorije - Blok: 0, Data: " << read({0}) << "; Blok: 1, Data: " << read({1});
    mcLog(msg);
}

void MemoryController::flush() // onaj koji ima M flushovao
{
    if (!busy)
    {

        msg << "Presretnut signal " << bus->getCurrentSignal().type << " sa magistrale. Pokrenut upis podatka " << bus->getCurrentSignal().data << " na adresu " << bus->getCurrentSignal().address << ".";
        mcLog(msg);
        busy = true;
        readyAtCycle = Simulation::cycleCount + Latency::MEMORY;
    }
}

void MemoryController::mcLog(std::stringstream &ss, LogLevel level)
{
    ::log(ss, mcName, level);
}
