

#include "CPU.hpp"

// procesor dobija ulaznu datoteku sa asembler kodom
// neću modelovati hardver riscv procesora ovde
// fokus je na hardveru za koherenciju keša
// tri tipa instrukcija
// 1. grananja - na visokom nivou (preko pc)
// 2. memorijske instrukcije - interesantne, komunikacija sa kontrolerom keša
// 3. ostale (npr. aritmetika) - na visokom nivou

CPU::CPU(CacheController *cacheContr) : cacheController(cacheContr), pc(0), load_reservation_address(0), reservation_active(false)
{
}

// tick - jedan cilkus takta

void CPU::execute(Instruction &instr)
{
    // Pomoćna lambda funkcija za dobijanje vrednosti iz registra po imenu
    auto getReg = [&](const std::string &regName)
    {
        return (regName == "x0") ? 0 : registers[regName];
    };

    if (instr.mnemonic == "addi")
    {
        // addi rd, rs1, imm
        registers[instr.rd] = getReg(instr.rs1) + instr.imm;
        pc++; // Prelazimo na sledeću instrukciju u vektoru
    }

    else if (instr.mnemonic == "lr")
    {
        // lr rd, 0(rs1) -> Load Reserved
        uint32_t adresa = getReg(instr.rs1) + instr.imm;
        registers[instr.rd] = cacheController->read(adresa);

        // Postavljamo rezervaciju za ovu adresu
        load_reservation_address = adresa;
        reservation_active = true;

        std::cout << "[" << cpuName << "] [LR] Rezervacija postavljena na adresu: " << adresa << "\n";
        pc++;
    }

    else if (instr.mnemonic == "sc")
    {
        // sc rd, rs2, 0(rs1) -> Store Conditional
        uint32_t adresa = getReg(instr.rs1) + instr.imm;

        if (reservation_active && load_reservation_address == adresa)
        {
            // Uspeh: upisujemo vrednost i u rd stavljamo 0
            cacheController->write(adresa, getReg(instr.rs2));
            registers[instr.rd] = 0;
            reservation_active = false; // Rezervacija se troši
            std::cout << "[" << cpuName << "] [SC] Uspeh! Vrednost upisana." << "\n";
        }
        else
        {
            // Neuspeh: ne upisujemo ništa, u rd stavljamo 1 (error)
            registers[instr.rd] = 1;
            std::cout << "[" << cpuName << "] [SC] Neuspeh! Rezervacija nevažeća." << "\n";
        }
        pc++;
    }

    else if (instr.mnemonic == "bne")
    {
        // bne rs1, rs2, label
        if (getReg(instr.rs1) != getReg(instr.rs2))
        {
            // Skok na labelu (ovde koristimo pomoćnu funkciju za nalaženje indeksa labele)
            pc = findLabelIndex(instr.label);
            std::cout << "[" << cpuName << "] [BNE] Skok na " << instr.label << " (PC postavljen na " << pc << ")" << "\n";
        }
        else
        {
            pc++;
        }
    }

    else if (instr.mnemonic == "ld" || instr.mnemonic == "lw")
    {
        // ld rd, 0(rs1) -> Obično čitanje
        uint32_t adresa = getReg(instr.rs1) + instr.imm;
        registers[instr.rd] = cacheController->read(adresa);
        pc++;
    }

    else if (instr.mnemonic == "st" || instr.mnemonic == "sw")
    {
        // st rs2, 0(rs1) -> Običan upis
        uint32_t adresa = getReg(instr.rs1) + instr.imm;
        cacheController->write(adresa, getReg(instr.rs2));

        // Bitno: Bilo koji "običan" Store na rezervisanu adresu od strane drugog CPU-a
        // bi trebalo da poništi rezervaciju, ali ovde simuliramo samo lokalni CPU.
        pc++;
    }
}

void CPU::step()
{
    if (pc < program.size())
    {
        execute(program[pc]);
    }
    else
    {
        std::cout << "[" << cpuName << "] Program završen.\n";
    }
}

void CPU::load(const ExecutableProgram &prog)
{
    this->program = prog.instructions;
    this->labelMap = prog.labelMap; // CPU sada ima spremnu mapu!
    this->pc = 0;
}

int CPU::findLabelIndex(const std::string &name)
{
    // Samo pogleda u mapu koju je Parser već popunio
    if (labelMap.count(name) != 0)
    {
        return labelMap[name];
    }
    return -1;
}

void CPU::setName(const std::string &name)
{
    cpuName = name;
}

void CPU::setCacheController(CacheController *cacheContr)
{
    cacheController = cacheContr;
}
