#include "Bus.hpp"

Bus::Bus()
{
    lastWinner = Controller::NONE;
    isBusy = false;
    currentOwner = Controller::NONE;
    currentSignal = BusSignal();
    sharedLine = false;
    canCollect = false;
    std::cout << "Sys: Magistrala na mrezi.\n";
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
    std::cout << Simulation::cycleCount << " Bus: magistrala oslobodjena\n";
    std::cout << "**************************************************************************************************\n";
}

void Bus::tick()
{
    // metode su koraci pipeline-a magistrale
    // 1. Ako niko ne koristi magistralu, pokušaj arbitražu
    if (!isBusy)
    {
        grantBus();
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
        else
        {
            // MAGISTRALA JE ZAMRZNUTA:
            // Ne radimo ništa, isBusy ostaje true, currentOwner ostaje isti.
            // U sledećem ciklusu main će opet pozvati tick() i opet ćemo doći ovde.
        }
    }
}
void Bus::requestBus(Controller devicePosting, const BusSignal &signal)
{
    activeSignals[devicePosting] = signal;
    std::cout << Simulation::cycleCount << " Bus: " << devicePosting << " zahteva pristup magistrali. Zeli da ";
    switch (signal.type)
    {
    case BusTransaction::READ:
        std::cout << "cita sa adrese " << signal.address.val;
        break;
    case BusTransaction::WRITE:
        std::cout << "upise " << signal.data.val << " na adresu " << signal.address.val;
        break;
    case BusTransaction::UPGRADE:
        std::cout << "invalidira tudje kopije na adresi " << signal.address.val;
        break;
    default:
        std::cerr << "GRESKA!";
    }
    std::cout << "\n";
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
    std::cout << Simulation::cycleCount << " Bus: Arbitriram signale ";
    for (auto it = activeSignals.begin(); it != activeSignals.end(); ++it)
    {
        // Koristimo it->first za ključ (kontroler) i it->second za vrednost (signal)
        const auto &kontroler = it->first;
        const auto &signal = it->second;

        std::cout << signal.type << " (" << kontroler << ")";

        // Provera da li je trenutni element poslednji u mapi
        if (std::next(it) == activeSignals.end())
        {
            std::cout << ".\n"; // Tačka na kraju poslednjeg
        }
        else
        {
            std::cout << ", "; // Zarez i novi red za ostale
        }
    }

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
    std::cout << "**************************************************************************************************\n";
    std::cout << Simulation::cycleCount << " Bus: Magistralu zauzeo " << currentOwner << ", transakcija " << currentSignal.type << "\n";
}

void Bus::broadcastRequest()
{
    // snoop se desava samo jednom, prilikom dodeljivanja magistrale

    // std::cout << "[BUS] REQUEST: CPU" << currentOwner << " emituje zahtev\n";

    // ovde se dešava snooping
    Controller winner = Controller::NONE; // ovde se desava arbitracija unutar arbitracije
    if (currentSignal.type != BusTransaction::UPGRADE)
    {
        Address addr = {currentSignal.address};
        for (auto &controller : cacheControllers)
        {
            if (controller->isHit(addr))
            {
                pullSharedLine(); // C signal
                std::cout << Simulation::cycleCount << " Bus: " << controller->getController()
                          << ": Imam kesiran blok na adresi " << addr.val << " u stanju " << controller->getState(addr) << ". Postavljam C liniju na visok napon.\n";
                // ARBITRACIJA: Prvi (najmanji ID) koji ima podatak postaje zadužen za slanje
                if (winner == Controller::NONE)
                {
                    winner = controller->getController();
                }
            }
        }
        if (winner != Controller::NONE) // nece se ispisivati ako je tran upgrade ili niko nema podatak
        {

            std::cout << Simulation::cycleCount << " Bus: Unutrasnja arbitraza - " << winner << " ce dostavljati podatak. \n";
        }
        if (sharedLineActive())
        {
            std::cout << Simulation::cycleCount << " Bus: Detektovana kopija u drugom kesu. C linija visoka.\n";
        }
        else
        {
            std::cout << Simulation::cycleCount << " Bus: Nijedan drugi kes nema podatak. C linija niska.\n";
        }
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
    // std::cout << Simulation::cycleCount << " Uredjaji na magistrali zavrsili osluskivanje. \n";
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
    std::cout << Simulation::cycleCount << " Bus: podatak postavljen na magistralu.\n";
    acknowledgeResponse();
}

void Bus::flush(Data data)
{
    currentSignal.type = BusTransaction::FLUSH;
    currentSignal.data = data;
    // nema acknowledge response jer magistrala treba da bude blokirana dok se podatak vadi iz mem
}

void Bus::collectResponses()
{
    // current signal je azuriran sa odgovorom
    // current owner je master
    // ovde se odgovor "vraca" onome ko je trazio podatak (masteru)
    // prvo upis u memoriju, isto ovde, pa tek onda vracanje masteru

    std::cout << Simulation::cycleCount << " Bus: Vracam podatak " << currentOwner << " (masteru).\n";
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
        break;
    }
}

void Bus::acknowledgeResponse()
{
    // Ovu metodu poziva RAM ili Cache kada završe
    canCollect = true;
    std::cout << Simulation::cycleCount << " Bus: kontroleri zavrsili sa odgovorom / presretanjem.\n";
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
        std::cout << Simulation::cycleCount << " " << controller << ": Odustajem od pristupa magistrali.\n";
    }
}
