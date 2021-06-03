#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 512

#define PORT 8888

void die(char *s){
    perror(s);
    exit(1);
}

void dg_cli(FILE *fp, int sockfd, struct sockaddr* servaddr, socklen_t serv_len){
    int n;
    int num;
    char sendline[BUFLEN], recvline[BUFLEN];

    for(;;){
        fprintf(stdout, "Guess the number b/w 1 and 6:\n");
        fscanf(stdin, "%s", sendline);
        if(atoi(sendline)<0){
            die("string format error client\n");
        }
        fflush(stdin);
        fflush(stdout);

        if(sendto(sockfd, (void*) sendline, strlen(sendline), 0, (struct sockaddr*) servaddr, serv_len) == -1){
            die("sendto() client\n");
        }

        bzero(&sendline, sizeof(sendline));
        bzero(&recvline, sizeof(recvline));

        if(recvfrom(sockfd, recvline, BUFLEN, 0, (struct sockaddr*) servaddr, &serv_len) == -1){
            die("recvfrom client\n");
        }
        else{
            // printf("CCCC\n");
            fprintf(stdout, "CLIENT: (received) %s\n", recvline);
            fflush(stdout);
        }
    }

    close(sockfd);
}

void udp_client(int argc, char **argv){
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    
    if(argc < 3 || argc > 4){
        die("usage: ./a.out <server ip> <server port> [echo word]\n");
    }
    else{
        servaddr.sin_family = AF_INET;        
        if((servaddr.sin_addr.s_addr = inet_addr(argv[1])) < 0){
            die("servid format\n");
        }
        servaddr.sin_port = htons(atoi(argv[2]));
    }

    int sockfd, serv_len = sizeof(servaddr);

    if((sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1){
        die("socket() failed\n");
    }

    fprintf(stdout, "Client socket created\n");

    fprintf(stdout, "CLIENT: Server IP: %s ;; Port: %d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

    dg_cli(stdin, sockfd, (struct sockaddr*) &servaddr, serv_len);
}

int main(int argc, char **argv){
    udp_client(argc, argv);
}