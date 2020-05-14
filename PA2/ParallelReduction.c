 /*  Cpt S 411, Introduction to Parallel Computing
 *  School of Electrical Engineering and Computer Science
 *
 *  INCLUDE NOTE: Had to use -lm when running for math.h
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>
#include <sys/time.h>

int max(int num1, int num2);

int *GenerateArray(int n, int seed){
  int *array = (int *) malloc(sizeof(int) * n);
  int i = 0;
  srand(seed);
  for (i = 0; i < (n); i++) {
       array[i] = rand() % 10;
  }
  return array;
}

int MyReduce(int argc, char *argv[], int *array, int n, int type){
   int rank,p, partner;
   int i, k, t;

   int sum, receive;

   MPI_Status status;

   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   MPI_Comm_size(MPI_COMM_WORLD,&p);

     sum = 0;

     for (i = 0; i < ((n)); i++) {
       if (type == 1) {
         sum = max(sum,array[i]);
       }
       else {
         sum += array[i];
       }
     }
     if (p > 1){
       for (k = 0, t = 1; k < log(p)/log(2); k++, t = t << 1) {
         partner = rank ^ (t);
         MPI_Sendrecv(&sum, 1, MPI_INT, partner, 123, &receive, 1,
         MPI_INT, partner, 123, MPI_COMM_WORLD, &status);
         if (type == 1) {
           sum = max(sum,receive);
         }
         else {
           sum += receive;
         }
       }
     }
     if (rank == p-1){
       printf("my_reduce: rank = %d Number of Ints: %d   Output: %d \n",rank, n, sum);
     }
  return 0;
}

int NaiveReduce(int argc, char *argv[], int *array, int n, int type){
   int rank,p, partner;
   // Number of elements
   int i, k, t;

   int sum, receive;

   MPI_Status status;
   MPI_Request request;

   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   MPI_Comm_size(MPI_COMM_WORLD,&p);

     sum = 0;

     for (i = 0; i < ((n)); i++) {
       if (type == 1) {
         sum = max(sum,array[i]);
       }
       else {
         sum += array[i];
       }
     }

     for (i = 0; i < p-1; i++){
       if (rank == i){
         MPI_Send(&sum, 1, MPI_INT, i + 1, 32, MPI_COMM_WORLD);
       }
       if (rank == i + 1){
         MPI_Recv(&receive, 1, MPI_INT, i, 32, MPI_COMM_WORLD, &status);
         if (type == 1) {
           sum = max(sum,receive);
         }
         else {
           sum += receive;
         }
       }
     }
     for (i = p-1; i >0; i--) {
       if (rank == i) {
         MPI_Send(&sum,1, MPI_INT, i - 1, 32, MPI_COMM_WORLD);
       }
       if (rank == i - 1) {
         MPI_Recv(&receive, 1, MPI_INT, i, 32, MPI_COMM_WORLD, &status);
         sum = receive;
       }
     }
     if (rank == 0){
      printf("naive_reduce; rank = %d Number of Ints: %d   Output: %d \n",rank, n, sum);
    }
  return 0;
}

int AllReduce(int argc, char *argv[], int *array, int n, int type){
   int rank,p, partner;
   int i, k, t;

   int sum, receive;

   MPI_Status status;
   MPI_Request request;

   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   MPI_Comm_size(MPI_COMM_WORLD,&p);

     sum = 0;

     for (i = 0; i < (n); i++) {
       if (type == 1) {
         sum = max(sum,array[i]);
       }
       else {
         sum += array[i];
       }
     }

     if (p > 1){
       if (type == 1) {
         MPI_Allreduce(&sum, &receive, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
         sum = receive;
       }
       else {
         MPI_Allreduce(&sum, &receive, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
         sum = receive;
       }
     }

     if (rank == p-1){
       printf("all_reduce; rank = %d Number of Ints: %d   Output: %d \n",rank, n, sum);
     }

   return 0;
}


int main(int argc, char *argv[]){
  MPI_Init(&argc,&argv);
  int *array;
  int n, tCount = 0, seed = 0, rank, p;
  int type = 0;  // Default type is  SUM
  struct timeval t1,t2;

  if (argc > 1) {
    if (strcmp(argv[1],"MAX")==0) {
      type = 1;
    }
  }

  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  MPI_Comm_size(MPI_COMM_WORLD,&p);

  FILE *my_reduce, *naive_reduce, *all_reduce;
 if (rank == 0){
  my_reduce = fopen("my_reduce.csv", "a");
  naive_reduce = fopen("naive_reduce.csv", "a");
  all_reduce = fopen("all_reduce.csv", "a");
 }

  double avg_my = 0.0, avg_all = 0.0, avg_naive = 0.0;

  for (n = 1024; n < 1000000000; n = n *2) {
   avg_my = 0.0;
   avg_naive = 0.0;
   avg_my = 0.0;
   for (seed = 0; seed < 10; seed++){
     array = GenerateArray(n/p, seed*rank);

     gettimeofday(&t1,NULL);
     MyReduce(argc, argv, array, n/p, type);
     gettimeofday(&t2,NULL);
     tCount = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
     avg_my += tCount;

     gettimeofday(&t1,NULL);
     NaiveReduce(argc, argv, array, n/p, type);
     gettimeofday(&t2,NULL);
     tCount = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
     avg_naive += tCount;


     gettimeofday(&t1,NULL);
     AllReduce(argc, argv, array, n/p, type);
     gettimeofday(&t2,NULL);
     tCount = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
     avg_all += tCount;

     free(array);
   }
   if (rank == 0){
    fprintf(my_reduce,"%lf,", avg_my/10);
    fprintf(naive_reduce, "%lf,", avg_naive/10);
    fprintf(all_reduce, "%lf,", avg_all/10);
   }
  }
 if (rank == 0){
 fprintf(my_reduce, "\n");
 fprintf(all_reduce, "\n");
 fprintf(naive_reduce, "\n");

  fclose(all_reduce);
  fclose(my_reduce);
  fclose(naive_reduce);
 }
  MPI_Finalize();
  return 0;
}


int max(int num1, int num2){
  if (num1 > num2) {
    return num1;
  }
  else {
    return num2;
  }
}
