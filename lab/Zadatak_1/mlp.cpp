#include "mlp.hpp"

// Konstruktor
MLP::MLP(int layers, int dim) {
    this->num_hidden = layers;
    this->hidden_dim = dim;
    std::random_device rd; 
    trenutni_seed = rd();
    inicijalizuj_strukturu();
    inicijalizuj_tezine(); // Prva inicijalizacija
}

float MLP::relu(float x) { 
    return x > 0 ? x : 0; 
}
    
float MLP::relu_derivative(float x) { 
    return x > 0 ? 1 : 0; 
}

// Inicijalizacija

// Forward Pass 
std::vector<float> MLP::forward_seq(const std::vector<float>& input) {
   // Ova funkcija prima ulaznu sliku i provlaci je kroz sve slojeve do izlaza
    activations.clear(); // brise podatke prethodnog prolaska
    activations.push_back(input); // ulaz je pocetna aktivacija

    std::vector<float> current = input; // nosi vrednosti iz sloja u sloj

    for (size_t s = 0; s < weights.size(); ++s) { // po svim matricama tezina
        std::vector<float> next(weights[s].size()); // prostor rezervisan za izlaz trenutnog sloja
        // velicine je broj neurona trenutnog sloja

        for (size_t i = 0; i < weights[s].size(); ++i) { // i je neuron sloja
            float sum = 0.0f;
            for (size_t j = 0; j < weights[s][i].size(); ++j) { // izlaz neurona i zavisi od svih prethodnih izlaza
                sum += current[j] * weights[s][i][j]; //  ulaz iz prethodnog sloja mnozi se odg. tezinom
                // 
            }
            
            // Ako je skriveni sloj, koristi se relu
            if (s < weights.size() - 1) {
                next[i] = relu(sum); 
            } else {
                next[i] = sum; // Za poslednji sloj čuvamo "sirove" vrednosti (logits)
            }
        }
        activations.push_back(next); // Pamti se aktivacija za backprop
        current = next;
    }

    // Na samom kraju primenjujemo softmax na poslednji sloj
    activations.back() = softmax(current); 
    return activations.back();
}

std::vector<float> MLP::softmax(const std::vector<float>& z) {
    std::vector<float> res(z.size());
    float sum = 0.0;
    float max_z = *std::max_element(z.begin(), z.end()); // Stabilnost
    for (float val : z) sum += std::exp(val - max_z);
    for (int i = 0; i < z.size(); ++i) res[i] = std::exp(z[i] - max_z) / sum;
    return res;
}

void MLP::inicijalizuj_tezine() {
    // Koristimo sačuvani seed da bi sekvencijalni i paralelni test imali iste težine
    std::mt19937 gen(this->trenutni_seed);

    for (size_t s = 0; s < weights.size(); ++s) {
        // n_in je broj kolona u matrici (povezuje se sa prethodnim slojem)
        // weights[s][0].size() nam daje broj ulaza za taj sloj
        int n_in = weights[s][0].size();

        // He (Kaiming) Uniform formula
        float a = std::sqrt(6.0f / (float)n_in);
        
        // Definišemo distribuciju u opsegu [-a, a]
        std::uniform_real_distribution<float> dist(-a, a);

        // Popunjavamo matricu s
        for (int i = 0; i < weights[s].size(); ++i) { // Redovi (neuroni trenutnog sloja)
            for (int j = 0; j < weights[s][i].size(); ++j) { // Kolone (veze sa prethodnim)
                weights[s][i][j] = dist(gen);
            }
        }
    }
}

unsigned int MLP::get_seed() {
     return trenutni_seed; 
}

void MLP::inicijalizuj_strukturu() 
{
    // postavlja dimenzije strukture weights

    weights.clear(); // briše prethodne 

    // red neuron u skrivenom sloju, kolona piksel ulazne slike
    weights.push_back(std::vector<std::vector<float>>(hidden_dim, std::vector<float>(input_dim)));

    // spajanja skrivenih slojeva ukoliko ih ima vise
    for (int i = 0; i < num_hidden - 1; ++i) {
        weights.push_back(std::vector<std::vector<float>>(hidden_dim, std::vector<float>(hidden_dim)));
    }

    // spaja poslednji sloj neurona sa 10 izlaznih neurona
    weights.push_back(std::vector<std::vector<float>>(output_dim, std::vector<float>(hidden_dim)));
}

void MLP::train_step_sequential(const std::vector<float>& input, const std::vector<float>& target)
{
    forward_seq(input);
    backward_seq(target);
    
}

void MLP::backward_seq(const std::vector<float>& target) {
    // inicijalna greska na izlazu
    // Softmax i Cross-Entropy Loss
    std::vector<float> delta(output_dim); // vektor greske
    for (int i = 0; i < output_dim; ++i) {
        delta[i] = activations.back()[i] - target[i]; // razlika izlaza mreze i tacnog odgovora
        // greska je direktna razlika između onoga što je mreža predvidela i onoga što je zapravo tačno
        // vrednost gradijenta
    }

    // glavna petlja unazad
    for (int s = weights.size() - 1; s >= 0; --s) {
        std::vector<float> next_delta(weights[s][0].size(), 0.0f); // next_delta čuva grešku koja će se preneti u prethodni sloj iza sebe (s-1)
        
        for (int i = 0; i < weights[s].size(); ++i) { // prolazak kroz svaku pojedinacnu vezu (tezinu) izmedju dva sloja
            float d_i = delta[i]; // // Trenutna greška neurona i
            // čita se jednom za celu j petlju
            
            for (int j = 0; j < weights[s][i].size(); ++j) {
                // Akumuliramo grešku za sloj ispod
                next_delta[j] += d_i * weights[s][i][j]; // prenosimo grešku sa trenutnog neurona i nazad na neuron j u prethodnom sloju
                
                // Ažuriramo težinu (Gradijentni spust)
                // activations[s] su zapravo ulazi koji su došli u ovaj sloj 's'
                weights[s][i][j] -= learning_rate * d_i * activations[s][j]; // gradijentni spust
            }
        }

        // Ako nismo stigli do samog ulaza (slike), primeni izvod aktivacione funkcije
        if (s > 0) { // samo ako iza nas postoji još skrivenih slojeva
            for (int k = 0; k < next_delta.size(); ++k) {
                next_delta[k] *= relu_derivative(activations[s][k]); // gasi protok greške kroz mrtve neurone
            }
            // Prenosimo obrađenu grešku u sledeću iteraciju (sloj unazad)
            delta = next_delta;
            // ono što je bila sledeća greška sada postaje trenutna greška
        }
    }
}

void MLP::set_learning_rate(float lr) {
     learning_rate = lr; 
}

void MLP::train_step_parallel(const std::vector<float>& input, const std::vector<float>& target)
{
    forward_par(input);
    backward_par(target);
}

float MLP::calculate_loss(const std::vector<float>& output, const std::vector<float>& target) {
    float loss = 0.0f;
    for (size_t i = 0; i < output.size(); ++i) {
        if (target[i] > 0.5f) { // Ovo je nasa tacna klasa
        // Dodajemo mali broj (1e-7) da izbegnemo log(0) sto bi dalo gresku
            loss = -std::log(output[i] + 1e-7f);
            break;
        }
    }
    return loss;
}

std::vector<float> MLP::forward_par(const std::vector<float>& input) {

}


void MLP::backward_par(const std::vector<float>& target) {

}