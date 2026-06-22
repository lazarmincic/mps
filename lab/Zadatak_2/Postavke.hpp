#pragma once

enum Rezim {
    SAMO_BRZINA,
    SAMO_BROJ
};

class Postavke {
public:
    static const int SIRINA_EKRANA = 1140; // 1.9x1 aspect ratio
    static const int VISINA_EKRANA = 600;
    static const int LIMIT_ASTEROIDA = 100; 
    static const int MIN_ASTEROIDA = 2;
    static const int WIN_POINTS = 100;
    static const Rezim REZIM = SAMO_BROJ;
};