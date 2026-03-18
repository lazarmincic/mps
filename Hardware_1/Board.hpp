#ifndef BOARD_HPP
#define BOARD_HPP

#include "CPU.hpp"
#include "CacheController.hpp"
#include "Bus.hpp"
#include "Parser.hpp"

class Board {
private:
    Bus bus;
    CacheController cc0, cc1, cc2;
    CPU p0, p1, p2;

public:
    Board();
    void step();
};

#endif

