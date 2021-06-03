/* instructions */
/* 
* PACKET_SIZE variable can be changes by changing the preprocessor directive #define PACKET_SIZE

* Run the server first to generate its IP address.

* Usage: ./a.out <inputfile name> <Server IP(string)> <Server Port(Integer)>
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

#define PACKET_SIZE 512 //Denotes maximum limit on sizeof a packet dataload
#define TIMEOUT 2

typedef struct Packet{
    size_t size; //no. of bytes in the data of packet, for all except maybe the last
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

int str_len(char *str){
    int i = 0;
    int len = 0;
    while(str[i] != '\n'){
        ++len;
        ++i;
    }

    ++len;

    return len;
}

void prepare_packet(packet* packets, int no_of_packets, FILE *fp2){
    // packet packets[no_of_packets];
    curr_ind = 0;
    size_t offset1 = 0;

    char buffer[PACKET_SIZE];
    for(int i=0; i<PACKET_SIZE; ++i){
        buffer[i] = '\0';
    }
    while(fgets(buffer, PACKET_SIZE-1, fp2)){
        printf("%s", buffer);
        // buffer[strcspn(buffer, "\0")] = 0;
        printf("Offset: %lu\n", offset1);

        fill_packet(buffer, PACKET_SIZE, &packets[curr_ind], str_len(buffer), offset1, 'F', 'D');
        offset1 += str_len(buffer);

        ++curr_ind;
    }

    packets[--curr_ind].last_pkt = 'T';
}

void printAddress(struct sockaddr_in *addr){
    printf("----------------------------------------\n");
    printf("Family: %d\n", addr->sin_family);
    printf("IP: %s\n", inet_ntoa(addr->sin_addr));
    printf("Port: %d\n", ntohs(addr->sin_port));
    printf("----------------------------------------\n");
}

int get_number_of_lines(char *filename){
    FILE *fptr;
    int count_lines = 0;
    char chr;

    fptr = fopen(filename, "r");
    
    chr = getc(fptr);
    while(chr != EOF){
        if(chr == '\n'){
            ++count_lines;
        }

        chr = getc(fptr);
    }

    fclose(fptr);
    return count_lines;
}

int main(int argc, char *argv[]){
    srand(time(0));
    signal(SIGALRM, handler);
    if(argc != 4){
        printf("Usage: %s <inputfile name> <Server IP(string)> <Server Port(Integer)>\n", argv[0]);
        exit(0);
    }

    int no_of_lines = get_number_of_lines(argv[1]);

    printf("Total no. of lines: %d\n", no_of_lines);

    int no_of_packets = no_of_lines;

    printf("No. of packets in file %s = %d\n", argv[1], no_of_packets);

    packets = (packet*)malloc(sizeof(packet)*no_of_packets);
    FILE *fp2 = fopen(argv[1], "r");

    //Packetization
    prepare_packet(packets, no_of_packets, fp2);

    print_packet(packets, no_of_packets);

    //Implement stop-and-wait ARQ

    if((client_sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        die_with_error("4. socket() in client");
    }

    bzero((void*)&servAddr, slen);
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[3]));
    servAddr.sin_addr.s_addr = inet_addr(argv[2]);

    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("Server address according to CLIENT\n");
    printAddress(&servAddr);

    // if(connect(client_sockfd, (struct sockaddr*)&servAddr, sizeof(struct sockaddr)) < 0){
    //     die_with_error("connect() failed");
    // }

    int state = 0;
    curr_ind = 0;
    packet recv_packet;

    while(curr_ind < no_of_packets){
        switch (state)
        {
        case 0:

            if(sendto(client_sockfd, (void*)&packets[curr_ind], sizeof(packet), 0, (struct sockaddr*) &servAddr, slen) == -1){
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
 
            if(recvfrom(client_sockfd, &recv_packet, sizeof(packet), 0, (struct sockaddr*)&servAddr, &slen) == -1){
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
