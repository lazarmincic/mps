#pragma once

#include "NitBroda.hpp"
#include "NitMetaka.hpp"
#include <string>
#include <unordered_map>

class NitAsteroida;

class Iscrtavanje {
private:
    NitBroda brod;
    NitAsteroida* asteroidi;
    NitMetaka meci;
    bool is_hard_mode;
    const char* ime_prozora = "Space Defender";
    int broj_asteroida;
    int max_asteroida;
    int pocetni_broj_asteroida;
public:
    Iscrtavanje(int pocetni_broj, bool hard_mode);
    ~Iscrtavanje();
    void pokreniPetljuPrikaza();

    static constexpr double Y_POC_M = NitBroda::Y-NitMetaka::FAKTOR_SKALIRANJA*NitMetaka::M_VISINA;
    static constexpr double X_POMERAJ_M = NitBroda::FAKTOR_SKALIRANJA*(NitBroda::B_SIRINA/2-1);
};