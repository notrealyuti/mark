#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>


//
// this code only works in unix-like 
//
// author: yuti

// time of benchmark default: 7
#define MAX_TIME   7

// sleep time btw the 2 benchmarks
#define SLEEP_TIME 3

// how much the score is divided
#define DIV_SCORE 100000

// Start seed of srand
#define START_SEED 1000000000


// some typedef to work better with ints

// unsigned
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// signed
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;



// used for the benchmark thread
typedef struct {
	u64 score;
	// in a future maybe i'll add other data 
	// so i prefer to maintain the struct 
} chData;  // means child data


// take a number and put a . every 3 digit from right (chatGPT did a goodjob)
char* parseScore(u64 score) {
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

	// prevent the first char to be a .
	if (formatted_str[0] == '.')
		formatted_str[0] = '\b';  // also ' ' is good in case of problems


    return formatted_str;	
}

void removeSubstr (char *string, char *sub) {
    char *match;
    int len = strlen(sub);
    while ((match = strstr(string, sub))) {
        *match = '\0';
        strcat(string, match+len);
    }
}

// gives a score on the benchamrk done (the important func in this program)
u64 bench() {
	srand(START_SEED);
	register u64 score = 0;	
	
	for (register u32 then = time(0); time(0) - then <= MAX_TIME; score++)
		rand();
	
	return score / DIV_SCORE;
}

void getCPUinfos(char* output, u16 size) {
	FILE* cmd = popen("cat /proc/cpuinfo", "r");
	if (!cmd)
		return;


	while(fgets(output, size, cmd) != NULL) {
		char* p = strstr(output, "model name");
		if (p != NULL) {
			//printf("model name found!\n");
			//printf("%s\n", output);
			removeSubstr(output, "model name	: ");
			break;
		}
		output[0] = '\0';
	}
}



int main() {
	// get the cores number
	u16 cores = sysconf(_SC_NPROCESSORS_ONLN);
	u32 freq  = sysconf(_SC_CLK_TCK);
	u64 singleScore = 0;
	u64 multiScore  = 0;
	pid_t pid;
    int status;
	char infos[100];



	// create a shared memory for an array of chDatas i
	// this array will contain all scores from all process
	// so needs to be shared
	int shmid = shmget(IPC_PRIVATE, sizeof(chData) * cores, IPC_CREAT | 0666);
    if (shmid < 0) {
        printf("shared memory creation errror");
        return 1;
    }


	chData* datas = (chData*)shmat(shmid, NULL, 0);
	if (datas == (chData*)-1) {
		printf("shared memory attach error");
		return 1;	
	}

	// we do 2 benchmark so MAX_TIME*2 + the sleep time btw the 2
	// + 1 for a bit of tolerance 
	printf("running... wait %ds\n", MAX_TIME*2 + SLEEP_TIME + 1);

//
// SINGLE CORE BENCHMARK
//
	singleScore = bench();

	// sleep btw 2 benchs
	sleep(SLEEP_TIME);

//
// MULTI CORE BENCHMARK
//                        a bit difficult

	for (u16 i=0; i < cores; i++) {
		pid = fork();
		if (pid < 0) {
			printf("fork error");
			// free shared memory 
			shmctl(shmid, IPC_RMID, NULL);
			return 1;
		}

		else if (pid == 0) {
			//printf("child[%d#%d] hi\n", i, getpid());

			// from the child process
			datas[i].score = bench();
			//printf("child[%d#%d] score: %lu\n", i, getpid(), datas[i].score);

			// deteach the shared memory from this child process
			// so this proc cant access to it anymore (i think for security reasons)
			shmdt(datas);
			exit(0);
		}
	}

	// wait for all process to end
    while ((pid = wait(&status)) > 0);


	// sum all scores
	for (u16 i=0; i < cores; i++)
		multiScore += datas[i].score;

	// free shared memory 
	shmctl(shmid, IPC_RMID, NULL);


// print results
	getCPUinfos(infos, sizeof(infos));
	printf("\n%s", infos);


	char* tmp = parseScore(singleScore);
	printf("single-core: %s\n", tmp);
	free(tmp);

	tmp = parseScore(multiScore);
	printf("multi-core:  %s | cores: %u\n", tmp, cores);
	free(tmp);

	return 0;
}

