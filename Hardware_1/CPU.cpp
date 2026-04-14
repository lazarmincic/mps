#include "CPU.hpp"

// procesor dobija ulaznu datoteku sa asembler kodom
// neću modelovati hardver riscv procesora ovde
// fokus je na hardveru za koherenciju keša
// tri tipa instrukcija
// 1. grananja - na visokom nivou (preko pc)
// 2. memorijske instrukcije - interesantne, komunikacija sa kontrolerom keša
// 3. ostale (npr. aritmetika) - na visokom nivou

CPU::CPU(const std::string &name)
{
    cacheController = NULL;
    pc = {0};
    load_reservation_address = {0};
    reservation_active = false;
    done = false;
    stalling = false;

    cacheHits = 0;
    cacheMisses = 0;
    pendingAddr = {0};
    pendingData = {0};

    handlers["addi"] = &CPU::handleAddi; // Moguce mapiranje više imena na istu funkciju
    handlers["bne"] = &CPU::handleBne;
    handlers["ld"] = &CPU::handleLoad;
    handlers["lw"] = &CPU::handleLoad;
    handlers["lr"] = &CPU::handleLoadReserved;
    handlers["st"] = &CPU::handleStore;
    handlers["sw"] = &CPU::handleStore;
    handlers["sc"] = &CPU::handleStoreCond;

    cpuName = name;
    std::cout << "Sys: " << cpuName << " na mrezi.\n";
}

void CPU::execute(Instruction &instr)
{
    // handler umesto switch-a (ne radi za std::string tip)
    Address oldPc = pc;
    auto iter = handlers.find(instr.mnemonic);
    if (iter != handlers.end())
    {
        InstructionHandler handler = iter->second;
        (this->*handler)(instr); // Pozovi metodu preko pokazivača
    }
    else
    {
        std::cerr << "Nepoznata instrukcija: " << instr.mnemonic << "\n";
        // ne bi trebalo nikada da se desi
    }

    // Uslov za kretanje dalje:
    // 1. Nismo u stall-u (nismo krenuli u dugačku operaciju)
    // 2. Kontroler nije bio zauzet (instrukcija nije "odbijena")
    // oldpc - Ako instrukcija NIJE izazvala stall i NIJE se zaglavila (kao SC iznad), uvećaj PC
    if (!stalling && pc.val == oldPc.val)

    // nema cache to cache transfera koji je u toku
    // drugim recima cache controller nije zauzet
    {

        pc.val++; // Prelazimo na sledeću instrukciju u vektoru
    }
}

void CPU::tick()
{
    if (done)
        return;

    // 1. HITNA INTERVENCIJA: Rezervacija pukla dok smo čekali SC
    if (reservation_active && cacheController->isReservationBroken())
    {
        reservation_active = false;
        std::cout << Simulation::cycleCount << " " << cpuName << ": Rezervacija na adresi " << load_reservation_address.val
                  << " ponistena eksternim upisom!\n";
        // Ako smo bili u stall-u baš zbog SC instrukcije, moramo je prekinuti
        if (stalling && prog.instructions[pc.val].mnemonic == "sc")
        {
            finnishPendingInstruction(); // Ovo će upisati 1 u rd (neuspeh)
            cacheController->resetReady();
            pc.val++;
            stalling = false;
            return; // Kraj za ovaj ciklus
        }
    }
    // 2. ČEKANJE NA MEMORIJU (STALL)
    if (stalling)
    {

        // pitaj keš da li je završio
        //  koliko ciklusa staluje
        if (cacheController->isReady())

        // citanje : podatak spreman
        // pisanje : potvrda da je podatak upisan
        {
            // statistika, koristi duration TODO
            // uint64_t duration = Simulation::cycleCount - stallStartCycle;
            //  RETRIEVING: Uzimamo podatak iz keša
            finnishPendingInstruction();
            // int32_t value = cacheController->retrieveData();
            //   EXECUTE
            //  izvrsava se ld, st, lr, sc bez cekanja
            //   CLEANUP
            cacheController->resetReady();
            // CONTINUE: Idemo na sledeću instrukciju
            pc.val++;
            stalling = false;
        }
        else
        {
            stallCycles++;
            // citanje: ceka se na podatak.
            // upis: ceka se na potvrdu
        }
    }
    // 3. IZVRŠAVANJE NOVE INSTRUKCIJE
    else if (pc.val < prog.instructions.size())
    {
        // Provera da li je kontroler zauzet snoop-ovanjem pre nego što uopšte pokušamo execute
        if (cacheController->isResponding())
        {
            return; // Čekaj da kontroler završi cache-to-cache transfer
        }
        execute(prog.instructions[pc.val]);
    }
    else
    {
        done = true;
        std::cout << Simulation::cycleCount << " " << cpuName << ": Program " << prog.programName << " uspesno izvrsen.\n";
    }
}

void CPU::load(const ExecutableProgram &prog)
{
    this->prog = prog;
    this->pc = {0};
    std::cout << cpuName << ": Program \"" << prog.programName << "\" ucitan u memoriju za instrukcije.\n";
}

int CPU::findLabelIndex(const std::string &name)
{
    // Samo pogleda u mapu koju je Parser već popunio
    if (prog.labelMap.count(name) != 0)
    {
        return prog.labelMap[name];
    }
    return -1;
}

void CPU::setCacheController(CacheController *cacheContr)
{
    cacheController = cacheContr;
}

bool CPU::isDone() const
{
    return done;
}

Data CPU::getReg(const std::string &regName) const
{
    // funkcija za dobijanje vrednosti iz registra po imenu
    if (regName == "x0")
    {
        return {0}; // x0 je u RISC-V uvek nula
    }
    // Provera da li registar postoji
    auto iter = registers.find(regName);
    if (iter != registers.end())
    {
        return iter->second;
    }

    return {0}; // Ili baci grešku ako registar ne postoji
}

void CPU::handleAddi(const Instruction &instr)
{
    // add immidiate op
    // addi rd, rs1, imm
    registers[instr.rd] = {getReg(instr.rs1).val + instr.imm.val};
    stalling = false;
    std::cout << Simulation::cycleCount << " " << cpuName << ": Izvrsena instrukcija " << pc.val << " - " << instr << "\n";
}

void CPU::handleBne(const Instruction &instr)
{
    // branch not equal op
    // bne rs1, rs2, label
    if (getReg(instr.rs1).val != getReg(instr.rs2).val)
    {
        // Skok na labelu (ovde koristimo pomoćnu funkciju za nalaženje indeksa labele)
        pc.val = findLabelIndex(instr.label);
    }
    stalling = false;
    std::cout << Simulation::cycleCount << " " << cpuName << ": Izvrsena instrukcija " << pc.val << " - " << instr << "\n";
}

void CPU::handleLoad(const Instruction &instr)
{
    // load (word) op
    // ld rd, 0(rs1) -> Obično čitanje
    std::cout << Simulation::cycleCount << " " << cpuName << " krecem sa izvrsavanjem instrukcije " << instr << "\n";
    Address adresa = {(uint32_t)(getReg(instr.rs1).val + instr.imm.val)};
    if (cacheController->isHit(adresa))
    {
        registers[instr.rd] = cacheController->getData(adresa);
    }
    else
    {
        cacheController->readMiss(adresa);
    }
}

void CPU::handleLoadReserved(const Instruction &instr)
{
    std::cout << Simulation::cycleCount << " " << cpuName << " krecem sa izvrsavanjem instrukcije " << instr << "\n";
    // load reserved op
    // lr rd, 0(rs1) -> Load Reserved
    Address addr = {uint32_t(getReg(instr.rs1).val + instr.imm.val)};
    pendingAddr = {addr.val};
    // 1. Postavljamo CPU u stanje zastoja (Stall)
    stalling = true;
    stallStartCycle = Simulation::cycleCount;
    if (cacheController->isHit(addr))
    {
        std::cout << Simulation::cycleCount << " " << cpuName << ": pogodak u kesu pri citanju sa adrese " << addr.val << ".\n";
        cacheController->startInternalRead(addr);
        // ukupno 2 ciklusa - 1 za pristup i 1 za dek. instr.
    }
    else
    {
        std::cout << Simulation::cycleCount << " " << cpuName << ": promasaj u kesu pri citanju sa adrese " << addr.val << ".\n";
        cacheController->readMiss(addr); // I stanje
        // koliko ceka zavisi od cekanja na magistralu..
        // stallCycles zavisi od toga da li podatak dolazi iz mem ili iz c
    }
}

void CPU::handleStore(const Instruction &instr)
{
    std::cout << Simulation::cycleCount << " " << cpuName << " krecem sa izvrsavanjem instrukcije " << instr << "\n";
    // store (word) op
    // st rs2, 0(rs1) -> Običan upis
    Address addr = {getReg(instr.rs1).val + (uint32_t)instr.imm.val}; // imm može biti negativan
    Data data = {getReg(instr.rs2)};
    if (cacheController->isHit(addr))
    {
        cacheController->storeData(addr, data);
    }
    else
    {
        cacheController->writeMiss(addr, data);
    }

    // Bitno: Bilo koji "običan" Store na rezervisanu adresu od strane drugog CPU-a
    // bi trebalo da poništi rezervaciju, ali ovde simuliramo samo lokalni CPU.
}

void CPU::handleStoreCond(const Instruction &instr)
{

    std::cout << Simulation::cycleCount << " " << cpuName << " krecem sa izvrsavanjem instrukcije " << instr << "\n";

    uint32_t adresa = getReg(instr.rs1).val + instr.imm.val;
    Address addr = {adresa};

    if (!reservation_active || load_reservation_address.val != addr.val)
    {
        // Neuspeh: ne upisujemo ništa, u rd stavljamo 1 (error)
        registers[instr.rd] = {1}; // Neuspeh
        reservation_active = false;
        stalling = false; //
        // PC će se uvećati u execute() jer nismo u stall-u
        std::cout << Simulation::cycleCount << " " << cpuName << " izvrsio instrukciju " << instr << " (neuspesno).\n";
        return;
    }
    // store conditional op
    // sc rd, rs2, 0(rs1) -> Store Conditional

    if (cacheController->isResponding()) // busy, u procesu slanja
    {
        std::cout << Simulation::cycleCount << " " << cpuName << " " << instr << " ne moze da se izvrsi, cache-to-cache transfer u toku\n";
        std::cerr << Simulation::cycleCount << " " << cpuName << " Pecovano, ne bi trebalo nikada da se desi\n";
        return;
    }

    std::cout << Simulation::cycleCount << " " << cpuName << " " << instr << " instrukcija moze da se izvrsi, kontroler kesa nije zauzet\n";

    pendingAddr = {adresa};
    Data data = {getReg(instr.rs2)};
    pendingData = data; // Podatak koji želimo da upišemo
    stalling = true;
    stallStartCycle = Simulation::cycleCount;

    // Uspeh: upisujemo vrednost i u rd stavljamo 0

    CacheState state = cacheController->getState(addr);
    if (cacheController->isHit(addr))
    {
        // M S E su hit
        // M i E - ne ide se na magistralu
        // S i I - ide se na magistralu
        std::cout << Simulation::cycleCount << " " << cpuName << ": pogodak u kesu pri pisanju podatka " << data.val << " na adresu " << addr.val << "\n";

        if (state == CacheState::E || state == CacheState::M)
        {
            cacheController->startInternalWrite(addr, data);
            // Imamo ekskluzivno pravo, možemo odmah upisati lokalno
            // ukupno 2 ciklusa - 1 za pristup i 1 za dek. instr.
            // E, magistrala tiha, samo prelazi u M
            // ako je M, ostaje u M
            // Uspeh će biti upisan u finnishPendingInstruction
        }
        else if (state == CacheState::S)
        {
            // ako je S, stavlja upgrade signal na magistralu
            cacheController->broadcastInvalidate(addr);
        }
    }
    else
    {
        // I stanje, ide se na magistralu
        cacheController->writeMiss(addr, data);
        // kako znati da je upis uspeo? treba potvrda sa magistrale, tek onda registar na nulu i fleg menjati
    }
}

void CPU::finnishPendingInstruction()
{
    auto &instr = prog.instructions[pc.val];
    Data dataFromCache = cacheController->retrieveData();
    if (instr.mnemonic == "lr")
    {
        registers[instr.rd] = dataFromCache;
        load_reservation_address = pendingAddr;
        reservation_active = true;
        cacheController->armMonitor({load_reservation_address}); // cc je taj koji nadgleda adresu
        std::cout << Simulation::cycleCount << " " << cpuName << ": Izvrsena instrukcija " << instr << ".\n";
    }
    else if (instr.mnemonic == "ld")
    {
        registers[instr.rd] = dataFromCache;
        std::cout << Simulation::cycleCount << " " << cpuName << ": Izvrsena instrukcija " << instr << ".\n";
    }
    else if (instr.mnemonic == "sc")
    {
        // ovde upis u kes, vreme proslo
        if (reservation_active)
        // Ako je i nakon čekanja na magistralu rezervacija i dalje tu
        {
            // Uspeh
            // ovde promena stanja (ovo je nakon ack sa magistrale ili nakon sto je prosao latency za local hit)
            cacheController->storeData(Address{pendingAddr}, {pendingData});
            std::cout << Simulation::cycleCount << " " << cacheController->getController() << ": Upisan podatak " << pendingData.val << " na adresu " << pendingAddr.val << ", promena stanja bloka iz " << cacheController->getState(pendingAddr) << " u M.\n";
            cacheController->setState(CacheState::M, {pendingAddr}); // koje god da je bilo nakon uspesnog upisa ide u M
            registers[instr.rd] = {0};                               // USPEH
            std::cout << Simulation::cycleCount << " " << cpuName << ": Uspesno izvrsena instrukcija " << instr << ". " << instr.rd << " = 0.\n";
        }
        else
        {
            // Neuspeh
            // nema upisa, ostaje stara vrednost
            // Neko nas je preduhitrio dok smo čekali na magistralu!
            registers[instr.rd] = {1}; // NEUSPEH
            std::cout << Simulation::cycleCount << " " << cpuName << ": Neuspesno izvrsena instrukcija " << instr << ". " << instr.rd << " = 1.\n";
        }
        reservation_active = false; // Svakako se briše
    }
    else if (instr.mnemonic == "st")
    {
        std::cout << Simulation::cycleCount << " " << cpuName << ": Izvrsena instrukcija " << instr << ".\n";
        // ne vraca nista u registre
    }
}
