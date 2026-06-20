#pragma once

#include <pthread.h>
#include "Postavke.hpp"
#include <chrono>

class NitMetaka;

struct TasterStanje {
    bool aktivan = false;
    std::chrono::steady_clock::time_point vreme_poslednjeg_impulsa;
};

struct KontroleIgre {
    TasterStanje levo;   
    TasterStanje desno;  
    TasterStanje pucaj;
};

class NitBroda {
private:
    pthread_t nit_id;
    pthread_mutex_t lock;
    int x;
    bool radi;
    static void* statickiMost(void* arg);
    void petljaFizike();
    NitMetaka* meci;
    const int fps = 120;
    KontroleIgre kontrole;
    bool ima_metaka = false;
public:
    static const int B_VISINA = 12;
    static const int B_SIRINA = 10;
    static const int FAKTOR_SKALIRANJA = 5; // eksperimentalno
    static const int Y = Postavke::VISINA_EKRANA - B_VISINA*FAKTOR_SKALIRANJA - 4*FAKTOR_SKALIRANJA;

    static constexpr int dizajn_broda[12][10] = {
        {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 2, 2, 0, 0, 0, 0},
        {0, 0, 0, 1, 2, 2, 1, 0, 0, 0}, 
        {4, 0, 0, 1, 2, 2, 1, 0, 0, 4},
        {1, 0, 1, 1, 1, 1, 1, 1, 0, 1},
        {1, 0, 1, 1, 1, 1, 1, 1, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 0, 1, 1, 1, 1, 0, 1, 1}, 
        {4, 0, 0, 0, 1, 1, 0, 0, 0, 4},
        {3, 0, 0, 0, 3, 3, 0, 0, 0, 3},
        {0, 0, 0, 0, 0, 3, 0, 0, 0, 0}, 
        {0, 0, 0, 0, 3, 0, 0, 0, 0, 0}
    };
    
    NitBroda();

    ~NitBroda();

    void start();

    void stop();

    int uzmi_poziciju_x_osvezi_kontrole(KontroleIgre kontrole, bool program_radi) ;
    void proslediMetke(NitMetaka* meci);

};

