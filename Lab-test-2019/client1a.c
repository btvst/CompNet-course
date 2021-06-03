/* Running instructions */
/* 
* PACKET_SIZE variable can be changes by changing the preprocessor directive #define PACKET_SIZE

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#define PACKET_SIZE 100
#define TIMEOUT 2

typedef struct Packet{
    size_t size; //no. of bytes in the data of packet, 100 for all except maybe the last
    unsigned int seq_no; //byte offset from the start
    char data[PACKET_SIZE];
    char last_pkt; //'T' is last packet, else 'F'
    char TYPE; //'D' if data packet, 'A' is ACK packet
}packet;

int client_sockfd, i, ran, recv_flag;
struct sockaddr_in servAddr;
socklen_t slen = sizeof(struct sockaddr_in);
int curr_ind = 0;
packet *packets;

void die_with_error(char *s){
    perror(s);
    exit(0);
}

void handler(int signo){
    if(recv_flag != 1){
        printf("RETRANSMITTING PKT: Seq. no. = %d Size = %lu\n", packets[curr_ind-1].seq_no, packets[curr_ind-1].size);
        if(sendto(client_sockfd, &packets[curr_ind-1], sizeof(packet), 0, (struct sockaddr*)&servAddr, slen) == -1){
            die_with_error("Sendto() in signal handler\n");
        }
        sleep(1);
    }
}

void fill_packet(char *buffer, size_t bufsize, packet *pack, size_t size, unsigned int seq_no, char last_pkt, char TYPE){
    bzero(pack->data, PACKET_SIZE);

    for(int i=0; i<bufsize; ++i){
        (pack->data)[i] = buffer[i];
    }
    pack->size = size;
    pack->seq_no = seq_no;
    pack->last_pkt = last_pkt;
    pack->TYPE = TYPE;
}

void print_packet(packet *packets, size_t size){
    printf("-----------------------------------Packet details-----------------------------------------\n");
    for(int i=0; i<size; ++i){
        printf("-----------------------------------Packet %d---------------------------------------\n", i+1);

        /* As packet data is not NULL terminated to prevent inconsistency and extra storage, hence printing the data in the form of a string will show undefined behaviour, due to the data being not NULL terminated 

         * Printing the string character by character prevents us from this problem
        */

        printf("Data: ");
        for(int j=0; j<packets[i].size; ++j){
            printf("%c", packets[i].data[j]);
        }
        printf("\n");

        printf("Size of payload: %lu\n", packets[i].size);
        printf("Sequence no: %d\n", packets[i].seq_no);
        printf("Last packet field: %c\n", packets[i].last_pkt);
        printf("TYPE field: %c\n", packets[i].TYPE);
        printf("------------------------------------------------------------------------------------\n");
    }
}

void prepare_packet(packet* packets, int no_of_packets, FILE *fp2){
    // packet packets[no_of_packets];
    curr_ind = 0;

    char buf[PACKET_SIZE+1];
    char buf1[PACKET_SIZE+1];
    size_t offset = 0;

    /* PACKET_SIZE + 1; To accomodate the null character for the string to show defined behaviour */

    bzero(buf, PACKET_SIZE+1);
    bzero(buf1, PACKET_SIZE+1);

    while(curr_ind != no_of_packets){
        size_t bytesRead;
        printf("curr_ind: %d\n", curr_ind);

        if(curr_ind < no_of_packets-1){
            bytesRead = read(fileno(fp2), buf, PACKET_SIZE);
            printf("Bytes read: %lu\n", bytesRead);
            printf("%s\n", buf);

            if(bytesRead == 0){
                die_with_error("2. fread()");
            }
            else{
                fill_packet(buf, bytesRead, &packets[curr_ind], bytesRead, offset, 'F', 'D');
            }

        }
        else{
            bytesRead = read(fileno(fp2), buf1, PACKET_SIZE);
            printf("Bytes read: %lu\n", bytesRead);
            printf("%s\n", buf1);

            if(bytesRead == 0){
                die_with_error("3. fread()");
            }
            else{
                fill_packet(buf1, bytesRead, &packets[curr_ind], bytesRead, offset, 'T', 'D');
            }

        }

        offset += bytesRead;
        ++curr_ind;
    }
}

void printAddress(struct sockaddr_in *addr){
    printf("----------------------------------------\n");
    printf("Family: %d\n", addr->sin_family);
    printf("IP: %s\n", inet_ntoa(addr->sin_addr));
    printf("Port: %d\n", ntohs(addr->sin_port));
    printf("----------------------------------------\n");
}

int main(int argc, char *argv[]){
    srand(time(0));
    signal(SIGALRM, handler);
    if(argc != 4){
        printf("Usage: %s <inputfile name> <Server IP(string)> <Server Port(Integer)>\n", argv[0]);
        exit(0);
    }

    FILE* fp1 = fopen(argv[1], "r");

    size_t total_file_size = 0;
    if(fseek(fp1, 0, SEEK_END) < 0){
        die_with_error("1. fseek()");
    }

    printf("Size of file: %ld\n", ftell(fp1));

    total_file_size = ftell(fp1);
    int no_of_packets = (int)((total_file_size/PACKET_SIZE)+1);

    printf("No. of packets in file %s = %d\n", argv[1], no_of_packets);

    packets = (packet*)malloc(sizeof(packet)*no_of_packets);
    FILE *fp2 = fopen(argv[1], "r");

    //Packetization
    prepare_packet(packets, no_of_packets, fp2);

    print_packet(packets, no_of_packets);

    //Implement stop-and-wait ARQ

    if((client_sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
        die_with_error("4. socket() in client");
    }

    bzero((void*)&servAddr, slen);
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[3]));
    servAddr.sin_addr.s_addr = inet_addr(argv[2]);

    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("Server address according to CLIENT\n");
    printAddress(&servAddr);

    if(connect(client_sockfd, (struct sockaddr*)&servAddr, sizeof(struct sockaddr)) < 0){
        die_with_error("connect() failed");
    }

    int state = 0;
    curr_ind = 0;
    packet recv_packet;

    while(curr_ind < no_of_packets){
        switch (state)
        {
        case 0:

            if(send(client_sockfd, (void*)&packets[curr_ind], sizeof(packet), 0) == -1){
                die_with_error("5. sendto() in client");
            }
            else{
                printf("SENT PKT: Seq no. = %d, Size = %lu bytes\n", packets[curr_ind].seq_no, packets[curr_ind].size);
                sleep(1); //send a single packet once
            }
        
            state = 1;
            recv_flag = 0;
            ++curr_ind;
            alarm(TIMEOUT);

            break;
        case 1:
 
            if(recv(client_sockfd, &recv_packet, sizeof(packet), 0) == -1){
                die_with_error("6. recvfrom() in client");
            }

            if(recv_packet.seq_no == packets[curr_ind].seq_no && recv_packet.TYPE =='A'){
                printf("RCVD ACK: Seq. no. = %d\n", recv_packet.seq_no);
                state = 0;
                recv_flag = 1;
            }
            else{
                printf("Garbage received at client\n");
            }
            break;
        default:
            break;
        }
    }

    close(client_sockfd);
    return 0;
}