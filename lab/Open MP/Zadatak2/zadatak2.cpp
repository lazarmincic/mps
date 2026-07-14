#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <string>

//eksperimentisati sa razlicitim c.s. i n

using namespace std;

void stringifier (omp_sched_t s_type, char* ret)
{
	if (s_type==omp_sched_static)
		strcpy(ret,"static");
	else if (s_type==omp_sched_dynamic)
		strcpy(ret,"dynamic");
	else if (s_type==omp_sched_guided)
		strcpy(ret,"guided");
	else if (s_type==omp_sched_auto)
		strcpy(ret,"auto");
	return;
}

double SieveOfEratosthenes_ser(int tc, int n)
{
	vector<bool> prime(n + 1, true);
	double s = omp_get_wtime ();
	for (int p = 2; p * p <= n; p++) 
	{
		if (prime[p] == true) 
		{	
				for (int i = p * p; i <= n; i += p)
				{
		      	prime[i] = false;
				}
		}
	}
	s = omp_get_wtime () - s ;
	FILE *file = fopen("sieve_serial.txt", "w");
	if (file == NULL) 
	{
   	perror("Error opening file");
   	return -1;
   }
	for (int i = 2; i <= n; i++) 
	{
		if (prime[i]) 
		{
			fprintf(file, "%d\n", i); 
		}
	}
	fclose(file);
	return s;
}

double SieveOfEratosthenes(int tc, int n, omp_sched_t s_type, int chunk)
{
	omp_set_schedule(s_type, chunk);
	vector<bool> prime(n + 1, true);
	double s = omp_get_wtime ();
	#pragma omp parallel for num_threads (tc) schedule (runtime)
	for (int p = 2; p <= (int)floor(sqrt(n)); p++) 
	{
		if (prime[p] == true) 
		{	
				for (int i = p * p; i <= n; i += p)
				{
		      	prime[i] = false;
				}
		}
	}
	s = omp_get_wtime () - s ;
	char filename [30], type_str [20]; 
	strcpy(filename,"sieve_");
	stringifier(s_type,type_str);
	strcat(filename,type_str);
	strcat(filename,".txt");
	FILE *file = fopen(filename, "w");
   if (file == NULL) 
	{
   	perror("Error opening file");
   	return -1;
   }
	for (int i = 2; i <= n; i++) 
	{
		if (prime[i]) 
		{
			fprintf(file, "%d\n", i); 
		}
	}
	fclose(file);

	return s;
}

int main ( int argc , char * argv [])
{
	omp_sched_t scheds [] = {omp_sched_static, omp_sched_dynamic, omp_sched_guided, omp_sched_auto};
	int tc = strtol ( argv [1] , NULL , 10);
	int n ,chunk;
	printf ("Number: ");
	scanf ("%d",&n);
	printf("Chunk size: ");
	scanf ("%d",&chunk);
	double s[4];
	char type_str [20];
	for (int i=0; i<4; ++i)
	{
		s[i] = SieveOfEratosthenes (tc,n,scheds[i],chunk);
		if (s[i]==-1)
		{
			printf ("Error opening file!\n");
			return 1;
		}
		//printf("debug\n");
		stringifier(scheds[i],type_str);
		printf("%s : %lfs\n",type_str,s[i]);
	}
	double g = SieveOfEratosthenes_ser(tc,n);
	if (g==-1)
	{
		printf ("Error opening file!\n");
		return 1;
	}
	printf ("serial : %lfs\n",g);
	return 0;
}