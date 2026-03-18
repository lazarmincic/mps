#ifndef CPU_HPP
#define CPU_HPP

#include <vector>
#include <map>
#include <fstream>
#include <cstdint>
#include "CacheController.hpp"
#include "Parser.hpp"

class CPU {
private:
    uint32_t pc;
    // Ključ je ime registra (npr. "t0", "x1"), a vrednost je 32-bitni podatak
    std::map<std::string, int32_t> registers;
    uint32_t load_reservation_address;
    bool reservation_active;

// spoljne komponente (pokazivači - "povezani kablovima")
    CacheController* cacheController; // CPU ima pristup svom kontroleru

    
    std::vector<Instruction> program;
    std::map<std::string, int> labelMap;

    std::string cpuName;


public:
    CPU(CacheController* cc);

    void execute(Instruction& instr);

    void load(ExecutableProgram p);

    void step();

    void setName(std::string name) { cpuName = name; }

private:
    int findLabelIndex(std::string name);
};

#endif