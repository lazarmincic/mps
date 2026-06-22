
#include "Iscrtavanje.hpp"
#include "Merac.hpp"
#include "NitAsteroida.hpp"
#include <random>
#include <SFML/Graphics.hpp>
#include <iostream>

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

    if (is_hard_mode && Postavke::REZIM == SAMO_BROJ) {
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
    struct Zvezda {
        double x;
        double y;
    };
    Zvezda* zvezde = new Zvezda[BROJ_ZVEZDA];
    for (int i = 0; i < BROJ_ZVEZDA; i++) {
        zvezde[i].x = dist_sirina(engine);
        zvezde[i].y = dist_visina(engine);
    }
    
    sf::RenderWindow window(sf::VideoMode(Postavke::SIRINA_EKRANA, Postavke::VISINA_EKRANA), ime_prozora);
    window.setFramerateLimit(60); /////////////////
    sf::Event event;

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

    sf::Font font;
    if (!font.loadFromFile("font.ttf")) {
        std::cerr << "font.tff datoteka nije pronađena.\n";
        return;
    }
    sf::Text tekst_objekat;
    tekst_objekat.setFont(font);
    tekst_objekat.setCharacterSize(20);
    tekst_objekat.setFillColor(sf::Color::White);
    tekst_objekat.setPosition(10.f, 10.f);

    while (true) {
        std::chrono::time_point trenutno_vreme = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> proteklo = trenutno_vreme - staro_vreme;
        double dt = proteklo.count(); //delta vreme 
        staro_vreme = trenutno_vreme;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // uslov za izlazak
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape) || !window.isOpen()){
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
        if(!won) window.clear(sf::Color::Black);

        // zvezde
        double radijus = 2.0;
        sf::CircleShape zvezda_oblik(radijus);
        zvezda_oblik.setFillColor(sf::Color::White);
        for (int i = 0; i < BROJ_ZVEZDA; i++) {
            // Pomeramo zvezdu nadole 
            zvezde[i].y += BRZINA_ZVEZDE_PO_S * dt;
            if (zvezde[i].y >= Postavke::VISINA_EKRANA) {
                zvezde[i].y = 0;
                zvezde[i].x = dist_sirina(engine);
            }
            zvezda_oblik.setPosition(zvezde[i].x - radijus, zvezde[i].y - radijus);
            if (!won) window.draw(zvezda_oblik);
        }
        
        // brod
        double brod_x, brod_y; // lokalne prom
        brod_x = brod.uzmi_poziciju_x_prosledi_radi(petlja_radi);
        brod_y = NitBroda::Y;
        for (int r = 0; r < NitBroda::B_VISINA; r++) {
            for (int k = 0; k < NitBroda::B_SIRINA; k++) {
                
                int tip_piksela = NitBroda::dizajn_broda[r][k];

                if (tip_piksela == 0) {
                    continue; 
                }

                double ekran_x = brod_x + (k * NitBroda::FAKTOR_SKALIRANJA);
                double ekran_y = brod_y + (r * NitBroda::FAKTOR_SKALIRANJA);

                sf::RectangleShape veliki_piksel(sf::Vector2f(NitBroda::FAKTOR_SKALIRANJA, NitBroda::FAKTOR_SKALIRANJA));
                sf::Color boja_piksela;
                switch (tip_piksela) {
                    case 1: boja_piksela = sf::Color(192, 192, 192); break; // Svetlo siva
                    case 2: boja_piksela = sf::Color(255, 255, 255); break; // Bela
                    case 3: boja_piksela = sf::Color(0, 191, 255); break; // Plava
                    case 4: boja_piksela = sf::Color(128, 128, 128); break; // tamno siva
                    default: continue;
                }
                veliki_piksel.setPosition(ekran_x, ekran_y);
                veliki_piksel.setFillColor(boja_piksela);
                if (!won) window.draw(veliki_piksel);
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
                    double ekran_x = x_i + (k * NitMetaka::FAKTOR_SKALIRANJA);
                    double ekran_y = y_i + (r * NitMetaka::FAKTOR_SKALIRANJA);
                    sf::Color boja_piksela = sf::Color(221, 31, 19);
                    sf::RectangleShape veliki_piksel(sf::Vector2f(NitMetaka::FAKTOR_SKALIRANJA, NitMetaka::FAKTOR_SKALIRANJA));
                    veliki_piksel.setPosition(ekran_x, ekran_y);
                    veliki_piksel.setFillColor(boja_piksela);
                    if (!won) window.draw(veliki_piksel);
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
                    double ekran_x = ast_x + (k * NitAsteroida::FAKTOR_SKALIRANJA);
                    double ekran_y = ast_y + (r * NitAsteroida::FAKTOR_SKALIRANJA);
                    sf::Color boja_piksela;
                    
                    switch (tip_piksela) {
                        case 1: boja_piksela = sf::Color(121, 91, 54); break;  // tamno braon
                        case 2: boja_piksela = sf::Color(161, 133, 98); break; // svetlo braon
                        case 3: boja_piksela = sf::Color(208, 192, 170); break;  // krem
                        default: continue;
                    }
                    sf::RectangleShape veliki_piksel(sf::Vector2f(NitAsteroida::FAKTOR_SKALIRANJA, NitAsteroida::FAKTOR_SKALIRANJA));
                    veliki_piksel.setPosition(ekran_x, ekran_y);
                    veliki_piksel.setFillColor(boja_piksela);
                    if (!won) window.draw(veliki_piksel);
                }
            }
        }

        //score
        std::string tekst_score = "Score: " + std::to_string(skor);
        tekst_objekat.setString(tekst_score);
        if (!won) window.draw(tekst_objekat);
        
        window.display();
        
        // nakon svih iscrtavanja
        
        if (!petlja_radi) break;
        
        grafika_fps.zabeleziFrejm();
        
    }

    std::string tekst_za_ispis;
    sf::Color boja;
    if (asteroid_na_dnu)
    {
        tekst_za_ispis = "Game over";
        boja = sf::Color (255, 0, 0);
    }
    else if (won)
    {
        tekst_za_ispis = "You won";
        boja = sf::Color (0, 255, 0);
    }
    if (!tekst_za_ispis.empty())
    {
        ispisKrajIgre(window,font, tekst_za_ispis, boja);
        window.display();
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
            while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape) || !window.isOpen()) 
            {
                break; 
            }
            sf::sleep(sf::milliseconds(30));
        }
    }
    
}

Iscrtavanje::~Iscrtavanje() {
    delete[] asteroidi;
}

void Iscrtavanje::ispisKrajIgre(sf::RenderWindow& window, sf::Font& font, std::string poruka, sf::Color boja) {
    sf::Text tekst(poruka, font, 75); 
    tekst.setFillColor(boja);
    sf::FloatRect granice_teksta = tekst.getLocalBounds();
    float x = (Postavke::SIRINA_EKRANA - granice_teksta.width) / 2.f - granice_teksta.left;
    float y = (Postavke::VISINA_EKRANA - granice_teksta.height) / 2.f - granice_teksta.top;
    tekst.setPosition(x, y);
    window.draw(tekst);
}
