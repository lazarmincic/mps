zadatak4: zadatak4.cpp
	mpic++ -o zadatak4 zadatak4.cpp

# run with 4 processes ...
run: zadatak4
	mpiexec -n 2 ./zadatak4 vector.txt matrix.txt

# remove compiled files
clean:
	rm -f zadatak4
	rm -f res.txt
