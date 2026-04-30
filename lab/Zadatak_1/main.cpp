#include <iostream>
#include "mlp.hpp"
#include "data_loader.hpp"
#include <utility> // za std::pair
#include <thread>
#include <chrono>


// Kontroliše ceo proces — od pokretanja do ispisa rezultata

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Upotreba: ./mlp <threads> <layers> <hidden_dim>" << std::endl;
        return 1;
    }

    int threads = atoi(argv[1]);
    omp_set_num_threads(threads); // Podešavanje OpenMP-a
    std::cout << "Broj niti: " << omp_get_max_threads() << std::endl;

    int num_hidden = atoi(argv[2]);
    int hidden_dim = atoi(argv[3]);
    std::cout << "Arhitektura: " << num_hidden << " sloja, " << hidden_dim << " neurona" << std::endl;

    // Učitavanje podataka
    Dataset training_data = load_csv("fashion-mnist_train.csv");
    if (training_data.n_samples == 0) return 1;

    Dataset test_data = load_csv ("fashion-mnist_test.csv");
    if (test_data.n_samples == 0) return 1;

    // Napravi mrežu
    // Inicijalizacija na početku (rezerviše 128 mesta)
    MLP mreza (num_hidden, hidden_dim, threads);

    std::cout << "Koristim nasumicni seed za ovo pokretanje: " << mreza.trenutni_seed << std::endl;
    
    float learning_rate = 0.025f; // menjaj po potrebi
    mreza.learning_rate = learning_rate;

    std::cout << "Postavljen faktor ucenja: " << learning_rate << std::endl;

    int broj_epoha = 10; // menjati po potrebi
    std::cout << "Broj epoha ucenja: " << broj_epoha << std::endl;

    omp_set_nested(0); // Onemogući gnežđenje ako nije potrebno
    omp_set_dynamic(0); // Reci OS-u da ne oduzima niti usred rada

    std::vector<double> vremena;
    for (int e = 0; e < broj_epoha; ++e) {
        vremena.push_back(mreza.train_parallel(training_data, e, test_data));
    }
    double vreme_par = std::accumulate(vremena.begin(), vremena.end(), 0.0);
    // ukupno trajanje treninga

    std::cout << std::endl;
    std::cout << "===================================" << std::endl;
    std::cout << "Ukupno vreme treninga mreze:     " << vreme_par << " s" << std::endl;
    std::cout << "===================================" << std::endl;

    return 0;
}

