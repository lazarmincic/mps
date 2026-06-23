#include <iostream>
#include <string.h>
#include "Iscrtavanje.hpp"


int main(int argc, char** argv )
{
    if (argc != 3)
    {
        std::cout << "Nacin pokretanja: ./SpaceDefender [broj_asteroida] [standard/hard]\n";
        return 1;
    }
    int broj_asteroida = atoi(argv[1]);
    if (broj_asteroida < Postavke::MIN_ASTEROIDA)
    {
        std::cerr << "Mora biti makar "<< Postavke::MIN_ASTEROIDA << " asteroida.\n";
        return 1;
    }
    if (broj_asteroida>Postavke::LIMIT_ASTEROIDA)
    {
        broj_asteroida = Postavke::LIMIT_ASTEROIDA;
    }
    if (strcmp(argv[2],"standard") && strcmp (argv[2], "hard"))
    {
        std::cerr << "Ne postoji mod "<< argv[2] << ".\n";
        return 1;
    }
    // ------------------------
    
    bool hard_mode = (strcmp(argv[2], "hard") == 0);

    Iscrtavanje ekran(broj_asteroida, hard_mode);
    ekran.pokreniPetljuPrikaza();

    return 0;
}