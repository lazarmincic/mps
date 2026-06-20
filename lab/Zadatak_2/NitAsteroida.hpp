#pragma once

#include <pthread.h>
#include "Postavke.hpp"

struct PozicijeSvih {
    pthread_mutex_t muteks = PTHREAD_MUTEX_INITIALIZER;
    int x[Postavke::LIMIT_ASTEROIDA];
    int y[Postavke::LIMIT_ASTEROIDA];
    bool unisten [Postavke::LIMIT_ASTEROIDA];
    bool program_radi;
    int skor;
    int broj_asteroida;
};

class NitAsteroida {
    
private:
    pthread_t nit_id;
    int x;
    int y;
    static void* statickiMost(void* arg);
    void petljaFizike();
    bool aktivan;
    pthread_cond_t uslov_aktivacije;
    pthread_mutex_t muteks_aktivacije;
    bool radi;
    double brzina_po_sekundi;
    int i;
    const int fps = 60;
    PozicijeSvih* tabla_crtanje;
    PozicijeSvih* tabla_sudar;
    bool hard_mode;
    int POCETNI_BROJ_ASTEROIDA;
public:

    static const int A_VISINA = 8;
    static const int A_SIRINA = 8;
    static const int FAKTOR_SKALIRANJA = 5;
    static constexpr int dizajn_ast[8][8] = {
        {0, 0, 1, 2, 2, 1, 0, 0},
        {0, 1, 2, 2, 2, 2, 1, 0},
        {1, 2, 2, 2, 2, 2, 2, 1}, 
        {1, 2, 2, 3, 3, 2, 2, 1},
        {2, 2, 2, 3, 3, 2, 2, 2},
        {2, 2, 3, 3, 3, 3, 2, 2},
        {0, 2, 2, 3, 3, 2, 2, 0},
        {0, 0, 2, 2, 2, 2, 0, 0}, 
    };

    NitAsteroida();
    ~NitAsteroida();
    void start ();
    void stop();
    void aktiviraj();

    void inicijalizuj(bool akt, int id, PozicijeSvih* crt, PozicijeSvih* sud, bool hard, int pocetni_br_ast);
};


