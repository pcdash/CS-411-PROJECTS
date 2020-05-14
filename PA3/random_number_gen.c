//Parallel random number series generator
//October 30th 2019
//Paul Valdez and Benjamin Hellwig

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <sys/time.h>
#include <math.h>

//Need to have a serial baseline function and a serial matrix function
int A = 0, B = 0, P = 0, seed = 0;
int **alloc_2d_contig(int rows, int cols);

int *initialize_array(int n){
  int *ret = (int *) malloc(sizeof(int) * n);
  ret[0] = seed;
  return ret;
}

int deallocate1d(int *ret){
  free(ret);
  return 0;
}

int deallocate3d(int n, int ***ret){
  int i = 0, j = 0, k = 0;
  for (i = 0; i < n; i++){
    for (j = 0; j < 2; j++){
        free(ret[i][j]);
    }
    free(ret[i]);
  }
  free(ret);
  return 0;
}

int ***initialize_matrix(int n){
  int ***ret = (int ***) malloc(sizeof(int **)*n);
  int i = 0, j = 0;
  for (i = 0; i < n; i++){
    ret[i] = (int **) malloc(sizeof(int *)*2);
    for (j=0; j < 2; j++){
      ret[i][j] = (int *) malloc(sizeof(int)*2);
    }
  }
  //Initialize M_0
  ret[0][0][0] = 1;
  ret[0][0][1] = 0;
  ret[0][1][0] = 0;
  ret[0][1][1] = 1;

  for (i = 1; i < n; i++){
    ret[i][0][0] = A;
    ret[i][0][1] = 0;
    ret[i][1][0] = B;
    ret[i][1][1] = 1;
  }
  return ret;
}

//Precondition: ret must be dynamically allocated
int serial_baseline(int n, int *ret){
  int i = 0, x_0 = seed;
  ret[0] = x_0;

  for (i = 1; i < n; i++){
    ret[i] = (A * ret[i-1] + B) % P;
  }
  return 0;
}

//Precondition: ret, x_vals, M_off  must be dynamically allocated
int serial_matrix(int n, int ***ret, int *x_vals, int **M_off){
  int i = 0, j = 0, x_0 = seed, temp_a;
  temp_a = ret[0][0][0];

  //After modifying serial_matrix function to accept Moffset
  ret[0][1][0] = (M_off[0][0] * ret[0][1][0] + M_off[1][0]) % P;
  ret[0][0][0] = (M_off[0][0] * ret[0][0][0]) % P;
//  ret[0][1][0] = (M_off[1][0] * temp_a + ret[0][1][0]) % P;
  for (i = 1; i < n; i++){
    ret[i][0][0] = (ret[i-1][0][0] * A) % P;
    ret[i][1][0] = (ret[i-1][1][0]*A + B) % P;
  }
  //Should fill in all the x values
  for (i = 0; i < n; i++){
    x_vals[i] = (seed*ret[i][0][0] + ret[i][1][0]) % P;
  }
  return 0;
}

int ***initialize_parallel_matrix(int n, int rank){
  int ***ret = initialize_matrix(n);
  if (rank != 0){
    ret[0][0][0] = A;
    ret[0][1][0] = B;
  }
  return ret;
}

int test(int n, int proc, int rank, int *x_parallel){
  int k = 0, i = 0, start, end, parallel_end, error = 0;
  if (rank == 0){
    start = 0;
    end = (n/proc) * (rank + 1) + 1;
    parallel_end = (n/proc) + 1;
  } else{
    start = n/proc * rank + 1;
    end = (n/proc) * (rank + 1) + 1;
    parallel_end = n/proc;
  }

  int *serial_base = initialize_array(n+1);
  serial_baseline(n+1, serial_base);

  for (i = start, k = 0; i < end, k < parallel_end; i++, k++){
    if (serial_base[i] != x_parallel[k]){
      printf("Something is messed up!\n");
      printf("serial=%d, parallel=%d\n", serial_base[i + 1], x_parallel[k]);
      error = 1;
    }
  }

  if (!error) {
//    printf("Serial and parallel results match!\n");
  }
  /*for (i = 0; i < parallel_end; i++){
    printf("%d ", x_parallel[i]);
  }
  printf("\n\n");
  for (i = 0; i < n+ 1; i++){
    printf("%d ", serial_base[i]);
  }*/

  deallocate1d(serial_base);
}

int calculate_M_local(int n, int **M_local, int ***serial_matrix){
  int i;
  for (i = 0; i < n; i++){
    M_local[0][0] = (M_local[0][0]*serial_matrix[i][0][0]) % P;
    M_local[1][0] = (serial_matrix[i][0][0]*M_local[1][0] + serial_matrix[i][1][0]) % P;
  }
  return 0;
}

int p_element_parallel_prefix(int rank, int proc, int **M_global, int **M_local){
  int partner, t, k;
  MPI_Status status;
  int **recptr = alloc_2d_contig(2,2);
  for (k = 0, t = 1; k < log(proc)/log(2); k++, t = t << 1) {
    partner = rank ^ (t);
    //MPI_Sendrecv(&(M_global[0][0]), 2, MPI_INT, partner, 123, &(recptr[0][0]), 2, MPI_INT, partner, 123, MPI_COMM_WORLD, &status);
    MPI_Sendrecv(&(M_global[0][0]), 2*2, MPI_INT, partner, 123, &(recptr[0][0]), 2*2, MPI_INT, partner, 123, MPI_COMM_WORLD, &status);
   // printf("Testing M_local\n");
   // printf("1: %d %d \n2: %d %d\n\n", recptr[0][0], recptr[0][1], recptr[1][0],recptr[1][1]);


    //Update M_local if rank is greater than partner
    if (partner < rank) {
      M_local[0][0] = (M_local[0][0] * recptr[0][0]) % P;
      M_local[1][0] = (M_local[1][0] * recptr[0][0] + recptr[1][0]) % P;
    }
    //Update M_global
    M_global[0][0] = (M_global[0][0] * recptr[0][0]) % P;
    M_global[1][0] = (M_global[1][0] * recptr[0][0] + recptr[1][0]) % P;
  }
//  for (t = 0; t< 2; t++){
//    free(recptr[t]);
//  }
  free(recptr[0]);
  free(recptr);
  //Need to output L now?
  return 0;
}

int **alloc_2d_contig(int rows, int cols) {
    int i;
    int *data = (int *)malloc(rows*cols*sizeof(int));
    int **array= (int **)malloc(rows*sizeof(int*));
    for (i=0; i<rows; i++){
        array[i] = &(data[cols*i]);
    }
    array[0][0] = 1;
    array[0][1] = 0;
    array[1][0] = 0;
    array[1][1] = 1;
    return array;
}

int main(int argc, char *argv[]){
  int rank, proc;
  struct timeval t1, t2;
  FILE *outfile;
  if (rank == 0){
    outfile = fopen("random.txt", "a");
  }

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &proc);

  if (argc < 5){
    printf("One of the user arguments are missing\n");
    exit(0);
  }

  //A, B, P, seed are global values
  A = atoi(argv[1]);
  B = atoi(argv[2]);
  P = atoi(argv[3]);
  seed = atoi(argv[4]);
  int ***x_local, *rand_results, **M_global, **M_local, *serial_values;
  int n, k, m = 0, tCount = 0.0;
  double avg_time = 0.0;
  for (n = 64; n < 10000000; n = n*2){
    tCount = 0;
    avg_time = 0.0;
    for (m = 0; m < 10; m++){

      if (proc == 1){
        gettimeofday(&t1,NULL);
        serial_values = initialize_array(n + 1);
        serial_baseline(n + 1, serial_values);
        gettimeofday(&t2,NULL);
        deallocate1d(serial_values);
      } else{
        M_global = alloc_2d_contig(2,2);
        M_local = alloc_2d_contig(2,2);

        gettimeofday(&t1,NULL);
        if (rank == 0){
          x_local = initialize_parallel_matrix(n/proc + 1, rank);
          rand_results = initialize_array(n/proc + 1);
          calculate_M_local(n/proc + 1, M_global, x_local);
        } else{
          x_local = initialize_parallel_matrix(n/proc, rank);
          rand_results = initialize_array(n/proc);
          calculate_M_local(n/proc, M_global, x_local);
        }

        //Start of p-element-parallel-prefix
        p_element_parallel_prefix(rank, proc, M_global, M_local);

        //Now we call serial matrix with M_local
        if (rank == 0){
          serial_matrix(n/proc + 1, x_local, rand_results, M_local);
        } else{
          serial_matrix(n/proc, x_local, rand_results, M_local);
        }
        gettimeofday(&t2,NULL);
      //Test our implementation; Only prints out if something is wrong
      //REMOVED BECAUSE WE RUN SERIAL EACH TIME TEST RUNS
//        test(n, proc, rank, rand_results);

        //Deallocate matrices
        if (rank == 0){
          deallocate3d(n/proc + 1, x_local);
        } else{
          deallocate3d(n/proc, x_local);
        }
        deallocate1d(rand_results);

        free(M_global[0]);
        free(M_local[0]);
        free(M_global);
        free(M_local);
      }
      tCount = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
      avg_time += tCount;
    }
    if (rank == 0 && outfile > 0){
      fprintf(outfile,"%lf,", avg_time/10);
    }
  }

  if (rank == 0){
    fprintf(outfile, "\n");
    fclose(outfile);
  }
  MPI_Finalize();
  return 0;
}