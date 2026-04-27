#include <iostream>
#include "mlp.hpp"
#include "data_loader.hpp"
#include <utility> // za std::pair

// Kontroliše ceo proces — od pokretanja do ispisa rezultata

double izvrsi_trening(MLP& mreza, const Dataset& training_data, int epohe, bool parallel, const Dataset& test_data);
std::pair<float, float> evaluate(MLP& network, const Dataset& data, bool calculate_loss_flag);

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
    if (training_data.images.empty()) return 1;

    Dataset test_data = load_csv ("fashion-mnist_test.csv");
    if (test_data.images.empty()) return 1;

    // Napravi mrežu
    MLP mreza (num_hidden, hidden_dim);

    std::cout << "Koristim nasumicni seed za ovo pokretanje: " << mreza.get_seed() << std::endl;

    float faktorUcenja = 0.01f;
    
    mreza.set_learning_rate(faktorUcenja);

    std::cout << "Postavljen faktor ucenja: " << faktorUcenja << std::endl;

    int broj_epoha = 3; // menjati po potrebi
    std::cout << "Broj epoha ucenja: " << broj_epoha << std::endl;

    bool isParallel = false;
    double vreme_seq = izvrsi_trening(mreza, training_data, broj_epoha, isParallel, test_data);
    vreme_seq = vreme_seq * broj_epoha; // ukupno trajanje treninga

    // Nakon što se štoperica zaustavi, proveravamo kvalitet mreže
    // 1. Jedan prolaz kroz trening podatke (računamo i tačnost i gubitak)
    auto train_results = evaluate(mreza, training_data, true);
    float train_acc = train_results.first;
    float train_loss = train_results.second;
    // 2. Jedan prolaz kroz test podatke (računamo samo tačnost, loss nam ne treba)
    auto test_results = evaluate(mreza, test_data, false);
    float test_acc = test_results.first;

    isParallel = true;

    // oba testa kecu od istih pocetnih vrednosti
    mreza.inicijalizuj_tezine();

    double vreme_par = izvrsi_trening(mreza, training_data, broj_epoha, isParallel, test_data);

    // --- ISPIS REZULTATA ---
    std::cout << "\n===================================" << std::endl;
    std::cout << "Zavrsene sve epohe treninga sekvencijalne i paralelne mreze." << std::endl;
    std::cout << "Broj epoha: " << broj_epoha << std::endl;
    std::cout << "Vreme treninga sekvencijalne mreze: " <<  vreme_seq <<  " s"<< std::endl;
    std::cout << "Prosecan gubitak na trening skupu: " << train_loss << std::endl;
    std::cout << "Tacnost na trening skupu za sekvencijalnu mrezu: " << train_acc << "%" << std::endl; 
    std::cout << "Tacnost na test skupu za sekvencijalnu mrezu:    " << test_acc << "%" << std::endl;
    std::cout << std::endl;
    std::cout << "Vreme treniranja paralelne mreze:     " << vreme_par << " s" << std::endl;
    std::cout << "TODO" << std::endl;

    if (vreme_par > 0) {
        std::cout << "Ubrzanje:      " << vreme_seq / vreme_par << " x" << std::endl;
    }
    std::cout << "===================================" << std::endl;

    return 0;
}

double izvrsi_trening(MLP& mreza, const Dataset& training_data, int epohe, bool parallel, const Dataset& test_data) {
    std::string mod_ime = parallel ? "PARALELNI" : "SEKVENCIJALNI";
    std::cout << "Zapocinjem " << mod_ime << " trening..." << std::endl;

    std::vector<double> vremena; // za racunanje prosecnog vremena svih epoha

    for (int e = 0; e < epohe; ++e) {
        double start = omp_get_wtime();
        // pristup slici i labeli preko size_t i indeksa
        if (parallel) {
            // N / num_threads = chunk_size
            #pragma omp parallel for schedule(static) // chunk size za static, simd TODO!!!!!
            for (size_t i = 0; i < training_data.images.size(); ++i) {
                // Obrada slike i akumulacija u LOKALNE gradijente niti
                mreza.train_step_parallel(training_data.images[i], training_data.targets[i]);
            }
        } else {
            for (size_t i = 0; i < training_data.images.size(); ++i) {
                mreza.train_step_sequential(training_data.images[i], training_data.targets[i]);
            }
        }
        double end = omp_get_wtime(); 
        double vreme = end-start;
        if (e != 0)
        {
            vremena.push_back(vreme); // odbacujemo prvu epohu zbog overhead-a procesora i os-a
            // steady state merenje
        }
        // ispis
        // Statistika (loss/accuracy) van merenja
        auto train_results = evaluate(mreza, training_data, true);
        float train_acc = train_results.first;
        float train_loss = train_results.second;
        auto test_results = evaluate(mreza, test_data, false);
        float test_acc = test_results.first;

        std::cout << "Epoha " << e+1 << " zavrsena." << std::endl; // od 1
        std::cout << "Vreme treninga za epohu " << e + 1  << ": " <<  vreme <<  " s" << std::endl;
        std::cout << "Prosecan gubitak na trening skupu za epohu " << e+1 << ": " << train_loss << std::endl;
        std::cout << "Tacnost na trening skupu za epohu "<< e+1 <<": " << train_acc << "%" << std::endl; 
        std::cout << "Tacnost na test skupu za epohu "<< e+1 <<":    " << test_acc << "%" << std::endl;
        
    }

    double suma = 0;
    for(double v : vremena){
        suma += v;
    }
    double prosek = suma / vremena.size();
    std::cout << "Prosecno vreme po epohi: " << prosek << " s" << std::endl;
    return prosek;

}


std::pair<float, float> evaluate(MLP& network, const Dataset& data, bool calculate_loss_flag) {
    int correct = 0;
    // gubitak
    float total_loss = 0.0f;
    size_t n = data.images.size();

    // Paralelizujemo jedan prolaz sa dve redukcije istovremeno
    #pragma omp parallel for num_threads(omp_get_max_threads()) reduction(+:correct, total_loss) schedule(static)
    // Ovo će se izvršiti na SVIM dostupnim jezgrima sistema
    for (size_t i = 0; i < n; ++i) {
        // SAMO JEDAN forward pass - ovo je najveća ušteda!
        std::vector<float> prediction = network.forward_seq(data.images[i]);
        
        // 1. Izračunaj tačnost
        // Pronađi indeks sa najvećom verovatnoćom (ono što mreža misli)
        int predicted_label = std::distance(prediction.begin(), std::max_element(prediction.begin(), prediction.end()));
        // Uporedi sa stvarnom labelom
        if (predicted_label == data.labels[i]) {
            correct++;
        }

        // 2. Izračunaj gubitak (samo ako nam treba, npr. za trening skup)
        if (calculate_loss_flag) {
            total_loss += network.calculate_loss(prediction, data.targets[i]);
        }
    }

    float accuracy = (float)correct / n * 100.0f;
    // Izračunamo prosek tako što podelimo sa ukupnim brojem slika
    float avg_loss = calculate_loss_flag ? (total_loss / n) : 0.0f;

    return {accuracy, avg_loss};
}