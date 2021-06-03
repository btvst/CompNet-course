#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFLEN 512
#define PORT 8882

void die(char *s){
    perror(s);
    exit(1);
}

typedef struct packet1{
    int sq_no;
}ACK_PKT;

typedef struct packet2{
    int sq_no;
    char data[BUFLEN];
}DATA_PKT;

void printAddress(struct sockaddr_in *addr){
    printf("----------------------------------------\n");
    printf("Family: %d\n", addr->sin_family);
    printf("IP: %s\n", inet_ntoa(addr->sin_addr));
    printf("Port: %d\n", ntohs(addr->sin_port));
    printf("----------------------------------------\n");
}

int main(int argc, char *argv[]){
    if(argc!=3){
        printf("Usage: %s <Server IP(string)> <Server Port(Integer)>\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in servAddr, clntAddr;
    int s, i;
    socklen_t slen = sizeof(struct sockaddr_in);
    int recv_len;

    DATA_PKT rcv_pkt;
    ACK_PKT ack_pkt;

    if((s = socket(AF_INET, SOCK_DGRAM, 0))==-1){
        die("socket() in server");
    }

    bzero((void*)&servAddr, sizeof(struct sockaddr));
    bzero((void*)&clntAddr, sizeof(struct sockaddr));

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);
    servAddr.sin_port = htons(atoi(argv[2]));

    printf("Server address according to SERVER\n");
    printAddress(&servAddr);

    if(bind(s, (struct sockaddr*)&servAddr, slen)==-1){
        die("bind() in server");
    }

    int state = 0;
    while(1){
        switch (state)
        {
        case 0:
            if((recv_len=recvfrom(s, &rcv_pkt, BUFLEN, 0, (struct sockaddr*)&clntAddr, &slen)==-1)){
                die("recvfrom() in state 0--SERVER");
            }

            if(rcv_pkt.sq_no==0){
                printf("Client address according to SERVER\n");
                printAddress(&clntAddr);
                printf("Packet received with seq. no. %d\n", rcv_pkt.sq_no);
                printf("Message: %s\n", rcv_pkt.data);

                ack_pkt.sq_no = 0;
                if(sendto(s, &ack_pkt, sizeof(ACK_PKT), 0, (struct sockaddr*)&clntAddr, slen)==-1){
                    die("ack sendto()--server\n");               
                }
                state = 1;
                break;
            }
            // else{
            //     ack_pkt.sq_no = 1;
            //     if(sendto(s, &ack_pkt, sizeof(ACK_PKT), 0, (struct sockaddr*)&clntAddr, &slen)==-1){
            //         die("ack sendto()--server\n");               
            //     }
            //     state = 0;
            //     break;
            // }
            break;
        case 1:
            printf("Waiting for packet 1 from sender...\n");
            fflush(stdout);

            if((recv_len=recvfrom(s, &rcv_pkt, BUFLEN, 0, (struct sockaddr *) &clntAddr, &slen))==-1){
                die("recvfrom() in state 1--SERVER");
            }
            printf("Client address according to SERVER\n");
            printAddress(&clntAddr);

            if(rcv_pkt.sq_no == 1){
                printf("Packet received with seq. no. %d\n", rcv_pkt.sq_no);
                printf("Packet message: %s\n", rcv_pkt.data);

                ack_pkt.sq_no = 1;
                if(sendto(s, &ack_pkt, sizeof(ACK_PKT), 0, (struct sockaddr*)&clntAddr, slen)==-1){
                    die("sendto() in state 1--SERVER");
                }
                state = 0;
                break;
            }
            // else{
            //     ack_pkt.sq_no = 0;
            //     if(sendto(s, &ack_pkt, sizeof(ACK_PKT), 0, (struct sockaddr*)&clntAddr, &slen)==-1){
            //         die("ack sendto()--server\n");               
            //     }
            //     state = 1;
            //     break;
            // }
            break;

        default:
            break;
        }
    }
    close(s);
    return 0;
}
