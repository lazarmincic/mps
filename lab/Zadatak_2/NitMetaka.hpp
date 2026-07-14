#pragma once

#include <pthread.h>
#include "Postavke.hpp"
#include "NitAsteroida.hpp"


class NitMetaka {
public:
    static const int MAX_METAKA = 25;
private:
    pthread_t nit_id;
    double x_crtanje [MAX_METAKA];
    double y_crtanje [MAX_METAKA];
    double x_fizika [MAX_METAKA];
    double y_fizika [MAX_METAKA];
    int broj_metaka_crtanje = 0;
    int broj_metaka_fizika = 0;
    static void* statickiMost(void* arg);
    void petljaFizike();
    pthread_mutex_t muteks_crtanje;
    pthread_mutex_t muteks_fizika;
    int skor = 0;
    PozicijeSvih* tabla;
    bool radi = true;
    const double BRZINA_PO_SEKUNDI = Postavke::VISINA_EKRANA*1.1;
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
    void pristupi_deljenoj_iz_gl(double x_out[], double y_out [], int& broj, int& skor_out, int radi_in, int& broj_ast_out);
    void napraviNovi(double x_brod);
    void start();
    void stop();
    void proslediTablu(PozicijeSvih* tabla_sudar);
    void inicijalizuj (int pa, int ma, bool hard, NitAsteroida* asteroidi);
};