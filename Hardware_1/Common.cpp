#include "Common.hpp"

LogLevel maxLogLevel = INFO;

std::ostream &operator<<(std::ostream &os, const Latency &latency)
{
    os << std::to_string(latency);
    return os;
}

std::ostream &operator<<(std::ostream &os, const Address &address)
{
    os << std::to_string(address.val);
    return os;
}

std::ostream &operator<<(std::ostream &os, const Data &data)
{
    os << std::to_string(data.val);
    return os;
}

std::ostream &operator<<(std::ostream &os, const InstructionType &it)
{
    switch (it)
    {
    case InstructionType::R:
        os << "R";
        break;
    case InstructionType::I:
        os << "I";
        break;
    case InstructionType::S:
        os << "S";
        break;
    case InstructionType::B:
        os << "B";
        break;
    case InstructionType::U:
        os << "U";
        break;
    case InstructionType::J:
        os << "J";
        break;
    case InstructionType::UNKNOWN:
        os << "UNKNOWN";
        std::stringstream msg;
        msg << "Greska! Nepostojeci tip instrukcije";
        log(msg, "SYS", ERROR);
        break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const Instruction &instr)
{
    os << instr.mnemonic << " [" << instr.type << "]";
    if (!instr.rd.empty())
    {
        os << " rd = " << instr.rd;
    }
    if (!instr.rs1.empty())
    {
        os << " rs1 = " << instr.rs1;
    }
    if (!instr.rs2.empty())
    {
        os << " rs2 = " << instr.rs2;
    }
    if (instr.imm.val != 0)
    {
        os << " imm = " << instr.imm.val;
    }
    if (!instr.label.empty())
    {
        os << " label = " << instr.label;
    };
    return os;
}

void progIspis(ExecutableProgram prog)
{
    /*
    Funkcija ispisuje ono što se nalazi u strukturi ExecutableProgram
    Služi za debug - da bismo videli da li je ulazna datoteka ispravno parsovana.
    */
    std::stringstream msg;
    msg << "Parsirano " << prog.instructions.size() << " instrukcija";
    log(msg, "SYS");

    for (size_t i = 0; i < prog.instructions.size(); i++)
    {
        const auto &instr = prog.instructions[i];
        msg << "Instrukcija " << i << ": " << instr;
        log(msg, "SYS");
    }

    msg << "Labele:  ";
    for (const auto &[labelName, instrIdx] : prog.labelMap)
    {
        msg << labelName << " -> instrukcija " << instrIdx;
        // treba dopuniti ako program ima vise od jedne labele..
        log(msg, "SYS");
    }
}

std::string toString(const Controller &cntrl)
{
    std::string ret;
    switch (cntrl)
    {
    case Controller::CC0:
        ret = "CC0";
        break;
    case Controller::CC1:
        ret = "CC1";
        break;
    case Controller::CC2:
        ret = "CC2";
        break;
    case Controller::MEMC:
        ret = "MEMC";
        break;
    default:
        ret = "Unknown";
        std::stringstream msg;
        msg << "Greska! Kontroler ne postoji";
        log(msg, "SYS", FATAL);
        break;
    }
    return ret;
}

std::ostream &operator<<(std::ostream &os, const Controller &cntrl)
{
    os << toString(cntrl);
    return os;
}

std::ostream &operator<<(std::ostream &os, const CacheState &state)
{
    switch (state)
    {
    case CacheState::M:
        os << "M";
        break;
    case CacheState::E:
        os << "E";
        break;
    case CacheState::S:
        os << "S";
        break;
    case CacheState::I:
        os << "I";
        break;
    default:
        os << "Unknown";
        std::stringstream msg;
        msg << "Greska! Nepostojece stanje kesa";
        log(msg, "SYS", ERROR);
        break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const CacheLine &line)
{
    os << "State: " << line.state << ", Data: " << line.data.val;
    return os;
}

std::ostream &operator<<(std::ostream &os, const BusTransaction &trans)
{
    switch (trans)
    {
    case BusTransaction::FLUSH:
        os << "flush";
        break;
    case BusTransaction::FLUSHOPT:
        os << "flushopt";
        break;
    case BusTransaction::IDLE:
        os << "idle";
        break;
    case BusTransaction::READ:
        os << "read";
        break;
    case BusTransaction::UPGRADE:
        os << "upgrade";
        break;
    case BusTransaction::WRITE:
        os << "write";
        break;
    }
    return os;
}

void log(std::stringstream &ss, std::string comp, LogLevel level)
{
    if (level < maxLogLevel)
    {
        ss.str(std::string());
        ss.clear(); // oslobodi nakon svakog ispisa
        return;
    }
    std::string levelStr[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    uint64_t cycle = Simulation::cycleCount;
    // [0778][INFO ][CC0] Poruka
    /*
    std::cout
        << "[" << cycle << "]"
        //<< "[" << levelStr[level] << "]"
        << "[" << comp << "] "
        << ss.str() << "\n";
    */
    // Svaki poziv funkcije pravi jedan RED u tabeli
    std::cout << "<tr class='" << comp << "'>" // Dodajemo klasu komponente za bojenje pozadine
              << "<td>" << cycle << "</td>"
              << "<td class='" << levelStr[level] << "'>" << levelStr[level] << "</td>"
              << "<td><b>" << comp << "</b></td>"
              << "<td>" << ss.str() << "</td>"
              << "</tr>\n";

    ss.str(std::string());
    ss.clear(); // oslobodi nakon svakog ispisa
}

void podesiZaglavljeHtml()
{

    std::cout << "<html><head><style>"
              << "table { border-collapse: collapse; width: 100%; font-family: sans-serif; }"
              << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }"
              << "th { background-color: #333; color: white; }"

              // SISTEMSKI DELOVI (Potpuno različiti)
              << ".SYS { background-color: #f8f9fa; color: #666; }" // Neutralna siva
              << ".BUS { background-color: #e3f2fd; }"              // Svetlo plava
              << ".MMC { background-color: #f3e5f5; }"              // Svetlo ljubičasta

              // PAR 0: Jezgro 0 i Keš 0 (Zeleni tonovi)
              << ".C0  { background-color: #e8f5e9; }" // Svetlo zelena
              << ".CC0 { background-color: #c8e6c9; }" // Tamnija zelena sa ivicom

              // PAR 1: Jezgro 1 i Keš 1 (Žuto/Narandžasti tonovi)
              << ".C1  { background-color: #fffde7; }" // Svetlo žuta
              << ".CC1 { background-color: #fff9c4; }" // Jača žuta sa ivicom

              // PAR 2: Jezgro 2 i Keš 2 (Crveno/Roze tonovi)
              << ".C2  { background-color: #ffebee; }" // Svetlo roze
              << ".CC2 { background-color: #ffcdd2; }" // Jača roze sa ivicom

              << ".WARN { background-color: #ffd700 !important; /* Zlatna pozadina */color: black !important;"
              << "border: 2px solid #b8860b;"

              << "tr:hover { filter: brightness(92%); }"
              << "</style></head><body>"
              << "<table><tr><th>Takt</th><th>Nivo</th><th>Izvor</th><th>Poruka</th></tr>";
}
