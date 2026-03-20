#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <mqueue.h>
#include <string.h>

#define MQ_NAME "/mq"

int main() {

    mqd_t mq = mq_open(MQ_NAME, O_RDONLY);

    int fd = open("log.txt", O_CREAT | O_RDWR | O_APPEND, 0666);

    char buf[100];

    while(1) {

        mq_receive(mq, buf, sizeof(buf), NULL);

        if(strcmp(buf,"exit")==0) break;

        printf("Logger: Player %d logged\n", buf[0]-'0'+1);

        write(fd, buf, strlen(buf));
        write(fd, "\n", 1);
    }

    close(fd);
    return 0;
}
