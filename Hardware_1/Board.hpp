#ifndef BOARD_HPP
#define BOARD_HPP

#include "CPU.hpp"
#include "CacheController.hpp"
#include "Asembler.hpp"

class Board {
private:
    CacheController cacheController;
    CPU cpu;
public: 
    Board();

    void step ();
};

#endif

