#pragma once

#include <pthread.h>
#include "Postavke.hpp"
#include "NitAsteroida.hpp"


class NitMetaka {
public:
    static const int MAX_METAKA = 25;
private:
    pthread_t nit_id;
    int x_crtanje [MAX_METAKA];
    int y_crtanje [MAX_METAKA];
    int x_fizika [MAX_METAKA];
    int y_fizika [MAX_METAKA];
    int broj_metaka_crtanje = 0;
    int broj_metaka_fizika = 0;
    static void* statickiMost(void* arg);
    void petljaFizike();
    pthread_mutex_t muteks_crtanje;
    pthread_mutex_t muteks_fizika;
    int skor = 0;
    PozicijeSvih* tabla;
    bool radi = true;
    static const int BRZINA_PO_SEKUNDI = Postavke::VISINA_EKRANA*2;
    int max_asteroida = Postavke::LIMIT_ASTEROIDA;
    const int fps = 60;
    pthread_cond_t uslov_aktivacije;
    bool ima_aktivnih_metkova = false;
    NitAsteroida* asteroidi;
    int pocetni_broj_asteroida;
    int broj_asteroida;
    bool hard_mode;
public:
    static const int M_VISINA = 2;
    static const int M_SIRINA = 2;
    static const int FAKTOR_SKALIRANJA = 5;
    
    NitMetaka();
    ~NitMetaka();
    void uzmiPoziciju(int x_out[], int y_out [], int& broj, int& skor_out, int radi_in, int& broj_ast_out);
    void napraviNovi(int x_brod);
    void start();
    void stop();
    void proslediTablu(PozicijeSvih* tabla_sudar);
    void prosledi_max_asteroida (int pa, int ma, bool hard, NitAsteroida* asteroidi);
};