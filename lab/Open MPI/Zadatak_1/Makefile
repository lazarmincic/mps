zadatak1: zadatak1.cpp
	mpic++ -o zadatak1 zadatak1.cpp

# run with 6 processes
run: zadatak1
	mpiexec -n 6 ./zadatak1

# remove compiled files
clean:
	rm -f zadatak1
