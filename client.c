#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <signal.h>

#define BUFSIZE 2048

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char **argv)
{

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    char buf2[BUFSIZE];
    char buf3[BUFSIZE];

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <hostname> <port>\n", argv[0]);
        exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0)
        error("ERROR connecting");

    printf("Connecting to %s...\n", hostname);

    pid_t pid;
    pid = fork(); //fork
    if (pid == -1)
    {
        printf("fork error\n");
        exit(0);
    }

    if (pid == 0)
    { //child
        while (1)
        {
            bzero(buf, BUFSIZE);
            n = read(sockfd, buf, BUFSIZE); //blocking mode
            printf("\n----------------Message from server----------------\n%s\n", buf);
            printf("Your message: ");
        }
    }
    else
    { //parent
        while (1)
        {
            fgets(buf, sizeof(buf), stdin);
            printf("%d", strlen(buf));
            if (buf[0] == 'q'){
                n = write(sockfd, buf, strlen(buf));
                close(sockfd);
                kill(0, SIGINT);
                return 0;
            }
            n = write(sockfd, buf, strlen(buf));
            if (n < 0)
                        error("ERROR writing to socket");
            printf("------------Message Successfully Sent------------\n");
        }
    }

    close(sockfd);
    return 0;
}