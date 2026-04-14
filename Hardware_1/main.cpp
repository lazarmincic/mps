#include "CPU.hpp"
#include "CacheController.hpp"
#include "Bus.hpp"
#include "Parser.hpp"
#include "MemoryController.hpp"
#include "Common.hpp"

void progIspis(ExecutableProgram prog);

uint64_t Simulation::cycleCount = 0; // globalni takt

// main pokreće jedan korak simulacije
int main()
{

    Bus bus;                   // napravljen objekat klase Bus
    MemoryController memContr; // napravljen objekat klase mc

    // Kreiraj sve komponente
    CacheController cc0(Controller::CC0); // objekti klase cc0, sa svojim imenima
    CacheController cc1(Controller::CC1);
    CacheController cc2(Controller::CC2);
    CPU cpu0("C0"); // objekti klase cpu
    CPU cpu1("C1"); // dajemo imena cpu objektima, poslužiće za debug
    CPU cpu2("C2");

    // kontroleri se povezuju na magistralu
    bus.registerMemoryController(&memContr);
    bus.registerCacheController(0, &cc0);
    bus.registerCacheController(1, &cc1);
    bus.registerCacheController(2, &cc2);

    // CC i MC ima pristup magistrali
    cc0.setBus(&bus);
    cc1.setBus(&bus);
    cc2.setBus(&bus);
    memContr.setBus(&bus);
    // kruzna zavisnost

    std::cout << "Sys: Kontroleri povezani sa magistralom.\n";

    // CPU ima direktan pristup svom CC
    cpu0.setCacheController(&cc0);
    cpu1.setCacheController(&cc1);
    cpu2.setCacheController(&cc2);
    std::cout << "Sys: Procesori povezani sa odgovarajucim kontrolerima kesa.\n";

    // učitavanje programa u programsku memoriju odgovarajućeg procesora

    ExecutableProgram prog = Parser::parseFile("input.txt");
    // Debug ispis
    progIspis(prog);

    // isti program se izvršava na tri ista procesora
    // nije moralo tako, moglo je biti tri različita programa
    cpu0.load(prog);
    cpu1.load(prog);
    cpu2.load(prog);

    // stanje kesa i memorije na pocetku, ispis
    cc0.stateOfCache();
    cc1.stateOfCache();
    cc2.stateOfCache();
    memContr.stateOfMem();
    // petlja u kojoj se vrši simulacija, jedan krug je jedan "tick" simulatora
    while (true)
    {

        cpu0.tick();
        cpu1.tick();
        cpu2.tick();

        // keš hit - kraj, nema interakcije sa magistralom, jezgro je "tiho"
        // keš mis - šalje se signal na magistralu
        // može biti više keš misova u isto vreme, unutar Bus se vrši arbitracija

        bus.tick(); // snooping, C signal, dostavljanje podatka onome ko ga je tražio

        cc0.tick(); // dovlačenje podataka
        cc1.tick();
        cc2.tick();

        memContr.tick(); // dovlacenje podatka iz gl. memorije

        Simulation::cycleCount++; // globalni "takt"

        // uslov za izlazak iz petlje / simulacije
        // sva tri jezgra su izvršila svoje programe
        if (cpu0.isDone() && cpu1.isDone() && cpu2.isDone())
        {
            break;
        }
        if (Simulation::cycleCount == 120)
            return 1; // debug
    }
    std::cout << "\nStatistika: ...\n";
    return 0;
}

void progIspis(ExecutableProgram prog)
{
    /*
    Funkcija ispisuje ono što se nalazi u strukturi ExecutableProgram
    Služi za debug - da bismo videli da li je ulazna datoteka ispravno parsovana.
    */
    std::cout << "Parsirano " << prog.instructions.size() << " instrukcija\n\n";

    for (size_t i = 0; i < prog.instructions.size(); i++)
    {
        const auto &instr = prog.instructions[i];
        std::cout << "Instrukcija " << i << ": " << instr;
        std::cout << "\n";
    }

    std::cout << "\nLabele:\n";
    for (const auto &[labelName, instrIdx] : prog.labelMap)
    {
        std::cout << "  " << labelName << " -> instrukcija " << instrIdx << "\n\n";
    }
}

void instrIspis(Instruction isntr)
{
}

std::ostream &operator<<(std::ostream &os, const Controller &controller)
{
    switch (controller)
    {
    case Controller::CC0:
        os << "CC0";
        break;
    case Controller::CC1:
        os << "CC1";
        break;
    case Controller::CC2:
        os << "CC2";
        break;
    case Controller::MEMC:
        os << "MEMC";
        break;
    default:
        os << "Unknown";
        break;
    }
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
        break;
    }
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
