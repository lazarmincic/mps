#include "mlp.hpp"

// Konstruktor
MLP::MLP(int layers, int dim, int threads) {
    this->num_hidden = layers;
    this->hidden_dim = dim;
    num_threads = threads;
    trenutni_seed = 42; //reproducibilnost
    inicijalizuj_strukturu();
    inicijalizuj_tezine(); 
}

float MLP::relu(float x) { 
    return x > 0 ? x : 0; 
}

// Forward Pass 
float* MLP::forward_thread(float* input, int b_idx) {
   // prima ulaznu sliku i provlaci je kroz sve slojeve do izlaza
   // Kopiramo ulaz u prvi sloj aktivacija 
    int in_dim = layer_dims[0];
    // Koristimo b_idx da pomerimo pokazivač
    float* dest_start = activations_1d[0] + (b_idx * in_dim);
    for (int i = 0; i < in_dim; ++i) {
        dest_start[i] = input[i];
    }
    // ulaz je pocetna aktivacija

    // Prolaz kroz slojeve težina
    for (int s = 0; s < num_weights_layers; ++s) { // po svim matricama tezina
        int n_out = layer_dims[s + 1]; // Broj neurona u trenutnom sloju
        int n_in = layer_dims[s];      // Broj ulaza (neuroni prethodnog sloja)
        
        float* W = weights_1d[s];             // Težine trenutnog sloja
        float* in = activations_1d[s] + (b_idx * n_in);      // Pomeramo ulaz    // Aktivacije prethodnog sloja
        float* out = activations_1d[s + 1] + (b_idx * n_out);  //pomeramo izlaz // Gde pišemo rezultat

        for (int i = 0; i < n_out; ++i) { // i je neuron sloja
            float sum = 0.0f;
            
            // izlaz neurona i zavisi od svih prethodnih izlaza
            for (int j = 0; j < n_in; ++j) {
                // W[i * n_in + j] je težina koja povezuje ulaz j sa neuronom i
                sum += in[j] * W[i * n_in + j]; //  ulaz iz prethodnog sloja mnozi se odg. tezinom
            }

            // Aktivaciona funkcija
            // Ako je skriveni sloj, koristi se relu
            if (s < num_weights_layers - 1) {
                out[i] = relu(sum);
            } else {
                out[i] = sum; // Logits za poslednji sloj
            }
        }
    }

    // Na samom kraju primenjujemo softmax na poslednji sloj
    int broj_neurona = layer_dims.back(); // 10 za Fashion-MNIST
    float* izlazni_sloj = activations_1d[num_weights_layers] + (b_idx * broj_neurona);
    
    softmax(izlazni_sloj, broj_neurona);
    return izlazni_sloj; // Vraćamo direktan pokazivač na memoriju unutar activations_1d
}

void MLP::softmax(float* data, int size) {
    if (size <= 0) return;
    // pronalaženje maksimalne vrednosti radi numeričke stabilnosti
    float max_z = data[0];
    for (int i = 1; i < size; ++i) {
        if (data[i] > max_z) {
            max_z = data[i];
        }
    }
    // izračunavanje sume eksponenata
    float sum = 0.0f;
    for (int i = 0; i < size; ++i) {
        // Svaki element menjamo njegovim eksponentom (e^(z - max))
        data[i] = std::exp(data[i] - max_z);
        sum += data[i];
    }
    // normalizacija (deljenje svakog elementa sumom)
    for (int i = 0; i < size; ++i) {
        data[i] /= sum;
    }
}

void MLP::inicijalizuj_tezine() {
    std::mt19937 gen(this->trenutni_seed);

    // layer_dims niz: [input, hidden, ..., output]
    for (int s = 0; s < num_weights_layers; ++s) {
        int n_out = layer_dims[s + 1]; // Broj neurona (redovi)
        int n_in = layer_dims[s];     // Broj ulaza (kolone)

        // He (Kaiming) Uniform formula
        float a = std::sqrt(6.0f / (float)n_in);
        // Definišemo distribuciju u opsegu [-a, a]
        std::uniform_real_distribution<float> dist(-a, a);

        // Pokazivač na početak memorijskog bloka za sloj s
        float* layer_weights = weights_1d[s];

        // Popunjavamo 1D niz kao da je matrica [n_out x n_in]
        for (int i = 0; i < n_out; ++i) {   // Redovi (neuroni trenutnog sloja)
            for (int j = 0; j < n_in; ++j) {    // Kolone (veze sa prethodnim)
                // Linearno indeksiranje: red * broj_kolona + kolona
                layer_weights[i * n_in + j] = dist(gen);
            }
        }
    }
}

void MLP::inicijalizuj_strukturu() {
    num_weights_layers = num_hidden + 1;
    num_layers = num_weights_layers + 1; 
    
    // Standardne alokacije
    weights_1d = (float**)malloc(num_weights_layers * sizeof(float*));
    activations_1d = (float**)malloc(num_layers * sizeof(float*));
    deltas_1d = (float**)malloc(num_layers * sizeof(float*));

    // Postavljanje dimenzija slojeva
    layer_dims.clear();
    layer_dims.push_back(input_dim);
    for(int i = 0; i < num_hidden; ++i) layer_dims.push_back(hidden_dim);
    layer_dims.push_back(output_dim);

    // Alokacija gradijenata za svaku nit
    private_weight_grads = (float***)malloc(num_threads * sizeof(float**));

    for (int t = 0; t < num_threads; t++) {
        private_weight_grads[t] = (float**)malloc(num_weights_layers * sizeof(float*));
        for (int s = 0; s < num_weights_layers; s++) {
            int rows = layer_dims[s + 1];
            int cols = layer_dims[s];
            size_t size = (size_t)rows * cols * sizeof(float);
            size_t aligned_size = ((size + 63) / 64) * 64;

            // Svaka nit dobija svoj poravnat blok za gradijente
            private_weight_grads[t][s] = (float*)aligned_alloc(64, aligned_size);
            std::fill(private_weight_grads[t][s], private_weight_grads[t][s] + (rows * cols), 0.0f);
        }
    }

    // Alokacija težina 
    for (int i = 0; i < num_weights_layers; ++i) {
        int rows = layer_dims[i + 1];
        int cols = layer_dims[i];
        size_t size = (size_t)rows * cols * sizeof(float);
        size_t aligned_size = ((size + 63) / 64) * 64;
        
        weights_1d[i] = (float*)aligned_alloc(64, aligned_size);
        std::fill(weights_1d[i], weights_1d[i] + (rows * cols), 0.0f);
    }

    // alokacija aktivacija i delti za svaku nit
    // svaki sloj ima prostor za: (broj_neurona * num_threads)
    for (int i = 0; i < num_layers; ++i) {
        int num_neurons = layer_dims[i];
        
        // Rezervišemo prostor tako da svaka nit ima svoj red aktivacija
        size_t size = (size_t)num_neurons * num_threads * sizeof(float);
        size_t aligned_size = ((size + 63) / 64) * 64;
        
        activations_1d[i] = (float*)aligned_alloc(64, aligned_size);
        deltas_1d[i] = (float*)aligned_alloc(64, aligned_size);
        
        if (activations_1d[i] == nullptr || deltas_1d[i] == nullptr) {
            std::cerr << "Greška pri alokaciji aktivacija/delti" << std::endl;
            exit(1);
        }
        
        std::fill(activations_1d[i], activations_1d[i] + (num_neurons * num_threads), 0.0f);
        std::fill(deltas_1d[i], deltas_1d[i] + (num_neurons * num_threads), 0.0f);
    }
}


float MLP::calculate_loss(const float* output, const float* target, int size) {
    float loss = 0.0f;
    for (int i = 0; i < size; ++i) {
        // "one-hot" kodiran target
        if (target[i] > 0.5f) { // Ovo je naša tačna klasa
            // Cross-Entropy Loss formula: -log(predviđena_verovatnoća_tačne_klase)
            // Dodajemo 1e-7f (epsilon) da bismo izbegli log(0) koji bi srušio program (NaN)
            loss = -std::log(output[i] + 1e-7f);
            break; 
        }
    }
    return loss;
}


 // oslobađanje memorije
MLP::~MLP() {
    // Brisanje privatnih gradijenata niti 
    if (private_weight_grads) {
        for (int t = 0; t < num_threads; t++) {
            if (private_weight_grads[t]) {
                for (int s = 0; s < num_weights_layers; s++) {
                    if (private_weight_grads[t][s]) {
                        free(private_weight_grads[t][s]); // Oslobodi poravnati blok (aligned_alloc)
                    }
                }
                free(private_weight_grads[t]); // Oslobodi niz pokazivača slojeva za nit
            }
        }
        free(private_weight_grads); // Oslobodi glavni niz pokazivača niti
        private_weight_grads = nullptr;
    }

    // Brisanje težina
    if (weights_1d) {
        for (int i = 0; i < num_weights_layers; ++i) {
            if (weights_1d[i]) free(weights_1d[i]);
        }
        free(weights_1d);
        weights_1d = nullptr;
    }   

    // Brisanje aktivacija
    if (activations_1d) {
        for (int i = 0; i < num_layers; ++i) {
            if (activations_1d[i]) free(activations_1d[i]);
        }
        free(activations_1d);
        activations_1d = nullptr;
    }

    // Brisanje delti
    if (deltas_1d) {
        for (int i = 0; i < num_layers; ++i) {
            if (deltas_1d[i]) free(deltas_1d[i]);
        }
        free(deltas_1d);
        deltas_1d = nullptr;
    }
}

double MLP::train_parallel(Dataset& data, int epoch, Dataset& test_data)
{
    double start = 0;
    double end = 0;
    start = omp_get_wtime();
    #pragma omp parallel for schedule(static) 
    for (int i = 0; i < data.n_samples; i++) {
        // Forward (koristimo b_idx = tid jer svaka nit ima svoje mesto u activations_1d)
        // activations_1d i deltas_1d alocirani za num_threads
        // Računamo gde počinje slika i target za i-ti uzorak
        // Offset za slike: i * 784 (input_dim)
        // Offset za targete: i * 10 (output_dim)
        int tid = omp_get_thread_num();
        float** my_grads = private_weight_grads[tid];
        float* current_input = data.images + (i * input_dim);
        float* current_target = data.targets + (i * output_dim);
        forward_thread(current_input, tid);
        // Backward Accumulate
        backward_accumulate(current_target, tid, my_grads);
    }
    // SEKVENCIJALNI UPDATE (Samo jednom na kraju epohe)
    // Ovde primenjujemo prosek gradijenata sa learning_rate
    for (int s = 0; s < num_weights_layers; s++) {
        int layer_size = layer_dims[s] * layer_dims[s+1];
        float* W = weights_1d[s];
        float scaled_lr = learning_rate / data.n_samples;

        // petlja po težinama
        #pragma omp simd
        for (int w = 0; w < layer_size; w++) {
            float total_grad = 0.0f;
            
            // Unutrašnja petlja po nitima 
            for (int t = 0; t < num_threads; t++) {
                total_grad += private_weight_grads[t][s][w];
                private_weight_grads[t][s][w] = 0.0f; // reset bafera
            }
            
            W[w] -= scaled_lr * total_grad;
        }
    }

    end = omp_get_wtime();
    double vreme = end - start;

    // ovde evaluate
    std::pair<float, float> trening_rezultati = evaluate(data, true);
    float trening_loss = trening_rezultati.first;
    float trening_tacnost = trening_rezultati.second;

    // evaluacija na test skupu (loss nije bitan)
    std::pair<float, float> test_rezultati = evaluate(test_data, false);
    float test_tacnost = test_rezultati.second;

    // ispis rezultata
    printf("Epoha [%d]:\n", epoch + 1);
    printf("  - Trajanje treninga: %.4f\n", vreme);
    printf("  - Gubitak (Loss) na trening skupu: %.4f\n", trening_loss);
    printf("  - Tacnost na trening skupu: %.2f%%\n", trening_tacnost * 100.0f);
    printf("  - Tacnost na test skupu:    %.2f%%\n", test_tacnost * 100.0f);
    printf("-------------------------------------------\n");

    return vreme;
    
}

void MLP::backward_accumulate(float* target, int b_idx, float* my_grads[]) {
    int out_dim = layer_dims.back();
    float* last_activations = activations_1d[num_layers - 1] + (b_idx * out_dim);
    float* current_delta = deltas_1d[num_layers - 1] + (b_idx * out_dim);

    // Početna greška
    for (int i = 0; i < out_dim; ++i) {
        current_delta[i] = last_activations[i] - target[i];
    }

    for (int s = num_weights_layers - 1; s >= 0; --s) {
        int n_out = layer_dims[s + 1];
        int n_in = layer_dims[s];
        
        float* W = weights_1d[s];
        float* delta_out = deltas_1d[s + 1] + (b_idx * n_out);
        float* delta_in = deltas_1d[s] + (b_idx * n_in);
        float* act_in = activations_1d[s] + (b_idx * n_in);
        float* grad_s = my_grads[s]; // Bafer gradijenata za ovaj sloj i ovu nit

        for (int j = 0; j < n_in; ++j) delta_in[j] = 0.0f;

        for (int i = 0; i < n_out; ++i) {
            float d_i = delta_out[i];
            for (int j = 0; j < n_in; ++j) {
                int w_idx = i * n_in + j;
                // Prenos greške nazad
                delta_in[j] += d_i * W[w_idx];
                // Akumulacija gradijenta 
                grad_s[w_idx] += d_i * act_in[j];
            }
        }
        // ReLU izvod
        if (s > 0) {
            for (int k = 0; k < n_in; ++k) {
                if (act_in[k] <= 0.0f) delta_in[k] = 0.0f;
            }
        }
    }
}

std::pair<float, float> MLP::evaluate(Dataset& data, bool calculate_loss_flag) {
    int correct = 0;
    float total_loss = 0.0f;
    int n = data.n_samples;

    #pragma omp parallel for reduction(+:correct, total_loss) schedule(static)
    for (int i = 0; i < n; ++i) {
        int tid = omp_get_thread_num();
        
        float* trenutna_slika = data.images + (i * 784);
        float* trenutni_target = data.targets + (i * 10);
        
        // svaka nit ima svoj radni prostor u activations_1d
        float* prediction = forward_thread(trenutna_slika, tid);
        
        // argmax
        int predicted_label = 0;
        float max_val = prediction[0];
        for (int j = 1; j < 10; ++j) {
            if (prediction[j] > max_val) {
                max_val = prediction[j];
                predicted_label = j;
            }
        }

        if (predicted_label == data.labels[i]) {
            correct++;
        }

        if (calculate_loss_flag) {
            total_loss += calculate_loss(prediction, trenutni_target, 10);
        }
    }

    // VRATI normalizovane vrednosti (0.0 do 1.0)
    float accuracy = (float)correct / n;
    float avg_loss = calculate_loss_flag ? (total_loss / n) : 0.0f;

    // redosled mora da odgovara onome što ispis u main-u očekuje
    return {avg_loss, accuracy}; 
}
    