// ova klasa transformiše pseudoinstrukcije u instrukcije koje riscv procesor može da razume
// ta konverzija se naziva ekspanzija pseudoinstrukcija
// dobija se mašinski kod koji se preko instruction loader-a učitava u programsku memoriju procesora  
// ja ću se zadržati na nivou teksta, tako će kod biti razumljiviji.. - I DALJE NE ZNAM

#include "Asembler.hpp"



    uint8_t Asembler::regToNum(std::string reg) {
        if (reg == "x0" || reg == "zero") return 0;
        if (reg == "t0") return 5;
        if (reg == "t1") return 6;
        // ... mogu se dodati ostali po potrebi
        return 0;
    }


    std::vector<Instruction> Asembler::parseAssembler(std::string filename) {
        std::ifstream file(filename);
        std::string line;
        std::vector<std::string> instructions;
        std::map<std::string, uint32_t> labels;
        uint32_t currentAddr = 0;

        // --- PRVI PROLAZ: Pronalaženje labela ---
        while (getline(file, line)) {
            // Očisti razmake i zareze
            line.erase(std::remove(line.begin(), line.end(), ','), line.end());
            std::stringstream ss(line);
            std::string word;
            ss >> word;

            if (word.empty()) continue;

            // Ako je labela (završava se sa :)
            if (word.back() == ':') {
                word.pop_back();
                labels[word] = currentAddr;
                if (!(ss >> word)) continue; // Uzmi instrukciju nakon labele ako postoji
            }
            
            instructions.push_back(line); // Čuvamo celu očišćenu liniju
            currentAddr += 4;
        }

        // --- DRUGI PROLAZ: Generisanje binarnog koda ---
        std::vector<Instruction> binaryProgram;
        currentAddr = 0;

        for (auto& rawLine : instructions) {
            Instruction instr;
            instr.raw = 0; // Reset

            std::stringstream ss(rawLine);
            std::string op, rd_s, rs1_s, rs2_s;

            ss >> op;
            if (op.back() == ':') ss >> op; // Preskoči labelu ako je u istoj liniji

            if (op == "addi") {
                ss >> rd_s >> rs1_s >> rs2_s;
                instr.i_type.opcode = 0x13;
                instr.i_type.rd = regToNum(rd_s);
                instr.i_type.rs1 = regToNum(rs1_s);
                instr.i_type.funct3 = 0x0;
                instr.i_type.imm11_0 = std::stoi(rs2_s);
            }
            else if (op == "bne") {
                ss >> rs1_s >> rs2_s >> rd_s; // rd_s je ovde labela "try_lock"
                instr.b_type.opcode = 0x63;
                instr.b_type.funct3 = 0x1;
                instr.b_type.rs1 = regToNum(rs1_s);
                instr.b_type.rs2 = regToNum(rs2_s);
                
                // Izračunavanje offseta (cilj - trenutni PC)
                int32_t offset = labels[rd_s] - currentAddr;
                // RISC-V B-type pakuje bite na specifičan način (skraćeno ovde)
                instr.b_type.imm11 = (offset >> 11) & 0x1;
                instr.b_type.imm4_1 = (offset >> 1) & 0xF;
                instr.b_type.imm10_5 = (offset >> 5) & 0x3F;
                instr.b_type.imm12 = (offset >> 12) & 0x1;
            }
            else if (op == "lr") {
                // lr t0, 0(x0) -> format: lr rd, offset(rs1)
                // Hardverski: R-format, funct7 = 0x02
                std::string temp;
                ss >> rd_s >> temp; // temp je "0(x0)"
                instr.r_type.opcode = 0x2F;
                instr.r_type.rd = regToNum(rd_s);
                instr.r_type.rs1 = regToNum(temp.substr(temp.find('(')+1, 2));
                instr.r_type.funct3 = 0x2;
                instr.r_type.funct7 = 0x02 << 2; 
            }
            else if (op == "sc") {
                // Format: sc rd, rs2, 0(rs1)
                // Primer: sc t1, t1, 0(x0) -> rd=t1, rs2=t1, rs1=x0
                std::string temp;
                ss >> rd_s >> rs2_s >> temp; 
                instr.r_type.opcode = 0x2F; // AMO opcode
                instr.r_type.rd = regToNum(rd_s);
                instr.r_type.rs2 = regToNum(rs2_s);
                instr.r_type.rs1 = regToNum(temp.substr(temp.find('(')+1, 2));
                instr.r_type.funct3 = 0x2;      // 32-bit
                instr.r_type.funct7 = 0x03 << 2; // 0x03 je funkcija za SC
            }
            else if (op == "ld") {
                // Format: ld rd, offset(rs1) -> I-tip
                // Primer: ld t1, 0(t0)
                std::string temp;
                ss >> rd_s >> temp;
                instr.i_type.opcode = 0x03; // Load opcode
                instr.i_type.rd = regToNum(rd_s);
                instr.i_type.funct3 = 0x3; // 64-bit (ili 0x2 za 32-bitni lw)
                
                size_t bracket = temp.find('(');
                instr.i_type.imm11_0 = std::stoi(temp.substr(0, bracket));
                instr.i_type.rs1 = regToNum(temp.substr(bracket + 1, 2));
            }
            else if (op == "st") {
                // Format: st rs2, offset(rs1) -> S-tip
                // Primer: st t1, 0(t0)
                std::string temp;
                ss >> rs2_s >> temp;
                instr.s_type.opcode = 0x23; // Store opcode
                instr.s_type.rs2 = regToNum(rs2_s);
                instr.s_type.funct3 = 0x3; // sd (64-bit) ili 0x2 za sw
                
                size_t bracket = temp.find('(');
                int32_t offset = std::stoi(temp.substr(0, bracket));
                instr.s_type.rs1 = regToNum(temp.substr(bracket + 1, 2));

                // PAKOVANJE OFFSET-A ZA S-TIP (Hardversko sečenje)
                instr.s_type.imm4_0 = offset & 0x1F;        // Donjih 5 bitova (0-4)
                instr.s_type.imm11_5 = (offset >> 5) & 0x7F; // Gornjih 7 bitova (5-11)
            }

            binaryProgram.push_back(instr);
            currentAddr += 4;
        }
        return binaryProgram;
    }
