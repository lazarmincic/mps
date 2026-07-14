#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>

#define MAX_NUM 10
#define DIM 14

int main() 
{
  
    MPI_Init (NULL, NULL);
    int csize , prank ;
    MPI_Comm_size ( MPI_COMM_WORLD , &csize );
    MPI_Comm_rank ( MPI_COMM_WORLD , &prank );
    
    int A[DIM], B[DIM];
    if (prank==0)
    {
      srand(time(NULL));
      for (int i = 0; i < DIM; ++i) 
      {
          A[i] = rand() % MAX_NUM;  
          B[i] = rand() % MAX_NUM;  
      }
      FILE *fileA = fopen("vector_A.txt", "w");
      FILE *fileB = fopen("vector_B.txt", "w");
      if (fileA == NULL || fileB == NULL) 
      {
          printf("Error opening file.\n");
          return 1;
      }
      for (int i = 0; i < DIM; ++i) 
      {
          fprintf(fileA, "%d\n", A[i]);
          fprintf(fileB, "%d\n", B[i]);
      }
      fclose(fileA);
      fclose(fileB);
    }
    
    MPI_Bcast (A , DIM , MPI_INT , 0 , MPI_COMM_WORLD );
    MPI_Bcast (B , DIM , MPI_INT , 0 , MPI_COMM_WORLD );
    
    double s = MPI_Wtime ();
    int result = 0;
    /*
    for (int i = 0; i < DIM; i++) sequential version
    {
        result += A[i] * B[i];
    }
    */
    int i = prank;
    while ( i < DIM )
    {
      //printf ("%d: %d\n" ,prank, i );
      result += A[i] * B[i] ;
      i += csize ;
    }
    int tsum ;
    MPI_Reduce (&result , &tsum , 1 , MPI_INT , MPI_SUM , 0 , MPI_COMM_WORLD );
    double e = MPI_Wtime ();
    double d = e - s ;
    double min_d ;
    MPI_Reduce (&d , &min_d , 1 , MPI_DOUBLE , MPI_MAX , 0 , MPI_COMM_WORLD );
    if ( prank == 0)
    {
      FILE *fileResult = fopen("result.txt", "w");
      if (fileResult == NULL) 
      {
          printf("Error opening file for scalar product result!\n");
          return 1;
      }
      fprintf(fileResult, "%d\n", tsum);
      fclose(fileResult);

      printf("Results written to files. Elapsed time: %lf\n", min_d);
    }
    
    MPI_Finalize ();

    
    
    
    return 0;
}

