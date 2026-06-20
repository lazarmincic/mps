#include "NitMetaka.hpp"
#include "Merac.hpp"
#include "Iscrtavanje.hpp"
#include "NitAsteroida.hpp"
#include <unistd.h>

NitMetaka::NitMetaka(){
    pthread_mutex_init(&muteks_crtanje, nullptr);
    pthread_mutex_init(&muteks_fizika, nullptr);
    pthread_cond_init(&uslov_aktivacije, NULL);
    broj_metaka_fizika = 0;
    broj_metaka_crtanje = 0;
    broj_asteroida = pocetni_broj_asteroida;
}

NitMetaka::~NitMetaka(){
    pthread_mutex_destroy(&muteks_crtanje);
    pthread_mutex_destroy(&muteks_fizika);
    pthread_cond_destroy(&uslov_aktivacije);
}

void* NitMetaka::statickiMost(void* arg)  
{
    NitMetaka* instanca = static_cast<NitMetaka*>(arg);
    instanca->petljaFizike(); 
    return nullptr;
}

void NitMetaka::petljaFizike() {
    Merac met_fps("METKOVI");
    const long FRAME_TIME_NS = 1000000000/fps;
    double y_prec[MAX_METAKA];
    int y_lok[MAX_METAKA];
    int x_lok[MAX_METAKA];
    int x_ast[max_asteroida];
    int y_ast[max_asteroida];
    int skor_lok = 0;
    bool radi_lok = true;
    int stari_broj_asteroida = pocetni_broj_asteroida;
    const int y_min = -FAKTOR_SKALIRANJA * M_VISINA;
    int broj_metaka_lok = 0;
    int broj_asteroida_lok = pocetni_broj_asteroida;

    for(int i = 0; i < MAX_METAKA; i++) {
        y_prec[i] = 0.0; 
    }
    
    const double FAKTOR_RASTA_A = static_cast<double>(Postavke::LIMIT_ASTEROIDA - pocetni_broj_asteroida) / Postavke::WIN_POINTS;
    std::chrono::time_point staro_vreme = std::chrono::high_resolution_clock::now();
    
    while(true) {
        std::chrono::time_point trenutno_vreme = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> proteklo = trenutno_vreme - staro_vreme;
        double dt = proteklo.count(); // delta vreme
        staro_vreme = trenutno_vreme;
        
        // racunanje broja asteroida
        if (hard_mode){
            broj_asteroida_lok = pocetni_broj_asteroida + static_cast<int>(skor_lok * FAKTOR_RASTA_A);
            if (broj_asteroida_lok > Postavke::LIMIT_ASTEROIDA) {
                broj_asteroida_lok = Postavke::LIMIT_ASTEROIDA;
            }
        }
        else
        {
            broj_asteroida_lok = pocetni_broj_asteroida;
        }
        // budi asteroide 
        if (broj_asteroida_lok > stari_broj_asteroida) {
            for (int i = stari_broj_asteroida; i < broj_asteroida_lok; i++) {
                if (i < Postavke::LIMIT_ASTEROIDA) { 
                    asteroidi[i].aktiviraj();
                }
            }
            stari_broj_asteroida = broj_asteroida_lok;
        }

        // pokupi pozicije svih asteroida
        pthread_mutex_lock(&tabla->muteks);
        tabla->broj_asteroida = broj_asteroida_lok;
        for (int i = 0; i < broj_asteroida_lok; i++) {
            x_ast[i] = tabla->x[i];
            y_ast[i] = tabla->y[i];
        }
        pthread_mutex_unlock(&tabla->muteks);

        // nit za iscrtavanje zastita
        pthread_mutex_lock(&muteks_crtanje);
        skor = skor_lok;
        broj_metaka_crtanje = broj_metaka_lok;
        for (int i = 0; i < broj_metaka_crtanje; i++){
            x_crtanje[i] = x_lok[i];
            y_crtanje[i] = y_lok[i];
        }
        radi_lok = radi;
        broj_asteroida = broj_asteroida_lok;
        pthread_mutex_unlock(&muteks_crtanje);
        if (!radi_lok) break;

        // nit broda zastita
        pthread_mutex_lock(&muteks_fizika);
        while (broj_metaka_fizika > 0) {
            // Ako ima slobodnog mesta u lokalnoj fizici, prebaci metak unutra
            if (broj_metaka_lok < MAX_METAKA) {
                x_lok[broj_metaka_lok] = x_fizika[broj_metaka_fizika - 1];
                y_lok[broj_metaka_lok] = y_fizika[broj_metaka_fizika - 1];
                y_prec[broj_metaka_lok] = static_cast<double>(y_fizika[broj_metaka_fizika - 1]);
                broj_metaka_lok++;
            }
            // Skidamo metak sa deljene table, brod sada može bezbedno da piše ovde
            broj_metaka_fizika--;
        }
        if (broj_metaka_lok <= 0 && radi_lok) {
            ima_aktivnih_metkova = false;
        }
        
        
        while (!ima_aktivnih_metkova)
        {
            pthread_cond_wait(&uslov_aktivacije, &muteks_fizika);
        }
            
        staro_vreme = std::chrono::high_resolution_clock::now();
        pthread_mutex_unlock(&muteks_fizika);

        

        // racunanje
        for (int i = 0; i < broj_metaka_lok; i++){
            y_prec[i] -= BRZINA_PO_SEKUNDI * dt * 0.5;
            
            // Provera izlaska sa ekrana
            if (y_prec[i] <= y_min) {
                if (i < broj_metaka_lok - 1) {
                    x_lok[i] = x_lok[broj_metaka_lok - 1];
                    y_lok[i] = y_lok[broj_metaka_lok - 1];
                    y_prec[i] = y_prec[broj_metaka_lok - 1];
                }
                broj_metaka_lok--;
                i--; 
                continue; 
            }
            else {
                y_lok[i] = static_cast<int>(y_prec[i]);
            }

            // Računanje sudara sa asteroidima (AABB sa tolerancijom)
            bool metak_unistio_nesto = false;
            const int TOLER = 25;

            for (int k = 0; k < broj_asteroida_lok; k++) {
                if ((x_lok[i] < x_ast[k] + NitAsteroida::A_SIRINA + TOLER) && 
                    (x_lok[i] + M_SIRINA > x_ast[k] - TOLER) && 
                    (y_lok[i] < y_ast[k] + NitAsteroida::A_VISINA + TOLER) && 
                    (y_lok[i] + M_VISINA > y_ast[k] - TOLER))
                {
                    skor_lok++;
                    // Šaljemo signal niti konkretnog asteroida da se resetuje
                    pthread_mutex_lock(&tabla->muteks);
                    tabla->unisten[k] = true;
                    pthread_mutex_unlock(&tabla->muteks);

                    // Brišemo metak lokalno tako što ga zamenimo poslednjim aktivnim
                    if (i < broj_metaka_lok - 1) {
                        x_lok[i] = x_lok[broj_metaka_lok - 1];
                        y_lok[i] = y_lok[broj_metaka_lok - 1];
                        y_prec[i] = y_prec[broj_metaka_lok - 1];
                    }
                    broj_metaka_lok--;
                    i--;
                    metak_unistio_nesto = true;
                    break;
                }
            }
            if (metak_unistio_nesto) {
                continue; 
            }
        }

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

        met_fps.zabeleziFrejm();
    }
}

void NitMetaka::uzmiPoziciju(int x_out[], int y_out [], int& broj, int& skor_out, int radi_in, int &broj_ast_out)
{
    pthread_mutex_lock(&muteks_crtanje);
    skor_out = skor;
    broj = broj_metaka_crtanje;
    for (int i = 0; i < broj_metaka_crtanje; i++) {
        x_out[i] = x_crtanje[i];
        y_out[i] = y_crtanje[i];
    }
    radi = radi_in;
    broj_ast_out = broj_asteroida;
    pthread_mutex_unlock(&muteks_crtanje);
}

void NitMetaka::napraviNovi(int x_brod){
    const int x_pomeraj = NitBroda::FAKTOR_SKALIRANJA * (NitBroda::B_SIRINA / 2 - 1);
    pthread_mutex_lock(&muteks_fizika);
    if (broj_metaka_fizika < MAX_METAKA){
        x_fizika[broj_metaka_fizika] = x_brod + x_pomeraj;
        y_fizika[broj_metaka_fizika] = Iscrtavanje::Y_POC_M;
        broj_metaka_fizika++;
    }
    if (!ima_aktivnih_metkova)
    {
        ima_aktivnih_metkova = true;
        pthread_cond_signal(&uslov_aktivacije);
    }
    pthread_mutex_unlock(&muteks_fizika);
}

void NitMetaka::start(){
    pthread_create(&nit_id, nullptr, &statickiMost, this);
}

void NitMetaka::stop(){
    pthread_mutex_lock(&muteks_fizika);
    ima_aktivnih_metkova = true;
    pthread_cond_signal(&uslov_aktivacije);
    pthread_mutex_unlock(&muteks_fizika);
    int rez = pthread_join(nit_id, nullptr);
    if (rez == 0){
        std::cout << "Nit metaka uspesno joinovana\n";
    }
}

void NitMetaka::proslediTablu(PozicijeSvih* tabla_sudar){
    this->tabla = tabla_sudar;
}

void NitMetaka::prosledi_max_asteroida (int pa,int ma, bool hard, NitAsteroida* asteroidi){
    pocetni_broj_asteroida = pa;
    max_asteroida = ma;
    hard_mode = hard;
    this->asteroidi = asteroidi;
}
