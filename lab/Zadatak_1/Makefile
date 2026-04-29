TARGET = mlp

CXX = g++

# izbaciti -g pri merenju ono je za debug!
CXXFLAGS = -fopenmp -Wall -O3 -mavx2 -ffast-math -march=native #-fsanitize=address #-g

SRCS = main.cpp mlp.cpp

# broj niti
THREADS = 4

# broj skrivenih slojeva
NUM_HIDDEN = 2

# dimenzija svakog skrivenog sloja
HIDDEN_DIM = 128

# OpenMP podešavanja
export OMP_PROC_BIND = spread
export OMP_PLACES = cores
export OMP_WAIT_POLICY=ACTIVE

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

run: all
	./$(TARGET) $(THREADS) $(NUM_HIDDEN) $(HIDDEN_DIM)

clean:
	rm -f $(TARGET)