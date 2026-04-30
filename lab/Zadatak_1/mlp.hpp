#ifndef MLP_HPP
#define MLP_HPP 

#include <vector>
#include <cmath>
#include <algorithm>
#include <omp.h>
#include <cstdlib>
#include <random>
#include "data_loader.hpp"

// Definiše strukturu neuronske mreže i obavlja matematičke operacije (množenje matrica).

class MLP {
public:
    // Podrazumevane vrednosti
    int input_dim = 784;    // 28x28 piksela
    int output_dim = 10;    // 10 kategorija odeće

    int hidden_dim;         // Fiksna dimenzija
    int num_hidden;             // broj skrivenih slojeva

    unsigned int trenutni_seed;

    float learning_rate; //Kontroliše koliko je "nagli" korak popravke tezina
    // za sve tezine u mrezi
    // weights[0] spaja ulaz i prvi skriveni sloj
    // weights[1] spaja prvi i drugi skriveni sloj...
    // weights.back() spaja poslednji skriveni i izlazni sloj
    float** weights_1d;      // Niz pokazivača na blokove težina po slojevima
    int num_weights_layers;  // Broj slojeva težina (num_hidden + 1)
    std::vector<int> layer_dims;
    // Pomoćni niz dimenzija da lakše prolazimo kroz petlju
    // Primer: [784, 64, 64, 10]
    float** activations_1d;
    float ** deltas_1d;
    int num_layers; // broj slojeva aktivacija
    // Ukupan broj slojeva (npr. input + hidden + output)
    float*** private_weight_grads;
    int num_threads = 1;
    //static const int max_threads = 6; // za evaluate

    MLP(int layers, int dim, int threads);

    // Za oslobađanje memorije
    ~MLP();

    // Aktivaciona funkcije
    float relu(float x);
    // Softmax računa verovatnoće za ceo izlazni sloj
    void softmax(float* data, int size);
    // za testiranje
    float* forward_seq(float* input, int b_idx = 0);

    void inicijalizuj_tezine() ;

    void inicijalizuj_strukturu() ;

    void backward_accumulate(float* target, int b_idx, float* my_grads[]);

    float calculate_loss(const float* output, const float* target, int size);

    double train_parallel(Dataset& data, int epoch, Dataset& test_data);

    std::pair<float, float> evaluate( Dataset& data, bool calculate_loss_flag);

    
};

#endif