// Paul Valdez & Benjamin Hellwig
// November 13th 2019
// CPTS 411 Introduction to Parallel Computing
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <math.h>
#include <assert.h>

int main(int argc, char *argv[])
{
	long long int i, loops;

	// loop {number of iterations} [number of threads]

	if(argc<2) {
//		printf("Usage: loop {number of iterations} [number of threads]\n");
		exit(1);
	}

	loops = atoll(argv[1]);

	int p=1;
	if(argc==3) {
		p = atoi(argv[2]);
		assert(p>=1);
//		printf("Debug: number of requested threads = %d\n",p);
	}
        omp_set_dynamic(0);
	omp_set_num_threads(p);

	#pragma omp parallel
	{
		assert(p==omp_get_num_threads());
		//printf("Debug: number of threads set = %d\n",omp_get_num_threads());

		int rank = omp_get_thread_num();
//		printf("Rank=%d: my world has %d threads\n",rank,p);
	}  // end of my omp parallel region

	long long int hits = 0;
	struct drand48_data buf;
        long long int seed;
        double x, y;
	double time_s = omp_get_wtime();
	#pragma omp parallel for schedule(static) private(buf, x, y, i, seed) reduction(+:hits)	//creates N threads to run the next enclosed block
	for(i = 0; i < loops; i++)  //or line in parallel
	{
		seed = (omp_get_thread_num() + 1) * omp_get_wtime();
		seed = seed ^ i ^ getpid() ^ time(NULL);
		srand48_r(seed, &buf);
		drand48_r(&buf, &x);
		drand48_r(&buf, &y);
		if (sqrt((x - 0.5)*(x - 0.5) + (y - 0.5)*(y - 0.5)) <= 0.5){
		    hits++;
		}
	}
	// end of the second parallel region for FOR LOOP

	time_s = omp_get_wtime() - time_s;
	FILE *pi_output, *time_output;
	pi_output = fopen("pi_results.txt", "a");
	time_output = fopen("time_results.txt", "a");
	fprintf(pi_output, "%.20f,", (hits/(double) loops) * 4);
	fprintf(time_output, "%f,", time_s);
//	if (p == 8){
//		fprintf(pi_output, "\n");
//		fprintf(time_output, "\n");
//	}
	fclose(time_output);
	fclose(pi_output);
//	printf("\n %f seconds \n ", time);
//	printf("Number inside circle: %d\n", hits);
//	printf("Out pi calculation is: %.10lf\n", (hits/(double) loops) * 4);
	return 0;
}

