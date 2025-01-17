#include <stdio.h>
#include <mpi.h>
#include <math.h>

// n mora biti vise od 1

// deli na tri slucaja a ne na dva kao sada

int returnSize ( char * fname )
{
  FILE * f = fopen ( fname , "r" );
  int dim = 0;
  double tmp ;
  while ( fscanf (f , "%lf" , &tmp ) != EOF )
    dim ++;
  fclose ( f );
  return dim ;
}


double * loadVec ( char * fname , int n )
{
  FILE * f = fopen ( fname , "r" );
  double * res = new double [n];
  double * it = res;
  while ( fscanf (f , "%lf" , it++) != EOF );
  fclose (f);
  return res ;
}


double * loadMat ( char * fname , int n )
{
  FILE * f = fopen ( fname , "r" );
  double * res = new double [ n*n ];
  double * it = res ;
  while ( fscanf (f , "%lf" , it++) != EOF );
  fclose ( f );
  return res ;
}


void logRes ( const char * fname , double * res , int n )
{
  FILE * f = fopen ( fname , "w" );
  for ( int i = 0; i != n ; ++ i )
    fprintf (f , "%lf " , res [i]);
  fclose ( f );
}


int main ( int argc , char * argv [])
{
  int csize ;
  int prank ;
  MPI_Init (&argc , &argv );
  MPI_Comm_size ( MPI_COMM_WORLD , &csize );
  MPI_Comm_rank ( MPI_COMM_WORLD , &prank );
  char * vfname = argv [1];
  char * mfname = argv [2];
  int dim ;
  double * mat ;
  double * vec ;
  double * tmat ;
  double * lres ;
  double * res ;
  if ( prank == 0)
    dim = returnSize ( vfname );
  MPI_Bcast (&dim , 1 , MPI_INT , 0 , MPI_COMM_WORLD );
  if ( prank == 0)
    vec = loadVec ( vfname , dim );
  else
    vec = new double [ dim ];
  MPI_Bcast ( vec , dim , MPI_DOUBLE , 0 , MPI_COMM_WORLD );
  if ( prank == 0)
    tmat = loadMat ( mfname , dim );
  int msize = dim ;
  /*
  if (prank!=csize-1)
    mat = new double [ msize ];
  else
  */
    mat = new double [ msize * 2 ];
  int sendcounts[csize];
  int displs[csize];
  
  
  if (csize<=dim)
    for (int i = 0; i<csize; ++i)  // radi samo ako je csize<=dim
    {
      displs[i] = dim*i;  
      if (i!=csize-1)
        sendcounts[i] = dim;
      else
        sendcounts[i] = dim*2;
    }
  else
    for (int i = 0; i<csize; ++i)  
    {
      if (i<dim)
      {
        displs[i] = dim*i;  
        sendcounts[i] = dim;
      }
      else
      {
        displs[i] = 0;  
        sendcounts[i] = 0;
      }
    }
  
  MPI_Scatterv ( tmat , sendcounts , displs , MPI_DOUBLE , mat , msize * 2 , MPI_DOUBLE , 0 , MPI_COMM_WORLD );
  /*     
  for (int i = 0; i<csize; ++i) //debug
    for (int j = 0; j<msize*2; ++j)
    {
      if (i == prank)
      printf("P%d : mat[%d] = %.2f\n",i , j, mat[j]);
    }
  */
  int to ;  
  if (prank!=csize-1 || csize>dim) 
  {
    to = 1;
    lres = new double [ dim ];
  }
  else 
  {
    to = 2;
    lres = new double [ 2*dim ];
  }
  
  
  for ( int i = 0; i != to ; ++ i )
  {
    double s = 0;
    for ( int j = 0; j != dim ; ++ j )
      s += mat [ i * dim + j ] * vec [ j ];
    lres [ i ] = s ;
  }
  int recvcounts[csize], displs_g[csize];
  if ( prank == 0)
  {
    res = new double [ dim ];
    if (csize<=dim)
      for (int i =0; i<csize;++i)
      {
        displs_g[i]=i;
        if (i!=csize-1)
          recvcounts[i]=1;
        else
          recvcounts[i]=2;
      }
    else
      for (int i =0; i<csize;++i)
      {
        if (i<dim)
        {
          displs_g[i]=i;
          recvcounts[i]=1;
        }
        else
        {
          displs_g[i]=0;
          recvcounts[i]=0;
        }
      }
    
  }
  MPI_Gatherv ( lres , to , MPI_DOUBLE , res , recvcounts, displs_g , MPI_DOUBLE , 0 , MPI_COMM_WORLD );
  if ( prank == 0) 
  {
    logRes ( "res.txt" , res , dim );
  }
  if ( prank == 0)
  {
    delete [] tmat ;
    delete [] res ;
  }
  delete [] vec ;
  delete [] mat ;
  delete [] lres ;
  
  MPI_Finalize ();
  return 0;
  
}
