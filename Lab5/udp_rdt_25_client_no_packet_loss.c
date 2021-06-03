#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFLEN 512
#define PORT 8882

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

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Usage: %s <Server IP(string)> <Server Port(Integer)>\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in servAddr;
    int s, i;
    socklen_t slen = sizeof(struct sockaddr_in);
    char buf[BUFLEN];
    char message[BUFLEN];
    DATA_PKT send_pkt, rcv_ack;

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

            break;
        case 1:
            if(recvfrom(s, &rcv_ack, sizeof(ACK_PKT), 0, (struct sockaddr*)&servAddr, &slen) ==-1){
                die("recvfrom() in client");
            }
            if(rcv_ack.seq_no == 0){
                printf("Received ACK seq. no. : %d\n", rcv_ack.seq_no);
                state = 2;
                break;
            }
            //should add break here, if rcv_ack.seq_no is 1, to come back to the same state. But, this is no packet loss case, hence panket 0, sent by sender is received buy receiver.
            break;

        case 2:
            printf("Enter message 1: ");
            fgets(send_pkt.data, sizeof(DATA_PKT), stdin);
            send_pkt.seq_no = 1;
            if(sendto(s, &send_pkt, sizeof(DATA_PKT), 0, (struct sockaddr*)&servAddr, slen) == -1){
                die("sendto() state 1--client");
            }
            state = 3;
            
            break;
        case 3:
            if(recvfrom(s, &rcv_ack, sizeof(ACK_PKT), 0, (struct sockaddr*)&servAddr, &slen)==-1){
                die("recvfrom() state 3--client");
            }
            if(rcv_ack.seq_no==1){
                printf("Received ack seq. no. %d\n", rcv_ack.seq_no);
                state = 0;
                break;
            }

            break;
        default:
            break;
        }
    }

    close(s);
    return 0;
}
