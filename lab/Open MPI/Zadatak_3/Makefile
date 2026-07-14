zadatak3: zadatak3.cpp
	mpic++ -o zadatak3 zadatak3.cpp

# run with 4 processes
run: zadatak3
	mpiexec -n 4 ./zadatak3

# remove compiled files
clean:
	rm -f zadatak3
	rm -f result.txt
	rm -f vector_A.txt
	rm -f vector_B.txt
