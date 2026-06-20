#pragma once

#include "NitBroda.hpp"
#include "NitMetaka.hpp"
#include <opencv2/opencv.hpp>


class NitAsteroida;

class Iscrtavanje {
private:
    NitBroda brod;
    NitAsteroida* asteroidi;
    NitMetaka meci;
    bool is_hard_mode;
    const std::string ime_prozora = "Space Defender";
    int broj_asteroida;
    int max_asteroida;
    int pocetni_broj_asteroida;

public:
    Iscrtavanje(int pocetni_broj, bool hard_mode);
    ~Iscrtavanje();
    void pokreniPetljuPrikaza();

    static const int Y_POC_M = NitBroda::Y-NitMetaka::FAKTOR_SKALIRANJA*NitMetaka::M_VISINA;
    static const int X_POMERAJ_M = NitBroda::FAKTOR_SKALIRANJA*(NitBroda::B_SIRINA/2-1);
private:
    void ispisKrajIgre(cv::Mat& ekran, std::string poruka, cv::Scalar boja);
};