// Otvara ogroman fashion_train.csv fajl, čita ga red po red i pretvara tekst (zarezima odvojene brojeve) u C++ vektore.

#ifndef DATA_LOADER_HPP
#define DATA_LOADER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

// Struktura koja cuva ceo dataset u memoriji

struct Dataset {
    std::vector<int> labels;                      // Odgovori (klase)
    std::vector<std::vector<float>> images;       // Slike (pikseli)
    std::vector<std::vector<float>> targets;      // One-hot vektori, korisno za trening
};

// Funkcija za ucitavanje
inline Dataset load_csv(const std::string& path) {
    Dataset data;
    std::ifstream file(path);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Greska: Ne mogu otvoriti fajl " << path << std::endl;
        return data;
    }

    std::cout << "Ucitavam dataset: " << path << " ... molim sacekajte." << std::endl;

    // 1. Preskoci prvi red (zaglavlje/header gde pise "label,pixel1...")
    if (!std::getline(file, line)) return data;

    // 2. Citaj red po red
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value;

        // Prvi broj u redu je labela (klasa odece)
        if (std::getline(ss, value, ',')) {
            int current_label = std::stoi(value);
            data.labels.push_back(current_label);
            // Kreiramo target vektor od 10 nula.
            std::vector<float> target(10, 0.0f);
            // Postavljamo 1.0 na poziciju labele
            if (current_label >= 0 && current_label < 10) {
                target[current_label] = 1.0f;
            }
            data.targets.push_back(target);
        }

        // Ostali brojevi u redu (784 broja) su pikseli
        std::vector<float> pixels;
        pixels.reserve(784); // Rezervisemo prostor radi brzine
        
        while (std::getline(ss, value, ',')) {
            // NORMALIZACIJA: Delimo sa 255.0 da dobijemo opseg [0, 1]
            pixels.push_back(std::stof(value) / 255.0f);
        }
        
        data.images.push_back(pixels);
    }

    file.close();
    std::cout << "Ucitano " << data.images.size() << " slika." << std::endl;
    
    return data;
}

#endif