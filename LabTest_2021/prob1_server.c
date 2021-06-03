/* Usage: ./a.out <outputfile name> <Server Port(Integer)> */

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

#define PACKET_SIZE 512
#define TIMEOUT 2

typedef struct Packet{
    size_t size; //no. of bytes in the data of packet, 100 for all except maybe the last
    unsigned int seq_no; //byte offset from the start
    char data[PACKET_SIZE];
    char last_pkt; //'T' is last packet, else 'F'
    char TYPE; //'D' if data packet, 'A' is ACK packet
}packet;

char buffer[100000];
int curr_offset = 0;

void die_with_error(char *s){
    perror(s);
    exit(0);
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

void printAddress(struct sockaddr_in *addr){
    printf("----------------------------------------\n");
    printf("Family: %d\n", addr->sin_family);
    printf("IP: %s\n", inet_ntoa(addr->sin_addr));
    printf("Port: %d\n", ntohs(addr->sin_port));
    printf("----------------------------------------\n");
}

void write_in_buffer(packet *pack){
    for(int i=0; i<pack->size; ++i){
        buffer[i+curr_offset] = pack->data[i];
    }

    curr_offset += pack->size;
}

void handle_client(int acceptsock, char *filename){
    int packet_drop_counter = 1;
    int state = 0;
    packet recv_packet;
    packet send_packet;

    int recv_len;

    while(1){
        switch (state)
        {
        case 0:
            printf("--------------------------------------------------------------------------\n");
            printf("Waiting for packet...\n");
            fflush(stdout);

            ++packet_drop_counter;
            if(packet_drop_counter == 10){
                packet_drop_counter = 1;
                recv_len = recv(acceptsock, &recv_packet, sizeof(packet), 0);
                printf("DROP PKT: Seq. No. %d\n", curr_offset);
            }
            else{
                if((recv_len = recv(acceptsock, &recv_packet, sizeof(packet), 0)) == -1){
                    die_with_error("recv in SERVER");
                }

                printf("RCVD message: ");
                for(int i=0; i<recv_packet.size; ++i){
                    printf("%c", recv_packet.data[i]);
                }
                printf("\n");
                printf("Current file offset: %d ;; Sequence_no: %d\n", curr_offset, recv_packet.seq_no);
                printf("RCVD PKT: Seq. No. = %d, Size = %lu Bytes\n", curr_offset, recv_packet.size);

                // printf("EEEE. Curr_offset: %d\n", curr_offset);
                // curr_offset += recv_packet.size;

                // printf("FFFF. Curr_offset: %d\n", curr_offset);

                //Write received packet contents in buffer
                write_in_buffer(&recv_packet);

                // printf("RRRR. Curr_offset: %d\n", curr_offset);

                //clear receive packet data buffer
                for(int i=0; i<PACKET_SIZE; ++i){
                    recv_packet.data[i] = '\0';
                }

                // printf("TTTT. Curr_offset: %d\n", curr_offset);

                char message[20] = "SERVER ACK";
                // printf("QQQQ. Curr_offset: %d\n", curr_offset);

                fill_packet(message, 20, &send_packet, sizeof(packet), curr_offset, 'F', 'A');

                // printf("WWWW. Curr_offset: %d\n", curr_offset);
                if(send(acceptsock, &send_packet, sizeof(packet), 0) <= 0){
                    die_with_error("send() in server");
                }
                else{
                    printf("Send ACK: Seq. no. = %d\n", recv_packet.seq_no);
                }

                printf("YYYY. Curr_offset: %d\n", curr_offset);

                if(recv_packet.last_pkt=='T'){
                    FILE *fp = fopen(filename, "w");
                    fprintf(fp, "%s", buffer);
                    goto label;
                }
            }
            break;
        
        default:
            break;
        }
    }

    label: 
        printf("Server exiting....\n");
        exit(1);
}

int main(int argc, char *argv[]){
    srand(time(0));

    if(argc != 3){
        printf("Usage: %s <outputfile name> <Server Port(Integer)>\n", argv[0]);
        exit(0);
    }

    struct sockaddr_in servAddr, clntAddr;
    int sockfd, i, recv_len, accept_sock;
    socklen_t slen = sizeof(struct sockaddr_in);

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))== -1){
        die_with_error("socket() in server");
    }

    bzero((void*)&servAddr, sizeof(struct sockaddr));
    bzero((void*)&clntAddr, sizeof(struct sockaddr));

    // printf("BBBB\n");

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(atoi(argv[2]));

    // printf("AAAA\n");

    printf("Server address according to SERVER\n");
    printAddress(&servAddr);

    if(bind(sockfd, (struct sockaddr*)&servAddr, slen) == -1){
        die_with_error("2. bind() error in server");
    }

    // if(listen(sockfd, 5) < 0){
    //     die_with_error("listen() failed\n");
    // }

    int packet_drop_counter = 1;
    int state = 0;
    packet recv_packet;
    packet send_packet;

    while(1){
        switch (state)
        {
        case 0:
            printf("--------------------------------------------------------------------------\n");
            printf("Waiting for packet...\n");
            fflush(stdout);

            ++packet_drop_counter;
            if(packet_drop_counter == 10){
                packet_drop_counter = 1;
                if((recv_len = recvfrom(sockfd, &recv_packet, sizeof(packet), 0, (struct sockaddr*)&clntAddr, &slen)) == -1){
                    die_with_error("recvfrom in server");
                }
                printf("DROP PKT: Seq. No. %d\n", curr_offset);
            }
            else{
                if((recv_len = recvfrom(sockfd, &recv_packet, sizeof(packet), 0, (struct sockaddr*)&clntAddr, &slen)) == -1){
                    die_with_error("recv in SERVER");
                }

                // printf("RCVD message: ");
                // for(int i=0; i<recv_packet.size; ++i){
                //     printf("%c", recv_packet.data[i]);
                // }
                // printf("\n");
                // printf("Current file offset: %d ;; Sequence_no: %d\n", curr_offset, recv_packet.seq_no);
                printf("RCVD PKT: Seq. No. = %d, Size = %lu Bytes\n", curr_offset, recv_packet.size);

                // printf("EEEE. Curr_offset: %d\n", curr_offset);
                // curr_offset += recv_packet.size;

                // printf("FFFF. Curr_offset: %d\n", curr_offset);

                //Write received packet contents in buffer
                write_in_buffer(&recv_packet);

                // printf("RRRR. Curr_offset: %d\n", curr_offset);

                //clear receive packet data buffer
                for(int i=0; i<PACKET_SIZE; ++i){
                    recv_packet.data[i] = '\0';
                }

                // printf("TTTT. Curr_offset: %d\n", curr_offset);

                char message[20] = "SERVER ACK";
                // printf("QQQQ. Curr_offset: %d\n", curr_offset);

                fill_packet(message, 20, &send_packet, sizeof(packet), curr_offset, 'F', 'A');

                // printf("WWWW. Curr_offset: %d\n", curr_offset);
                if(sendto(sockfd, &send_packet, sizeof(packet), 0, (struct sockaddr*)&clntAddr, slen) <= 0){
                    die_with_error("send() in server");
                }
                else{
                    printf("Send ACK: Seq. no. = %d\n", recv_packet.seq_no);
                }

                // printf("YYYY. Curr_offset: %d\n", curr_offset);

                if(recv_packet.last_pkt=='T'){
                    FILE *fp = fopen(argv[1], "w");
                    fprintf(fp, "%s", buffer);
                    goto label;
                }
            }
            break;
        
        default:
            break;
        }
    }

    label: 
        printf("Server exiting....\n");
        exit(1);
}
