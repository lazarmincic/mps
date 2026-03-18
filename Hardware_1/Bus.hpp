#ifndef BUS_HPP
#define BUS_HPP

#include <cstdint>
#include <iostream>
#include <queue>

enum class BusTransaction { READ, WRITE, UPGRADE };

enum class BusPhase { ARBITRATION, REQUEST, RESPONSE, COMPLETION };

struct BusSignal {
    BusTransaction type;
    uint32_t address;
    int32_t data;
    int initiatorCPU;

    bool copiesExist; 
    bool hasData;

    bool hitInCache;
    bool shared;
    int32_t responseData;
    bool dataValid;
    bool abort;
};

class Bus {
private:
    BusSignal currentSignal;
    bool isBusy;
    int cycleCount;
    std::queue<int> waitQueue;
    int currentOwner;
    BusPhase currentPhase;
    
public:
    Bus();
    
    void requestBus(int cpuID);
    void emit(BusSignal sig);
    
    const BusSignal& getCurrentSignal() const { return currentSignal; }
    bool isBusFree() const { return !isBusy; }
    int getCurrentOwner() const { return currentOwner; }
    

    void step();
    
private:
    void grantBus();
    void broadcastRequest();
    void collectResponses();
    void releaseBus();
};

#endif
