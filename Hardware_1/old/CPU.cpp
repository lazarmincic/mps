#include "CacheController.cpp"

#include <vector>
#include <map>
#include <fstream>

// procesor dobija ulaznu datoteku sa asembler kodom
// neću modelovati hardver riscv procesora ovde
// fokus je na hardveru za koherenciju keša
// tri tipa instrukcija 
// 1. grananja - na visokom nivou (preko pc)
// 2. memorijske instrukcije - interesantne, komunikacija sa kontrolerom keša
// 3. ostale (npr. aritmetika) - na visokom nivou

class CPU {
private:
    CacheController* cacheController; // CPU ima pristup svom kontroleru
    std::vector<Instruction> program; // dinamički niz svih instrukcija iz fajla
    std::map<std::string, int> labels; // labele i njihove vrednosti pc

public:
    CPU(CacheController* cacheController){
        cacheController = cacheController;
    }
    int pc = 0;
    void loadProgram(const std::string& filename);
    void step();

};

struct Instruction {
    std::string name;
    std::vector<std::string> args;
};

void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Greška: Ne mogu da otvorim " << filename << std::endl;
            return;
        }
}