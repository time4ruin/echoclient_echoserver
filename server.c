#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>

#define BUFSIZE 1024

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int parentfd;
int sockfd;
int portno;
int clientlen;
struct sockaddr_in serveraddr;
struct sockaddr_in clientaddr;
char buf[BUFSIZE];
int optval;
int n;

void *threadfunc(void *arg);

int socket_id[100];
int socket_num = 0;
int b_option = 0;
int main(int argc, char **argv)
{

    int th_id;
    pthread_t thread_t;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    if (argc != 2 && argc != 3)
    {
        fprintf(stderr, "usage 1: %s <port>\n", argv[0]);
        fprintf(stderr, "usage 2: %s <port> -b\n", argv[0]);
        exit(1);
    }
    else if (argc == 3 && argv[2][1] != 'b')
    {
        printf("No option \"%s\" found!\n", argv[2]);
        exit(1);
    }
    else if (argc == 3 && argv[2][1] == 'b')
    {
        b_option = 1;
        printf("-b option enabled\n");
    }
    portno = atoi(argv[1]);

    parentfd = socket(AF_INET, SOCK_STREAM, 0);
    if (parentfd < 0)
        error("ERROR opening socket");

    optval = 1;
    setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    if (bind(parentfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        error("ERROR on binding");

    if (listen(parentfd, 5) < 0)
        error("ERROR on listen");

    clientlen = sizeof(clientaddr);
    while (1)
    {
        sockfd = accept(parentfd, (struct sockaddr *)&clientaddr, &clientlen);
        if (sockfd < 0)
            error("ERROR on accept");

        socket_id[socket_num++] = sockfd;
        th_id = pthread_create(&thread_t, NULL, threadfunc, (void *)&sockfd);
        if (th_id != 0)
        {
            perror("Thread Create Error");
            return 1;
        }
        pthread_detach(thread_t);
    }
}

void *threadfunc(void *arg)
{
    int sockfd;
    int tmp_socket_num;
    int readn, writen;
    char buf[BUFSIZE];
    char tmp[BUFSIZE];
    struct hostent *hostp;
    char *hostaddrp;
    sockfd = *((int *)arg);

    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                          sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
        error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
        error("ERROR on inet_ntoa\n");

    printf("Socket#%d: established connection with %s (%s)\n", sockfd, hostp->h_name, hostaddrp);
    snprintf(buf, BUFSIZE, "Your ID: %d\n", sockfd);
    n = write(sockfd, buf, strlen(buf));
    while (1)
    {
        bzero(buf, BUFSIZE);
        n = read(sockfd, buf, BUFSIZE); //blocking mode
        if (buf[0]=='q' && strlen(buf)==2){
            printf("q signal received\n");
            break;
        }
        printf("---------Received from %s (%s)---------\n", hostp->h_name, hostaddrp);
        printf("%s", buf);
        if (b_option == 1)
        { //to all client
            for (int i = 0; i < socket_num; i++)
            {
                if (sockfd == socket_id[i]){
                    snprintf(tmp, BUFSIZE, "Echo: ");
                    strncat(tmp, buf, strlen(buf));
                }
                else {
                    snprintf(tmp, BUFSIZE, "User#%d: ", sockfd);
                    strncat(tmp, buf, strlen(buf));
                }
                n = write(socket_id[i], tmp, strlen(tmp));
                if (n < 0)
                    error("ERROR writing to socket");
                printf("--------Echo Successfully Sent to Socket#%d--------\n", socket_id[i]);
            }
        }
        else {
            snprintf(tmp, BUFSIZE, "Echo: ");
            strncat(tmp, buf, strlen(buf));
            n = write(sockfd, buf, strlen(buf));
            if (n < 0)
                error("ERROR writing to socket");
            printf("--------Echo Successfully Sent to Socket#%d--------\n", sockfd);
            
        }
    }
    for (int i=0;i<socket_num;i++){
        if (socket_id[i]==sockfd){
            tmp_socket_num=i;
            break;
        }
    }
    for (int j=tmp_socket_num;j<socket_num-1;j++){
        socket_id[j] = socket_id[j+1];
    }
    socket_num--;
    printf("Socket#%d: server closed connection with %s (%s)\n", sockfd, hostp->h_name, hostaddrp);
    close(sockfd);
}