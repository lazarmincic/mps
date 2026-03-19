#include <iostream>
#include "CPU.hpp"
#include "CacheController.hpp"
#include "Bus.hpp"
#include "Parser.hpp"
#include "MemoryController.hpp"

void progIspis(ExecutableProgram prog);

// main pokreće jedan korak simulacije
int main()
{
    Bus bus;
    MemoryController memContr;

    // Kreiraj sve komponente
    CacheController cc0(0);
    CacheController cc1(1);
    CacheController cc2(2);
    CPU cpu0(&cc0);
    CPU cpu1(&cc1);
    CPU cpu2(&cc2);

    // Poveži sve
    bus.registerMemoryController(&memContr);
    bus.registerCacheController(0, &cc0);
    bus.registerCacheController(1, &cc1);
    bus.registerCacheController(2, &cc2);

    // CPU ima direktan pristup svom CC
    cpu0.setCacheController(&cc0);
    cpu1.setCacheController(&cc1);
    cpu2.setCacheController(&cc2);

    // CC ima pristup Bus-u
    cc0.setBus(&bus);
    cc1.setBus(&bus);
    cc2.setBus(&bus);

    // učitavanje programa u programsku memoriju odgovarajućeg procesora
    ExecutableProgram prog = Parser::parseFile("input.txt");
    // Debug ispis
    progIspis(prog);

    // isti program se izvršava na tri ista procesora (ne mora tako)
    cpu0.load(prog);
    cpu1.load(prog);
    cpu2.load(prog);

    // simulacija
    while (true)
    {
        // keš hit - kraj, nema interakcije sa magistralom
        // keš mis - šalje se signal na magistralu
        cpu0.step();
        cpu1.step();
        cpu2.step();

        bus.step(); // Snooping i sintetizovanje C signala
    }

    return 0;
}

void progIspis(ExecutableProgram prog)
{
    // Debug ispis
    std::cout << "Parsovano " << prog.instructions.size() << " instrukcija\n\n";

    for (size_t i = 0; i < prog.instructions.size(); i++)
    {
        const auto &instr = prog.instructions[i];
        std::cout << "Instrukcija " << i << ": " << instr.mnemonic << " [";

        // Ispis tipa instrukcije
        switch (instr.type)
        {
        case InstructionType::R:
            std::cout << "R-type";
            break;
        case InstructionType::I:
            std::cout << "I-type";
            break;
        case InstructionType::S:
            std::cout << "S-type";
            break;
        case InstructionType::B:
            std::cout << "B-type";
            break;
        case InstructionType::U:
            std::cout << "U-type";
            break;
        case InstructionType::J:
            std::cout << "J-type";
            break;
        case InstructionType::UNKNOWN:
            std::cout << "UNKNOWN";
            break;
        }

        std::cout << "]";

        if (!instr.rd.empty())
        {
            std::cout << " rd=" << instr.rd;
        }
        if (!instr.rs1.empty())
        {
            std::cout << " rs1=" << instr.rs1;
        }
        if (!instr.rs2.empty())
        {
            std::cout << " rs2=" << instr.rs2;
        }
        if (instr.imm != 0)
        {
            std::cout << " imm=" << instr.imm;
        }
        if (!instr.label.empty())
        {
            std::cout << " label=" << instr.label;
        }

        std::cout << "\n";
    }

    std::cout << "\nLabele:\n";
    for (const auto &[labelName, instrIdx] : prog.labelMap)
    {
        std::cout << "  " << labelName << " -> instrukcija " << instrIdx << "\n";
    }
}
