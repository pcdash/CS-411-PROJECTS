//
//  main.cpp
//  graph_representation
//
//  Created by Paul Valdez on 12/3/19.
//  Copyright © 2019 Paul Valdez. All rights reserved.
//

extern "C"{
    #include <stdio.h>
    #include <stdlib.h>
    #include <time.h>
    #include <string.h>
    #include <math.h>
    #include <assert.h>

    //This was for use while running on a computer without omp
    #if defined(_OPENMP)
        #include <omp.h>
    #else
	//I didnt include all the functions in here but it doesnt need to be used anyways
        typedef int omp_int_t;
        inline omp_int_t omp_get_thread_num() { return 0;}
        inline omp_int_t omp_get_max_threads() { return 1;}
        inline omp_int_t omp_get_wtime() {return 1;}
    #endif
}

#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <set>


using namespace std;

struct classcomp {
  bool operator() (const pair<int, int>& lhs, const pair<int, int>& rhs) const
    {
        if (lhs.second != rhs.second){
            return lhs.second > rhs.second;
        }
        return lhs.first < rhs.first;
    }
};

//Variables
int p = 1;
char file_name[256] = "web-BerkStan_Sorted.txt";

//Functions
int initialize_array(int *arr, int size);
int get_data(fstream &input, vector<int> *edges, set<int> &nodes, int &max_node);
int rank_pages(vector<int> *edges, vector<int> nodes, int *visits_per_node, int max_node, int K, double D, int N);

int main(int argc, const char * argv[]) {
    srand(time(nullptr));
    vector<int> * edges;
    set<int> node_set;
    vector<int> nodes;

    int *visits_per_node;
    int max_node = 0;
    int K = 0;
    int N = -1;
    double D = 0.0;
    fstream input;

    //Make sure variables D and K were entered
    if (argc < 4){
         printf("Usage: [K pathsize] [D damping ratio] [Name of File] [P number of threads]\n");
//         printf("Optional: [N number of iterations]\n");
         exit(1);
    }
    K = atoll(argv[1]);
    D = atof(argv[2]);
    strcpy(file_name, argv[3]);

    // Get number of threads
    if (argc >= 5){
        p = atoi(argv[4]);
        assert(p>=1);
        printf("Debug: number of requested threads = %d\n",p);
    }
    // Set N if given
    if (argc == 6){
        N = atoll(argv[5]);
    }

    //Set number of threads
    omp_set_dynamic(0);
    omp_set_num_threads(p);

    #pragma omp parallel
    {
        assert(p==omp_get_num_threads());
        //printf("Debug: number of threads set = %d\n",omp_get_num_threads());
        int rank = omp_get_thread_num();
        // printf("Rank=%d: my world has %d threads\n",rank,p);
    }  // end of my omp parallel region

    //Start the timer for total time
    double time_tot = omp_get_wtime();

    //Open the file
    input.open(file_name);
    if (input.is_open()){
        cout << "file is open" << endl;
    } else{
        cout << "file is not open" << endl;
    }

    int num_node = -1, num_edges = -1;
    char temp_str[256];
    char temp_c;
    string str;
    //Clear the comments if there are any, from the top of the text file
    while (!input.eof() && getline(input, str)){
        //Get number of visits_per_node and edges
        sscanf(str.c_str(), "%c%s%d%s%d", &temp_c, temp_str, &num_node, temp_str, &num_edges);
        if (strcmp(temp_str, "Edges:") == 0){
            break;
        }
    }
    //Only set N if it is unitialized
    if (N == -1){
        N = num_node;
    }
    //Make sure num visits_per_node account for 0th node
    cout << "Nodes: " << num_node  << " Edges: " << num_edges << endl;

    //Initalize edge arrays
    edges = new vector<int> [num_edges];

    //Initalize number of edges for each node
    for (int i = 0; i < num_edges; i++){
        edges[i].clear();
    }
    get_data(input, edges, node_set, max_node);

    //Initialize visits per nodes array, with 1 more than the max node
    visits_per_node = new int [max_node + 2];
    initialize_array(visits_per_node, max_node + 2);

    //Put nodes from set into vector
    set<int>::iterator it;
    for (it = node_set.begin(); it != node_set.end(); it++){
        nodes.push_back(*it);
    }

    //Start the timer
    double time_p = omp_get_wtime();

    //Now to count the number of paths
    rank_pages(edges, nodes, visits_per_node, max_node, K, D, N);

    //Get the end time and output
    time_p = omp_get_wtime() - time_p;
    cout << "PARALLEL TIME: " << time_p << endl;

    //Sort into table and get max 5
    set<pair<int, int>, classcomp> table;
    for (it = node_set.begin(); it != node_set.end(); it++){
        table.insert(make_pair(*it, visits_per_node[*it]));
    }

    int i = 1;
    cout << "Size of table: " << table.size() << endl;
    for (set<pair<int, int>, classcomp>::iterator itr = table.begin(); itr != table.end(); itr++){
        cout << itr->first << ": " << itr->second << endl;
        if (i >= 5){
            break;
        }
        i++;
    }

    //Get the end time and output
    time_tot = omp_get_wtime() - time_tot;
    cout << "TOTAL TIME: " << time_tot << endl;
    //Output values
   input.open("parallel_time.txt", fstream::app);
   input << time_p << ",";
   input.close();
   
   input.open("total_time.txt", fstream::app);
   input << time_tot << ",";
   input.close();
   
   input.open("top_five_ranks.txt", fstream::app);
   i = 1;
   cout << "Size of table: " << table.size() << endl;
   for (set<pair<int, int>, classcomp>::iterator itr = table.begin(); itr != table.end(); itr++){
       input << itr->first << ": " << (itr->second)/((double) N*K) << ", ";
       //input << itr->first << ":" << itr->second << ", ";
       //cout << itr->first << ":" << itr->second << endl;
       if (i >= 5){
           break;
       }
       i++;
   }
   input.close();
   
   //Clear all values
   edges->clear();
   nodes.clear();
   node_set.clear();
   table.clear();
   delete visits_per_node;

    return 0;
}

int initialize_array(int *arr, int size){
    int i = 0;
    //Initalize number of visits in each node
    for (i = 0; i < size; i++){
        arr[i] = 0;
    }
    return 0;
}

int get_data(fstream &input, vector<int> *edges, set<int> &nodes, int &max_node){
    string str;
    int edge_count = 0;
    int x, y;
    //Clear the comments if there are any, from the top of the text file
    while (!input.eof() && getline(input, str)){
        x = y = -1;
        //Make sure its not a comment that was read in
        if (str[0] != '#'){
	    edge_count++;
            //Want to now parse the string into x and y parts
            sscanf(str.c_str(), "%d%d", &x, &y);
            // if x or y still are -1, then there is either no string there or no edge
            if (x == -1)
                break;
            //Add edge to list
            edges[x].push_back(y);
            //Insert x and y into the set of nodes
            nodes.insert(x);
            nodes.insert(y);
            //Updat max node
            if (x > max_node){
                max_node = x;
            }
            if (y > max_node){
                max_node = y;
            }
        } else{
            cout << str << endl;
        }
    }
    cout << "EDGE COUNT: "<< edge_count << endl;
    //Close the fstream object
    input.close();
    return 0;
}

int rank_pages(vector<int> *edges, vector<int> nodes, int *visits_per_node, int max_node, int K, double D, int N){
    long int target = 0;
    int j = 0, i = 0;
    long long int seed = 0;
    double rand_num = 0.0;
    struct drand48_data buf;

    //Iterate through all the nodes: We could actually change this to iterate N times
    #pragma omp parallel for default(none) private(buf, i, j, target, seed, rand_num) shared(N, K, D, edges, visits_per_node, nodes, max_node)
    for (i = 0; i < N; i++){
        int cur_node = nodes[i];

        for (j = 0; j < K; j++){
            //Update the count at the current node
            #pragma omp atomic
                visits_per_node[cur_node]++;

            seed = omp_get_wtime() * (omp_get_thread_num() + 1);
            seed = seed ^ j ^ time(NULL);
            #if defined(_OPENMP)
                srand48_r(seed, &buf);
                drand48_r(&buf, &rand_num);
                lrand48_r(&buf, &target);
            #else
		cout << "OpenMP is not running!! PROBLEM" << endl;
                rand_num = drand48();
                target = rand();
            #endif

            //Get target node and also make sure that it leads somewhere
            if (rand_num > D && edges[cur_node].size() != 0){
                //Put it in the list of neighbors
                target = target % (edges[cur_node].size());
                cur_node = edges[cur_node][target];
            } else{
                //Update the current node
                target = target % (max_node + 1);
                cur_node = target;
            }
        }
    }
    return 0;
}

