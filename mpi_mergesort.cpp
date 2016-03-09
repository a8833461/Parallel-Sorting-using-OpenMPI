/*
 compile: mpicxx mergeSort.cpp
 run: mpiexec -configfile mpiConfig
*/

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int numTasks, rank;

int min(int a, int b) {
  if (a <= b) return a;
  return b;
}

int max(int a, int b) {
  if (a >= b) return a;
  return b;
}


int *createNumbers(int howMany) {
  int * numbers = (int *) malloc(sizeof(int) * howMany);

  if (numbers == NULL) {
    printf("Error: malloc failed.\n");
    return NULL;
  }
  
  // note: if all processes seed with 0, then they will
  // all have the same random numbers.
  srand(rank+3);

  for(int i=0; i < howMany; i++) 
    numbers[i] = rand();

  return numbers;
}

void printNumbers(int * numbers, int howMany) {
  for(int i=0; i < howMany; i++)
    printf("%d\n", numbers[i]);
}


void merge(int *A, int iLeft, int iRight, int iEnd, int *B)
{
  int i0 = iLeft;
  int i1 = iRight;
  int j;
  
  /* While there are elements in the left or right lists */
  for (j = iLeft; j < iEnd; j++) {
    /* If left list head exists and is <= existing right list head */
    if (i0 < iRight && (i1 >= iEnd || A[i0] <= A[i1])) {
      B[j] = A[i0];
      i0 = i0 + 1;
    }
    else {
      B[j] = A[i1];
      i1 = i1 + 1;
    }
  }
}

// startWidth is how many are already sorted.  set it to 1
// by default if list is not pre-sorted at all.
void mergeSort(int *list, int n, int startWidth)
{
  int *A = list;
  int *B = (int *)malloc(sizeof(int)*n);

  if (B == NULL) {
    printf("Error: malloc failed.\n");
    return;
  }

  int width;
 
  /* Each 1-element run in A is already "sorted". */
  /* Make successively longer sorted runs of length 
     2, 4, 8, 16... until whole array is sorted. */
  for (width = startWidth; width < n; width = 2 * width) {
    int i;
    
    /* Array A is full of runs of length width. */
    for (i = 0; i < n; i = i + 2 * width)  {
      /* Merge two runs: A[i:i+width-1] and A[i+width:i+2*width-1] to B[] */          /* or copy A[i:n-1] to B[] ( if(i+width >= n) ) */
      merge(A, i, min(i+width, n), min(i+2*width, n), B);
    }
    
    /* Now work array B is full of runs of length 2*width. */
    /* Copy array B to array A for next iteration. */
    /* A more efficient implementation would swap the roles of A and B */
    int *temp = B; B = A; A = temp;
    /* Now array A is full of runs of length 2*width. */
  }
  
  // make sure results are in list.  
  if (A == list)
    free(B);
  else {
    for(int i=0; i < n; i++)
      list[i] = A[i];
    free(A);
  }
}

int isSorted(int *numbers, int howMany) {
  for(int i=1; i<howMany; i++) {
    if (numbers[i] < numbers[i-1]) return 0;
  }
  return 1;
}

// merge all the sorted lists from all the different processes.
int * mergeAll(int *list, int howMany) {
  // let's assume process 0 will do all the merging.

  // we want to gather all of the sorted stuff at process 0.
  int * A = NULL;
  int n = howMany * numTasks;

  if (rank == 0)
    A = (int *) malloc(n * sizeof(int));
  MPI_Gather(list, howMany, MPI_INT, 
	     A, howMany, MPI_INT, 
	     0, MPI_COMM_WORLD);
  
  if (rank != 0) return NULL;

  //printf("\n printing numbers...\n");
  //printNumbers(A,n);
  //printf("\n\n");

  // merge the numbers that came back.  note that howMany in 
  // a row are already sorted, so just pass that into mergeSort
  // and let it handle things.
  mergeSort(A, n, howMany);

  return A;
}
 
int main(int argc, char *argv[]) {
  double timeStart, timeEnd;
  int howMany;

  if (argc < 2) {
    printf("  Usage: ./a.out howMany.\n");
  }
  howMany = atoi(argv[1]);

  long int returnVal;
  int len;
  char hostname[MPI_MAX_PROCESSOR_NAME];

  // initialize
  returnVal = MPI_Init(&argc, &argv);
  if (returnVal != MPI_SUCCESS) {
    printf("Error starting MPI program.  Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, returnVal);
  }

  // stuff about me...
  MPI_Comm_size(MPI_COMM_WORLD, &numTasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name(hostname, &len);

  //printf ("Number of tasks= %d My rank= %d Running on %s\n", 
  //	  numTasks,rank,hostname);

  timeStart = MPI_Wtime();


  int * numbers = createNumbers(howMany);

  //printNumbers(numbers, howMany);

  //printf("merging...\n");

  mergeSort(numbers, howMany, 1);

  int *allNumbers = mergeAll(numbers, howMany);

  if (rank == 0) {
    if (isSorted(allNumbers, howMany*numTasks))
      printf("Numbers are sorted now!\n");
    else
      printf("Problem - numbers are not sorted!\n");

    //printNumbers(allNumbers, howMany*numTasks);
    free(allNumbers);

    timeEnd = MPI_Wtime();
    printf("Total # seconds elapsed is %f.\n", timeEnd-timeStart);
  }
  
  free(numbers);


  MPI_Finalize();
  return 0;
}
