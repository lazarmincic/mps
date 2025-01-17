CXX = g++
CXXFLAGS = -fopenmp -Wall
TARGET = exe
SRC = zadatak2.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -f $(TARGET) sieve_static.txt sieve_dynamic.txt sieve_guided.txt sieve_auto.txt sieve_serial.txt
