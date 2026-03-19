#ifndef CPU_HPP
#define CPU_HPP

#include <vector>
#include <map>
#include <fstream>
#include <cstdint>
#include "CacheController.hpp"
#include "Parser.hpp"

class CacheController;

class CPU
{
private:
    uint32_t pc;
    // Ključ je ime registra (npr. "t0", "x1"), a vrednost je 32-bitni podatak
    std::map<std::string, int32_t> registers;
    uint32_t load_reservation_address;
    bool reservation_active;

    // spoljne komponente (pokazivači - "povezani kablovima")
    CacheController *cacheController; // CPU ima pristup svom kontroleru

    std::vector<Instruction> program;
    std::map<std::string, int> labelMap;

    std::string cpuName;

public:
    CPU(CacheController *cacheContr);

    void execute(Instruction &instr);

    void load(const ExecutableProgram &prog);

    void step();

    void setName(const std::string &name);

    void setCacheController(CacheController *cacheContr);

private:
    int findLabelIndex(const std::string &name);
};

#endif
