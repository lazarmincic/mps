#ifndef CACHECONTROLLER_HPP
#define CACHECONTROLLER_HPP

#include <cstdint>
#include <iostream>

struct StateBits {
    uint8_t modified : 1; // prljav (1) ili čist (0)
    uint8_t shared   : 1; // deljen (1) ili eksluzivan (0)
    
    // izuzetak je I stanje - nije prisutan u kešu ili linija prisutna ali nije validna (11)

    void setS() { modified = 0; shared = 1; }
    void setM() { modified = 1; shared = 0; }
    void setE() { modified = 0; shared = 0; }
    void setI() { modified = 1; shared = 1; }

    StateBits() : modified(1), shared(1) {} // konstrutor se poziva pri pravljenju strukture

    // sve linije počinju u I (11) stanju
};

struct CacheLine {
    StateBits state; // stanje keš linije
    uint8_t tag      : 1; // viši bit adrese
    uint8_t index    : 1; // niži bit adrese
    uint8_t data     : 2; // podatak u kešu

    CacheLine() : state(), tag(0), index(0), data(0) {}  // keš kreće "hladan"
};

class CacheController {  
private:
    CacheLine cache[2]; // dva seta, dvobitni podatak
public:
    CacheController();
};

#endif