#ifndef CPU_HPP
#define CPU_HPP


#include <cstdint>
#include "CacheController.hpp"

union Instruction {

//union smešta sve svoje članove na istu memorijsku lokaciju

    uint32_t raw; // Pristup svih 32 bita odjednom 

    // R
    struct {
        uint32_t opcode : 7;
        uint32_t rd     : 5;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        uint32_t rs2    : 5;
        uint32_t funct7 : 7;
    } r_type;

    // I
    struct {
        uint32_t opcode : 7;
        uint32_t rd     : 5;
        uint32_t funct3 : 3;
        uint32_t rs1    : 5;
        int32_t  imm11_0 : 12; 
    } i_type;

    struct {
        uint32_t opcode  : 7;
        uint32_t imm4_0  : 5; // Donji deo offseta
        uint32_t funct3  : 3;
        uint32_t rs1     : 5; // Bazni registar
        uint32_t rs2     : 5; // Registar koji se upisuje
        uint32_t imm11_5 : 7; // Gornji deo offseta
    } s_type;

    struct {
        uint32_t opcode  : 7;
        uint32_t imm11   : 1;
        uint32_t imm4_1  : 4;
        uint32_t funct3  : 3;
        uint32_t rs1     : 5;
        uint32_t rs2     : 5;
        uint32_t imm10_5 : 6;
        uint32_t imm12   : 1;
    } b_type;
};

class InstructionMemory {
private:
    Instruction storage [128]; // dosta za mali program
public:
    Instruction fetch(uint32_t address);
    void load(uint32_t address, uint32_t binaryValue) ;
        
};

class ProgramCounter {
private:
    uint32_t address;
public:
    uint32_t read(); 
    void write(uint32_t nextAddress);
};

class Adder {
    // čista funkcija - simulira prolaz signala kroz logička kola
public:
    uint32_t add(uint32_t op1, uint32_t op2);
};

class RegisterBank{
private:
    uint8_t registers[32];
public:
    RegisterBank();
    
    uint8_t rs1_data(uint8_t rs1_address);

    uint8_t rs2_data(uint8_t rs2_address);

    void write(uint8_t rd_address, uint8_t rd_data, uint8_t rd_we) ;
};

class CPU {
private:
// obični objekti - nema deljenja, svaki objekat cpu ima svoj pc itd.
    InstructionMemory instrMem ; 
    ProgramCounter pc;
    Adder pcAdder;
    RegisterBank regBank;

// spoljne komponente (pokazivači - "povezani kablovima")
    CacheController* cacheController; // CPU ima pristup svom kontroleru

public:
    CPU(CacheController* cc);

    void tick ();
};

#endif