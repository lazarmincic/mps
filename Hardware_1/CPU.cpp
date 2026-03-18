#include <vector>
#include <map>
#include <fstream>

#include "CPU.hpp"

// procesor dobija ulaznu datoteku sa asembler kodom
// neću modelovati hardver riscv procesora ovde
// fokus je na hardveru za koherenciju keša
// tri tipa instrukcija 
// 1. grananja - na visokom nivou (preko pc)
// 2. memorijske instrukcije - interesantne, komunikacija sa kontrolerom keša
// 3. ostale (npr. aritmetika) - na visokom nivou


Instruction InstructionMemory::fetch(uint32_t address) {
    // Pomeranje za 2 bita udesno je identično ignorisanju žica A0 i A1
    // neophodno jer je instrukciona memorija bajt asesiblna
    return storage[address >> 2]; // aproksimacija adresnog dekodera
}
void InstructionMemory::load(uint32_t address, uint32_t binaryValue) {
    storage[address >> 2].raw = binaryValue;  // ovom metodom moguće je učitati program u programsku memoriju.
}

uint32_t ProgramCounter::read() { return address; } // Izlazni pin
void ProgramCounter::write(uint32_t nextAddress) { address = nextAddress; } // Ulazni pin


uint32_t Adder::add(uint32_t op1, uint32_t op2) {
    return op1 + op2;
}


RegisterBank::RegisterBank() // inicijalizacija svih registara na nulu
{
    for (int i = 0; i < 32; i++) 
        registers[i] = 0;
}

uint8_t RegisterBank::rs1_data(uint8_t rs1_address)
{
    if (rs1_address == 0) return 0; // x0 je uvek 0
    return registers[rs1_address];
}

uint8_t RegisterBank::rs2_data(uint8_t rs2_address)
{
    if (rs2_address == 0) return 0; // x0 je uvek 0
    return registers[rs2_address];
}

void RegisterBank::write(uint8_t rd_address, uint8_t rd_data, uint8_t rd_we) 
{
    if (rd_we != 1) return;
    if (rd_address == 0) return; // x0 se ne menja
    registers[rd_address] = rd_data;
}


CPU::CPU(CacheController* cc) : cacheController(cc) {}

// tick - jedan cilkus takta
// data-path su klase, control-path se modeluje unutar CPU::tick()

void CPU::tick () {

    // 1. FETCH
    uint32_t currentAddr = pc.read();
    Instruction currentInstr = instrMem.fetch(currentAddr);

    // 2. DECODE & EXECUTE
    switch(currentInstr.r_type.opcode) {

        case 0x13: // I-type

        case 0x63: // B-type

        case 0x23: // S-type

        case 0x2F: // R-type (atomične operacije)
        {}

    }

    // 3. PC UPDATE
    uint32_t nextAddr = pcAdder.add(currentAddr, 4);
    pc.write(nextAddr);

}


