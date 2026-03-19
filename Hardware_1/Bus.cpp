#include "Bus.hpp"

Bus::Bus() : isBusy(false), currentOwner(-1), cycleCount(0), currentPhase(BusPhase::ARBITRATION)
{
}

void Bus::requestBus(int cpuID)
{
    if (isBusy)
    {
        waitQueue.push(cpuID);
    }
    else
    {
        currentOwner = cpuID;
        isBusy = true;
    }
}

void Bus::emit(BusSignal sig)
{
    currentSignal = sig;
    std::cout << "[BUS] Emituje signal od CPU" << sig.initiatorCPU << "\n";
}

void Bus::step()
{
    switch (currentPhase)
    {
    case BusPhase::ARBITRATION:
        grantBus();
        break;
    case BusPhase::REQUEST:
        broadcastRequest();
        break;
    case BusPhase::RESPONSE:
        collectResponses();
        break;
    case BusPhase::COMPLETION:
        releaseBus();
        break;
    }
}

void Bus::grantBus()
{
    if (!isBusy && !waitQueue.empty())
    {
        currentOwner = waitQueue.front();
        waitQueue.pop();
        isBusy = true;
        std::cout << "[BUS] CPU" << currentOwner << " dobija pristup\n";
        currentPhase = BusPhase::REQUEST;
    }
}

void Bus::broadcastRequest()
{
    std::cout << "[BUS] REQUEST: CPU" << currentOwner << " emituje zahtev\n";
    currentPhase = BusPhase::RESPONSE;
}

void Bus::collectResponses()
{
    std::cout << "[BUS] RESPONSE: Prikupljam odgovore...\n";
    currentPhase = BusPhase::COMPLETION;
}

void Bus::releaseBus()
{
    std::cout << "[BUS] COMPLETION: CPU" << currentOwner << " završio\n";
    isBusy = false;
    currentOwner = -1;
    currentPhase = BusPhase::ARBITRATION;
}

void Bus::registerMemoryController(MemoryController *memContr)
{
    memoryController = memContr;
}

int32_t Bus::readMiss(uint32_t address, int cpuID)
{
    // 1. Emituj signal
    BusSignal signal;
    signal.type = BusTransaction::READ;
    signal.address = address;
    signal.initiatorCPU = cpuID;
    emit(signal);

    // 2. Ostali CC-evi sluškaju (snoop)
    for (int i = 0; i < 3; i++)
    {
        if (i != cpuID)
        {
            cacheControllers[i]->onBusSignal(signal);
        }
    }

    // 3. Sintetizuj C signal
    bool copies = hasLineInOtherCaches(address, cpuID);
    signal.copiesExist = copies;

    // 4. Učitaj iz memorije ako C=LOW
    int32_t data = memoryController->read(address);

    return data;
}

bool Bus::hasLineInOtherCaches(uint32_t address, int cpuID)
{
    for (int i = 0; i < 3; i++)
    {
        if (i != cpuID && cacheControllers[i]->hasLine(address))
        {
            return true; // C = HIGH
        }
    }
    return false; // C = LOW
}

void Bus::registerCacheController(int cpuID, CacheController *cacheContr)
{
    cacheControllers[cpuID] = cacheContr;
}
