#include "mlp.hpp"

// Konstruktor
MLP::MLP(int layers, int dim, int max_batch_size) {
    this->num_hidden = layers;
    this->hidden_dim = dim;
    std::random_device rd; 
    //trenutni_seed = rd();
    trenutni_seed = 42; //reproducibilnost
    inicijalizuj_strukturu(max_batch_size);
    inicijalizuj_tezine(); // Prva inicijalizacija
}

float MLP::relu(float x) { 
    return x > 0 ? x : 0; 
}

// Forward Pass 
float* MLP::forward_seq(float* input, int b_idx) {
   // Ova funkcija prima ulaznu sliku i provlaci je kroz sve slojeve do izlaza
   // Kopiramo ulaz u prvi sloj aktivacija (activations_1d[0])
   // activations_1d[0] alociran na veličinu input_dim
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
            
            // Unutrašnja petlja - ovde se dešava keš magija
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
    return izlazni_sloj; // Vraćam direktan pokazivač na memoriju unutar activations_1d
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
        // Svaki element menjam njegovim eksponentom (e^(z - max))
        data[i] = std::exp(data[i] - max_z);
        sum += data[i];
    }
    // normalizacija (deljenje svakog elementa sumom)
    for (int i = 0; i < size; ++i) {
        data[i] /= sum;
    }
}

void MLP::inicijalizuj_tezine() {
    // Koristimo sačuvani seed da bi sekvencijalni i paralelni test imali iste težine
    std::mt19937 gen(this->trenutni_seed);

    // Moramo znati dimenzije da bismo znali gde su granice u 1D nizu
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

unsigned int MLP::get_seed() {
     return trenutni_seed; 
}


void MLP::inicijalizuj_strukturu(int max_batch_size) {
    this->max_batch = max_batch_size;
    num_weights_layers = num_hidden + 1;
    num_layers = num_weights_layers + 1; // sloj aktivacija
    
    // Alociramo niz pokazivača (jedan za svaki sloj)
    weights_1d = (float**)malloc(num_weights_layers * sizeof(float*));
    activations_1d = (float**)malloc(num_layers * sizeof(float*));
    deltas_1d = (float**)malloc(num_layers * sizeof(float*));

    // layer_dims: [784, 64, 64, 10]
    layer_dims.clear();
    layer_dims.push_back(input_dim);
    for(int i = 0; i < num_hidden; ++i) layer_dims.push_back(hidden_dim);
    layer_dims.push_back(output_dim);

    // alokacija tezina
    for (int i = 0; i < num_weights_layers; ++i) {
        int rows = layer_dims[i + 1]; // broj neurona u trenutnom sloju
        int cols = layer_dims[i];     // broj ulaza iz prethodnog sloja
        
        size_t size = rows * cols * sizeof(float);
        
        // Alokacija poravnata na 64 bajta (Cache line size)
        // Vodimo računa da size bude deljiv sa 64
        size_t aligned_size = ((size + 63) / 64) * 64; // najmanji broj veći od size koji je savršeno deljiv sa 64
        
        weights_1d[i] = (float*)aligned_alloc(64, aligned_size);
        
        if (weights_1d[i] == nullptr) {
            std::cerr << "Greška pri alokaciji memorije za sloj " << i << std::endl;
            exit(1);
        }
        
        // Inicijalizacija na nule (opciono, pre pravog punjenja težina)
        std::fill(weights_1d[i], weights_1d[i] + (rows * cols), 0.0f);
    }

    // alokacija aktivacija i delti (gresaka)
    // Svaki sloj s ima layer_dims[s] neurona, puta batch_size
    for (int i = 0; i < num_layers; ++i) {
        int num_neurons = layer_dims[i];
        // Rezervišemo prostor za ceo batch odjednom
        size_t size = (size_t)num_neurons * max_batch_size * sizeof(float);
        size_t aligned_size = ((size + 63) / 64) * 64;
        activations_1d[i] = (float*)aligned_alloc(64, aligned_size);
        deltas_1d[i] = (float*)aligned_alloc(64, aligned_size);
        if (activations_1d[i] == nullptr || deltas_1d[i] == nullptr) {
            std::cerr << "Greška pri alokaciji aktivacija/delti za sloj " << i << std::endl;
            exit(1);
        }
        std::fill(activations_1d[i], activations_1d[i] + (num_neurons * max_batch_size), 0.0f);
        std::fill(deltas_1d[i], deltas_1d[i] + (num_neurons * max_batch_size), 0.0f);
    }
}

void MLP::train_step_sequential(float* input, float* target)
{
    // jedna slika napred
    // forward_seq puni activations_1d[0] iz input-a, 
    // a zatim izračunava sve do activations_1d[zadnji]
    forward_seq(input);
    // jedna slika nazad
    // backward_seq koristi te iste activations_1d i upoređuje ih sa target-om
    // da bi ažurirala weights_1d
    backward_seq(target);
}

void MLP::backward_seq(float* target) {
    // 1. Izračunaj početnu grešku (delta) za poslednji sloj
    // Poslednji sloj aktivacija je activations_1d[num_layers - 1]
    
    int out_dim = layer_dims.back();
    float* last_activations = activations_1d[num_layers - 1];
    
    //  pomoćni prostor za delte
    
    float* current_delta = deltas_1d[num_layers - 1];
    // niz gresaka
    // razlika izlaza mreze i tacnog odgovora
    // greska je direktna razlika između onoga što je mreža predvidela i onoga što je zapravo tačno
    // vrednost gradijenta

    // inicijalna greska na izlazu
    // Softmax i Cross-Entropy Loss
    for (int i = 0; i < out_dim; ++i) {
        current_delta[i] = last_activations[i] - target[i];
    }

    // 2. Glavna petlja unazad kroz slojeve težina
    for (int s = num_weights_layers - 1; s >= 0; --s) {
        int n_out = layer_dims[s + 1]; // broj neurona u trenutnom sloju (gledano unapred)
        int n_in = layer_dims[s];     // broj neurona u prethodnom sloju
        
        float* W = weights_1d[s];
        float* delta_out = deltas_1d[s + 1]; // greska koju procesiramo
        float* delta_in = deltas_1d[s];      // greska koju prenosimo nazad
        float* act_in = activations_1d[s];   // aktivacije iz sloja ispod

        // Resetujemo delta_in na nulu jer ćemo tu akumulirati grešku
        for (int j = 0; j < n_in; ++j) delta_in[j] = 0.0f;

        // Prolaz kroz neurone trenutnog sloja
        for (int i = 0; i < n_out; ++i) {
            float d_i = delta_out[i];
            
            // Unutrašnja petlja: prenos greške i ažuriranje težina
            for (int j = 0; j < n_in; ++j) {
                // Indeksiranje u 1D nizu težina: W[i * n_in + j]
                int w_idx = i * n_in + j;

                // Akumulacija greške za prethodni sloj
                delta_in[j] += d_i * W[w_idx];

                // Ažuriranje težina (Gradijentni spust)
                W[w_idx] -= learning_rate * d_i * act_in[j];
            }
        }

        // 3. Primena izvoda ReLU funkcije (osim ako nismo na ulaznom sloju)
        if (s > 0) {
            for (int k = 0; k < n_in; ++k) {
                // delta_in[k] *= (act_in[k] > 0 ? 1 : 0)
                if (act_in[k] <= 0.0f) {
                    delta_in[k] = 0.0f;
                }
            }
        }
    }
}

void MLP::set_learning_rate(float lr) {
     learning_rate = lr; 
}


float MLP::calculate_loss(const float* output, const float* target, int size) {
    float loss = 0.0f;
    for (int i = 0; i < size; ++i) {
        // Pošto je target (labela) u Fashion-MNIST obično "one-hot" kodiran 
        // (samo jedna jedinica, ostalo nule), tražimo taj jedan tačan indeks.
        if (target[i] > 0.5f) { // Ovo je nasa tacna klasa
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
    // Brisanje težina
    if (weights_1d) {
        for (int i = 0; i < num_weights_layers; ++i) {
            if (weights_1d[i]) free(weights_1d[i]);
            }
        free(weights_1d);
    }   
    // Brisanje aktivacija
    if (activations_1d) {
        for (int i = 0; i < num_layers; ++i) {
            if (activations_1d[i]) free(activations_1d[i]);
        }
        free(activations_1d);
    }
    // brisanje delti
    if (deltas_1d)
    {
        for (int i = 0; i < num_layers; ++i) {
            if (deltas_1d[i]) free(deltas_1d[i]);
        }
        free(deltas_1d);
    }
    weights_1d = nullptr;
    activations_1d = nullptr;
    deltas_1d = nullptr;
}

void MLP::train_parallel(Dataset& data, int epochs, int batch_size, Dataset& test_data) {
    // Deklarisano van paralelne regije, shared
    int epoch_correct = 0; 
    float epoch_loss = 0.0f;
    std::vector<double> vremena_epoha;
    double start_time = 0.0;
    double end_time = 0.0;
    int shared_correct_test = 0;
    int current_batch;
    float* b_images;
    float* b_targets;
    float dummy = 0.0f;
    // pravim niti jednom za ceo trening
   // #pragma omp parallel //shared(start_time, end_time, epoch_correct, epoch_loss, shared_correct_test)
    // samo pali radnike
    // Svaka nit zna svoj ID i ukupan broj niti
    for (int epoch = 0; epoch < epochs; ++epoch) {

        start_time = omp_get_wtime();
        
        // Unutrašnja petlja po batch-evima
        for (int i = 0; i < data.n_samples; i += batch_size) {
            // master nit dodeljuje posao ostalima unutar internal
            current_batch = std::min(batch_size, data.n_samples - i);
            b_images = data.images + (i * 784);
            b_targets = data.targets + (i * 10);
            forward_batch(b_images, current_batch);
            // Svaka funkcija unutra ima #pragma omp for koja koristi postojeće niti
            // Implicitna barijera je na kraju svakog #pragma omp for unutar funkcija
            backward_batch(b_targets, current_batch);
            update_weights_batch(current_batch);
        }
        end_time = omp_get_wtime();
        epoch_correct = 0;
        epoch_loss = 0.0f;
        shared_correct_test = 0;
        dummy = 0.0f; // test ne treba loss
        // evaluacija van mernog opsega
        // Svi radnici će uskočiti unutar ove funkcije čim vide 'omp for'
        evaluate_internal(data, true,epoch_correct, epoch_loss);
        evaluate_internal(test_data, false,shared_correct_test, dummy);
    
        double vreme = end_time - start_time;
        if (epoch > 0) vremena_epoha.push_back(vreme);
        // Preskačemo prvu za prosek
        float train_acc = (float)epoch_correct / data.n_samples * 100.0f;
        float train_loss = epoch_loss / data.n_samples;
        float test_acc = (float)shared_correct_test / test_data.n_samples * 100.0f;
        printf("Epoha %d | Trening: %.4f s | Train Acc: %.2f%% | Test Acc: %.2f%% | Loss: %.4f\n", 
                epoch + 1, vreme, train_acc, test_acc, train_loss);
        
    }
    
    // TEK OVDE SE NITI GASE (kraj treninga / svih epoha)
    // Kraj paralelne regije
    // Na kraju izračunamo prosek i sačuvamo u klasu da bi main mogao da ga pročita
    double suma = 0;
    for(double v : vremena_epoha) suma += v;
    this->prosecno_vreme_epohe = vremena_epoha.empty() ? 0 : suma / vremena_epoha.size();
}

void MLP::forward_batch(float* input_batch, int current_batch_size) {
    // SEKVENCIJALNO
    int in_dim = layer_dims[0];

    // Kopiranje celog batch-a u prvi sloj aktivacija
    // Svaka nit uzima svoj deo slika iz batch-a
    #pragma omp parallel for schedule(static)
    for (int b = 0; b < current_batch_size; ++b) {
        float* src = input_batch + (b * in_dim);
        float* dest = activations_1d[0] + (b * in_dim);
        for (int i = 0; i < in_dim; ++i) {
            dest[i] = src[i];
        }
    }

    // 2. Prolaz kroz slojeve težina
    for (int s = 0; s < num_weights_layers; ++s) {
        int n_out = layer_dims[s + 1];
        int n_in = layer_dims[s];
        
        float* W = weights_1d[s];
        float* in_base = activations_1d[s];
        float* out_base = activations_1d[s + 1];

        // Schedule(static) osigurava da nit radi na istim slikama kroz slojeve (Cache locality)
        #pragma omp parallel for schedule(static)
        for (int b = 0; b < current_batch_size; ++b) {
            float* curr_in = in_base + (b * n_in);
            float* curr_out = out_base + (b * n_out);

            for (int i = 0; i < n_out; ++i) {
                float sum = 0.0f;
                // Unutrašnja petlja ostaje ista - nju će kompajler sam vektorizovati (AVX2)
                for (int j = 0; j < n_in; ++j) {
                    sum += curr_in[j] * W[i * n_in + j];
                }

                // Aktivaciona funkcija
                if (s < num_weights_layers - 1) {
                    curr_out[i] = relu(sum);
                } else {
                    curr_out[i] = sum; // Logits
                }
            }
        }
        // Ovde je implicitna barijera - sve niti moraju završiti sloj 's' 
        // pre nego što bilo koja krene na sloj 's+1'
    }

    // 3. Softmax na izlazni sloj za svaku sliku u batch-u
    int out_dim = layer_dims.back();
    #pragma omp parallel for schedule(static)
    for (int b = 0; b < current_batch_size; ++b) {
        softmax(activations_1d[num_layers - 1] + (b * out_dim), out_dim);
    }
}

void MLP::backward_batch(float* target_batch, int batch_sz) {
    // SEKVENCIJALNO
    int out_dim = layer_dims.back();
    int last_layer_idx = num_layers - 1;

    // Izračunaj početnu grešku (delta) za poslednji sloj za ceo batch
    #pragma omp parallel for schedule(static)
    for (int b = 0; b < batch_sz; ++b) {
        float* out_act = activations_1d[last_layer_idx] + (b * out_dim);
        float* out_delta = deltas_1d[last_layer_idx] + (b * out_dim);
        float* t_batch = target_batch + (b * out_dim);

        for (int i = 0; i < out_dim; ++i) {
            out_delta[i] = out_act[i] - t_batch[i];
        }
    }

    // Glavna petlja unazad kroz slojeve težina
    for (int s = num_weights_layers - 1; s >= 0; --s) {
        int n_out = layer_dims[s + 1];
        int n_in = layer_dims[s];
        
        float* W = weights_1d[s];
        float* d_out_base = deltas_1d[s + 1];
        float* d_in_base = deltas_1d[s];
        float* a_in_base = activations_1d[s];

        // Paralelizacija po slikama u batch-u
        #pragma omp parallel for schedule(static)
        for (int b = 0; b < batch_sz; ++b) {
            float* curr_d_out = d_out_base + (b * n_out);
            float* curr_d_in = d_in_base + (b * n_in);
            float* curr_a_in = a_in_base + (b * n_in);

            // Resetujemo delta_in za ovu sliku (jer sakupljamo sumu iz n_out neurona)
            for (int j = 0; j < n_in; ++j) {
                curr_d_in[j] = 0.0f;
            }

            // Prenos greške unazad (propagacija unazad)
            // i je neuron trenutnog sloja, j je neuron sloja ispod
            for (int i = 0; i < n_out; ++i) {
                float d_i = curr_d_out[i];
                for (int j = 0; j < n_in; ++j) {
                    curr_d_in[j] += d_i * W[i * n_in + j];
                }
            }

            // 3. Primena izvoda ReLU funkcije (ako nismo na ulaznom sloju)
            // Ovo radimo odmah unutar 'b' petlje dok je memorija u kešu
            if (s > 0) {
                for (int j = 0; j < n_in; ++j) {
                    if (curr_a_in[j] <= 0.0f) {
                        curr_d_in[j] = 0.0f;
                    }
                }
            }
        }
        // Implicitna barijera ovde: sve niti moraju završiti proračun delti za sloj 's'
        // pre nego što pređemo na sloj 's-1'
    }
}

void MLP::update_weights_batch(int batch_sz) {
    float m_lr = learning_rate / (float)batch_sz;

    for (int s = 0; s < num_weights_layers; ++s) {
        int n_out = layer_dims[s + 1];
        int n_in = layer_dims[s];
        
        float* W = weights_1d[s];
        float* d_out_base = deltas_1d[s + 1]; 
        float* a_in_base = activations_1d[s];  

        // OPTIMIZACIJA: Menjamo redosled petlji (Loop Interchange)
        // Spoljašnja petlja po batch-u omogućava bolju lokalnost podataka
        for (int b = 0; b < batch_sz; ++b) {
            
            // Paralelizujemo po neuronima izlaznog sloja
            // Svaka nit dobija grupu neurona 'i' i ažurira njihove težine
            //#pragma omp parallel for schedule(static)
            for (int i = 0; i < n_out; ++i) {
                // d_i se učitava jednom za ceo red težina (Register Reuse)
                float d_i = d_out_base[b * n_out + i];
                
                // Ako je delta nula, preskačemo ceo red (sitna optimizacija)
                if (d_i == 0.0f) continue;

                float scaled_d_i = m_lr * d_i;
                int row_offset = i * n_in;
                int batch_offset = b * n_in;

                for (int j = 0; j < n_in; ++j) {
                    // Sekvencijalni pristup memoriji: W[row_offset + j] 
                    // i a_in_base[batch_offset + j] su jedan do drugog u RAM-u
                    W[row_offset + j] -= scaled_d_i * a_in_base[batch_offset + j];
                }
            }
        }
    }
}

void MLP::evaluate_internal(const Dataset& data, bool calculate_loss_flag, int& shared_correct, float& shared_loss) {

    int n = data.n_samples;
    int batch_sz = max_batch; 

    for (int i = 0; i < n; i += batch_sz) {
        int current_batch = std::min(batch_sz, n - i);
        // 2. Obrada rezultata unutar ovog batch-a
        #pragma omp parallel for schedule(static) reduction(+:shared_correct, shared_loss)
        for (int b = 0; b < current_batch; ++b) {
            // Svaka nit obrađuje svoje slike sekvencijalno, pišući u svoj b_idx
            forward_seq(data.images + ((i + b) * 784), b);
            float* prediction = activations_1d[num_layers - 1] + (b * 10);
            float* target = data.targets + ((i + b) * 10);

            // Argmax (Pronalaženje predviđene klase)
            int predicted_label = 0;
            float max_val = prediction[0];
            for (int j = 1; j < 10; ++j) {
                if (prediction[j] > max_val) {
                    max_val = prediction[j];
                    predicted_label = j;
                }
            }

            if (predicted_label == data.labels[i + b]) {
                shared_correct++; // Reduction brine o privatnosti
            }

            if (calculate_loss_flag) {
                shared_loss += calculate_loss(prediction, target, 10);
            }
        }
    }
}

void MLP::zero_gradients() {
    // Paralelizujemo čišćenje niza po niza
    // Koristimo memset jer je to najbrži način u C++ za postavljanje nule
    
    // 1. Čišćenje svih aktivacija (batch_size * layer_dim)
    for (int i = 0; i < num_layers; ++i) {
        int total_elements = max_batch * layer_dims[i];
        #pragma omp parallel for schedule(static)
        for (int j = 0; j < total_elements; ++j) {
            activations_1d[i][j] = 0.0f;
        }
    }

    // 2. Čišćenje svih delti (batch_size * layer_dim)
    for (int i = 0; i < num_layers; ++i) {
        int total_elements = max_batch * layer_dims[i];
        #pragma omp parallel for schedule(static)
        for (int j = 0; j < total_elements; ++j) {
            deltas_1d[i][j] = 0.0f;
        }
    }
}