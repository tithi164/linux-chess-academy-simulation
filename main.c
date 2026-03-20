#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#define MQ_NAME "/mq"
#define SHM_NAME "/shm"
#define SEM_NAME "/sem"

#define MAX_PLAYERS 15
#define MATCHES 10

int matches_played[MAX_PLAYERS] = {0};
int matches_won[MAX_PLAYERS] = {0};
int puzzles_correct[MAX_PLAYERS] = {0};
int puzzles_wrong[MAX_PLAYERS] = {0};

int *lb;
sem_t *sem;

int mode = 0;   // 0 idle, 1 match, 2 puzzle

// ---------------- PUZZLE THREAD ----------------
void* puzzle_thread(void* arg) {

    while(1) {

        if(mode == 2) {

            printf("\n--- Puzzle Session STARTED ---\n");

            sem_wait(sem);

            for(int i=0;i<MAX_PLAYERS;i++) {
                for(int j=0;j<5;j++) {
                    int r = rand()%2;
                    if(r) puzzles_correct[i]++;
                    else puzzles_wrong[i]++;
                }
            }

            sem_post(sem);

            printf("Puzzle session DONE\n\n");
            fflush(stdout);

            mode = 0; // stop after one session
        }

        sleep(1);
    }

    return NULL;
}

// ---------------- SIGNAL HANDLERS ----------------
void start_matches(int sig) {
    printf("\nSwitching to MATCH mode\n");
    mode = 1;
}

void start_puzzles(int sig) {
    printf("\nSwitching to PUZZLE mode\n");
    mode = 2;
}

void stop_all(int sig) {
    printf("\nStopping system\n");
    exit(0);
}

void show_leaderboard(int sig) {

    sem_wait(sem);

    printf("\n========== LEADERBOARD ==========\n");
    printf("Current Mode: MATCH / PUZZLE\n");

    for(int i=0;i<MAX_PLAYERS;i++) {
        printf("Player %d -> Played:%d Won:%d | Correct:%d Wrong:%d\n",
               i+1,
               matches_played[i],
               matches_won[i],
               puzzles_correct[i],
               puzzles_wrong[i]);
    }

    printf("=================================\n");

    sem_post(sem);
}
// ---------------- MAIN ----------------
int main() {

    printf("System Ready\n");

    signal(SIGUSR1, start_matches);
    signal(SIGUSR2, start_puzzles);
    signal(SIGINT, stop_all);
    signal(SIGTERM, show_leaderboard);

    // Shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(int)*MAX_PLAYERS);

    lb = mmap(0, sizeof(int)*MAX_PLAYERS,
              PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);

    for(int i=0;i<MAX_PLAYERS;i++) lb[i]=0;

    // Semaphore
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);

    // Message Queue
    struct mq_attr attr = {0,10,100,0};
    mqd_t mq = mq_open(MQ_NAME, O_CREAT|O_RDWR, 0666, &attr);

    // Pipe
    int pipefd[2];
    pipe(pipefd);

    // Logger
    if(fork()==0) {
        char *args[] = {"./logger", NULL};
        execv("./logger", args);
    }

    // Puzzle thread
    pthread_t pt;
    pthread_create(&pt, NULL, puzzle_thread, NULL);

    char buf[100];

    while(1) {

        // -------- MATCH MODE --------
        if(mode == 1) {

            printf("Starting Matches...\n");

            for(int i=0;i<MATCHES;i++) {
                if(fork()==0) {

                    close(pipefd[0]);

                    char fd_str[10];
                    sprintf(fd_str, "%d", pipefd[1]);

                    char *args[] = {"./match", fd_str, NULL};
                    execv("./match", args);
                }
            }

            close(pipefd[1]);

            while(1) {

                int n = read(pipefd[0], buf, sizeof(buf)-1);

                if(n <= 0) break;

                buf[n] = '\0';

                int player = atoi(buf);

                sem_wait(sem);

                matches_played[player]++;
                matches_won[player]++;
                lb[player]++;

                sem_post(sem);

                printf("Controller: Player %d won\n", player+1);
                fflush(stdout);

                mq_send(mq, buf, strlen(buf)+1, 0);
            }

            mode = 0; // stop after matches
            printf("Matches completed\n");
        }

        sleep(1);
    }

    return 0;
}
