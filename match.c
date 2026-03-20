#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_PLAYERS 15

int delay;

void* run(void* arg) {

    delay = rand()%5 + 1;

    printf("Match %d started (%d sec)\n", getpid(), delay);
    fflush(stdout);

    usleep((rand()%1000)*1000);
    sleep(delay);

    return NULL;
}

int main(int argc, char *argv[]) {

    srand(getpid());

    int fd = atoi(argv[1]);

    pthread_t t;
    pthread_create(&t,NULL,run,NULL);
    pthread_join(t,NULL);

    int player = rand()%MAX_PLAYERS;

    char msg[10];
    sprintf(msg, "%d", player);

    write(fd, msg, strlen(msg)+1);

    printf("Match %d finished -> Winner Player %d\n", getpid(), player+1);
    fflush(stdout);

    return 0;
}
