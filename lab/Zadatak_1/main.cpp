#include <iostream>
#include "mlp.hpp"
#include "data_loader.hpp"
#include <utility> // za std::pair
#include <thread>
#include <chrono>

// Kontroliše ceo proces — od pokretanja do ispisa rezultata

double izvrsi_trening(MLP& mreza, Dataset& training_data, int epohe, bool parallel, Dataset& test_data, const int batch_size);
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
    if (training_data.n_samples == 0) return 1;

    Dataset test_data = load_csv ("fashion-mnist_test.csv");
    if (test_data.n_samples == 0) return 1;

    int max_batch_size = 480; // batch slika u isto vreme
    // treba da bude deljiv sa brojem procesa!
    std::cout << "Paralelno pokretanje obradjuje " << max_batch_size << " slika odjednom."<< std::endl;
    // Napravi mrežu
    // Inicijalizacija na početku (rezerviše 128 mesta)
    MLP mreza (num_hidden, hidden_dim, max_batch_size);

    std::cout << "Koristim nasumicni seed za ovo pokretanje: " << mreza.get_seed() << std::endl;

    float faktorUcenja = 0.01f;
    
    mreza.set_learning_rate(faktorUcenja);

    std::cout << "Postavljen faktor ucenja: " << faktorUcenja << std::endl;

    int broj_epoha = 2; // menjati po potrebi
    std::cout << "Broj epoha ucenja: " << broj_epoha << std::endl;

    // Testiram sekvencijalno
    bool isParallel = false;
    /*
    mreza.batch_size = 1; // jedna po jedna slika
    

    double vreme_seq = izvrsi_trening(mreza, training_data, broj_epoha, isParallel, test_data, max_batch_size);
    vreme_seq = vreme_seq * broj_epoha; // ukupno trajanje treninga

    // Nakon što se štoperica zaustavi, proveravamo kvalitet mreže
    // 1. Jedan prolaz kroz trening podatke (računamo i tačnost i gubitak)
    auto train_results = evaluate(mreza, training_data, true);
    float train_acc = train_results.first;
    float train_loss = train_results.second;
    // 2. Jedan prolaz kroz test podatke (računamo samo tačnost, loss nam ne treba)
    auto test_results = evaluate(mreza, test_data, false);
    float test_acc = test_results.first;
*/
    isParallel = true;
    mreza.batch_size = max_batch_size; // vise slika istovremeno

    // oba testa kecu od istih pocetnih vrednosti
    mreza.inicijalizuj_tezine();

    // čisti aktivacije i delte 
    mreza.zero_gradients();

    // 3. Opciono: "Ohladi" procesor (malo pauziraj ili flush-uj keš)
    std::this_thread::sleep_for(std::chrono::seconds(2));
    omp_set_nested(0); // Onemogući gnežđenje ako nije potrebno
    omp_set_dynamic(0); // Reci OS-u da ne oduzima niti usred rada

    double vreme_par = izvrsi_trening(mreza, training_data, broj_epoha, isParallel, test_data, max_batch_size);
    vreme_par *= broj_epoha; // ukupno trajanje treninga
    auto train_results_p = evaluate(mreza, training_data, true);
    float train_acc_p = train_results_p.first;
    float train_loss_p = train_results_p.second;
    auto test_results_p = evaluate(mreza, test_data, false);
    float test_acc_p = test_results_p.first;
/*
    // --- ISPIS REZULTATA ---
    std::cout << "\n===================================" << std::endl;
    std::cout << "Zavrsene sve epohe treninga sekvencijalne i paralelne mreze." << std::endl;
    std::cout << "Broj epoha: " << broj_epoha << std::endl;
    std::cout << "Vreme treninga sekvencijalne mreze: " <<  vreme_seq <<  " s"<< std::endl;
    std::cout << "Prosecan gubitak na trening skupu: " << train_loss << std::endl;
    std::cout << "Tacnost na trening skupu za sekvencijalnu mrezu: " << train_acc << "%" << std::endl; 
    std::cout << "Tacnost na test skupu za sekvencijalnu mrezu:    " << test_acc << "%" << std::endl;
    std::cout << std::endl;
    */
    std::cout << "Vreme treninga paralelne mreze:     " << vreme_par << " s" << std::endl;
    std::cout << "Prosecan gubitak na trening skupu: " << train_loss_p << std::endl;
    std::cout << "Tacnost na trening skupu za paralelnu mrezu: " << train_acc_p << "%" << std::endl; 
    std::cout << "Tacnost na test skupu za paralelnu mrezu:    " << test_acc_p << "%" << std::endl;
    std::cout << std::endl;

    if (vreme_par > 0) {
        //std::cout << "Ubrzanje:      " << vreme_seq / vreme_par << " x" << std::endl;
    }
    std::cout << "===================================" << std::endl;

    return 0;
}

double izvrsi_trening(MLP& mreza, Dataset& training_data, int epohe, bool parallel, Dataset& test_data, const int batch_size) {
    std::string mod_ime = parallel ? "PARALELNI" : "SEKVENCIJALNI";
    std::cout << "Zapocinjem " << mod_ime << " trening..." << std::endl;

    std::vector<double> vremena; // za racunanje prosecnog vremena svih epoha

    for (int e = 0; e < epohe; ++e) {
        double start = omp_get_wtime();
        // pristup slici i labeli preko size_t i indeksa
        if (parallel) {
            mreza.train_parallel(training_data, epohe, batch_size, test_data);
            return mreza.prosecno_vreme_epohe;    
        } else {
            for (int i = 0; i < training_data.n_samples; ++i) {
                // Izračunaj gde počinje i-ta slika u velikom nizu
                // i * 784 (input_dim)
                float* trenutna_slika = training_data.images + (i * mreza.input_dim);
                // Izračunaj gde počinje i-ti target (one-hot)
                // i * 10 (output_dim)
                float* trenutni_target = training_data.targets + (i * mreza.output_dim);
                // Pozovi trening korak
                mreza.train_step_sequential(trenutna_slika, trenutni_target);
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
    // serijski se vrsi
    int correct = 0;
    // gubitak
    float total_loss = 0.0f;
    int n = data.n_samples;

    // Paralelizujemo jedan prolaz sa dve redukcije istovremeno
    //#pragma omp parallel for num_threads(omp_get_max_threads()) reduction(+:correct, total_loss) schedule(static)
    // Ovo će se izvršiti na SVIM dostupnim jezgrima sistema
    for (int i = 0; i < n; ++i) {
        // Dobijanje pokazivača na sliku i target
        float* trenutna_slika = data.images + (i * 784); // input_dim = 784
        float* trenutni_target = data.targets + (i * 10); // output_dim = 10
        // Forward pass (vraća float* na activations_1d[zadnji])
        // SAMO JEDAN forward pass - ovo je najveća ušteda!
        float* prediction = network.forward_seq(trenutna_slika);
        
        // Izračunaj tačnost (Argmax)
        // Pronađi indeks sa najvećom verovatnoćom (ono što mreža misli)
        int predicted_label = 0;
        float max_val = prediction[0];
        for (int j = 1; j < 10; ++j) {
            if (prediction[j] > max_val) {
                max_val = prediction[j];
                predicted_label = j;
            }
        }
        // Uporedi sa stvarnom labelom
        if (predicted_label == data.labels[i]) {
            correct++;
        }

        // Izračunaj gubitak (samo ako nam treba, npr. za trening skup)
        if (calculate_loss_flag) {
            total_loss += network.calculate_loss(prediction, trenutni_target, 10);
        }
    }

    float accuracy = (float)correct / n * 100.0f;
    // Izračunamo prosek tako što podelimo sa ukupnim brojem slika
    float avg_loss = calculate_loss_flag ? (total_loss / n) : 0.0f;

    return {accuracy, avg_loss};
}
    