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
    int* labels = nullptr;;                      // Odgovori (klase)
    float* images = nullptr;       // Slike (pikseli)
    float* targets = nullptr;      // One-hot vektori, korisno za trening
    int n_samples = 0;

    Dataset() = default;

    // Funkcija za alokaciju memorije kada saznamo broj slika
    void allocate(int n) {
        n_samples = n;
        labels = (int*)aligned_alloc(64, n * sizeof(int));
        images = (float*)aligned_alloc(64, n * 784 * sizeof(float));
        targets = (float*)aligned_alloc(64, n * 10 * sizeof(float));
        
        // Inicijalizacija targeta na nulu
        // Kreiramo target vektor od 10 nula.
        std::fill(targets, targets + (n * 10), 0.0f);
    }

    // Destruktor koji sprečava curenje memorije
    ~Dataset() {
        if (labels) free(labels);
        if (images) free(images);
        if (targets) free(targets);
    }

    // Zabrana kopiranja
    Dataset(const Dataset&) = delete;
    Dataset& operator=(const Dataset&) = delete;

    // Dozvoli premeštanje
    Dataset(Dataset&& other) noexcept : 
        labels(other.labels), images(other.images), targets(other.targets), n_samples(other.n_samples) {
        other.labels = nullptr; other.images = nullptr; other.targets = nullptr;
    }
};

// Funkcija za ucitavanje
inline Dataset load_csv(const std::string& path) {
    std::ifstream file(path);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Greska: Ne mogu otvoriti fajl " << path << std::endl;
        return Dataset();
    }

    // 1. Prebrojavanje redova (da bismo znali koliko da alociramo)
    int count = 0;
    // 1. Preskoci prvi red (zaglavlje/header gde pise "label,pixel1...")
    std::getline(file, line); // Preskoči header
    while (std::getline(file, line)) count++;
    
    Dataset data;
    data.allocate(count);
    
    // 2. Vratimo se na početak fajla za čitanje podataka
    file.clear();
    file.seekg(0);
    std::getline(file, line); // Preskoči header ponovo

    std::cout << "Ucitavam " << count << " slika iz: " << path << std::endl;

    int row = 0;
    while (std::getline(file, line) && row < count) {
        std::stringstream ss(line);
        std::string value;

        // A. Čitanje labele
        // Prvi broj u redu je labela (klasa odece)
        if (std::getline(ss, value, ',')) {
            int lbl = std::stoi(value);
            data.labels[row] = lbl;
            // One-hot encoding direktno u 1D niz
            if (lbl >= 0 && lbl < 10) {
                // Postavljamo 1.0 na poziciju labele
                data.targets[row * 10 + lbl] = 1.0f;
            }
        }
        // Ostali brojevi u redu (784 broja) su pikseli
        // B. Čitanje piksela (784 komada)
        int col = 0;
        while (std::getline(ss, value, ',') && col < 784) {
            // data.images[row * 784 + col] je 1D indeksiranje
            // NORMALIZACIJA: Delimo sa 255.0 da dobijemo opseg [0, 1]
            data.images[row * 784 + col] = std::stof(value) / 255.0f;
            col++;
        }
        row++;
    }

    file.close();
    return data; // Zahvaljujući move konstruktoru, ovo je sigurno
}

#endif