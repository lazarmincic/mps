#include "CacheController.hpp"

CacheController::CacheController(Controller contr)
{
    // podrazumevani konstruktor CacheLine su sve 0
    bus = nullptr;
    myCC = contr;
    ready = false; // podatak dostupan
    bufferForCPU = {0};
    busy = false;
    busyCycles = 0;
    monitoredAddress = {0};
    reservationBroken = false;
    pendingTransactionType = BusTransaction::IDLE;
    pendingState = CacheState::I;
    std::cout << "Sys: " << myCC << " na mrezi.\n";
}

bool CacheController::isHit(Address address)
{
    CacheLine cacheLine = getCacheLine(address);
    uint32_t tag = (address.val >> 3);
    return (cacheLine.tag == tag && cacheLine.state.getState() != CacheState::I);
}

Data CacheController::getData(Address address)
{
    return getCacheLine(address).data;
}

CacheState CacheController::getState(Address address)
{
    return getCacheLine(address).state.getState();
}

void CacheController::setState(CacheState state, Address address)
{
    CacheLine toChange = getCacheLine(address);
    toChange.state.setState(state);
    setCacheLine(address, toChange);
}

CacheLine CacheController::getCacheLine(Address address)
{
    uint32_t index = addressToIndex(address);
    return cache[index];
}

void CacheController::setCacheLine(Address address, CacheLine cacheLine)
{
    cache[addressToIndex(address)] = cacheLine;
    stateOfCache();
}

uint32_t CacheController::addressToIndex(Address address)
{
    return (address.val >> 2) & 0x1;
}

void CacheController::storeData(Address address, Data data)
{
    // keš hit
    uint32_t index = (address.val >> 2) & 0x1;
    cache[index].data = data;
    cache[index].tag = (address.val >> 3);
    cache[index].state.setM(); // Nakon upisa, linija je Modified
    // to do. postoji interakcija sa magistralom
}

void CacheController::setBus(Bus *bus)
{
    this->bus = bus;
}

void CacheController::snoop(const BusSignal &signal, bool isWinner)
{
    // Pronađi liniju
    if (myCC == bus->getCurrentOwner())
    {
        return;
    }
    Address addr = {signal.address};
    int setIndex = findLineInCache(addr);
    std::cout << Simulation::cycleCount << " " << myCC << ": Presretnut signal " << signal.type << " sa magistrale.";
    if (setIndex != -1)
    {
        std::cout << " Snoop pogodak na adresi " << addr.val << ", stanje " << getState(addr) << ", podatak " << getData(addr).val << ".";
        // snupovana linija JESTE keširana
        // dakle E, M, ili S
        CacheState state = getState(addr);

        if ((state == CacheState::E || state == CacheState::S))
        {
            if (signal.type == BusTransaction::READ || signal.type == BusTransaction::WRITE)
            {
                if (isWinner) // pobedio u arbitrazi
                {
                    busy = true;
                    busyCycles = Latency::REMOTE_CACHE; // Npr. 8 ciklusa za Cache-to-Cache
                    // ovde arbitracija
                    std::cout << " Pocinjem sa slanjem podatka " << bus->getCurrentOwner() << "\n";
                }
                else
                {
                    // izgubio arbitrazu
                    // Kontroler ima podatak, ali drugi kontroler je zadužen za slanje.
                    // On samo tiho menja svoje stanje bez postavljanja busy.
                    busy = false;
                    // Ovde odmah promeni stanje
                    if (signal.type == BusTransaction::WRITE)
                    {
                        if (state == CacheState::S)
                        {
                            setState(CacheState::I, addr); // ako gubi arbitraciju, samo menja stanje
                            // i to mora ovde da se desi kao i sve ostale "tihe" promene stanja
                            // ako je blok bio u stanju E svakako nema potrebe za arbitracijom
                            std::cout << " Izgubio sam unutrasnju arbitrazu, tiho prelazim u stanje I.\n";
                        }
                    }
                }
            }
            else if (signal.type == BusTransaction::UPGRADE)
            {
                if (state == CacheState::S)
                {
                    std::cout << " Prelazim u stanje I.\n";
                    setState(CacheState::I, addr);
                    busy = false;
                    // nista na magistralu
                }
            }
        }
        else if (state == CacheState::M)
        {
            if (signal.type == BusTransaction::READ || signal.type == BusTransaction::WRITE)

            {

                // address je vec na magistrali
                // nextState = CacheState::S;
                std::cout << " Flush-ujem blok na adresi " << addr.val << " na magistralu.\n";
                bus->flush({getData(addr)});
                busy = true;
                if (signal.type == BusTransaction::READ)
                {
                    pendingState = CacheState::S;
                    std::cout << " Prelazim u medjustanje MS.\n";
                }
                else if (signal.type == BusTransaction::WRITE)
                {
                    pendingState = CacheState::I;
                    std::cout << " Prelazim u medjustanje MI.\n";
                    // ne prelazi zapravo, vec pamti stanje u koje treba da predje..
                }
                // promene stanja nakon sto se zavrsi upis u mem
            }
        }
        // DODATAK ZA LR/SC:
        if (signal.type == BusTransaction::WRITE || signal.type == BusTransaction::UPGRADE)
        {
            if (monitoredAddress.val == signal.address.val)
            {

                reservationBroken = true; // Samo podigni zastavicu
                // ukoliko je trazio magistralu, ponistava se jer mu ona vise nije potrebna, sc moze da se izvrsi sada i bez nje..
                bus->dropBusRequest(myCC);
                ready = false;
            }
        }
    }
    else
    {
        // I stanje, jer blok ne postoji u kesu
        // nista na magistralu i ostaje I
        busy = false;
        std::cout << " Blok u stanju I. Snoop promasaj. Nema promene stanja. Nista na magistralu.\n";
        return;
    }
}

int CacheController::findLineInCache(Address address)
{
    uint32_t tag = (address.val >> 3);
    for (int i = 0; i < 2; i++)
    {
        if (cache[i].tag == tag && !cache[i].state.isI())
        {
            return i;
        }
    }
    return -1; // Nema linije ili je linija I
}

void CacheController::readMiss(Address address)
{
    // 1. Emituj signal
    BusSignal signal;
    signal.type = BusTransaction::READ;
    pendingTransactionType = signal.type;
    signal.address = address;
    bus->requestBus(myCC, signal);
    ready = false;
}

void CacheController::writeMiss(Address address, Data data)
{
    // 1. Emituj signal
    BusSignal signal;
    signal.type = BusTransaction::WRITE;
    pendingTransactionType = signal.type;
    signal.address = address;
    signal.data = data;
    bus->requestBus(myCC, signal);
}

Controller CacheController::getController() const
{
    return myCC;
}

void CacheController::broadcastInvalidate(Address addr)
{
    // poziva se kada se iz stanja S se ide u stanje M nad blokom u addr
    BusSignal signal;
    signal.type = BusTransaction::UPGRADE;
    pendingTransactionType = signal.type;
    signal.address = addr;
    bus->requestBus(myCC, signal);

    busy = true;
    busyCycles = Latency::BUS_UPGRADE;
    ready = false;
}

bool CacheController::isReady() const
{
    return ready;
}

void CacheController::receiveBusResponse()
{
    BusTransaction bt = bus->getCurrentSignal().type;
    Address addr = bus->getCurrentSignal().address;
    if (bt == BusTransaction::READ || bt == BusTransaction::FLUSHOPT || bt == BusTransaction::FLUSH)
    {
        // citanje
        // iz Data u CacheLine konverzija potrebna.

        Data val = bus->getCurrentSignal().data;
        CacheLine newLine;
        newLine.setAddress(addr);
        newLine.data = val;
        if (bus->sharedLineActive()) // gleda se shared linija
        {
            if (bt == BusTransaction::FLUSHOPT)
            {
                std::cout << Simulation::cycleCount << " " << myCC << ": Primljen podatak " << val.val << " adrese " << addr.val << " od "
                          << bus->getCurrentOwner() << ". Promena stanja I -> S\n";
                newLine.state.setS();
            }
        }
        else
        {
            if (bt == BusTransaction::READ)
            {
                std::cout << Simulation::cycleCount << " " << myCC << ": Primljen podatak " << val.val << " adrese " << addr.val << " od MEMC."
                          << " Promena stanja I -> E\n";
                newLine.state.setE();
            }
        }
        if (bt == BusTransaction::FLUSH)
        {
            if (pendingTransactionType == BusTransaction::READ)
            {
                std::cout << Simulation::cycleCount << " " << myCC << ": Primljen podatak " << val.val << " adrese " << addr.val << " od MEMC. I -> S\n";
                newLine.state.setS();
            }
            else if (pendingTransactionType == BusTransaction::WRITE)
            {
                std::cout << Simulation::cycleCount << " " << myCC << ": Pokupljen podatak sa magistrale. I -> M\n";
                newLine.state.setM();
            }
        }
        setCacheLine(addr, newLine);

        bufferForCPU = val;
        ready = true;

        std::cout << Simulation::cycleCount << " " << myCC << ": Podatak upisan u interni bafer kontrolera.\n";
    }
    // invalidacija
    else if (bt == BusTransaction::UPGRADE)
    {
        ready = true;
        // nema bafera, nema vracanja podataka cpu-u.
        std::cout << Simulation::cycleCount << " " << myCC << ": Sve invalidacije za adresu " << addr.val << " zavrsene (ack).\n";
        // promene stanja i upis su u CPU
    }
}

Data CacheController::retrieveData() const
{
    return bufferForCPU;
}

void CacheController::resetReady()
{
    ready = false;
}

void CacheController::startInternalRead(Address address)
{
    bufferForCPU = getCacheLine(address).data;
    busy = true;
    busyCycles = Latency::HIT; // TO DO: preimenuj ovo bez snoop - u ovom slucaju snoop se ne desava
    ready = false;
    // lokalni pogodak u kesu za citanje
    std::cout << Simulation::cycleCount << " " << myCC << ": Krenuo da citam podatak na adresi " << address.val << ".\n";
}

void CacheController::startInternalWrite(Address address, Data data)
{
    busy = true;
    busyCycles = Latency::HIT;
    ready = false;
    std::cout << Simulation::cycleCount << " " << myCC << ": Krenuo da pisem podatak " << data.val << " na adresu " << address.val << ".\n";
}

void CacheController::tick()
{
    if (busy)
    {
        // bez brojaca - flush
        if (bus->isAcknowledge())
        {
            if (bus->getCurrentSignal().type == BusTransaction::FLUSH)
            {
                // podatak dostavljen masteru
                // promeni stanje samo, podatak je pokupljen u recieveBusResponse()
                busy = false;
                Address addr = bus->getCurrentSignal().address;
                std::cout << Simulation::cycleCount << " " << myCC << " Flush gotov. " << getState(addr) << " -> " << pendingState << "\n";
                setState(pendingState, addr);
                ready = true;
                return;
            }
        }
        if (busyCycles > 0)
        {
            busyCycles--;
        }
        else
        {
            // stanje se menja nakon sto vreme istekne
            BusTransaction bt = bus->getCurrentSignal().type;
            if (bt == BusTransaction::READ || bt == BusTransaction::WRITE)
            {
                snoopingDone();
            }
            else if (bt == BusTransaction::IDLE)
            {
                // local read/write done
                localDone();
            }
        }
    }
    else
    {
        // za sada nista
        // kada nema cekanja, stanja se menjaju u metodi snoop
    }
}

void CacheController::snoopingDone()
{
    // VREME JE ISTEKLO: Stavljamo podatak na magistralu
    BusSignal busSignal = bus->getCurrentSignal();

    Address addr = busSignal.address;
    CacheState currentState = getState(addr);
    CacheState nextState = currentState;
    busSignal.data = getData(addr);

    // ovde promene stanja

    if (currentState == CacheState::E)
    {

        if (busSignal.type == BusTransaction::READ)
        {
            busSignal.type = BusTransaction::FLUSHOPT;
            nextState = CacheState::S;
            std::cout << Simulation::cycleCount << " " << myCC << ": FlushOpt završen nakon " << (unsigned int)Latency::REMOTE_CACHE << " ciklusa. ";
        }
        else if (busSignal.type == BusTransaction::WRITE)
        {
            busSignal.type = BusTransaction::FLUSHOPT;
            nextState = CacheState::I;
            std::cout << Simulation::cycleCount << " " << myCC << ": FlushOpt završen nakon " << (unsigned int)Latency::REMOTE_CACHE << " ciklusa. ";
        }
    }

    else if (currentState == CacheState::S)
    {
        if (busSignal.type == BusTransaction::READ)
        {
            busSignal.type = BusTransaction::FLUSHOPT;
            // ostaje u S
            std::cout << Simulation::cycleCount << " " << myCC << ": FlushOpt završen nakon " << (unsigned int)Latency::REMOTE_CACHE << " ciklusa. ";
        }
        else if (busSignal.type == BusTransaction::WRITE)
        {
            busSignal.type = BusTransaction::FLUSHOPT;
            nextState = CacheState::I;
            std::cout << Simulation::cycleCount << " " << myCC << ": FlushOpt završen nakon " << (unsigned int)Latency::REMOTE_CACHE << " ciklusa. ";
        }
    }

    std::cout << "Direktno saljem podatak kontroleru " << bus->getCurrentOwner() << ". Stanje podatka " << busSignal.data.val << " na adresi " << addr.val << " prelazi iz " << currentState << " u " << nextState << ". Nema pristupa RAM-u.\n";
    setState(nextState, addr);
    busy = false;
    bus->putDataOnBus(busSignal);
}

void CacheController::localDone()
{
    // SADA je prošao tačno jedan ciklus
    // ovde za pogodak prilikom pisanja kad nema magistrale
    std::cout << Simulation::cycleCount << " " << myCC << ": Zavrsen lokalni upis / citanje\n";
    ready = true;
    busy = false;
}

bool CacheController::isResponding()
{
    return busy;
}

void CacheController::stateOfCache()
{
    std::cout << Simulation::cycleCount << " " << myCC << ": Stanje kesa - Blok 0: " << cache[0] << "; Blok 1: "
              << cache[1] << "\n";
}

void CacheController::armMonitor(Address addr)
{
    monitoredAddress = addr;
    reservationBroken = false;
    std::cout << Simulation::cycleCount << " " << myCC << ": Postavljena rezervacija na adresu " << addr.val << ".\n";
}

bool CacheController::isReservationBroken() const
{
    return reservationBroken;
}
