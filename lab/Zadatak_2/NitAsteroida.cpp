#include "NitAsteroida.hpp"
#include <sstream>
#include "Merac.hpp"
#include <random>
#include "Postavke.hpp"
#include <chrono>
#include <iostream>

NitAsteroida::NitAsteroida(){
    pthread_cond_init(&uslov_aktivacije, NULL);
    pthread_mutex_init(&muteks_aktivacije, NULL);
    aktivan = false; // Na početku su svi neaktivni
    radi = true; // igrica radi
    brzina_po_sekundi = 0;
};

NitAsteroida::~NitAsteroida() {
        pthread_cond_destroy(&uslov_aktivacije);
        pthread_mutex_destroy(&muteks_aktivacije);
}

void* NitAsteroida::statickiMost(void* arg) {
    NitAsteroida* instanca = static_cast<NitAsteroida*>(arg);
    instanca->petljaFizike(); 
    return nullptr;
}

void NitAsteroida::start() {
    pthread_create(&nit_id, nullptr, &statickiMost, this);
}

void NitAsteroida::stop() {
    pthread_mutex_lock(&muteks_aktivacije);
    aktivan = true;
    pthread_cond_signal(&uslov_aktivacije);
    pthread_mutex_unlock(&muteks_aktivacije);
    int rez = pthread_join(nit_id, NULL);
    if (rez == 0)
    {
        //std::cout << "Nit asteroida " << i << " uspesno joinovana\n";
    }
    // nebezbedan ispis u debug svrhe.
}

void NitAsteroida::petljaFizike() {
    std::stringstream ispis;
    ispis << "ASTEROID " << i;
    Merac ast_fps(ispis.str());
    
    const long FRAME_TIME_NS = 1000000000/fps;
    int broj_asteroida = POCETNI_BROJ_ASTEROIDA;
    int skor = 0;
    
    std::random_device rd;
    std::default_random_engine engine(rd());
    std::uniform_int_distribution dist_sirina(0, Postavke::SIRINA_EKRANA - FAKTOR_SKALIRANJA*A_SIRINA);
    x =  dist_sirina(engine); 
    std::chrono::time_point staro_vreme = std::chrono::high_resolution_clock::now();
    while(radi) {
        std::chrono::time_point trenutno_vreme = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> proteklo = trenutno_vreme - staro_vreme;
        double dt = proteklo.count(); //delta vreme
        staro_vreme = trenutno_vreme;

        pthread_mutex_lock(&muteks_aktivacije);
        while (!aktivan) {
            pthread_cond_wait(&uslov_aktivacije, &muteks_aktivacije);
        }
        staro_vreme = std::chrono::high_resolution_clock::now();
        pthread_mutex_unlock(&muteks_aktivacije);
    
        // provera unistenja
        pthread_mutex_lock(&tabla_sudar->muteks);
        if (tabla_sudar->unisten[i] == true) {
            y = 0;
            x = dist_sirina(engine);
            tabla_sudar->unisten[i] = false;
        }
        if (hard_mode && Postavke::REZIM == SAMO_BROJ) {
            broj_asteroida = tabla_sudar->broj_asteroida;
        }
        else if (hard_mode && Postavke::REZIM == SAMO_BRZINA) {
            skor = tabla_sudar->skor;
        }
        pthread_mutex_unlock(&tabla_sudar->muteks);
        if (!radi) break; 

        // fizika
        const double FAKTOR_OPADANJA = 0.2; // eksp.
        const double MAX_BRZINA = Postavke::VISINA_EKRANA / 8;
        const double MIN_BRZINA = Postavke::VISINA_EKRANA / 40;
        int dodatni_asteroidi = POCETNI_BROJ_ASTEROIDA - Postavke::MIN_ASTEROIDA;
        if (!hard_mode){
            // sto je broj asteroida veci, brzina je manja. brzina se ne menja.
            dodatni_asteroidi = broj_asteroida - Postavke::MIN_ASTEROIDA;
            brzina_po_sekundi = MIN_BRZINA + (MAX_BRZINA - MIN_BRZINA) / (1.0 + FAKTOR_OPADANJA * dodatni_asteroidi);
        }
        if (hard_mode && Postavke::REZIM == SAMO_BRZINA)
        {
            // samo brzina se menja, broj asteroida konstanta.
            double BRZINA_STD = MIN_BRZINA + (MAX_BRZINA - MIN_BRZINA) / (1.0 + FAKTOR_OPADANJA * dodatni_asteroidi);
            const double POC_BRZINA = BRZINA_STD/5;
            const double MAX_BRZINA = BRZINA_STD;
            
            brzina_po_sekundi = POC_BRZINA + ((MAX_BRZINA - POC_BRZINA)*(double(skor)/Postavke::WIN_POINTS));
            if (brzina_po_sekundi > MAX_BRZINA) {
                brzina_po_sekundi = MAX_BRZINA;
            }
        }
        if (hard_mode && Postavke::REZIM == SAMO_BROJ){
            // samo broj ast. se menja, brzina konstanta.
            brzina_po_sekundi = (MAX_BRZINA + MIN_BRZINA)/2;
        }
            
        y += brzina_po_sekundi*dt;
        if (y >= Postavke::VISINA_EKRANA - A_VISINA*FAKTOR_SKALIRANJA) {
            y = Postavke::VISINA_EKRANA - A_VISINA*FAKTOR_SKALIRANJA;
        }

        pthread_mutex_lock(&tabla_crtanje->muteks);
        tabla_crtanje->x[i] = x;
        tabla_crtanje->y[i] = y;
        radi = tabla_crtanje->program_radi;
        pthread_mutex_unlock(&tabla_crtanje->muteks);
        

        pthread_mutex_lock(&tabla_sudar->muteks);
        tabla_sudar->x[i] = x;
        tabla_sudar->y[i] = y;
        pthread_mutex_unlock(&tabla_sudar->muteks);
        
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
        
        ast_fps.zabeleziFrejm();
    }
}

void NitAsteroida::aktiviraj() {
    pthread_mutex_lock(&muteks_aktivacije);
    aktivan = true;
    pthread_cond_signal(&uslov_aktivacije); 
    pthread_mutex_unlock(&muteks_aktivacije);
}

void NitAsteroida::inicijalizuj(bool akt, int id, PozicijeSvih* crt, PozicijeSvih* sud, bool hard, int pocetni_br_ast) {
    y = 0;
    aktivan = akt;
    i = id;
    tabla_crtanje = crt;
    tabla_sudar = sud;
    hard_mode = hard;
    POCETNI_BROJ_ASTEROIDA = pocetni_br_ast;
}