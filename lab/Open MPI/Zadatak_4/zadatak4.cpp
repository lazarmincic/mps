#include <stdio.h>
#include <mpi.h>
#include <math.h>

// dimenzije matrice moraju biti vece od broja procesa (ZA SADA)

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
  bool nesim = true;
  if (dim * dim % csize == 0) // deljivo
    nesim = false;
  int msize;
  int faktor = int (round(double(dim)/double(csize))); // zaokr na prvu najvecu
  if (dim<csize)
    faktor = 1;
  msize =  faktor * dim; 
  int visak_redova = 0;
  visak_redova = (dim-faktor*(csize-1));
  if (visak_redova<0) 
    visak_redova = 0;
  int recvcount = 0;
  if (nesim)
  {
    if (prank==csize-1)
    {
      recvcount = visak_redova*dim;
      if (dim % faktor == 0 && dim / faktor<csize)
      recvcount = 0;
    }
    else 
      recvcount = faktor * dim;
    if (dim<csize)
    {
      if (prank<dim)
        recvcount = dim; //faktor je 1
      else 
        recvcount = 0; 
    }
  }
  else
  {
    recvcount = faktor * dim;
  }
  mat = new double [ recvcount ];
  int sendcounts[csize];
  int displs[csize];
  if (prank==0)
  {
    for (int i = 0; i<csize; ++i)  // radi samo ako je csize<=dim dim%csize!=0
    {
       displs[i] = msize*i; 
       if (msize*i>=dim*dim)
         displs[i] = 0;
       if (!nesim || i!=csize-1)
         sendcounts[i] = msize;
    }
    if (nesim)
    {
      sendcounts[csize-1] = visak_redova*dim;
    }
    if (dim<csize)
    {
      for (int i = 0; i<csize;++i)
      {
        if (i<dim)
        {
          sendcounts[i]=dim; //1
          displs[i] = i*dim;
        }
        else
        {
          sendcounts[i]=0;
          displs[i] = 0;
        }
      }
    }
  }
   /*
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
    } */
  
  MPI_Scatterv ( tmat , sendcounts , displs , MPI_DOUBLE , mat , recvcount , MPI_DOUBLE , 0 , MPI_COMM_WORLD );

  
  /*     
  for (int i = 0; i<csize; ++i) //debug
    for (int j = 0; j<msize*2; ++j)
    {
      if (i == prank)
      printf("P%d : mat[%d] = %.2f\n",i , j, mat[j]);
    }
  */
  int to ;  
  if (nesim)
  {
    if (prank==csize-1 ) 
    {
      to = visak_redova;
    }
    else
    {
      to = faktor;
    }
    if (dim<csize)
    {
      if (prank<dim)
        to = 1;
      else 
        to = 0;
    }
  }
  else
  {
    to = dim / csize ;
  }
  lres = new double [ to ];
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
    for (int i =0; i<csize;++i)
    {
      displs_g[i]=i*faktor;
      if (i*faktor>=dim)
        displs_g[i]=0;
      if (!nesim || i!=csize-1)
      recvcounts[i]=faktor;
    }
    if (nesim)
      recvcounts[csize-1]=visak_redova;
    if (dim<csize)
    {
      for (int i =0; i<csize;++i)
      {
        if (i<dim)
        {
          recvcounts[i]=1;
          displs_g[i]=i;
        }
        else
        {
          recvcounts[i]=0;
          displs_g[i]=0;
        }
      }
    }
  }
    
      /*
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
    */
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
