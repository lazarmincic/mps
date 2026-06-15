#include "CPU.hpp"
#include "CacheController.hpp"
#include "Bus.hpp"
#include "Parser.hpp"
#include "MemoryController.hpp"
#include "Common.hpp"

uint64_t Simulation::cycleCount = 0; // globalni takt

// main pokreće jedan korak simulacije
int main()
{
    std::ofstream out("output.html");
    std::streambuf *coutbuf = std::cout.rdbuf();
    std::cout.rdbuf(out.rdbuf());

    // ZAGLAVLJE FAJLA - ovo ispisuje samo na početku
    podesiZaglavljeHtml();

    std::stringstream msg;

    Bus bus("BUS");                   // napravljen objekat klase Bus
    MemoryController memContr("MMC"); // napravljen objekat klase mc

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

    msg << "Kontroleri povezani sa magistralom.";
    log(msg, "SYS");

    // CPU ima direktan pristup svom CC
    cpu0.setCacheController(&cc0);
    cpu1.setCacheController(&cc1);
    cpu2.setCacheController(&cc2);
    msg << "Procesori povezani sa odgovarajucim kontrolerima kesa.";
    log(msg, "SYS");

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

        memContr.tick(); // dovlacenje podatka iz gl. memorije

        cc0.tick(); // dovlačenje podataka
        cc1.tick();
        cc2.tick(); //

        bus.tick(); // snooping, C signal, dostavljanje podatka onome ko ga je tražio

        Simulation::cycleCount++; // globalni "takt"

        // uslov za izlazak iz petlje / simulacije
        // sva tri jezgra su izvršila svoje programe
        if (cpu0.isDone() && cpu1.isDone() && cpu2.isDone())
        {
            break;
        }
    }
    msg << "Sva jezgra zavrsila posao.";
    log(msg, "SYS");

    cpu0.statIspis();
    cpu1.statIspis();
    cpu2.statIspis();

    bus.statIspis();

    std::cout << "</table></body></html>";
    std::cout.rdbuf(coutbuf);
    return 0;
}
