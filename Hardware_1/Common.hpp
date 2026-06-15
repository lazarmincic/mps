#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <queue>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

// ovde definisati sva MESI kasnjenja
enum Latency : uint8_t
{
    HIT = 1,          // pogodak magistrala tiha
    BUS_UPGRADE = 10, // pogodak ali potrebna magistrala
    REMOTE_CACHE = 8, // cache to cache transfer
    MEMORY = 80       // pristup memoriji
};

std::ostream &operator<<(std::ostream &os, const Latency &latency);

// wrapper za adresu (lako se pobrka uint i int)
struct Address
{
    uint32_t val;
};

std::ostream &operator<<(std::ostream &os, const Address &address);

// analogno za podatke
struct Data
{
    int32_t val;
};

std::ostream &operator<<(std::ostream &os, const Data &data);

// tip instrukcije, riscv notatura
enum class InstructionType : uint8_t
{
    R,
    I,
    S,
    B,
    U,
    J,
    UNKNOWN
};

std::ostream &operator<<(std::ostream &os, const InstructionType &it);

// ovo je format instrukcije sa kojim će cpu raditi
struct Instruction
{
    std::string mnemonic; // npr. "addi", "lr", "bne"
    InstructionType type; // Tip instrukcije

    // Apstrahovani operandi (koriste se po potrebi)
    std::string rd;    // Odredišni registar
    std::string rs1;   // Prvi izvorni registar
    std::string rs2;   // Drugi izvorni registar
    Data imm;          // Immediate vrednost (offset ili konstanta)
    std::string label; // Labela za skokove (npr. "try_lock")

    Instruction() : type(InstructionType::UNKNOWN), imm({0}) {}
};

std::ostream &operator<<(std::ostream &os, const Instruction &instr);

// potrebna još i mapa labela da bi procesor mogao da izvrši prog. skokove
struct ExecutableProgram
{
    std::vector<Instruction> instructions;
    std::map<std::string, int> labelMap;
    std::string programName; // debug ispis
};

void progIspis(ExecutableProgram prog);

// svaki kontroler ima svoj unos
enum class Controller : uint8_t
{
    NONE, // potreban za roundrobin
    CC0,  // Cache Controller 0
    CC1,
    CC2,
    MEMC // kada memorija odgovara
};
// Preklapanje operatora (mora biti izvan enuma)
std::string toString(const Controller &cntrl);

std::ostream &operator<<(std::ostream &os, const Controller &cntrl);

// MESI stanja u kojim se keš linija može naći
enum class CacheState : uint8_t
{
    M,
    E,
    S,
    I
};

std::ostream &operator<<(std::ostream &os, const CacheState &state);

// izgled jedne linije keša
struct CacheLine
{
    CacheState state; // stanje keš linije (MESI)
    Data data;        // 32-bitni podatak u kešu

    CacheLine() : state(CacheState::I), data({0}) {} // keš kreće "hladan"
};

std::ostream &operator<<(std::ostream &os, const CacheLine &line);

// tip transakcija na magistrali
enum class BusTransaction : uint8_t
{
    IDLE = 0, // Magistrala je slobodna/isključena
    READ,
    WRITE,
    UPGRADE,
    FLUSH,
    FLUSHOPT
};

std::ostream &operator<<(std::ostream &os, const BusTransaction &trans);

// izgled cele transakcije na magistrali
struct BusSignal
{
    BusTransaction type = BusTransaction::IDLE; // Podrazumevano stanje
    Address address = {0};
    Data data = {0};
};

// globalni takt treba da ima svoj namespace
class Simulation
{
public:
    static uint64_t cycleCount; // simulator zasnovan na taktu
    // globalni brojac
};

// za log
enum LogLevel
{
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

extern LogLevel maxLogLevel;

void log(std::stringstream &ss, std::string comp, LogLevel level = maxLogLevel);

void podesiZaglavljeHtml();

#endif
