#ifndef CPU_HPP
#define CPU_HPP

#include "CacheController.hpp"
#include "Parser.hpp"
#include "Common.hpp"

class CacheController;

class CPU
{
private:
    Address pc;                            // programski brojač
    std::map<std::string, Data> registers; // ključ je ime registra, vrednost je podatak
    Address load_reservation_address;      // za lr
    bool reservation_active;               // za lr

    CacheController *cacheController; // CPU ima pristup svom kontroleru

    ExecutableProgram prog; // program koji izvršava jezgro

    bool done; // uslov za izlaz iz simulacije

    bool stalling; // ceka se na podatak iz mem / susednog kesa

    using InstructionHandler = void (CPU::*)(const Instruction &); // Mapa koja povezuje ime instrukcije sa metodom
    std::unordered_map<std::string, InstructionHandler> handlers;  // po jedan handler za svaku od instrukcija procesora

    uint64_t cacheHits;   // statistika, broj pogodaka u kešu ovog procesora
    uint64_t cacheMisses; // analogno broj promašaja
    uint64_t pristupiMem;

    uint64_t stallCycles;     // ukoliko se podatak "dovlači", koliko proc. čeka na njega?
    uint64_t stallStartCycle; // ciklus globalnog sata od kog se čeka
    Address pendingAddr;      // adresa koju koristi trenutna instrukcija koja čeka na podatak
    Data pendingData;

    std::string cpuName;   // za debug, ime proc.
    std::stringstream msg; // za ispis

public:
    CPU(const std::string &name); // konstruktor

    void execute(Instruction &instr); // izvršavanje jedne instrkucije

    void load(const ExecutableProgram &prog); // izvršavanje celog programa

    void tick(); // jedan otkucaj u simulaciji

    void setCacheController(CacheController *cacheContr); // setter za cache controller

    bool isDone() const; // getter za done

    void statIspis();

private:
    int findLabelIndex(const std::string &name);   // vraća index instrukcije na koju se odnosi labela
    Data getReg(const std::string &regName) const; // vraća vrednost registra na osnovu imena registra

    void handleAddi(const Instruction &instr); // handleri za riscv instrukcije koje su u tekst. datoteci primera
    void handleBne(const Instruction &instr);
    void handleLoad(const Instruction &instr);
    void handleLoadReserved(const Instruction &instr);
    void handleStore(const Instruction &instr);
    void handleStoreCond(const Instruction &instr);
    void finnishPendingInstruction(); // poziva se nakon što se čekanje na podatak završilo

    void cLog(std::stringstream &ss, LogLevel level = maxLogLevel);
};

#endif
