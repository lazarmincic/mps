#ifndef CACHECONTROLLER_HPP
#define CACHECONTROLLER_HPP

#include "Bus.hpp"
#include "Common.hpp"

class Bus;

struct BusSignal;

class CacheController
{
private:
    CacheLine cache[2];
    Bus *bus;
    Controller myCC;
    bool ready;        // podatak dostupan
    Data bufferForCPU; // Privremeno čuvamo podatak koji je stigao sa magistrale
    // Holding register for data returning from the bus, preventing direct CPU-Bus coupling
    bool busy; // busy, u procesu dostavljanja podatka cache to cache, ne sme da se krene sa pisanjem dok ne zavrsi!
    int busyCycles;
    Address monitoredAddress;
    bool reservationBroken; // Zastavica koju procesor proverava
    BusTransaction pendingTransactionType;
    CacheState pendingState;
    std::stringstream msg; // za ispis

public:
    CacheController(Controller contr);

    void setBus(Bus *bus);
    void snoop(const BusSignal &signal, bool isWinner);

    void storeData(Address address, Data data);

    void readMiss(Address address); // pokuša da pristupi magistrali (magistrala može biti zauzeta)
    void writeMiss(Address address, Data data);

    bool isHit(Address address);
    Data getData(Address address);

    Controller getController() const;

    CacheState getState(Address address);

    void setState(CacheState state, Address address);

    void broadcastInvalidate(Address addr);

    bool isReady() const;

    void receiveBusResponse();

    Data retrieveData() const;

    void resetReady();

    void startInternalRead(Address address);

    void tick();

    void startInternalWrite(Address address, Data data);

    bool isResponding();

    void stateOfCache();

    void armMonitor(Address addr);

    bool isReservationBroken() const;

    void resetBusy();
    void ccLog(std::stringstream &ss, LogLevel level = maxLogLevel);

private:
    CacheLine getCacheLine(Address address);
    void setCacheLine(Address address, CacheLine cacheLine);
    void localDone();
};

#endif
