#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RCVBUFSIZE 32
#define I 10

typedef struct packet{
    int n;
} pack;

void die_with_error(char *msg){
    perror(msg);
    exit(1);
}

void print_address(struct sockaddr_in *addr){
    printf("--------------------------------------\n");
    printf("Addr family: %d\n", addr->sin_family);
    printf("Addr IP: %s\n", inet_ntoa(addr->sin_addr));
    printf("Addr Port: %d\n", ntohs(addr->sin_port));
    printf("--------------------------------------\n");
}

int main(int argc, char *argv[]){
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[RCVBUFSIZE];

    if(argc != 3){
        printf("Usage: %s <Server IP> <Server Port>\n",argv[0]);
        exit(1);
    }

    if((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        die_with_error("socket() failed in CLIENT");
    }

    bzero(&serv_addr, sizeof(struct sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in)) < 0){
        die_with_error("connect() failed in CLIENT");
    }

    printf("Server address according to CLIENT\n");
    print_address(&serv_addr);

    for(int i=0; i<I; ++i){
        pack p;
        p.n = i+1;

        if(send(sockfd, (void*)&p, sizeof(pack), 0) != sizeof(pack)){
            die_with_error("send() failed in CLIENT");
        }
        else{
            printf("Sent: %d\n", p.n);
        }

        int t;
        scanf("%d", &t);
    }
    
    close(sockfd);
    exit(0);
}