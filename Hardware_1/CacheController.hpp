#ifndef CACHECONTROLLER_HPP
#define CACHECONTROLLER_HPP

#include <cstdint>
#include <iostream>
#include "Bus.hpp"

class Bus;

// Enum za MESI stanja keš linije
enum class CacheState : uint8_t
{
    M,
    E,
    S,
    I
};

struct StateBits
{
    uint8_t modified : 1; // prljav (1) ili čist (0)
    uint8_t shared : 1;   // deljen (1) ili eksluzivan (0)

    // izuzetak je I stanje - nije prisutan u kešu ili linija prisutna ali nije validna (11)

    void setS()
    {
        modified = 0;
        shared = 1;
    }
    void setM()
    {
        modified = 1;
        shared = 0;
    }
    void setE()
    {
        modified = 0;
        shared = 0;
    }
    void setI()
    {
        modified = 1;
        shared = 1;
    }

    // Brze proveraške stanja
    bool isM() const { return modified == 1 && shared == 0; } // Modified
    bool isE() const { return modified == 0 && shared == 0; } // Exclusive
    bool isS() const { return modified == 0 && shared == 1; } // Shared
    bool isI() const { return modified == 1 && shared == 1; } // Invalid

    // Getter koji vraća enum reprezentaciju stanja
    CacheState getState() const
    {
        if (isM())
        {
            return CacheState::M;
        }
        if (isE())
        {
            return CacheState::E;
        }
        if (isS())
        {
            return CacheState::S;
        }
        return CacheState::I;
        return CacheState::I; // default
    };

    StateBits() : modified(1), shared(1) {} // konstrutor se poziva pri pravljenju strukture

    // sve linije počinju u I (11) stanju
};

struct CacheLine
{
    StateBits state; // stanje keš linije (MESI)
    int32_t data;    // 32-bitni podatak u kešu

    // Dekompozicija 32-bitne adrese:
    // [31:3] = tag (29 bita)
    // [2]    = index (1 bit) - bira jedan od 2 seta
    // [1:0]  = offset (2 bita) - pozicija unutar 32-bitne reči

    uint32_t tag : 29;  // viši bitovi adrese (bitovi 31:3)
    uint8_t index : 1;  // izbor seta (bit 2)
    uint8_t offset : 2; // offset unutar linije (bitovi 1:0)

    CacheLine() : data(0), tag(0), index(0), offset(0) {} // keš kreće "hladan"
};

struct BusSignal;

struct Address
{
    uint32_t val;
};
struct Data
{
    uint32_t val;
};

class CacheController
{
private:
    CacheLine cache[2];
    Bus *bus;
    int myID;

public:
    CacheController(int ccId);

    void setBus(Bus *bus);
    void onBusSignal(const BusSignal &signal);

    int32_t read(uint32_t address);
    void write(Address address, Data data);
    // Ostalim CPU-ima korisno da znaju
    bool hasLine(uint32_t address); // Da li ja imam tu liniju?

private:
    int findLineInCache(uint32_t address);
};

#endif
