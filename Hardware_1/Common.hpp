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

namespace fs = std::filesystem;

// ovde definisati sva MESI kasnjenja
enum Latency : uint8_t
{
    HIT = 1,          // pogodak magistrala tiha
    BUS_UPGRADE = 10, // pogodak ali potrebna magistrala
    REMOTE_CACHE = 8, // cache to cache transfer
    MEMORY = 80       // pristup memoriji
};

// wrapper za adresu (lako se pobrka uint i int)
struct Address
{
    uint32_t val;
};

// analogno za podatke
struct Data
{
    int32_t val;
};

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

    friend std::ostream &operator<<(std::ostream &os, const Instruction &instr)
    {
        os << instr.mnemonic << " [";

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
        if (instr.imm.val != 0)
        {
            std::cout << " imm=" << instr.imm.val;
        }
        if (!instr.label.empty())
        {
            std::cout << " label=" << instr.label;
        };
        return os;
    }
};

// potrebna još i mapa labela da bi procesor mogao da izvrši prog. skokove
struct ExecutableProgram
{
    std::vector<Instruction> instructions;
    std::map<std::string, int> labelMap;
    std::string programName; // debug ispis
};

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
std::ostream &operator<<(std::ostream &os, const Controller &controller);

// MESI stanja u kojim se keš linija može naći
enum class CacheState : uint8_t
{
    M,
    E,
    S,
    I
};

std::ostream &operator<<(std::ostream &os, const CacheState &state);

// MESI stanja, bit po bit (niži nivo)
struct StateBits
{
    uint8_t modified : 1; // prljav (1) ili čist (0)
    uint8_t shared : 1;   // deljen (1) ili eksluzivan (0)

    // izuzetak je I stanje - nije prisutan u kešu ili linija prisutna ali nije validna (11)

    void setS()
    {
        modified = 0;
        shared = 1;
    }
    void setM()
    {
        modified = 1;
        shared = 0;
    }
    void setE()
    {
        modified = 0;
        shared = 0;
    }
    void setI()
    {
        modified = 1;
        shared = 1;
    }
    void setState(CacheState cacheState)
    {
        switch (cacheState)
        {
        case CacheState::E:
            setE();
            break;
        case CacheState::M:
            setM();
            break;
        case CacheState::S:
            setS();
            break;
        default:
            setI();
        }
    }

    // Brze proveraške stanja
    bool isM() const { return modified == 1 && shared == 0; } // Modified
    bool isE() const { return modified == 0 && shared == 0; } // Exclusive
    bool isS() const { return modified == 0 && shared == 1; } // Shared
    bool isI() const { return modified == 1 && shared == 1; } // Invalid

    // Getter koji vraća enum reprezentaciju stanja
    CacheState getState() const
    {
        if (isM())
        {
            return CacheState::M;
        }
        if (isE())
        {
            return CacheState::E;
        }
        if (isS())
        {
            return CacheState::S;
        }
        return CacheState::I;
        return CacheState::I; // default
    };

    StateBits() : modified(1), shared(1) {} // konstrutor se poziva pri pravljenju strukture

    // sve linije počinju u I (11) stanju
};

// izgled jedne linije keša
struct CacheLine
{
    StateBits state; // stanje keš linije (MESI)
    Data data;       // 32-bitni podatak u kešu

    // Dekompozicija 32-bitne adrese:
    // [31:3] = tag (29 bita)
    // [2]    = index (1 bit) - bira jedan od 2 seta
    // [1:0]  = offset (2 bita) - pozicija unutar 32-bitne reči

    uint32_t tag : 29;  // viši bitovi adrese (bitovi 31:3)
    uint8_t index : 1;  // izbor seta (bit 2)
    uint8_t offset : 2; // offset unutar linije (bitovi 1:0)

    CacheLine() : data({0}), tag(0), index(0), offset(0) {} // keš kreće "hladan"

    // SETER
    void setAddress(Address addr)
    {
        tag = (addr.val >> 3);
        index = (addr.val >> 2) & 0x1;
        offset = addr.val & 0x3;
    }

    // GETER
    Address getAddress() const
    {
        return {(tag << 3) | (index << 2) | offset};
    }

    friend std::ostream &operator<<(std::ostream &os, const CacheLine &cl)
    {
        os << "State: " << cl.state.getState() << ", Address: " << cl.getAddress().val << ", Data: " << cl.data.val;
        return os;
    }
};

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

namespace Color // za ispis sa bojama u konzoli
{
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string BOLD = "\033[1m";
}
#endif
