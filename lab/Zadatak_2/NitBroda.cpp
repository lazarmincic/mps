
#include "NitBroda.hpp"
#include "Merac.hpp"
#include <chrono>
#include "NitMetaka.hpp"
#include <unistd.h>
#include <iostream>

void* NitBroda::statickiMost(void* arg)  
{
    NitBroda* instanca = static_cast<NitBroda*>(arg);
    instanca->petljaFizike(); 
    return nullptr;
}

void NitBroda::petljaFizike() {
    Merac brod_fps("BROD");
    double x_lok_kop;
    x_lok_kop = Postavke::SIRINA_EKRANA / 2;
    const int BROD_SIRINA_EKRAN = B_SIRINA*FAKTOR_SKALIRANJA;
    double BRZINA_PO_S = Postavke::SIRINA_EKRANA/(double)3.5; // 0.03
    const long FRAME_TIME_NS = 1000000000/fps;
    bool radi_lok = true;
    std::chrono::time_point staro_vreme = std::chrono::high_resolution_clock::now();
    std::chrono::steady_clock::time_point vreme_poslednjeg_metka;
    const int COOLDOWN_METKA_MS = 112;
    Taster taster_lok;
    taster_lok.desno = false;
    taster_lok.levo = false;
    taster_lok.pucaj = false;

    while (true) {
        std::chrono::time_point trenutno_vreme = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> proteklo = trenutno_vreme - staro_vreme;
        double dt = proteklo.count(); //delta vreme
        staro_vreme = trenutno_vreme;

        double brzina_broda =  dt*BRZINA_PO_S;
        if (taster_lok.levo) { 
            if (x_lok_kop - brzina_broda >= 0) {
                x_lok_kop -= brzina_broda;
            } else {
                x_lok_kop = 0; 
            }
        }
        if (taster_lok.desno) { 
            if (x_lok_kop + brzina_broda <= Postavke::SIRINA_EKRANA - BROD_SIRINA_EKRAN) {
                x_lok_kop += brzina_broda;
            } else {
                x_lok_kop = Postavke::SIRINA_EKRANA - BROD_SIRINA_EKRAN;
            }
        }
        if (taster_lok.pucaj)
        {
            std::chrono::steady_clock::time_point vreme_tren_metka = std::chrono::steady_clock::now();
            long proslo_ms = std::chrono::duration_cast<std::chrono::milliseconds>(vreme_tren_metka - vreme_poslednjeg_metka).count();
            if (proslo_ms >= COOLDOWN_METKA_MS){
                meci->napraviNovi(x_lok_kop);
                vreme_poslednjeg_metka = vreme_tren_metka;
            }
            taster_lok.pucaj = false;
        }

        if (!radi_lok) break;
        pthread_mutex_lock(&lock);
        x = x_lok_kop;
        radi_lok = radi;
        taster_lok = taster;
        pthread_mutex_unlock(&lock);

        std::chrono::time_point vreme_nakon_fizike = std::chrono::high_resolution_clock::now();
        long potroseno_vreme = std::chrono::duration_cast<std::chrono::nanoseconds>(vreme_nakon_fizike - trenutno_vreme).count();
        long preostalo_za_spavanje = FRAME_TIME_NS - potroseno_vreme;
        if (preostalo_za_spavanje > 0)
        {
            timespec sleep_time = {0,preostalo_za_spavanje};
            nanosleep(&sleep_time, NULL);
        }
        else
        {
        }

        brod_fps.zabeleziFrejm();
        
    }
}

NitBroda::NitBroda() {
    nit_id = -1;
    x = Postavke::SIRINA_EKRANA / 2;
    radi = true;
    pthread_mutex_init(&lock, nullptr);
}

NitBroda::~NitBroda() {
    pthread_mutex_destroy(&lock);
}

void NitBroda::start() {
    pthread_create(&nit_id, nullptr, &statickiMost, this);
}

void NitBroda::stop() {
    int rez = pthread_join(nit_id, nullptr);
    if (rez == 0){
        //std::cout << "Nit broda uspesno joinovana\n";
    }
}


double NitBroda::pristupi_deljenoj_mem(bool program_radi, Taster taster) { // zove nit 0 
    double ret;
    pthread_mutex_lock(&lock);
    ret = x; 
    radi = program_radi;
    this->taster = taster;
    pthread_mutex_unlock(&lock);
    return ret;
}

void NitBroda::proslediMetke(NitMetaka* meci){
    this->meci = meci;
}

