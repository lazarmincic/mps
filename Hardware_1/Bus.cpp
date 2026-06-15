#include "Bus.hpp"

Bus::Bus(std::string name)
{
    lastWinner = Controller::NONE;
    isBusy = false;
    currentOwner = Controller::NONE;
    currentSignal = BusSignal();
    sharedLine = false;
    canCollect = false;
    busyCycles = 0;
    pendingResponse = BusSignal();
    winner = Controller::NONE;
    busName = name;
    transNum = 0;
    msg << "Na mrezi.";
    busLog(msg);
}

const BusSignal &Bus::getCurrentSignal() const
{
    return currentSignal;
}

bool Bus::isBusFree() const
{
    return !isBusy;
}

Controller Bus::getCurrentOwner() const
{
    return currentOwner;
}

void Bus::registerCacheController(int cpuID, CacheController *cacheContr)
{
    cacheControllers[cpuID] = cacheContr;
}

void Bus::registerMemoryController(MemoryController *memContr)
{
    memoryController = memContr;
}

void Bus::releaseBus()
{
    isBusy = false;
    activeSignals.erase(currentOwner); // Uklanjamo signal pobednika iz aktivnih zahteva
    currentOwner = Controller::NONE;
    // "Gasimo" magistralu
    currentSignal.type = BusTransaction::IDLE;
    currentSignal.address = {0};
    currentSignal.data = {0};
    sharedLine = false;
    canCollect = false;
    busyCycles = 0;
    pendingResponse = BusSignal();
    msg << "Oslobadjam resurse.";
    busLog(msg);
}

void Bus::tick()
{
    // metode su koraci pipeline-a magistrale
    // 1. Ako niko ne koristi magistralu, pokušaj arbitražu
    if (!isBusy)
    {
        grantBus(); // u njemu se menja isBusy
        if (this->currentOwner != Controller::NONE)
        {
            canCollect = false;
            broadcastRequest();
        }
    }
    if (isBusy)
    {
        if (canCollect)
        { // Da li je neko (RAM ili drugi Keš) postavio 'ready' na true?
            collectResponses();
            releaseBus(); //
        }
        else if (busyCycles > 0) // neko je promenio vrednost, postoji cekanje..
        {
            // MAGISTRALA JE ZAMRZNUTA:
            // Ne radimo ništa, isBusy ostaje true, currentOwner ostaje isti.
            // U sledećem ciklusu main će opet pozvati tick() i opet ćemo doći ovde.

            // canCollect je false

            busyCycles--;
            if (busyCycles == 0)
            {
                canCollect = true;
            }
            // cekanje na magistrali za flushopt
        }
    }
}
void Bus::requestBus(Controller devicePosting, const BusSignal &signal)
{
    activeSignals[devicePosting] = signal;
    msg << devicePosting << " zahteva pristup magistrali. Zeli da ";
    switch (signal.type)
    {
    case BusTransaction::READ:
        msg << "cita sa adrese " << signal.address;
        break;
    case BusTransaction::WRITE:
        msg << "upise " << signal.data << " na adresu " << signal.address;
        break;
    case BusTransaction::UPGRADE:
        msg << "invalidira tudje kopije na adresi " << signal.address;
        break;
    default:
        msg << "Nemoguc signal.";
        busLog(msg, ERROR);
        return;
    }
    busLog(msg);
}

void Bus::grantBus()
{
    if (isBusy)
    {
        // neko je vec zauzeo magistralu te nema potrebe za arbitracijom
        return;
    }

    if (activeSignals.empty())
    {
        return;
    }

    // ispis
    msg << "Arbitriram signale ";
    for (auto it = activeSignals.begin(); it != activeSignals.end(); ++it)
    {
        // Koristimo it->first za ključ (kontroler) i it->second za vrednost (signal)
        const auto &kontroler = it->first;
        const auto &signal = it->second;

        msg << signal.type << " (" << kontroler << ")";

        // Provera da li je trenutni element poslednji u mapi
        if (std::next(it) == activeSignals.end())
        {
            msg << "."; // Tačka na kraju poslednjeg
        }
        else
        {
            msg << ", "; // Zarez za ostale
        }
    }
    busLog(msg);

    // Round Robin
    auto iter = activeSignals.upper_bound(lastWinner); // sledeci koji nije bio na redu
    if (iter == activeSignals.end())
    {
        iter = activeSignals.begin();
    }
    currentOwner = iter->first;
    lastWinner = currentOwner;
    currentSignal = iter->second;
    isBusy = true;
    busyCycles = 0;
    msg << "Magistralu zauzeo " << currentOwner << ", transakcija " << currentSignal.type;
    busLog(msg);
    transNum++;
}

void Bus::broadcastRequest()
{
    // ovde se dešava snooping
    winner = Controller::NONE; // ovde se desava arbitracija unutar arbitracije
    CacheState stateOfWinner = CacheState::I;
    if (currentSignal.type != BusTransaction::UPGRADE)
    {
        Address addr = {currentSignal.address};

        for (auto &controller : cacheControllers)
        {
            if (controller->isHit(addr))
            {
                pullSharedLine(); // C signal
                CacheState state = controller->getState(addr);

                msg << controller->getController() << " ima kesiran blok na adresi " << addr.val << " u stanju " << state << ". Postavljam C liniju na visok napon.";
                busLog(msg);
                // ARBITRACIJA: Prvi (najmanji ID) koji ima podatak postaje zadužen za slanje
                if (winner == Controller::NONE)
                {
                    winner = controller->getController();
                    stateOfWinner = state; // za ispis
                }
            }
        }
        if (winner != Controller::NONE)
        {
            if (stateOfWinner == CacheState::S)
            {
                msg << "Unutrasnja arbitraza - " << winner << " ce dostavljati podatak.";
            }
            else if (stateOfWinner == CacheState::E || stateOfWinner == CacheState::M)
            {
                msg << "Unutrasnja arbitraza nije potrebna, jedino " << winner << " moze dostaviti podatak.";
            }
            busLog(msg);
        }
        if (!sharedLineActive())
        {
            msg << "Nijedan drugi kes nema podatak. C linija niska.";
            busLog(msg);
        }
    }
    else
    {
        busyCycles = Latency::BUS_UPGRADE; // magistrala je uzrok kasnjenja
    }

    // Sada pozivamo pravi snoop, ali mu kažemo da li je on taj koji treba da šalje
    for (auto &controller : cacheControllers)
    {
        bool shouldRespond = (controller->getController() == winner);
        controller->snoop(currentSignal, shouldRespond);
        // u snoop je impl. da se ignorise sopstveni signal
    }
    memoryController->snoop(currentSignal);
    // snoop metode u isto vreme postavljaju signal na magistralu
    // on se obrađuje u collectResponses()

    // moraju prvo svi da zavrse sa osluskivanjem currentS. pre nego sto se on promeni..
    if (currentSignal.type != pendingResponse.type && pendingResponse.type != BusTransaction::IDLE) // neko odgovorio
    {
        currentSignal = pendingResponse; // flush ili flushopt sa podatkom je ovde
    }
    if (currentSignal.type == BusTransaction::FLUSH) // upgrade, flushopt
    {
        memoryController->flush();
    }
    msg << "Kontroleri zavrsili sa presretanjem.";
    busLog(msg);
}

// Kontroleri zovu ovo ako imaju pogodak (Snoop Hit)
void Bus::pullSharedLine()
{
    sharedLine = true; // Čim jedan "povuče", ostaje true
}

bool Bus::sharedLineActive() const
{
    return sharedLine;
}

void Bus::resetSharedLine()
{
    sharedLine = false; // Magistrala "pušta" žicu na kraju ciklusa
}

void Bus::putDataOnBus(const BusSignal &response)
{
    // Direktno stavljamo odgovor na magistralu
    // Bez arbitraže, jer memorija samo odgovara Masteru koji već drži bus
    currentSignal = response;
    msg << "Podatak postavljen.";
    busLog(msg);
    acknowledgeResponse();
}

void Bus::flush(Data data)
{
    pendingResponse = currentSignal; // address
    pendingResponse.type = BusTransaction::FLUSH;
    pendingResponse.data = data;
    // nema acknowledge response jer magistrala treba da bude blokirana dok se podatak vadi iz mem
}

void Bus::flushOpt(Data data)
{
    pendingResponse = currentSignal; // address
    pendingResponse.type = BusTransaction::FLUSHOPT;
    pendingResponse.data = data;
    busyCycles = Latency::REMOTE_CACHE; //

    // nema acknowledge response jer magistrala treba da bude blokirana dok traje cache to cache transfer
}

void Bus::upgrade()
{
    pendingResponse = currentSignal;
    pendingResponse.type = BusTransaction::UPGRADE; // vec je ovo
    busyCycles = Latency::BUS_UPGRADE;              // ovaj deo najbitniji
}

void Bus::collectResponses()
{
    // current signal je azuriran sa odgovorom
    // current owner je master
    // ovde se odgovor "vraca" onome ko je trazio podatak (masteru)
    // prvo upis u memoriju, isto ovde, pa tek onda vracanje masteru
    if (currentSignal.type != BusTransaction::UPGRADE)
    {
        msg << "Vracam podatak " << currentOwner << " (masteru).";
        busLog(msg);
    }
    else
    {
        msg << "Obavestavam " << currentOwner << " (mastera).";
        busLog(msg);
    }
    switch (currentOwner)
    {
    case Controller::CC0:
        cacheControllers[0]->receiveBusResponse();
        break;
    case Controller::CC1:
        cacheControllers[1]->receiveBusResponse();
        break;
    case Controller::CC2:
        cacheControllers[2]->receiveBusResponse();
        break;
    default:
        busLog(msg, ERROR);
    }
}

void Bus::acknowledgeResponse()
{
    // Ovu metodu poziva RAM ili Cache kada završe
    canCollect = true;
    msg << "Kontroleri zavesili sa odgovorom.";
    busLog(msg);
}

bool Bus::isAcknowledge()
{
    return canCollect;
}

void Bus::dropBusRequest(Controller controller)
{
    if (activeSignals.count(controller) > 0) // postoji u mapi
    {
        activeSignals.erase(controller); // obrisi ga
    }
}

Controller Bus::getWinner() const
{
    return winner;
}

void Bus::busLog(std::stringstream &ss, LogLevel level)
{
    ::log(ss, busName, level);
}

void Bus::statIspis()
{
    std::stringstream msg;
    msg << "Broj iniciranih transakcija na magistrali: " << transNum;
    ::log(msg, "SYS");
}
