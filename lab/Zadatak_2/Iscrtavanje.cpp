#include <opencv2/opencv.hpp>
#include "Iscrtavanje.hpp"
#include "Merac.hpp"
#include "NitAsteroida.hpp"
#include <random>
#include <iostream>

using namespace cv;

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

    namedWindow(ime_prozora, WINDOW_AUTOSIZE | WINDOW_GUI_NORMAL);
    Mat matrica_ekrana = Mat::zeros(Postavke::VISINA_EKRANA, Postavke::SIRINA_EKRANA, CV_8UC3);
    imshow(ime_prozora, matrica_ekrana);
    int taster = waitKey(1);

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

    const Scalar bela = { 255, 255, 255 };
    const Scalar svetlo_siva = { 192, 192, 192 };
    const Scalar plava = { 255, 191, 0 };
    const Scalar tamno_siva = { 128, 128, 128 };
    const Scalar narandzasta = { 19, 31, 221 };
    const Scalar tamno_braon = { 54, 91, 121 };
    const Scalar svetlo_braon = { 98, 133, 161 };
    const Scalar krem = { 170, 192, 208 };
    const Scalar crvena = { 0, 0, 255 };
    const Scalar zelena = { 0, 255, 0 };

    Taster kontrole;
    kontrole.levo = false;
    kontrole.desno = false;
    kontrole.pucaj = false;


    while (true) {
        std::chrono::time_point trenutno_vreme = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> proteklo = trenutno_vreme - staro_vreme;
        double dt = proteklo.count(); //delta vreme 
        staro_vreme = trenutno_vreme;

        // uslov za izlazak
        taster = pollKey();
        if (taster == 27 || (getWindowProperty(ime_prozora, WND_PROP_VISIBLE) < 1)) {
            petlja_radi = false;
        }
        if (skor >= Postavke::WIN_POINTS)
        {
            if (won){
                petlja_radi = false;
            }
            won = true;
        }
        if (asteroid_na_dnu)
        {
            petlja_radi = false;
        }  

        // pozadina
        if(!won) {
            matrica_ekrana = Mat::zeros(Postavke::VISINA_EKRANA, Postavke::SIRINA_EKRANA, CV_8UC3);
        }

        // zvezde
        int radijus = 2.0;
        for (int i = 0; i < BROJ_ZVEZDA; i++) {
            // Pomeramo zvezdu nadole 
            zvezde[i].y += BRZINA_ZVEZDE_PO_S * dt;
            if (zvezde[i].y >= Postavke::VISINA_EKRANA) {
                zvezde[i].y = 0;
                zvezde[i].x = dist_sirina(engine);
            }
            if (!won) {
                circle(matrica_ekrana, Point((int)zvezde[i].x, (int)zvezde[i].y), radijus, bela, FILLED);
            }
        }
        
        // kontrole
        // to do..

        // brod
        double brod_x, brod_y; // lokalne prom
        brod_x = brod.uzmi_poziciju_x_prosledi_radi(petlja_radi, kontrole);
        brod_y = NitBroda::Y;
        for (int r = 0; r < NitBroda::B_VISINA; r++) {
            for (int k = 0; k < NitBroda::B_SIRINA; k++) {
                
                int tip_piksela = NitBroda::dizajn_broda[r][k];

                if (tip_piksela == 0) {
                    continue; 
                }

                int ekran_x = (int)brod_x + (k * NitBroda::FAKTOR_SKALIRANJA);
                int ekran_y = (int)brod_y + (r * NitBroda::FAKTOR_SKALIRANJA);

                Scalar boja;
                switch (tip_piksela) {
                    case 1: boja = svetlo_siva; break;
                    case 2: boja = bela; break;
                    case 3: boja = plava; break;
                    case 4: boja = tamno_siva; break;
                    default: continue; 
                }
                if (!won){
                    Rect veliki_piksel(ekran_x, ekran_y, NitBroda::FAKTOR_SKALIRANJA, NitBroda::FAKTOR_SKALIRANJA);
                    rectangle(matrica_ekrana, veliki_piksel, boja, FILLED);
                }
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
                    if (!won) {
                    Rect veliki_piksel(ekran_x, ekran_y, NitAsteroida::FAKTOR_SKALIRANJA, NitAsteroida::FAKTOR_SKALIRANJA);
                    rectangle(matrica_ekrana, veliki_piksel, narandzasta, FILLED);
                    }
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

                    Scalar boja;
                    
                    switch (tip_piksela) {
                        case 1: boja = tamno_braon; break;
                        case 2: boja = svetlo_braon; break;
                        case 3: boja = krem; break;
                        default: continue; 
                    }
                    if (!won) {
                        Rect veliki_piksel(ekran_x, ekran_y, NitAsteroida::FAKTOR_SKALIRANJA, NitAsteroida::FAKTOR_SKALIRANJA);
                        rectangle(matrica_ekrana, veliki_piksel, boja, FILLED);
                    }
                }
            }
        }

        //score
        std::string tekst_score = "Score: " + std::to_string(skor);
        int font = FONT_HERSHEY_SIMPLEX;
        double skala = 0.55;
        int debljina = 1;
        int baseline = 0;
        Size velicina_teksta = getTextSize(tekst_score, font, skala, debljina, &baseline);
        Point pozicija(10, velicina_teksta.height + 10);
        if (!won) {
            putText(matrica_ekrana, tekst_score, pozicija, font, skala, bela, debljina, LINE_AA);
        }
        
        imshow(ime_prozora, matrica_ekrana);
        // nakon svih iscrtavanja
        
        if (!petlja_radi) break;
        
        grafika_fps.zabeleziFrejm();
        
    }
    std::string tekst_za_ispis;
    Scalar boja;
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
    if (!tekst_za_ispis.empty())
    {
        int font = FONT_HERSHEY_COMPLEX;
        double skala = 3.0;    
        int debljina = 4;      
        int baseline = 0;      
        Size velicina_teksta = getTextSize(tekst_za_ispis, font, skala, debljina, &baseline);
        int x = (Postavke::SIRINA_EKRANA - velicina_teksta.width) / 2;
        int y = (Postavke::VISINA_EKRANA + velicina_teksta.height) / 2;
        putText(matrica_ekrana, tekst_za_ispis, Point(x, y), font, skala, boja, debljina, LINE_AA);  //glatke ivice
        imshow(ime_prozora, matrica_ekrana);
    }

    brod.stop();
    for (int i = 0; i < max_asteroida; i++) {
        asteroidi[i].stop();
    }
    meci.stop();
    delete[] zvezde;

    if (!tekst_za_ispis.empty())
    {
        while (true) 
        {
            int taster = waitKey(30); // 30 fps
            if (getWindowProperty(ime_prozora, WND_PROP_VISIBLE) < 1 || taster==27) 
            {
                break; 
            }
        }
    }
    destroyAllWindows();
}

Iscrtavanje::~Iscrtavanje() {
    delete[] asteroidi;
}