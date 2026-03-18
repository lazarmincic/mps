// ova klasa transformiše pseudoinstrukcije u instrukcije koje riscv procesor može da razume
// ta konverzija se naziva ekspanzija pseudoinstrukcija
// dobija se mašinski kod koji se preko instruction loader-a učitava u programsku memoriju procesora  
// ja ću se zadržati na nivou teksta, tako će kod biti razumljiviji.. - I DALJE NE ZNAM

#include "Parser.hpp"


ExecutableProgram Parser::parseFile(std::string filename) {
        ExecutableProgram prog;
        std::ifstream file(filename);
        std::string line;
        int currentInstructionIndex = 0;

        while (std::getline(file, line)) {
            // 1. Čišćenje komentara i zareza
            size_t commentPos = line.find('#');
            if (commentPos != std::string::npos) line = line.substr(0, commentPos);
            std::replace(line.begin(), line.end(), ',', ' ');

            std::stringstream ss(line);
            std::string word;
            
            // 2. Provera da li linija počinje labelom (npr. "try_lock:")
            if (!(ss >> word)) continue; 

            if (word.back() == ':') {
                std::string labelName = word.substr(0, word.size() - 1);
                prog.labelMap[labelName] = currentInstructionIndex;
                
                // Proveri da li ima instrukcije u istom redu posle labele
                if (!(ss >> word)) continue; 
            }

            // 3. Ako smo ovde, 'word' je mnemonik instrukcije (npr. "lr", "addi")
            Instruction instr;
            instr.mnemonic = word;

            // Grubo parsiranje operanada na osnovu mnemonika
            if (instr.mnemonic == "addi") {
                instr.type = InstructionType::I;
                ss >> instr.rd >> instr.rs1 >> instr.imm;
            } 
            else if (instr.mnemonic == "ld") {
                // ld rd, offset(rs1)
                instr.type = InstructionType::I;
                std::string memOperand;
                ss >> instr.rd >> memOperand;
                parseMemoryOperand(memOperand, instr.imm, instr.rs1);
            }
            else if (instr.mnemonic == "st") {
                // st rs2, offset(rs1)
                instr.type = InstructionType::S;
                std::string memOperand;
                ss >> instr.rs2 >> memOperand;
                parseMemoryOperand(memOperand, instr.imm, instr.rs1);
            }
            else if (instr.mnemonic == "bne") {
                instr.type = InstructionType::B;
                ss >> instr.rs1 >> instr.rs2 >> instr.label;
            }
            else if (instr.mnemonic == "lr") {
                // lr rd, 0(rs1)
                instr.type = InstructionType::R;
                std::string memOperand;
                ss >> instr.rd >> memOperand; 
                parseMemoryOperand(memOperand, instr.imm, instr.rs1);
            }
            else if (instr.mnemonic == "sc") {
                // sc rd, rs2, offset(rs1)
                instr.type = InstructionType::R;
                std::string memOperand;
                ss >> instr.rd >> instr.rs2 >> memOperand;
                parseMemoryOperand(memOperand, instr.imm, instr.rs1);
            }
            // ... dodaj ostale po potrebi ...

            prog.instructions.push_back(instr);
            currentInstructionIndex++;
        }
        return prog;
    }

void Parser::parseMemoryOperand(std::string op, int32_t& imm, std::string& reg) {
    size_t openBracket = op.find('(');
    size_t closeBracket = op.find(')');
    if (openBracket != std::string::npos && closeBracket != std::string::npos) {
        imm = std::stoi(op.substr(0, openBracket));
        reg = op.substr(openBracket + 1, closeBracket - openBracket - 1);
    }
}
