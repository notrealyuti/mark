#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/sysinfo.h>

// Testing
#include <sys/wait.h>

//
// This code only works in unix-like 
//

#define MAX_TIME   7
#define START_SEED 1000000000

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



uint64_t bench() {
	srand(START_SEED);
	uint64_t score = 0;	
	
	for (uint32_t then = time(0); time(0) - then <= MAX_TIME; score++)
		rand();
	
	return score / 10000;
}


int main() {
	uint8_t cores = sysconf(_SC_NPROCESSORS_ONLN);
	pid_t pid;
	int status;
	

	printf("wait %ds...\n", MAX_TIME);

    for (int i = 0; i < cores; i++) {
        pid = fork();
        if (pid < 0) {
            puts("pid error.");
            exit(1);
        } 
		else if (pid == 0) {
			bench();
            exit(0);
        }
    }

	uint64_t score = bench();

    // Wait all process to end
    while ((pid = wait(&status)) > 0);

	char* sh = parseScore(score);

	printf("score: %s\n", sh);

	free(sh);	
	

	return 0;
}

// Compile this as: gcc -std=c99 mark.c -o mark
//
