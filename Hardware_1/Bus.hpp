#ifndef BUS_HPP
#define BUS_HPP

#include "MemoryController.hpp"
#include "CacheController.hpp"
#include "Common.hpp"

class CacheController;

class MemoryController;

class Bus
{
private:
    std::map<Controller, BusSignal> activeSignals;
    Controller lastWinner;
    bool isBusy;
    Controller currentOwner;

    BusSignal currentSignal;
    bool sharedLine;

    CacheController *cacheControllers[3];
    MemoryController *memoryController;

    bool canCollect;

public:
    Bus();

    void registerMemoryController(MemoryController *memContr);
    void registerCacheController(int cpuID, CacheController *cacheContr);

    void requestBus(Controller devicePosting, const BusSignal &signal);

    void pullSharedLine();

    const BusSignal &getCurrentSignal() const;
    bool isBusFree() const;
    Controller getCurrentOwner() const;
    bool sharedLineActive() const;
    void resetSharedLine();

    void tick();

    void putDataOnBus(const BusSignal &response);
    void acknowledgeResponse();

    void dropBusRequest(Controller controller);

    void flush(Data data);

    bool isAcknowledge();

private:
    void grantBus();
    void broadcastRequest();
    void collectResponses();
    void releaseBus();
};

#endif
