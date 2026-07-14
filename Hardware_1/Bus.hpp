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

    int busyCycles;
    bool canCollect;

    BusSignal pendingResponse; // za flushopt i flush
    Controller winner;         // unutrasnja arb
    std::string busName;       // za ispis

    std::stringstream msg;
    uint64_t transNum;

public:
    Bus(std::string name);

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
    void flushOpt(Data data);
    void upgrade();

    bool isAcknowledge();
    Controller getWinner() const;

    void statIspis();

private:
    void grantBus();
    void broadcastRequest();
    void collectResponses();
    void releaseBus();
    void busLog(std::stringstream &ss, LogLevel level = maxLogLevel);
};

#endif
