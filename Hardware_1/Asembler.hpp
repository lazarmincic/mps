#ifndef ASEMBLER_HPP
#define ASEMBLER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>

#include "CPU.hpp"

class Asembler {
private:

    static uint8_t regToNum(std::string reg);

public:
    static std::vector<Instruction> parseAssembler(std::string filename) ;
};

#endif