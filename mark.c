#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>

//
// This code only works in unix-like 
//

// time of benchmark default: 7
#define MAX_TIME   7
// Start seed of srand
#define START_SEED 1000000000

// used for the benchmark thread
typedef struct {
	pthread_t tid;
	uint64_t score;
} thData;


// take a number and put a . every 3 digit from right
char* parseScore(uint64_t score) {
    int num_digits = snprintf(NULL, 0, "%" PRIu64, score);
    int num_spaces = (num_digits - 1) / 3; 
    int len_with_spaces = num_digits + num_spaces;
    char* formatted_str = (char*)malloc((len_with_spaces + 1) * sizeof(char));
    sprintf(formatted_str, "%" PRIu64, score);

    int original_len = strlen(formatted_str);
    for (int i = original_len + num_spaces; i >= 0; i--) 
        if ( (original_len - i) % 3 == 0 && original_len - i != 0 ) {
            memmove(formatted_str + i + 1, formatted_str + i, strlen(formatted_str + i) + 1);
            formatted_str[i] = '.';
        } 

    return formatted_str;	
}


// gives a score on the benchamrk done
uint64_t bench() {
	srand(START_SEED);
	register uint64_t score = 0;	
	
	for (register uint32_t then = time(0); time(0) - then <= MAX_TIME; score++)
		rand();
	
	return score / 10000;
}

// func() to call as a thread
void* bench_th(void* ptr) {
	thData* datas = (thData*)ptr;

	//printf("[%ld] started...\n", datas->tid);
	datas->score = bench();
	//printf("[%ld] %lu\n", datas->tid, datas->score);
}


int main() {
	// get the corese number
	uint8_t cores = sysconf(_SC_NPROCESSORS_ONLN);
	
	// allocate an array of thData to share the scores
	// with all the thread function
	thData* datas = malloc(sizeof(thData) * cores); 
	if (!datas) {
		printf("datas allocate error");
		return 1;
	}	

	printf("wait %ds...\n", MAX_TIME);

	// start a thread for every core of this cpu
	for (uint8_t i = 0; i < cores; i++) {
		pthread_create(&datas[i].tid, NULL, bench_th, (void*)&datas[i]);
	}    

	// wait all the threads to end
	for (uint8_t i=0; i<cores; i++)
		pthread_join(datas[i].tid, NULL);

// sum all scores
	uint64_t score = 0;
	for (uint8_t i = 0; i < cores; i++)
		score += datas[i].score;

	// convert in a string with a . every 3 digits
	char* sh = parseScore(score);
	printf("score: %s\n", sh);

	// free the allocated
	free(sh);	
	free(datas);	

	return 0;
}

