
#include "Iscrtavanje.hpp"
#include "Merac.hpp"
#include "NitAsteroida.hpp"
#include <random>

using namespace cv;

Iscrtavanje::Iscrtavanje(int pocetni_broj, bool hard_mode) {
    pocetni_broj_asteroida = pocetni_broj;
    broj_asteroida = pocetni_broj_asteroida;
    is_hard_mode = hard_mode;
    asteroidi = new NitAsteroida [max_asteroida];
}

void Iscrtavanje::pokreniPetljuPrikaza() {
    std::random_device rd; 
    std::default_random_engine engine(rd()); 
    std::uniform_int_distribution dist_sirina(0, Postavke::SIRINA_EKRANA-1);  // []
    std::uniform_int_distribution dist_visina(0, Postavke::VISINA_EKRANA-1);

    if (is_hard_mode) {
        max_asteroida = Postavke::LIMIT_ASTEROIDA;
    }
    else {
        max_asteroida = pocetni_broj_asteroida;
    }
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
    const Scalar BOJA_ZVEZDE = Scalar(255, 255, 255);
    struct Zvezda {
        int x;
        double y;
    };
    Zvezda* zvezde = new Zvezda[BROJ_ZVEZDA];
    for (int i = 0; i < BROJ_ZVEZDA; i++) {
        zvezde[i].x = dist_sirina(engine);
        zvezde[i].y = dist_visina(engine);
    }
    
    namedWindow(ime_prozora, WINDOW_AUTOSIZE | WINDOW_GUI_NORMAL);
    
    int skor = 0;
    bool won = false;

    bool asteroid_na_dnu = false;
    Mat matrica_ekrana;

    int meci_x [NitMetaka::MAX_METAKA], meci_y [NitMetaka::MAX_METAKA];
    for (int i =0; i<NitMetaka::MAX_METAKA; i++)
    {
        meci_y[i] = Y_POC_M;
    }

    int x_svi_a [max_asteroida];
    int y_svi_a [max_asteroida];

    KontroleIgre kontrole;
    bool petlja_radi = true;
    int taster = -1;
    bool prvi_ciklus_petlje = true;
    int broj_metaka = 0;
    
    Merac grafika_fps("GRAFIKA");
    std::chrono::time_point staro_vreme = std::chrono::high_resolution_clock::now();
    enum Smer { STOP, LEVO, DESNO };
    Smer poslednji_smer = STOP;

    while (true) {
        std::chrono::time_point trenutno_vreme = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> proteklo = trenutno_vreme - staro_vreme;
        double dt = proteklo.count(); //delta vreme 
        staro_vreme = trenutno_vreme;

        // uslov za izlazak
        taster = pollKey(); 
        std::chrono::steady_clock::time_point vreme_pritiska = std::chrono::steady_clock::now();

        if (!prvi_ciklus_petlje)
        {
            if (taster == 27 || (getWindowProperty(ime_prozora, WND_PROP_VISIBLE) < 1)){
                petlja_radi = false;
            }
        }   
        if (skor >= Postavke::WIN_POINTS)
        {
            won = true;
            petlja_radi = false;
        }
        if (asteroid_na_dnu)
        {
            petlja_radi = false;
        }

        //kontrole
        
        int taster_maskiran = taster & 0xFF;
        if (taster_maskiran == 'a' || taster_maskiran == 'A' || taster == 81 || taster == 65361) {
            kontrole.levo.aktivan = true;
            kontrole.desno.aktivan = false;
            kontrole.levo.vreme_poslednjeg_impulsa = vreme_pritiska;
        }
        if (taster_maskiran == 'd' || taster_maskiran == 'D' || taster == 83 || taster == 65363) {
            kontrole.desno.aktivan = true;
            kontrole.levo.aktivan = false;
            kontrole.desno.vreme_poslednjeg_impulsa = vreme_pritiska;
        } 
        if (taster_maskiran == 'w' || taster_maskiran == 'W' || taster == 32)
        {
            kontrole.pucaj.aktivan = true;
            kontrole.pucaj.vreme_poslednjeg_impulsa = vreme_pritiska;
        }
        
        
        const int LIMIT_AMORTIZACIJE_MS = 50;
        if (kontrole.levo.aktivan && 
        std::chrono::duration_cast<std::chrono::milliseconds>(vreme_pritiska - kontrole.levo.vreme_poslednjeg_impulsa).count()
         > LIMIT_AMORTIZACIJE_MS) {
        kontrole.levo.aktivan = false;
        }
        if (kontrole.desno.aktivan && 
        std::chrono::duration_cast<std::chrono::milliseconds>(vreme_pritiska - kontrole.desno.vreme_poslednjeg_impulsa).count()
         > LIMIT_AMORTIZACIJE_MS) {
        kontrole.desno.aktivan = false;
        }
        if (kontrole.pucaj.aktivan && 
        std::chrono::duration_cast<std::chrono::milliseconds>(vreme_pritiska - kontrole.pucaj.vreme_poslednjeg_impulsa).count()
         > LIMIT_AMORTIZACIJE_MS) {
        kontrole.pucaj.aktivan = false;
        }

        // pozadina
        matrica_ekrana = Mat::zeros(Postavke::VISINA_EKRANA, Postavke::SIRINA_EKRANA, CV_8UC3);

        // zvezde
        for (int i = 0; i < BROJ_ZVEZDA; i++) {
            // Pomeramo zvezdu nadole 
            zvezde[i].y += BRZINA_ZVEZDE_PO_S * dt;
            if (zvezde[i].y >= Postavke::VISINA_EKRANA) {
                zvezde[i].y = 0;
                zvezde[i].x = dist_sirina(engine);
            }
            circle(matrica_ekrana, Point(zvezde[i].x, static_cast<int>(zvezde[i].y)), 2, BOJA_ZVEZDE, FILLED);
        }
        
        // brod
        int brod_x, brod_y; // lokalne prom
        brod_x = brod.uzmi_poziciju_x_osvezi_kontrole(kontrole, petlja_radi);
        brod_y = NitBroda::Y;
        for (int r = 0; r < NitBroda::B_VISINA; r++) {
            for (int k = 0; k < NitBroda::B_SIRINA; k++) {
                
                int tip_piksela = NitBroda::dizajn_broda[r][k];

                if (tip_piksela == 0) {
                    continue; 
                }

                int ekran_x = brod_x + (k * NitBroda::FAKTOR_SKALIRANJA);
                int ekran_y = brod_y + (r * NitBroda::FAKTOR_SKALIRANJA);

                Scalar boja_piksela;
                switch (tip_piksela) {
                    case 1: boja_piksela = Scalar(192, 192, 192); break; // Svetlo siva
                    case 2: boja_piksela = Scalar(255, 255, 255); break; // Bela
                    case 3: boja_piksela = Scalar(255, 191, 0); break; // Plava
                    case 4: boja_piksela = Scalar(128, 128, 128); break; // tamno siva
                    default: continue;
                }
                Rect veliki_piksel(ekran_x, ekran_y, NitBroda::FAKTOR_SKALIRANJA, NitBroda::FAKTOR_SKALIRANJA);
                rectangle(matrica_ekrana, veliki_piksel, boja_piksela, FILLED);
            }
        }

        //meci
        meci.uzmiPoziciju(meci_x, meci_y, broj_metaka, skor, petlja_radi, broj_asteroida);
        for (int i = 0;i < broj_metaka; i++)
        {
            int x_i = meci_x[i];
            int y_i = meci_y[i];
            for (int r = 0; r < NitMetaka::M_VISINA; r++) {
                for (int k = 0; k < NitMetaka::M_SIRINA; k++) {
                    int ekran_x = x_i + (k * NitMetaka::FAKTOR_SKALIRANJA);
                    int ekran_y = y_i + (r * NitMetaka::FAKTOR_SKALIRANJA);
                    Scalar boja = Scalar(19, 31, 221);
                    Rect veliki_piksel(ekran_x, ekran_y, NitAsteroida::FAKTOR_SKALIRANJA, NitAsteroida::FAKTOR_SKALIRANJA);
                    rectangle(matrica_ekrana, veliki_piksel, boja, FILLED);
                }
            }
        }

        //asteroidi
        const int y_min = Postavke::VISINA_EKRANA - NitAsteroida::A_VISINA*NitAsteroida::FAKTOR_SKALIRANJA;
        pthread_mutex_lock(&tabla_crtanje.muteks);
        for (int i = 0; i < broj_asteroida; i++) {
            x_svi_a[i] = tabla_crtanje.x[i];
            y_svi_a[i] = tabla_crtanje.y[i];
        }
        tabla_crtanje.program_radi = petlja_radi;
        pthread_mutex_unlock(&tabla_crtanje.muteks);
        for (int i = 0; i < broj_asteroida; i++) {
            int ast_x, ast_y;
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
                    int ekran_x = ast_x + (k * NitAsteroida::FAKTOR_SKALIRANJA);
                    int ekran_y = ast_y + (r * NitAsteroida::FAKTOR_SKALIRANJA);
                    Scalar boja_piksela;
                    switch (tip_piksela) {
                        case 1: boja_piksela = Scalar(54, 91, 121); break;  // tamno braon
                        case 2: boja_piksela = Scalar(98, 133, 161); break; // svetlo braon
                        case 3: boja_piksela = Scalar(170, 192, 208); break;  // krem
                        default: continue;
                    }
                    Rect veliki_piksel(ekran_x, ekran_y, NitAsteroida::FAKTOR_SKALIRANJA, NitAsteroida::FAKTOR_SKALIRANJA);
                    rectangle(matrica_ekrana, veliki_piksel, boja_piksela, FILLED);
                }
            }
        }
        //score
        
        std::string tekst_score = "Score: " + std::to_string(skor);
        int font = FONT_HERSHEY_SIMPLEX;
        double skala = 0.55;
        int debljina = 1;
        Point pozicija(10, 25);
        putText(matrica_ekrana, tekst_score, pozicija, font, skala, Scalar(255, 255, 255), debljina, LINE_AA);
        
        imshow(ime_prozora, matrica_ekrana);
        // nakon svih iscrtavanja
        
    
        if (prvi_ciklus_petlje) prvi_ciklus_petlje = false;
        if (!petlja_radi) break;
        
        grafika_fps.zabeleziFrejm();
        
    }

    std::string tekst_za_ispis;
    Scalar boja;
    if (asteroid_na_dnu)
    {
        tekst_za_ispis = "Game over";
        boja = Scalar (0, 0, 255);
    }
    else if (won)
    {
        tekst_za_ispis = "You won";
        boja = Scalar (0, 255, 0);
    }
    if (!tekst_za_ispis.empty())
    {
        ispisKrajIgre(matrica_ekrana,tekst_za_ispis, boja);
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

void Iscrtavanje::ispisKrajIgre(Mat& ekran, std::string poruka, Scalar boja) {
    int font = FONT_HERSHEY_COMPLEX;
    double skala = 3.0;    
    int debljina = 4;      
    int baseline = 0;        

    Size velicina_teksta = getTextSize(poruka, font, skala, debljina, &baseline);
    int x = (Postavke::SIRINA_EKRANA - velicina_teksta.width) / 2;
    int y = (Postavke::VISINA_EKRANA + velicina_teksta.height) / 2;
    putText(ekran, poruka, Point(x, y), font, skala, boja, debljina, LINE_AA);  //glatke ivice
}
