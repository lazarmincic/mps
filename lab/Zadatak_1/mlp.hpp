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
    int max_batch;
    int batch_size; // Ova promenljiva kaže funkcijama koliko mesta u memoriji da zapravo koriste

    double prosecno_vreme_epohe = 0;
    double poslednje_vreme_treninga = 0;

    MLP(int layers, int dim, int max_batch_size);

    // Za oslobađanje memorije
    ~MLP();

    // Aktivaciona funkcije
    float relu(float x);

    // Softmax računa verovatnoće za ceo izlazni sloj
    void softmax(float* data, int size);

    // Glavna funkcija za učenje

    void train_step_sequential(float* input, float* target);

    // za testiranje
    float* forward_seq(float* input, int b_idx = 0);

    void inicijalizuj_tezine() ;

    unsigned int get_seed();

    void inicijalizuj_strukturu(int max_batch_size) ;

    void backward_seq(float* target);

    void set_learning_rate(float lr);

    float calculate_loss(const float* output, const float* target, int size);

    void train_parallel(Dataset& data, int epochs, int batch_size, Dataset& test_data);

    void forward_batch(float* input_batch, int current_batch_size);

    void backward_batch(float* target_batch, int batch_sz);

    void update_weights_batch(int batch_sz);

    void evaluate_internal(const Dataset& data, bool calculate_loss_flag, int& shared_correct, float& shared_loss);

    void zero_gradients();
};

#endif