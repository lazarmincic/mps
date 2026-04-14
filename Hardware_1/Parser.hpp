#ifndef PARSER_H
#define PARSER_H

#include "Common.hpp"

class Parser
{
public:
    static ExecutableProgram parseFile(const std::string &filename);

private:
    static void parseMemoryOperand(const std::string &operand, Data &imm, std::string &reg);
};

#endif
