/* instructions */
/* 
* PACKET_SIZE variable can be changes by changing the preprocessor directive #define PACKET_SIZE

* IMPORTANT - For the program to work correctly, the receiver program must be run before the sender program.

* If only want to receive a file(no sending of file), put <input filename> in cmd as 'NULL'. This will be true for relay and receiver instance.

* If only want to send a file(no receiving of file), put <output filename> in cmd as 'NULL', This is true for sender instance.

* Usage in CMD: ./a.out <input filename> <Peer1(current)Port to bind> <Peer2 IP(string)> <Peer2 Port(Integer)> <output filename>

* Running instruction (single sender --> receiver)
    * Run the final receiver program instance first.
    * Then run the relay program instance
    * Finally, run the sender program instance
    
* Running instruction (multiple sender --> receiver)
    * Run any receiver program instance first.
    * Then run other receivers, in any order
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
#include <sys/wait.h>

#define PACKET_SIZE 512 //Denotes maximum limit on sizeof a packet dataload
#define TIMEOUT 2
#define READY_TIME 40

typedef struct Packet{
    size_t size; //no. of bytes in the data of packet, for all except maybe the last
    unsigned int seq_no; //byte offset from the start
    char data[PACKET_SIZE];
    char last_pkt; //'T' is last packet, else 'F'
    char TYPE; //'D' if data packet, 'A' is ACK packet

    int src_pid;
    int dst_pid;
}packet;

//sender variables
int peer1_binding_socket_to_receive, i, ran, recv_flag, peer1_sending_socket;
struct sockaddr_in peer2_addr, peer1_addr, sender_addr;
socklen_t slen = sizeof(struct sockaddr_in);
int curr_ind = 0;
packet *packets;

//receiver variables
int recv_len;
char buffer[100000];
int curr_offset = 0;
int nodeid;

void die_with_error(char *s){
    perror(s);
    exit(0);
}

void write_in_buffer(packet *pack){
    for(int i=0; i<pack->size; ++i){
        buffer[i+curr_offset] = pack->data[i];
    }

    curr_offset += pack->size;
}

void handler(int signo){
    if(recv_flag != 1){
        printf("NODE ID: %d---RESENT DATA: Seq. no. = %d Size = %lu\n", nodeid, packets[curr_ind-1].seq_no, packets[curr_ind-1].size);
        if(sendto(peer1_sending_socket, &packets[curr_ind-1], sizeof(packet), 0, (struct sockaddr*)&peer2_addr, slen) == -1){
            die_with_error("Sendto() in signal handler\n");
        }
        sleep(1);
    }
}

void fill_packet(char *buffer, size_t bufsize, packet *pack, size_t size, unsigned int seq_no, char last_pkt, char TYPE, int src_pid, int dst_pid){
    bzero(pack->data, PACKET_SIZE);

    for(int i=0; i<bufsize; ++i){
        (pack->data)[i] = buffer[i];
    }
    pack->size = size;
    pack->seq_no = seq_no;
    pack->last_pkt = last_pkt;
    pack->TYPE = TYPE;
    pack->src_pid = src_pid;
    pack->dst_pid = dst_pid;
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
        printf("Src PID: %d\n", packets[i].src_pid);
        printf("Dst PID: %d\n", packets[i].dst_pid);
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

void prepare_packet(packet* packets, int no_of_packets, int src_pid, int dst_pid, FILE *fp2){
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

        fill_packet(buffer, PACKET_SIZE, &packets[curr_ind], str_len(buffer), offset1, 'F', 'D', src_pid, dst_pid);
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
    printf("If only want to receive (no send), put <input filename> in cmd as 'NULL'\n");
    printf("If only want to send (no receive), put <output filename> in cmd as 'NULL'\n");

    srand(time(0));
    signal(SIGALRM, handler);

    if(argc != 6){
        printf("Usage: %s <input filename> <Peer1(current)Port to bind> <Peer2 IP(string)> <Peer2 Port(Integer)> <output filename>\n", argv[0]);
        exit(0);
    }
    nodeid = atoi(argv[2]);

    printf("Peer2 IP and Port via command line will be used to send file\n");

    pid_t pid;
    int status;
    char *nl = "NULL";

    if((peer1_binding_socket_to_receive = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        die_with_error("4. socket() in client");
    }

    bzero((void*)&peer1_addr, slen);

    peer1_addr.sin_family = AF_INET;
    peer1_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    peer1_addr.sin_port = htons(atoi(argv[2]));

    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("Peer1 address according to PEER1\n");
    printAddress(&peer1_addr);

    if(bind(peer1_binding_socket_to_receive, (struct sockaddr*)&peer1_addr, slen) == -1){
        die_with_error("bind() error in PEER1");
    }

    printf("Bind complete.\n");

    bzero((void*)&peer2_addr, slen);

    peer2_addr.sin_family = AF_INET;
    peer2_addr.sin_addr.s_addr = inet_addr(argv[3]);
    peer2_addr.sin_port = htons(atoi(argv[4]));

    printf("-----------------------------------------------------------------------------------------------------\n");
    printf("receiver address according to PEER1(sender)\n");
    printAddress(&peer2_addr);

    if((pid = fork()) == 0){
        die_with_error("fork() error\n");
    }
    else if(pid==0){ //sending file/ack code
        if(strcmp(argv[1], nl)!= 0){ //sender of file code, sender and relay are mutually exclusive
            int dst_port;
            printf("Enter destination port PID: ");
            scanf("%d", &dst_port);

            if((peer1_sending_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
                die_with_error("socket in PEER1");
            }

            int no_of_lines = get_number_of_lines(argv[1]);

            printf("Total no. of lines: %d\n", no_of_lines);

            int no_of_packets = no_of_lines;

            printf("No. of packets in file %s = %d\n", argv[1], no_of_packets);

            packets = (packet*)malloc(sizeof(packet)*no_of_packets);
            FILE *fp2 = fopen(argv[1], "r");

            //Packetization
            prepare_packet(packets, no_of_packets, atoi(argv[2]), dst_port, fp2);

            print_packet(packets, no_of_packets);

            int state = 0;
            curr_ind = 0;
            packet recv_packet;

            while(curr_ind < no_of_packets){
                switch (state)
                {
                case 0:
                    if(sendto(peer1_sending_socket, (void*)&packets[curr_ind], sizeof(packet), 0, (struct sockaddr*) &peer2_addr, slen) == -1){
                        die_with_error("5. sendto() in PEER1");
                    }
                    else{
                        printf("NODE ID: %d---SENT DATA: Seq no. = %d, Size = %lu bytes\n", atoi(argv[2]), packets[curr_ind].seq_no, packets[curr_ind].size);
                        sleep(1); //send a single packet once
                    }
                
                    state = 1;
                    recv_flag = 0;
                    ++curr_ind;
                    alarm(TIMEOUT);

                    break;
                case 1:
                    if(recvfrom(peer1_binding_socket_to_receive, &recv_packet, sizeof(packet), 0, (struct sockaddr*)&sender_addr, &slen) == -1){
                        die_with_error("6. recvfrom() in PEER1");
                    }

                    if(recv_packet.seq_no == packets[curr_ind].seq_no && recv_packet.TYPE =='A'){
                        printf("NODE ID: %d----RCVD ACK: Seq. no. = %d\n", atoi(argv[2]), recv_packet.seq_no);
                        state = 0;
                        recv_flag = 1;
                    }
                    else{
                        printf("Garbage received at file receiver\n");
                    }
                    break;
                default:
                    break;
                }
            }

            close(peer1_sending_socket);
            return 0;
        }
        else{ //true for relay node where input.txt is NULL
            if((peer1_sending_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
                die_with_error("socket in PEER1");
            }

            int packet_drop_counter = 1;
            int state = 0;
            packet recv_packet;
            packet send_packet;

            while(1){ //Relay till the end of time
                if((recv_len = recvfrom(peer1_binding_socket_to_receive, &recv_packet, sizeof(packet), 0, (struct sockaddr*)&peer2_addr, &slen)) == -1){
                    die_with_error("recvfrom in server");
                }

                if(recv_packet.TYPE == 'A'){ //cannot drop ACK
                    if(sendto(peer1_sending_socket, (void*)&recv_packet, sizeof(packet), 0, (struct sockaddr*)&peer2_addr, slen) <= 0){
                        die_with_error("10. sendto error for ACK\n");
                    }
                    printf("NODE ID: %d---RELAY ACK for packet with Seq. no.: %u\n", atoi(argv[2]), recv_packet.seq_no);
                }
                else if(recv_packet.TYPE == 'D'){
                    ++packet_drop_counter;
                    if(packet_drop_counter == 10){
                        packet_drop_counter = 1;
                        printf("NODE ID: %d--DROP DATA:Seq. no.: %u--Size: %lu\n", atoi(argv[2]), recv_packet.seq_no, recv_packet.size);
                    }

                    else{
                        if(sendto(peer1_sending_socket, (void*)&recv_packet, sizeof(packet), 0, (struct sockaddr*)&peer2_addr, slen) <= 0){
                            die_with_error("11. sendto error for data");
                        }
                        printf("NODE ID: %d--RELAY DATA: Seq. no.: %u--Size: %lu\n", atoi(argv[2]), recv_packet.seq_no, recv_packet.size);
                    }
                }
            }
        }
    }
    else{ //file receiver
        if(strcmp(argv[5], nl)!=0){
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
                        if((recv_len = recvfrom(peer1_binding_socket_to_receive, &recv_packet, sizeof(packet), 0, (struct sockaddr*)&peer2_addr, &slen)) == -1){
                            die_with_error("recvfrom in server");
                        }
                        printf("NODE ID: %d---DROP DATA: Seq. No. %d\n", atoi(argv[2]), curr_offset);
                    }
                    else{
                        if((recv_len = recvfrom(peer1_binding_socket_to_receive, &recv_packet, sizeof(packet), 0, (struct sockaddr*)&peer2_addr, &slen)) == -1){
                            die_with_error("recv in SERVER");
                        }

                        // printf("RCVD message: ");
                        // for(int i=0; i<recv_packet.size; ++i){
                        //     printf("%c", recv_packet.data[i]);
                        // }
                        // printf("\n");
                        // printf("Current file offset: %d ;; Sequence_no: %d\n", curr_offset, recv_packet.seq_no);
                        printf("NODE ID: %d---RCVD DATA: Seq. No. = %d, Size = %lu Bytes\n", atoi(argv[2]), curr_offset, recv_packet.size);

                        write_in_buffer(&recv_packet);

                        for(int i=0; i<PACKET_SIZE; ++i){
                            recv_packet.data[i] = '\0';
                        }

                        char message[20] = "SERVER ACK";

                        fill_packet(message, 20, &send_packet, sizeof(packet), curr_offset, 'F', 'A', atoi(argv[2]), atoi(argv[4]));

                        if(sendto(peer1_sending_socket, &send_packet, sizeof(packet), 0, (struct sockaddr*)&peer2_addr, slen) <= 0){
                            die_with_error("send() in server");
                        }
                        else{
                            printf("NODE ID: %d---SEND ACK: Seq. no. = %d\n",atoi(argv[2]), recv_packet.seq_no);
                        }

                        if(recv_packet.last_pkt=='T'){
                            FILE *fp = fopen(argv[5], "w");
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
        waitpid(pid, &status, 0);
    }
}
