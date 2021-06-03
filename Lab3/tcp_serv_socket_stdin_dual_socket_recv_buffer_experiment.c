#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RCVBUFSIZE 32
#define MAXPENDING 5
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

void handle_TCP_client(int accept_sock){
    pack p;
    int n;
    int recv_msg_size;
    
    for(;;){
        if((recv_msg_size = recv(accept_sock, (void*)&p, sizeof(pack), 0)) < 0){
            die_with_error("recv() failed in SERVER");
        }

        if(p.n != 0){
            printf("Received: %d\n", p.n);
            scanf("%d", &n);
        }
        else{
            printf("recv buffer empty. Exiting...\n");
            break;
        }

        p.n = 0;
    }
}

int main(int argc, char *argv[]){
    int welcome_sock, accept_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    if(argc != 2){
        printf("Usage: %s <Server Port>\n", argv[0]);
        exit(1);
    }

    if((welcome_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        die_with_error("socket() failed in SERVER");
    }

    bzero(&serv_addr, sizeof(struct sockaddr_in));

    // printf("BBBB\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    // printf("AAAA\n");

    if(bind(welcome_sock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0){
        die_with_error("bind() failed in SERVER");
    }

    if(listen(welcome_sock, MAXPENDING) < 0){
        die_with_error("listen() failed in SERVER");
    }

    printf("Server address according to SERVER\n");
    print_address(&serv_addr);

    for(;;){
        printf("LISTENING\n");
        if((accept_sock = accept(welcome_sock, (struct sockaddr *)&clnt_addr, &socket_len)) < 0){
            die_with_error("accept() failed in SERVER");
        }

        printf("Client address according to SERVER\n");
        print_address(&clnt_addr);

        handle_TCP_client(accept_sock);
    }

    close(accept_sock);
    close(welcome_sock);
}