#include "Board.hpp"

Board::Board() : cpu(&cacheController) { }

// Funkcija koja pokreće jedan korak simulacije
void Board::step (){

}


int main () {
    // ovde se simulira
    Asembler::parseAssembler("input.txt");
}


