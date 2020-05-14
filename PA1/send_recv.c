/*	Cpt S 411, Introduction to Parallel Computing
 *	School of Electrical Engineering and Computer Science
 *
 *	Example code
 *	Send receive test:
 *   	rank 1 sends to rank 0 (all other ranks sit idle)
 *   	For timing use of C gettimeofday() is recommended.
 * */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <assert.h>
#include <sys/time.h>

int main(int argc,char *argv[])
{

   int rank,p;
   struct timeval t1,t2;
   FILE *fp_rec, *fp_send;
   fp_rec = fopen("results_rec.csv", "w+");
   fp_send = fopen("results_send.csv", "w+");

   MPI_Init(&argc,&argv);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   MPI_Comm_size(MPI_COMM_WORLD,&p);

   printf("my rank=%d\n",rank);
   printf("Rank=%d: number of processes =%d\n",rank,p);

   assert(p>=2);
   int tSend, tRecv, dest, i, m;
   char *x, *y;
   double avg_send = 0, avg_rec = 0;

   for (m = 1; m < 100000000; m = m*2){
       avg_send = 0;
       avg_rec = 0;
       x = (char *) malloc(sizeof(char) * m);
       memset(x, 'a', sizeof(x));
       y = (char *) malloc(sizeof(char) * m);
       for (i = 0; i < 15; i++){
           if(rank==1) {
		dest = 0;
		gettimeofday(&t1,NULL);
		MPI_Send(x,m,MPI_CHAR,dest,0,MPI_COMM_WORLD);
		gettimeofday(&t2,NULL);
		tSend = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
		avg_send += tSend;

		printf("m=%d, Rank=%d: sent message to rank %d; Send time %d microsec, avg=%lf\n", m, rank,dest,tSend, avg_send);
            } else if (rank==0){
		MPI_Status status;
		gettimeofday(&t1,NULL);
   		MPI_Recv(y,m,MPI_CHAR,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
		gettimeofday(&t2,NULL);
		tRecv = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
		avg_rec += tRecv;

		printf("m=%d, Rank=%d: received message from rank %d; Recv time %d microsec\n", m, rank,status.MPI_SOURCE,tRecv);
            }
        }
	free(x);
	free(y);
	printf("avg_send = %lf\navg_rec = %lf\n", avg_send, avg_rec);
	avg_send /= 15.0;
	avg_rec /= 15.0;
	if (rank == 1){
	   fprintf(fp_send, "%d,%lf\n", m, avg_send);
	} else if (rank == 0){
	   fprintf(fp_rec, "%d,%lf\n", m, avg_rec);
	}
   }
   fclose(fp_rec);
   fclose(fp_send);
   MPI_Finalize();
}

