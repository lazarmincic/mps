#include "Merac.hpp"
#include <chrono>
#include <sstream>
#include <iostream>

Merac::Merac (std::string ime) {
    brojac_frejmova = 0;
    ime_niti = ime;
    poslednji_ispis = snimiTrenutnoVreme();
}

void Merac::zabeleziFrejm() {
    brojac_frejmova++;
    std::chrono::high_resolution_clock::time_point trenutno_vreme = snimiTrenutnoVreme();
    std::chrono::duration trajanje = trenutno_vreme - poslednji_ispis;
    double izm_vreme = std::chrono::duration<double>(trajanje).count();

    // ako je prošlo više od 1 sekunde ispiši fps u konzolu
    if (izm_vreme >= 1.0) {
        std::stringstream ss;
        ss << "[" << ime_niti << "] FPS: " << brojac_frejmova << "\n";
        //bezbedanIspis(ss.str());
        brojac_frejmova = 0;
        poslednji_ispis = trenutno_vreme;
    }
}

void Merac::bezbedanIspis(const std::string& poruka) {
    // Inicijalizuje se samo jednom
    static pthread_mutex_t muteks_funkcije = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&muteks_funkcije);
    std::cout << poruka;
    pthread_mutex_unlock(&muteks_funkcije);
}

std::chrono::high_resolution_clock::time_point Merac::snimiTrenutnoVreme (){
    return std::chrono::high_resolution_clock::now();
}
