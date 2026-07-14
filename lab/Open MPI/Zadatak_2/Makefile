zadatak2: zadatak2.cpp
	mpic++ -o zadatak2 zadatak2.cpp

# run with 4 processes
run: zadatak2
	mpiexec -n 4 ./zadatak2

# remove compiled files
clean:
	rm -f zadatak2
