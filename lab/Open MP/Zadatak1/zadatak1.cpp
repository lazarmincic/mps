#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main ( int argc , char * argv [])
{
	int tc = strtol ( argv [1] , NULL , 10);
	double n ;
	double sum = 0;
	printf ("Number: ");
	scanf ("%lf",&n);
	double s = omp_get_wtime ();
	#pragma omp parallel for num_threads (tc) reduction (+: sum )
	for ( int i = 1; i <= ( int ) n ; i ++)
		sum += ( double ) i ;
	s = omp_get_wtime () - s ;

	///////
	double sum_tot = 0;
	double sum_l [tc]; // l - lokalne
	for (int i=0; i<tc; ++i)
		sum_l[i] = 0;
	double g = omp_get_wtime ();
	#pragma omp parallel for num_threads (tc) 
	for (int i = 1; i <= (int)n ; ++i)
		sum_l[omp_get_thread_num()] += (double) i;
	for (int i =0; i<tc; ++i)
		sum_tot += sum_l[i];
	g = omp_get_wtime () - g ;
	//////
	printf ("\n1. Sum is %lf\n",sum);
	printf ("2. Sum is %lf\n",sum_tot);
	printf ("1. Executed for %lfs\n",s);
	printf ("2. Executed for %lfs\n",g);
	return 0;
}