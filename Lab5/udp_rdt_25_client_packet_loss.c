#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

#define BUFLEN 512
#define PORT 8882
#define TIMEOUT 2

int recv_flag=-1;
struct sockaddr_in servAddr;
int s, i, ran;
socklen_t slen = sizeof(struct sockaddr_in);
char buf[BUFLEN];
char message[BUFLEN];
DATA_PKT send_pkt, rcv_ack;

typedef struct packet1{
    int sq_no;
}ACK_PKT;

typedef struct packet2{
    int seq_no;
    char data[BUFLEN];
}DATA_PKT;

void die(char *s){
    perror(s);
    exit(1);
}

void printAddress(struct sockaddr_in *addr){
    printf("----------------------------------------\n");
    printf("Family: %d\n", addr->sin_family);
    printf("IP: %s\n", inet_ntoa(addr->sin_addr));
    printf("Port: %d\n", ntohs(addr->sin_port));
    printf("----------------------------------------\n");
}

void handler(int signo){
    if(recv_flag!=1){
        if(sendto(s, &send_pkt
        , sizeof(DATA_PKT), 0, (struct sockaddr*)&servAddr, sizeof(struct sockaddr)) == -1){
            die("Sendto() in sighandler");
        }

        alarm(TIMEOUT);
    }
}

int main(int argc, char *argv[]){
    srand(time(0));
    signal(SIGALRM, handler);

    if(argc != 3){
        printf("Usage: %s <Server IP(string)> <Server Port(Integer)>\n", argv[0]);
        exit(1);
    }

    if((s=socket(AF_INET, SOCK_DGRAM, 0))==-1){
        die("socket() in cleint");
    }

    bzero((void*)&servAddr, slen);
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[2]));
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);

    printf("Server address according to CLIENT\n");
    printAddress(&servAddr);

    int state = 0;
    while(1){
        switch (state)
        {
        case 0:
            printf("Enter message 0: ");
            fgets(send_pkt.data, sizeof(DATA_PKT), stdin);
            send_pkt.seq_no = 0;
            if(sendto(s, &send_pkt, sizeof(DATA_PKT), 0, (struct sockaddr*) &servAddr, slen) == -1){
                die("sendto() in client");
            }
            state = 1;
            recv_flag = 0;
            alarm(TIMEOUT);

            break;
        case 1:
            if(recvfrom(s, &rcv_ack, sizeof(ACK_PKT), 0, (struct sockaddr*)&servAddr, &slen) ==-1){
                die("recvfrom() in client");
            }
            // ran = rand()%2;
            // rcv_ack.seq_no = ran;
            // printf("ran in state 1 of client: %d\n", ran);

            if(rcv_ack.seq_no == 0){
                printf("Received ACK seq. no. : %d\n", rcv_ack.seq_no);
                state = 2;
                recv_flag = 1;
            }
            //should add break here, if rcv_ack.seq_no is 1, to come back to the same state. But, this is no packet loss case, hence panket 0, sent by sender is received buy receiver.
            else if(rcv_ack.seq_no == 1){
                printf("Packet 0 dropped\n");
                if(sendto(s, &send_pkt, sizeof(DATA_PKT), 0, (struct sockaddr*) &servAddr, slen) == -1){
                    die("sendto() in client");
                }
                state = 1;
                recv_flag = 0;
                recv_flag = 0;
            }
            else{
                printf("ACK dropped from SERVER to CLIENT\n");
                state = 1;
                recv_flag = 0;
            }
            
            break;
        case 2:
            printf("Enter message 1: ");
            fgets(send_pkt.data, sizeof(DATA_PKT), stdin);
            send_pkt.seq_no = 1;
            if(sendto(s, &send_pkt, sizeof(DATA_PKT), 0, (struct sockaddr*)&servAddr, slen) == -1){
                die("sendto() state 2--client");
            }
            recv_flag = 0;
            alarm(TIMEOUT);
            state = 3;
            
            break;
        case 3:
            if(recvfrom(s, &rcv_ack, sizeof(ACK_PKT), 0, (struct sockaddr*)&servAddr, &slen)==-1){
                die("recvfrom() state 3--client");
            }
            // ran = rand()%3;
            // rcv_ack.seq_no = ran;
            // printf("ran in state 3 of client: %d\n", ran);

            if(rcv_ack.seq_no==1){
                printf("Received ack seq. no. %d\n", rcv_ack.seq_no);
                recv_flag = 1;
                state = 0;
            }
            else if(rcv_ack.seq_no==0){
                printf("Packet 1 dropped\n");
                if(sendto(s, &send_pkt, sizeof(DATA_PKT), 0, (struct sockaddr*)&servAddr, slen) == -1){
                    die("sendto() state 1--client");
                }
                recv_flag = 0;
                state = 3;
            }
            else{
                printf("ACK dropped from SERVER to CLIENT\n");
                state = 3;
                recv_flag=0;
            }

            break;
        default:
            break;
        }
    }

    close(s);
    return 0;
}
