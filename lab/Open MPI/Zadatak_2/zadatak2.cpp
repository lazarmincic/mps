#include <stdio.h>
#include <mpi.h>
#include <math.h>

double getInput ()
{
  double res ;
  printf ( " Number: " );
  fflush ( stdout );
  scanf ( "%lf" , &res );
  return (double)(res);
}


int main ( int argc , char * argv [])
  {
  double n ;
  int csize , prank ;
  MPI_Init (&argc , &argv );
  MPI_Comm_size ( MPI_COMM_WORLD , &csize );
  MPI_Comm_rank ( MPI_COMM_WORLD , &prank );
  if ( prank == 0)
  {
    n = getInput ();
  }
  // MPI_Barrier ( MPI_COMM_WORLD );
  MPI_Bcast (&n , 1 , MPI_DOUBLE , 0 , MPI_COMM_WORLD );
  double s = MPI_Wtime ();
  int block_size = (int)ceil((n+1)/(double)csize); 
  int i = prank*block_size;
  int sum = 0;
  if (i <= n && n < i + block_size - 1)
  {
    for (int k = i; k<=n ; k++)
    {
      sum += k;
      printf ("%d: %d\n" ,prank, k ); 
    }
  }
  else 
  {
    for (int k = i; k < i + block_size ; k++)
    {
      sum += k;
      printf ("%d: %d\n" ,prank, k ); 
    }
  }
  /*
  while ( i <= n )
  {
    sum += i ;
    i += ds ;
   // printf ("%d: %lf\n" ,prank, i ); 
  }
  */
  
  int tsum ;
  MPI_Reduce (&sum , &tsum , 1 , MPI_INT , MPI_SUM , 0 , MPI_COMM_WORLD );
  double e = MPI_Wtime ();
  double d = e - s ;
  double min_d ;
  MPI_Reduce (&d , &min_d , 1 , MPI_DOUBLE , MPI_MAX , 0 , MPI_COMM_WORLD );
  if ( prank == 0)
  {
    printf ( "Sum first %d integer is %d\n" , (int)n , tsum );
    printf ( "Elapsed time: %f\n" , min_d );
  }
  MPI_Finalize ();
  return 0;
}
