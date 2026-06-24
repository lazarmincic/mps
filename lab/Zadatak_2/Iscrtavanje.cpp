
#include "Iscrtavanje.hpp"
#include "Merac.hpp"
#include "NitAsteroida.hpp"
#include <random>
#include <iostream>
#include "CImg.h"

using namespace cimg_library;

Iscrtavanje::Iscrtavanje(int pocetni_broj, bool hard_mode) {
    pocetni_broj_asteroida = pocetni_broj;
    broj_asteroida = pocetni_broj_asteroida;
    is_hard_mode = hard_mode;
    if (is_hard_mode && Postavke::REZIM == SAMO_BROJ) {
        max_asteroida = Postavke::LIMIT_ASTEROIDA;
    }
    else {
        max_asteroida = pocetni_broj_asteroida;
    }
    asteroidi = new NitAsteroida [max_asteroida];
}

void Iscrtavanje::pokreniPetljuPrikaza() {
    std::random_device rd; 
    std::default_random_engine engine(rd()); 
    std::uniform_int_distribution dist_sirina(0, Postavke::SIRINA_EKRANA-1);  // []
    std::uniform_int_distribution dist_visina(0, Postavke::VISINA_EKRANA-1);

    brod.proslediMetke(&meci);

    PozicijeSvih tabla_crtanje;
    PozicijeSvih tabla_sudar;
    for (int i = 0; i<max_asteroida;i++){
        tabla_sudar.unisten[i] = false;
    }
    tabla_crtanje.program_radi = true;
    tabla_sudar.skor = 0;

    tabla_crtanje.broj_asteroida = pocetni_broj_asteroida;
    tabla_sudar.broj_asteroida = pocetni_broj_asteroida;

    meci.prosledi_max_asteroida(pocetni_broj_asteroida, max_asteroida, is_hard_mode, asteroidi);
    brod.start();
    
    for (int i = 0; i < max_asteroida; i++) {
        bool aktivan = true;
        if (i >= pocetni_broj_asteroida) aktivan = false;
        asteroidi[i].inicijalizuj(aktivan, i, &tabla_crtanje, &tabla_sudar, is_hard_mode, pocetni_broj_asteroida);
        asteroidi[i].start();
    }
    
    meci.proslediTablu(&tabla_sudar);
    meci.start();

    const int BROJ_ZVEZDA = 95;
    const double BRZINA_ZVEZDE_PO_S = Postavke::VISINA_EKRANA*0.5;
    struct Zvezda {
        double x;
        double y;
    };
    Zvezda* zvezde = new Zvezda[BROJ_ZVEZDA];
    for (int i = 0; i < BROJ_ZVEZDA; i++) {
        zvezde[i].x = dist_sirina(engine);
        zvezde[i].y = dist_visina(engine);
    }

    int skor = 0;
    bool won = false;

    bool asteroid_na_dnu = false;

    double meci_x [NitMetaka::MAX_METAKA], meci_y [NitMetaka::MAX_METAKA];
    for (int i =0; i<NitMetaka::MAX_METAKA; i++)
    {
        meci_y[i] = Y_POC_M;
    }

    double x_svi_a [max_asteroida];
    double y_svi_a [max_asteroida];

    bool petlja_radi = true;
    int broj_metaka = 0;
    
    Merac grafika_fps("GRAFIKA");
    std::chrono::time_point staro_vreme = std::chrono::high_resolution_clock::now();

    const unsigned char bela[] = { 255, 255, 255 };
    const unsigned char svetlo_siva[] = { 192, 192, 192 };
    const unsigned char plava[] = { 0, 191, 255 };
    const unsigned char tamno_siva[] = { 128, 128, 128 };
    const unsigned char narandzasta[] = { 221, 31, 19 };
    const unsigned char tamno_braon[] = { 121, 91, 54 };
    const unsigned char svetlo_braon[] = { 161, 133, 98 };
    const unsigned char krem[] = { 208, 192, 170 };
    const unsigned char crvena[] = {255, 0, 0};
    const unsigned char zelena[] = {0, 255, 0};

    CImg<unsigned char> platno(Postavke::SIRINA_EKRANA, Postavke::VISINA_EKRANA, 1, 3, 0);
    CImgDisplay window(platno, ime_prozora);
    Taster taster;
    taster.levo = false;
    taster.desno = false;
    taster.pucaj = false;

    while (true) {
        std::chrono::time_point trenutno_vreme = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> proteklo = trenutno_vreme - staro_vreme;
        double dt = proteklo.count(); //delta vreme 
        staro_vreme = trenutno_vreme;

        // pozadina
        platno.fill(0);

        // zvezde
        int radijus = 2.0;
        for (int i = 0; i < BROJ_ZVEZDA; i++) {
            // Pomeramo zvezdu nadole 
            zvezde[i].y += BRZINA_ZVEZDE_PO_S * dt;
            if (zvezde[i].y >= Postavke::VISINA_EKRANA) {
                zvezde[i].y = 0;
                zvezde[i].x = dist_sirina(engine);
            }
            platno.draw_circle((int)zvezde[i].x, (int)zvezde[i].y, radijus, bela);
        }

        taster.levo = window.is_keyARROWLEFT() || window.is_keyA();
        taster.desno = window.is_keyARROWRIGHT() || window.is_keyD();
        taster.pucaj = window.is_keySPACE() || window.is_keyW();
        
        // brod
        double brod_x, brod_y; // lokalne prom
        brod_x = brod.uzmi_poziciju_x_prosledi_radi(petlja_radi, taster);
        brod_y = NitBroda::Y;
        for (int r = 0; r < NitBroda::B_VISINA; r++) {
            for (int k = 0; k < NitBroda::B_SIRINA; k++) {
                
                int tip_piksela = NitBroda::dizajn_broda[r][k];

                if (tip_piksela == 0) {
                    continue; 
                }

                int ekran_x = (int)brod_x + (k * NitBroda::FAKTOR_SKALIRANJA);
                int ekran_y = (int)brod_y + (r * NitBroda::FAKTOR_SKALIRANJA);

                int kraj_x = ekran_x + NitBroda::FAKTOR_SKALIRANJA - 1;
                int kraj_y = ekran_y + NitBroda::FAKTOR_SKALIRANJA - 1;

                const unsigned char* boja = nullptr;
                switch (tip_piksela) {
                    case 1: boja = svetlo_siva; break;
                    case 2: boja = bela; break;
                    case 3: boja = plava; break;
                    case 4: boja = tamno_siva; break;
                    default: continue; 
                }
                platno.draw_rectangle(ekran_x, ekran_y, kraj_x, kraj_y, boja);
            }
        }

        //meci
        meci.uzmiPoziciju(meci_x, meci_y, broj_metaka, skor, petlja_radi, broj_asteroida);
        for (int i = 0;i < broj_metaka; i++)
        {
            double x_i = meci_x[i];
            double y_i = meci_y[i];
            for (int r = 0; r < NitMetaka::M_VISINA; r++) {
                for (int k = 0; k < NitMetaka::M_SIRINA; k++) {
                    int ekran_x = (int)x_i + (k * NitMetaka::FAKTOR_SKALIRANJA);
                    int ekran_y = (int)y_i + (r * NitMetaka::FAKTOR_SKALIRANJA);
                    int kraj_x = ekran_x + NitMetaka::FAKTOR_SKALIRANJA - 1;
                    int kraj_y = ekran_y + NitMetaka::FAKTOR_SKALIRANJA - 1;
                    platno.draw_rectangle(ekran_x, ekran_y, kraj_x, kraj_y, narandzasta);
                }
            }
        }

        //asteroidi
        const double y_min = Postavke::VISINA_EKRANA - NitAsteroida::A_VISINA*NitAsteroida::FAKTOR_SKALIRANJA;
        pthread_mutex_lock(&tabla_crtanje.muteks);
        for (int i = 0; i < broj_asteroida; i++) {
            x_svi_a[i] = tabla_crtanje.x[i];
            y_svi_a[i] = tabla_crtanje.y[i];
        }
        tabla_crtanje.program_radi = petlja_radi;
        pthread_mutex_unlock(&tabla_crtanje.muteks);
        for (int i = 0; i < broj_asteroida; i++) {
            double ast_x, ast_y;
            ast_x = x_svi_a[i];
            ast_y = y_svi_a[i];
            if (ast_y == y_min)
            {
                asteroid_na_dnu = true;
            }
            for (int r = 0; r < NitAsteroida::A_VISINA; r++) {
                for (int k = 0; k < NitAsteroida::A_SIRINA; k++) {
                    int tip_piksela = NitAsteroida::dizajn_ast[r][k];
                    if (tip_piksela == 0) {
                        continue; 
                    }
                    int ekran_x = (int)ast_x + (k * NitAsteroida::FAKTOR_SKALIRANJA);
                    int ekran_y = (int)ast_y + (r * NitAsteroida::FAKTOR_SKALIRANJA);
                    int kraj_x = ekran_x + NitAsteroida::FAKTOR_SKALIRANJA - 1;
                    int kraj_y = ekran_y + NitAsteroida::FAKTOR_SKALIRANJA - 1;

                    const unsigned char* boja = nullptr;
                    
                    switch (tip_piksela) {
                        case 1: boja = tamno_braon; break;
                        case 2: boja = svetlo_braon; break;
                        case 3: boja = krem; break;
                        default: continue; 
                    }
                    platno.draw_rectangle(ekran_x, ekran_y, kraj_x, kraj_y, boja);
                }
            }
        }

        //score
        std::string tekst_score = "Score: " + std::to_string(skor);
        platno.draw_text(10, 10, tekst_score.c_str(), bela, 0, 1.0f, 20);
        
        
        window.display(platno);
        //window.wait(1);
        // nakon svih iscrtavanja
        
        if (!petlja_radi) break;

        // uslov za izlazak
        if (window.is_closed() || window.is_keyESC()) {
            petlja_radi = false;
            window.close(); 
        }
        if (skor >= Postavke::WIN_POINTS)
        {
            petlja_radi = false;
            won = true;
        }
        if (asteroid_na_dnu)
        {
            petlja_radi = false;
        }
        
        grafika_fps.zabeleziFrejm();
        
    }

    std::string tekst_za_ispis;
    const unsigned char* boja = nullptr;
    if (asteroid_na_dnu)
    {
        tekst_za_ispis = "Game over";
        boja = crvena;
    }
    else if (won)
    {
        tekst_za_ispis = "You won";
        boja = zelena;
    }

    bool joined = false;
    
    if (!tekst_za_ispis.empty())
    {
        while (true) 
        {
            // dt
            std::chrono::time_point trenutno_vreme = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> proteklo = trenutno_vreme - staro_vreme;
            double dt = proteklo.count(); //delta vreme 
            staro_vreme = trenutno_vreme;
            if (window.is_closed() || window.is_keyESC()) {
                window.close(); 
                break;
            }
            // pozadina
            platno.fill(0);

            // zvezde
            int radijus = 2.0;
            for (int i = 0; i < BROJ_ZVEZDA; i++) {
                zvezde[i].y += BRZINA_ZVEZDE_PO_S * dt;
                if (zvezde[i].y >= Postavke::VISINA_EKRANA) {
                    zvezde[i].y = 0;
                    zvezde[i].x = dist_sirina(engine);
                }
                platno.draw_circle((int)zvezde[i].x, (int)zvezde[i].y, radijus, bela);
            }
        
            // tekst
            const int fontSize = 113;
            CImg<unsigned char> imgtext;
            const unsigned char b = 1;
            imgtext.draw_text(0,0,tekst_za_ispis.c_str(),&b,0,1,fontSize);
            int x = (Postavke::SIRINA_EKRANA - imgtext.width()) / 2;
            int y = (Postavke::VISINA_EKRANA - imgtext.height()) / 2;

            // crtaj
            platno.draw_text(x, y, tekst_za_ispis.c_str(), boja, 0, 1.0f, fontSize);
            window.display(platno);
            if (!joined){
                brod.stop();
                for (int i = 0; i < max_asteroida; i++) {
                    asteroidi[i].stop();
                }
                meci.stop();
                joined = true;
            }
            else {
                window.wait(1);
            }
        }
    }
    delete[] zvezde;
    
}

Iscrtavanje::~Iscrtavanje() {
    delete[] asteroidi;
}

