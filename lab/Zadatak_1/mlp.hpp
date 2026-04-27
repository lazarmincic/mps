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
    std::vector<std::vector<std::vector<float>>> weights;

    std::vector<std::vector<float>> activations;

    MLP(int layers, int dim);

    // Aktivacione funkcije
    float relu(float x);
    float relu_derivative(float x);

    // Softmax računa verovatnoće za ceo izlazni sloj
    std::vector<float> softmax(const std::vector<float>& z);

    // Glavna funkcija za učenje
    void train_step_parallel(const std::vector<float>& input, const std::vector<float>& label);

    void train_step_sequential (const std::vector<float>& input, const std::vector<float>& label);

    // za testiranje
    std::vector<float> forward_seq(const std::vector<float>& input);

    void inicijalizuj_tezine() ;

    unsigned int get_seed();

    void inicijalizuj_strukturu() ;

    void backward_seq(const std::vector<float>& target);

    void set_learning_rate(float lr);

    float calculate_loss(const std::vector<float>& output, const std::vector<float>& target);

    std::vector<float> forward_par(const std::vector<float>& input);
    void backward_par(const std::vector<float>& target);
};

#endif