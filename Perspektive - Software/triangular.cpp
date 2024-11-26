#include <stdio.h>
#include <omp.h>
#include <math.h>
#include <chrono>

using namespace std;
using namespace chrono;

#define N_TRIANGULAR pow(2,6)*6  //384  // n do kog ce se racunati trougaoni brojevi
#define CHUNK_SIZE 2	// velicina bloka
#define THREAD_NUM 6	// broj niti u upotrebi

struct pairs_struct {
    int value;
    int time;
    int rank;
};

double triangular_sum_serial(struct pairs_struct pairs []) 
{
	int n = N_TRIANGULAR;
	double start_for, end_for, time_for;
	
    start_for = omp_get_wtime();
    for (int i = 0; i < n; i++) 
    {
    	auto start = high_resolution_clock::now();
        int sum = 0;
        for (int j = 0; j <= i; j++) // i puta
            sum += j;
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<nanoseconds>(end - start);
        pairs[i].value = sum;
        pairs[i].time = (int)duration.count();
        pairs[i].rank = omp_get_thread_num();
    }
    end_for = omp_get_wtime();
    time_for = (end_for - start_for);
    return time_for;
}

double triangular_sum(struct pairs_struct pairs [], const omp_sched_t schedule_type) 
{
	int n = N_TRIANGULAR;
	double start_for, end_for, time_for;
    omp_set_schedule(schedule_type,CHUNK_SIZE);
    start_for = omp_get_wtime();
	#pragma omp parallel for schedule (runtime) num_threads (THREAD_NUM)
    for (int i = 0; i < n; i++) 
    {
    	auto start = high_resolution_clock::now();
        int sum = 0;
        for (int j = 0; j <= i; j++) // i puta
            sum += j;
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<nanoseconds>(end - start);
        pairs[i].value = sum;
        pairs[i].time = (int)duration.count();
        pairs[i].rank = omp_get_thread_num();
    }
    end_for = omp_get_wtime();
    time_for = (end_for - start_for);
    
    return time_for;
}



double triangular_sum_dynamic_by_hand(struct pairs_struct pairs []) 
{
	int n = N_TRIANGULAR;
	double start_for, end_for, time_for;
	
    start_for = omp_get_wtime();
    ////////////////////////////////
    enum thread_state {busy,idle};
    struct queue_field {
    	int from;
    	int to;
    };
    int array_size = n / CHUNK_SIZE;
    int remainder = n % CHUNK_SIZE;
    if (remainder) 
    	array_size++;
    struct queue_field array [array_size];
    for (int i = 0;i < array_size; i++)
    {
    	if (remainder && i==array_size-1)
    		array[i].from = n - remainder;
    	else
    		array[i].from = CHUNK_SIZE * i;
    	
    	if (i==array_size-1)
    		array[i].to = n;
    	else 
    		array[i].to = CHUNK_SIZE * (i+1);
    }
    int index = 0;
    omp_lock_t lock;
	omp_init_lock(&lock);
	#pragma omp parallel num_threads (THREAD_NUM)
	{
		int rank = omp_get_thread_num(); //id trenutne niti
		enum thread_state state;
		state = idle;
		struct queue_field field;
		int index_local = 0;
		
		do 
		{
			omp_set_lock(&lock);
			if (state==idle)
				{	
					index_local = index;
					index++;
					state=busy;
					//printf("busy false thread %d\n",rank); //debug
				}
			omp_unset_lock(&lock);
			//printf("index = %d (thread %d)\n", index_local, rank); //debug
			
			if (state==busy)
			{
				field = array[index_local];
				for (int i = field.from ;i < field.to ;i++)
				{
					auto start = high_resolution_clock::now();
					int sum = 0;
					for (int j = 0; j <= i; j++) // i times
						sum += j;
					auto end = high_resolution_clock::now();
					auto duration = duration_cast<nanoseconds>(end - start);
					pairs[i].value = sum;
        			pairs[i].time = (int)duration.count();
        			pairs[i].rank = rank;
        			//fprintf(file,"%d %d %d\n",pairs[i].value,pairs[i].time,pairs[i].rank); //debug
        			
        			//printf("busy true thread %d\n",rank); //debug
        			//printf("%d %d\n",i,rank); //debug
				}
				state=idle;
			}
			
			//printf("busy[%d] = %d\n", rank, busy[rank]); //debug
			//printf("%d %d\n",index_local,rank); //debug
		} while (index<array_size);  
		
		#pragma omp barrier
		omp_destroy_lock(&lock);
		/////////////////////////
		end_for = omp_get_wtime();
	}
	
    time_for = (end_for - start_for);
    return time_for;
}



void write_to_file (struct pairs_struct pairs [],const char* filename, const int pairs_lenght)
{
	FILE *file;
	file = fopen(filename, "w");
	if (file == NULL) {
        perror("Error opening file for writing"); 
        return;
    }
    for (int i = 0; i < pairs_lenght; i++) 
    {
        fprintf(file, "%d %dns %d\n", pairs[i].value, pairs[i].time, pairs[i].rank);
        
    }
    fclose(file);  
}

int main() 
{
	double time_serial, time_static, time_dynamic, time_dynamic_bh;
	int pairs_lenght = N_TRIANGULAR;
	struct pairs_struct pairs[pairs_lenght];
	
	printf("Serial execution\n");  //debug
	time_serial = triangular_sum_serial(pairs);
	write_to_file(pairs,"serial_output.txt",pairs_lenght);
	
	printf("Static p. execution\n");  //debug
    time_static = triangular_sum(pairs,omp_sched_static);
    write_to_file(pairs,"static_output.txt",pairs_lenght);
    
    printf("Dynamic p. execution\n");  //debug
    time_dynamic = triangular_sum(pairs,omp_sched_dynamic);
    write_to_file(pairs,"dynamic_output.txt",pairs_lenght);
    
    printf("Dynamic by hand p. execution\n");  //debug
    time_dynamic_bh = triangular_sum_dynamic_by_hand(pairs);
    write_to_file(pairs,"dynamic_bh_output.txt",pairs_lenght);

    printf("Max usable number of threads: %d\n", omp_get_max_threads()); 
    printf("Number of threads used: %d\n", THREAD_NUM); 
    printf("Serial took %f s\n", time_serial); 
    printf("Static took %f s\n", time_static);
    printf("Dynamic took %f s\n", time_dynamic);
    printf("Dynamic faster by a factor of %f\n", time_static/time_dynamic);
    
    printf("Dynamic by hand took %f s\n", time_dynamic_bh);
     

    return 0;
}
