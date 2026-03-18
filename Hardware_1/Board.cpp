#include "Board.hpp"
#include <iostream>

Board::Board() : cc0(0), cc1(1), cc2(2), p0(&cc0), p1(&cc1), p2(&cc2) {
    // Konekcija sa magistralom
    cc0.setBus(&bus);
    cc1.setBus(&bus);
    cc2.setBus(&bus);

    // Setuj imena procesora
    p0.setName("Core0");
    p1.setName("Core1");
    p2.setName("Core2");
}

// Funkcija koja pokreće jedan korak simulacije
void Board::step (){
    // učitavanje programa u programsku memoriju odgovarajućeg procesora
    ExecutableProgram prog = Parser::parseFile("input.txt");

    // Debug ispis
    std::cout << "Parsovano " << prog.instructions.size() << " instrukcija\n\n";
    
    for (size_t i = 0; i < prog.instructions.size(); i++) {
        const auto& instr = prog.instructions[i];
        std::cout << "Instrukcija " << i << ": " << instr.mnemonic << " [";
        
        // Ispis tipa instrukcije
        switch (instr.type) {
            case InstructionType::R: std::cout << "R-type"; break;
            case InstructionType::I: std::cout << "I-type"; break;
            case InstructionType::S: std::cout << "S-type"; break;
            case InstructionType::B: std::cout << "B-type"; break;
            case InstructionType::U: std::cout << "U-type"; break;
            case InstructionType::J: std::cout << "J-type"; break;
            case InstructionType::UNKNOWN: std::cout << "UNKNOWN"; break;
        }
        
        std::cout << "]";
        
        if (!instr.rd.empty()) std::cout << " rd=" << instr.rd;
        if (!instr.rs1.empty()) std::cout << " rs1=" << instr.rs1;
        if (!instr.rs2.empty()) std::cout << " rs2=" << instr.rs2;
        if (instr.imm != 0) std::cout << " imm=" << instr.imm;
        if (!instr.label.empty()) std::cout << " label=" << instr.label;
        
        std::cout << "\n";
    }
    
    std::cout << "\nLabele:\n";
    for (const auto& [labelName, instrIdx] : prog.labelMap) {
        std::cout << "  " << labelName << " -> instrukcija " << instrIdx << "\n";
    }

    // isti program se izvršava na tri ista procesora (ne mora tako)
    p0.load(prog);
    p1.load(prog);
    p2.load(prog);

    // keš hit - kraj, nema interakcije sa magistralom
    // keš mis - šalje se signal na magistralu
    p0.step();
    p1.step();
    p2.step();
    
    // Bus snooping - svi keš kontroleri slušaju magistralu
    cc0.onBusSignal(bus.getCurrentSignal());
    cc1.onBusSignal(bus.getCurrentSignal());
    cc2.onBusSignal(bus.getCurrentSignal());
    
    // Bus faza
    bool copiesExist = false;
}

int main() {
    Board board;
    board.step();
    return 0;
}


