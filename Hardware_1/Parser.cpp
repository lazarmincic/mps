// ova klasa transformiše pseudoinstrukcije u instrukcije koje riscv procesor može da razume (mašinske instrukcije)
// ta konverzija se naziva ekspanzija pseudoinstrukcija
// dobijaju se mašinske instrukcije koji se preko instruction loader-a učitava u programsku memoriju procesora (klasa CPU)
// ja ću se zadržati na nivou teksta (umesto mašinskog koda, tj nula i jedinica), tako će kod biti razumljiviji i lakši za debug

#include "Parser.hpp"

ExecutableProgram Parser::parseFile(const std::string &filename)
{
    /*
    Uzima path do ulaznog tekstualnog fajla sa pseudoinstrukcijama
    Izlaz su instrukcije učitane u strukturu ExecutableProgram, koju klasa CPU direktno može da povuče i izvrši
    */
    std::stringstream msg;
    msg << "Parsiram datoteku " << filename;
    log(msg, "SYS");
    ExecutableProgram prog;
    fs::path p = filename;
    prog.programName = p.stem().string(); // bez ekstenzije

    std::ifstream file(filename);
    std::string line;
    int currentInstructionIndex = 0;

    while (std::getline(file, line))
    {
        // 1. Čišćenje komentara i zareza
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
        {
            line = line.substr(0, commentPos);
        }
        std::replace(line.begin(), line.end(), ',', ' ');

        std::stringstream sstream(line);
        std::string word;

        // 2. Provera da li linija počinje labelom (npr. "try_lock:")
        if (!(sstream >> word))
        {
            continue;
        }

        if (word.back() == ':')
        {
            std::string labelName = word.substr(0, word.size() - 1);
            prog.labelMap[labelName] = currentInstructionIndex;

            // Proveri da li ima instrukcije u istom redu posle labele
            if (!(sstream >> word))
            {
                continue;
            }
        }

        // 3. Ako smo ovde, 'word' je mnemonik instrukcije (npr. "lr", "addi")
        Instruction instr;
        instr.mnemonic = word;

        // Grubo parsiranje operanada na osnovu mnemonika
        if (instr.mnemonic == "addi")
        {
            instr.type = InstructionType::I;
            sstream >> instr.rd >> instr.rs1 >> instr.imm.val;
        }
        else if (instr.mnemonic == "ld")
        {
            // ld rd, offset(rs1)
            instr.type = InstructionType::I;
            std::string memOperand;
            sstream >> instr.rd >> memOperand;
            parseMemoryOperand(memOperand, instr.imm, instr.rs1);
        }
        else if (instr.mnemonic == "st")
        {
            // st rs2, offset(rs1)
            instr.type = InstructionType::S;
            std::string memOperand;
            sstream >> instr.rs2 >> memOperand;
            parseMemoryOperand(memOperand, instr.imm, instr.rs1);
        }
        else if (instr.mnemonic == "bne")
        {
            instr.type = InstructionType::B;
            sstream >> instr.rs1 >> instr.rs2 >> instr.label;
        }
        else if (instr.mnemonic == "lr")
        {
            // lr rd, 0(rs1)
            instr.type = InstructionType::R;
            std::string memOperand;
            sstream >> instr.rd >> memOperand;
            parseMemoryOperand(memOperand, instr.imm, instr.rs1);
        }
        else if (instr.mnemonic == "sc")
        {
            // sc rd, rs2, offset(rs1)
            instr.type = InstructionType::R;
            std::string memOperand;
            sstream >> instr.rd >> instr.rs2 >> memOperand;
            parseMemoryOperand(memOperand, instr.imm, instr.rs1);
        }
        // ... dodaj ostale po potrebi ...

        prog.instructions.push_back(instr);
        currentInstructionIndex++;
    }
    return prog;
}

void Parser::parseMemoryOperand(const std::string &operand, Data &imm, std::string &reg)
/*
Pomoćna privatna metoda za parsiranje memorijskih instrukcija
*/
{

    size_t openBracket = operand.find('(');
    size_t closeBracket = operand.find(')');
    if (openBracket != std::string::npos && closeBracket != std::string::npos)
    {
        imm = {std::stoi(operand.substr(0, openBracket))};
        reg = operand.substr(openBracket + 1, closeBracket - openBracket - 1);
    }
}
