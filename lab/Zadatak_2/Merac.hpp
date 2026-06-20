#pragma once

#include <string>
#include <chrono>

class Merac {
private:
    int brojac_frejmova;
    std::chrono::high_resolution_clock::time_point poslednji_ispis;
    std::string ime_niti;
public:
    Merac(std::string ime);
    void zabeleziFrejm();
private:
    void bezbedanIspis(const std::string&);
    std::chrono::high_resolution_clock::time_point snimiTrenutnoVreme ();
};