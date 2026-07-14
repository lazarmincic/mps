#include<stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>

//#define MAX_PROC_NUM 32

int main ( void ) 
{
  
  int csize ;
  int prank ;
  int package;
  MPI_Init ( NULL , NULL );
  MPI_Comm_size ( MPI_COMM_WORLD , &csize );
  MPI_Comm_rank ( MPI_COMM_WORLD , &prank );
  srand(time(NULL)*csize*1000*prank);
  for (int i=0;i<csize;i++)
    if (i!=prank)
    {
      package = prank*10 + rand() % 10;
      MPI_Send ( &package , 1 , MPI_INT , i , 0 , MPI_COMM_WORLD );
    }
  printf ( "Process %d received: " , prank );
  for (int i=0;i<csize;i++)
    if (i!=prank)
    {
      MPI_Recv ( &package , 1 , MPI_INT , i , 0 ,MPI_COMM_WORLD , MPI_STATUS_IGNORE );
      printf ( "%d " , package ); //Process 0 received: 13 27 37 44 54 
    }
  MPI_Finalize ();
  return 0;
}
