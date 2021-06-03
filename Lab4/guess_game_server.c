#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define BUFLEN 512

#define PORT 8888

char* itoa(int val, int base){
	static char buf[32] = {0};
	int i = 30;
	
	for(; val && i ; --i, val /= base){	
		buf[i] = "0123456789abcdef"[val % base];
    }
	
	return &buf[i+1];
	
}

void die(char *s){
    perror(s);
    exit(1);
}

void dg_serv(int sockfd, struct sockaddr_in* cliaddr, socklen_t clilen, struct sockaddr_in* servaddr){
    fprintf(stdout, "SERVER: Server IP: %s ;; Server port: %d\n", inet_ntoa((*servaddr).sin_addr), ntohs((*servaddr).sin_port));
    int n;
    int corr_num;
    char recvmsg[BUFLEN];

    char corr[2] = "1";
    char wrong[2] = "0";
    
    // fprintf(stdout, "SERVER: Server IP: %s ;; Server Port: %d\n", inet_ntoa((*servaddr).sin_addr), ntohs((*servaddr).sin_port));

    srand(time(0));
   
    for(;;){
        bzero(recvmsg, sizeof(recvmsg));

        corr_num = (rand()%6)+1;
        if(n=recvfrom(sockfd, recvmsg, BUFLEN, 0, (struct sockaddr*) cliaddr, &clilen) == -1){
            die("recvfrom() server error\n");
        }
        else{
            fprintf(stdout, "SERVER: Client IP: %s ;; Client Port: %d\n", inet_ntoa((*cliaddr).sin_addr), ntohs((*cliaddr).sin_port)); 
            fprintf(stdout, "SERVER: (received) %s correct num: %d\n", recvmsg, corr_num);
            fflush(stdout);
            if(corr_num == atoi(recvmsg)){
                if(sendto(sockfd, (void*)corr, 2, 0, (struct sockaddr*) cliaddr, sizeof((*cliaddr))) ==-1){
                    die("sendto() server error\n");
                }
            }
            else{
                if(sendto(sockfd, (void*)wrong, 2, 0, (struct sockaddr*) cliaddr, sizeof((*cliaddr))) ==-1){
                    die("sendto() server error\n");
                }
            }
            // cliaddr->sin_family = AF_INET;
            // cliaddr->sin_addr = inet_addr((*cliaddr).sin_addr);
            // cliaddr->sin_port = ntohs((*cliaddr).sin_port);
        }
    }

    close(sockfd);
}

void udp_server(int argc, char **argv){
    struct sockaddr_in servaddr, cliaddr;
    int sockfd;
    bzero(&servaddr, sizeof(servaddr));

    if(argc<2 || argc>2){
        die("Usage: ./a.out <server port>\n");
    }
    else{
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(atoi(argv[1]));
        servaddr.sin_addr.s_addr = htonl(inet_addr("0.0.0.0"));
    }

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        die("socket() server\n");
    }

    if(bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0){
        die("bind() failed\n");
    }
    else{
        fprintf(stdout, "Server ready\n");
    }

    fprintf(stdout, "AAAA: server port in network oreder: %d\n", servaddr.sin_port);
    fprintf(stdout, "AAAA: server port in host oreder: %d\n", ntohs(servaddr.sin_port));    

    dg_serv(sockfd, (struct sockaddr_in*) &cliaddr, sizeof(cliaddr), (struct sockaddr_in*) &servaddr);
}

int main(int argc, char **argv){
    udp_server(argc, argv);
}