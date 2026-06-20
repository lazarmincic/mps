
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
    int x_lok_kop;
    x_lok_kop = Postavke::SIRINA_EKRANA / 2;
    double x_prec = x_lok_kop; 
    const int BROD_SIRINA_EKRAN = B_SIRINA*FAKTOR_SKALIRANJA;
    double BRZINA_PO_S = Postavke::SIRINA_EKRANA/(double)2.5; // 0.03
    const long FRAME_TIME_NS = 1000000000/fps;
    bool radi_lok = true;
    bool idi_levo = false, idi_desno = false, ispali_metak = false;
    std::chrono::time_point staro_vreme = std::chrono::high_resolution_clock::now();
    std::chrono::steady_clock::time_point vreme_poslednjeg_metka;
    const int COOLDOWN_METKA_MS = 100;
    while (true) {
        if (radi_lok == false) break;
        std::chrono::time_point trenutno_vreme = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> proteklo = trenutno_vreme - staro_vreme;
        double dt = proteklo.count(); //delta vreme
        staro_vreme = trenutno_vreme;

        pthread_mutex_lock(&lock);
        idi_levo = kontrole.levo.aktivan;
        idi_desno = kontrole.desno.aktivan;
        ispali_metak = kontrole.pucaj.aktivan;
        x = x_lok_kop;
        radi_lok = radi;
        pthread_mutex_unlock(&lock);

        if (!radi_lok) break;

        double brzina_broda =  dt*BRZINA_PO_S;
        if (idi_levo) { 
            if (x_prec - brzina_broda >= 0) {
                x_prec -= brzina_broda;
            } else {
                x_prec = 0; 
            }
        }
        if (idi_desno) { 
            if (x_prec + brzina_broda <= Postavke::SIRINA_EKRANA - BROD_SIRINA_EKRAN) {
                x_prec += brzina_broda;
            } else {
                x_prec = Postavke::SIRINA_EKRANA - BROD_SIRINA_EKRAN;
            }
        }
        if (ispali_metak)
        {
            std::chrono::steady_clock::time_point vreme_tren_metka = std::chrono::steady_clock::now();
            long proslo_ms = std::chrono::duration_cast<std::chrono::milliseconds>(vreme_tren_metka - vreme_poslednjeg_metka).count();
            if (proslo_ms >= COOLDOWN_METKA_MS){
                meci->napraviNovi(x_lok_kop);
                vreme_poslednjeg_metka = vreme_tren_metka;
            }
        }

        x_lok_kop = static_cast<int>(x_prec);

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
        std::cout << "Nit broda uspesno joinovana\n";
    }
}


int NitBroda::uzmi_poziciju_x_osvezi_kontrole(KontroleIgre kontrole, bool program_radi) { // zove nit 0 
    int ret;
    pthread_mutex_lock(&lock);
    ret = x; 
    this->kontrole = kontrole;
    radi = program_radi;
    pthread_mutex_unlock(&lock);
    return ret;
}

void NitBroda::proslediMetke(NitMetaka* meci){
    this->meci = meci;
}

