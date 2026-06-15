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
    msg << "Na mrezi.";
    ccLog(msg);
}

CacheLine CacheController::getCacheLine(Address address)
{
    return cache[address.val];
}

bool CacheController::isHit(Address address)
{
    CacheLine cacheLine = getCacheLine(address);
    return (cacheLine.state != CacheState::I);
}

Data CacheController::getData(Address address)
{
    return getCacheLine(address).data;
}

CacheState CacheController::getState(Address address)
{
    return getCacheLine(address).state;
}

void CacheController::setCacheLine(Address address, CacheLine cacheLine)
{
    cache[address.val] = cacheLine;
    stateOfCache();
}

void CacheController::setState(CacheState state, Address address)
{
    CacheLine toChange = getCacheLine(address);
    toChange.state = state;
    setCacheLine(address, toChange);
}

void CacheController::storeData(Address address, Data data)
{
    // keš hit
    cache[address.val].data = data;
    cache[address.val].state = CacheState::M; // Nakon upisa, linija je Modified // bice eksplicitno uradjeno svakako
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
    msg << "Presretnut signal " << signal.type << " sa magistrale.";
    ccLog(msg);
    if (isHit(addr))
    {
        msg << "Snoop pogodak na adresi " << addr << ", stanje " << getState(addr) << ", podatak " << getData(addr) << ".";
        ccLog(msg);
        // snupovana linija JESTE keširana
        // dakle E, M, ili S
        BusTransaction bt = bus->getCurrentSignal().type;
        CacheState currentState = getState(addr);
        pendingState = currentState;
        Data data = getData(addr);
        bool flushopt = false;
        busy = false;
        // novi signal za magistralu

        if (currentState == CacheState::E)
        {
            if (bt == BusTransaction::READ)
            {
                pendingState = CacheState::S;
                msg << "Prelazim u medjustanje ES.";
                ccLog(msg);
            }
            else if (bt == BusTransaction::WRITE)
            {
                pendingState = CacheState::I;
                msg << "Prelazim u medjustanje EI.";
                ccLog(msg);
            }
            // E - on mora dati podatak
            flushopt = true;
        }
        else if (currentState == CacheState::S)
        {

            if (bt == BusTransaction::READ)
            {
                // ostaje u S
                msg << "Ostajem u stanju S.";
                ccLog(msg);
                if (isWinner)
                {
                    // pobedio unutrasnju arbitrazu
                    flushopt = true;
                }
                else
                {
                    // inace ne radi flushopt
                }
            }
            else if (bt == BusTransaction::WRITE)
            {
                if (isWinner)
                {
                    // pobedio unutrasnju arbitrazu
                    flushopt = true;
                    msg << "Prelazim u medjustanje SI.";
                    ccLog(msg);
                    // jer flushopt treba vremena da se izvrsi
                    pendingState = CacheState::I;
                }
                else
                {
                    // inace ne radi flushopt
                    pendingState = CacheState::I;
                    msg << "Izgubio sam unutrasnju arbitrazu. Necu slati podatak, samo prelazim u stanje I.";
                    ccLog(msg);
                    setState(CacheState::I, addr);
                    // moze odmah jer nema magistrale
                }
            }
            else if (bt == BusTransaction::UPGRADE)
            {
                msg << "Prelazim u medjustanje SI.";
                ccLog(msg);
                pendingState = CacheState::I;
                busy = true;
                bus->upgrade();
                // nista na magistralu
                // busy mora true jer je kontroler zakocen dok se ne zavrsi upgrade
            }
        }
        else if (currentState == CacheState::M)
        {
            if (signal.type == BusTransaction::READ || signal.type == BusTransaction::WRITE)

            {

                // address je vec na magistrali
                // nextState = CacheState::S;
                msg << "Flush-ujem blok " << getData(addr) << " na adresi " << addr << " na magistralu.";
                ccLog(msg);
                bus->flush({getData(addr)});
                busy = true;
                if (signal.type == BusTransaction::READ)
                {
                    pendingState = CacheState::S;
                    msg << "Prelazim u medjustanje MS.";
                    ccLog(msg);
                }
                else if (signal.type == BusTransaction::WRITE)
                {
                    pendingState = CacheState::I;
                    msg << "Prelazim u medjustanje MI.";
                    ccLog(msg);
                    // ne prelazi zapravo, vec pamti stanje u koje treba da predje..
                }
                // promene stanja nakon sto se zavrsi upis u mem
            }
        }

        if (flushopt)
        {
            busy = true;
            bus->flushOpt(data);
            msg << "Direktno saljem podatak kontroleru " << bus->getCurrentOwner() << ".";
            ccLog(msg);
        }
    }
    else
    {
        // I stanje, jer blok ne postoji u kesu
        // nista na magistralu i ostaje I
        busy = false;
        msg << "Blok u stanju I. Snoop promasaj. Nema promene stanja. Nista na magistralu.";
        ccLog(msg);
        return;
    }
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

    if (bt != BusTransaction::IDLE && bt != BusTransaction::UPGRADE)
    {
        // citanje
        // iz Data u CacheLine konverzija potrebna.

        Data data = bus->getCurrentSignal().data;
        CacheLine newLine;
        newLine.data = data;
        if (bus->sharedLineActive()) // gleda se shared linija
        {
            if (bt == BusTransaction::FLUSHOPT)
            {
                msg << "Primljen podatak " << data << " adrese " << addr << " od "
                    << bus->getCurrentOwner() << ". Promena stanja iz I u S.";
                ccLog(msg);
                newLine.state = CacheState::S;
            }
        }
        else
        {
            if (bt == BusTransaction::READ)
            {
                msg << "Primljen podatak " << data << " adrese " << addr << " od MEMC." << " Promena stanja iz I u E.";
                ccLog(msg);
                newLine.state = CacheState::E;
            }
            else if (bt == BusTransaction::WRITE)
            {
                msg << "Primljen podatak " << data << " adrese " << addr << " od MEMC. Promena stanja iz I u M.";
                ccLog(msg);
                newLine.state = CacheState::M;
            }
        }
        if (bt == BusTransaction::FLUSH)
        {
            if (pendingTransactionType == BusTransaction::READ)
            {
                msg << "Primljen podatak " << data << " adrese " << addr << " od " << bus->getWinner() << ". Promena stanja iz I u S.";
                ccLog(msg);
                newLine.state = CacheState::S;
            }
            else if (pendingTransactionType == BusTransaction::WRITE)
            {
                msg << "Primljen podatak " << data << " adrese " << addr << " od " << bus->getWinner() << ". Promena stanja iz I u M.";
                ccLog(msg);
                newLine.state = CacheState::M;
            }
        }
        setCacheLine(addr, newLine);

        bufferForCPU = data;
        ready = true;

        msg << "Podatak upisan u interni bafer kontrolera.";
        ccLog(msg);
    }
    // invalidacija
    else if (bt == BusTransaction::UPGRADE)
    {
        ready = true;
        // nema bafera, nema vracanja podataka cpu-u.
        msg << "Sve invalidacije za adresu " << addr << " zavrsene (ack).";
        ccLog(msg);
        // promene stanja i upis su u CPU
        // to do upis i promena stanja u M premesti ga ovde
        busy = false;
    }
    else
    {
        msg << "Greska";
        ccLog(msg, ERROR);
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
    bufferForCPU = getCacheLine(address).data; // kasnije
    busy = true;
    busyCycles = Latency::HIT; // TO DO: preimenuj ovo bez snoop - u ovom slucaju snoop se ne desava
    ready = false;
    // lokalni pogodak u kesu za citanje
    msg << "Krenuo da citam podatak na adresi " << address << ".";
    ccLog(msg);
}

void CacheController::startInternalWrite(Address address, Data data)
{
    msg << "Krenuo da pisem podatak " << data << " na adresu " << address << ".";
    ccLog(msg);
    // buffer nema smisla za pisanje
    busy = true;
    busyCycles = Latency::HIT;
    ready = false;
}

void CacheController::tick()
{
    if (busy)
    {
        // bez brojaca - flush / flush opt
        if (bus->isAcknowledge())
        {
            if (bus->getCurrentSignal().type == BusTransaction::FLUSH)
            {
                if (bus->getWinner() == myCC)
                {
                    // podatak dostavljen masteru
                    // master podatak kupi u recieveBusResponse()
                    // ovo je slave, on samo menja stanje
                    busy = false;
                    Address addr = bus->getCurrentSignal().address;
                    CacheState currentState = getState(addr);
                    msg << "Flush gotov.";
                    ccLog(msg);
                    if (currentState != pendingState)
                    {
                        msg << "Menjam stanje iz " << currentState << " u " << pendingState << ".";
                        ccLog(msg);
                        setState(pendingState, addr);
                    }
                    else
                    {
                        msg << "Ostajem u stanju " << currentState << ".";
                        ccLog(msg);
                    }
                    ready = true;
                    return;
                }
            }
            else if (bus->getCurrentSignal().type == BusTransaction::FLUSHOPT)
            {
                if (bus->getWinner() == myCC)
                {
                    busy = false;
                    Address addr = bus->getCurrentSignal().address;
                    CacheState currentState = getState(addr);
                    msg << "Flush-opt gotov.";
                    ccLog(msg);
                    if (currentState != pendingState)
                    {
                        msg << "Blok na adresi " << addr << " menja stanje iz " << currentState << " u " << pendingState << ".";
                        ccLog(msg);
                        setState(pendingState, addr);
                    }
                    else
                    {
                        msg << "Blok na adresi " << addr << " ostaje u stanju " << currentState << ".";
                        ccLog(msg);
                    }
                    ready = true;
                    return;
                }
            }
            else if (bus->getCurrentSignal().type == BusTransaction::UPGRADE)
            {
                // kako znati da je to bas on?
                Address addr = bus->getCurrentSignal().address;
                CacheState state = getState(addr);
                Controller master = bus->getCurrentOwner();
                if (state == CacheState::S && myCC != master) // nisam ja taj koji je master na magistrali (slao upgrade)
                {
                    msg << "Upgrade gotov.";
                    ccLog(msg);
                    // to je on, ima podatak na toj adresi u s i desava se upgrade
                    busy = false;
                    if (state != pendingState)
                    {
                        msg << "Blok na adresi " << addr << " menja stanje iz " << state << " u " << pendingState << ".";
                        ccLog(msg);
                        setState(pendingState, addr);
                    }
                    else
                    {
                        msg << "Blok na adresi " << addr << " ostaje u stanju " << state << ".";
                        ccLog(msg);
                    }

                    ready = true;
                    // DODATAK ZA LR/SC:
                    if (monitoredAddress.val == addr.val)
                    {

                        reservationBroken = true; // Samo podigni zastavicu
                        // ukoliko je trazio magistralu, ponistava se jer mu ona vise nije potrebna, sc moze da se izvrsi sada i bez nje..
                        bus->dropBusRequest(myCC);
                        msg << "Odustajem od pristupa magistrali.";
                        ccLog(msg);
                    }

                    return;
                }
            }
            else if (bus->getCurrentSignal().type == BusTransaction::WRITE)
            {
                // PROVERI DA LI JE NARUSENA REZERVACIJA TO DO
                msg << "Nikad se nece desiti za ovaj program";
                ccLog(msg, ERROR);
            }
        }
        if (busyCycles > 0)
        {
            busyCycles--; // local hit - tada je uzrok kasnjenja kes
            if (busyCycles == 0)
            {
                // stanje se menja nakon sto vreme istekne
                // local read/write done
                localDone();
            }
        }
        else
        {
        }
    }
    else
    {
        // za sada nista
        // kada nema cekanja, stanja se menjaju u metodi snoop
    }
}

void CacheController::localDone()
{
    // SADA je prošao tačno jedan ciklus
    // ovde za pogodak prilikom pisanja kad nema magistrale
    msg << "Zavrsen lokalni upis / citanje";
    ccLog(msg);
    ready = true;
    busy = false;
}

bool CacheController::isResponding()
{
    return busy;
}

void CacheController::stateOfCache()
{
    msg << "Stanje kesa - Blok 0: " << cache[0] << "; Blok 1: " << cache[1];
    ccLog(msg);
}

void CacheController::armMonitor(Address addr)
{
    monitoredAddress = addr;
    reservationBroken = false;
    msg << "Postavljena rezervacija na adresu " << addr << ".";
    ccLog(msg);
}

bool CacheController::isReservationBroken() const
{
    return reservationBroken;
}

void CacheController::resetBusy()
{
    busy = false;
}

void CacheController::ccLog(std::stringstream &ss, LogLevel level)
{
    ::log(ss, toString(myCC), level);
}
