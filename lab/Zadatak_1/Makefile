TARGET = mlp

CXX = g++

CXXFLAGS = -fopenmp -Wall -O3 -mavx2

SRCS = main.cpp mlp.cpp

# broj niti
THREADS = 4

# broj skrivenih slojeva
NUM_HIDDEN = 1

# dimenzija svakog skrivenog sloja
HIDDEN_DIM = 64

# OpenMP podešavanja
export OMP_PROC_BIND = spread
export OMP_PLACES = cores

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

run: all
	./$(TARGET) $(THREADS) $(NUM_HIDDEN) $(HIDDEN_DIM)

clean:
	rm -f $(TARGET)