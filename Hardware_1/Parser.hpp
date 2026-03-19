#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <algorithm>

enum class InstructionType : uint8_t
{
    R,
    I,
    S,
    B,
    U,
    J,
    UNKNOWN
};

struct Instruction
{
    std::string mnemonic; // npr. "addi", "lr", "bne"
    InstructionType type; // Tip instrukcije

    // Apstrahovani operandi (koriste se po potrebi)
    std::string rd;    // Odredišni registar
    std::string rs1;   // Prvi izvorni registar
    std::string rs2;   // Drugi izvorni registar
    int32_t imm;       // Immediate vrednost (offset ili konstanta)
    std::string label; // Labela za skokove (npr. "try_lock")

    Instruction() : type(InstructionType::UNKNOWN), imm(0) {}
};

struct ExecutableProgram
{
    std::vector<Instruction> instructions;
    std::map<std::string, int> labelMap;
};

class Parser
{
public:
    static ExecutableProgram parseFile(const std::string &filename);

private:
    static void parseMemoryOperand(const std::string &operand, int32_t &imm, std::string &reg);
};

#endif
