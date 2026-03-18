#include "CacheController.hpp"

CacheController::CacheController(int id) : myID(id), bus(nullptr) {}

int32_t CacheController::read(uint32_t address) {
        // 1. Izračunaj index (bit 2) i tag (bitovi 31:3)
    int index = (address >> 2) & 0x1;      // 1 bit
    uint32_t tag = (address >> 3);         // Bitovi 31:3
    
    // 2. Pretraži keš memoriju (dva seta)
    for (int i = 0; i < 2; i++) {
        if (cache[i].tag == tag && cache[i].state.getState() != CacheState::I) {
            // HIT - linija je pronađena i validna
            std::cout << "[" << myID << "] CACHE HIT @ 0x" << std::hex << address << std::dec << "\n";
            return cache[i].data;
        }
    }
    // 3. MISS - nema linije u kešu
    std::cout << "[" << myID << "] CACHE MISS @ 0x" << std::hex << address << std::dec << "\n";

    // učitavanje iz memorije, magistrala će vratiti podatak
    return bus->readMiss(address, myID);
    
}

void CacheController::write(uint32_t address, int32_t data) {
}

void CacheController::setBus(Bus* b) {
    bus = b;
}

void CacheController::onBusSignal(const BusSignal& signal) {
    if (signal.initiatorCPU == myID) return;

    // Pronađi liniju
    int setIndex = findLineInCache(signal.address);
    
    if (signal.type == BusTransaction::READ) {
        // Neko čita - ako imam liniju, postavi C signal
        if (setIndex != -1) {
            // Imam liniju!
            std::cout << "[CPU" << myID << "] SNOOP: Vidim READ @ 0x" << std::hex 
                      << signal.address << " → Assert C signal\n" << std::dec;
            
            // C signal ostaje HIGH jer vidim kopiju
            // (čuvamo u bus.copiesExist = true)
        }
    }
    
    if (signal.type == BusTransaction::WRITE) {
        if (setIndex != -1) {
            // Invalidiraj moju kopiju
            cache[setIndex].state.setI();
        }
    }
}

int CacheController::findLineInCache(uint32_t address) {
    uint32_t tag = (address >> 3);
    for (int i = 0; i < 2; i++) {
        if (cache[i].tag == tag && !cache[i].state.isI()) {
            return i;
        }
    }
    return -1;  // Nema linije
}

bool CacheController::hasLine(uint32_t address) {
    return findLineInCache(address) != -1;
}
