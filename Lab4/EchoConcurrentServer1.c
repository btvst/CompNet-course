#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     

#define MAXPENDING 5

void DieWithError(char *errorMessage);  
#include <stdio.h>      
#include <sys/socket.h> 
#include <unistd.h>     

#define RCVBUFSIZE 32   

void printAddress(struct sockaddr_in *addr){
    printf("--------------------------------------\n");
    printf("Addr family: %d\n", addr->sin_family);
    printf("Addr IP: %s\n", inet_ntoa(addr->sin_addr));
    printf("Addr Port: %d\n", ntohs(addr->sin_port));
    printf("--------------------------------------\n");
}

void DieWithError(char *errorMessage){
    perror(errorMessage);
    exit(1);
}

void HandleTCPClient(int clntSocket){
    char echoBuffer[RCVBUFSIZE];        
    int recvMsgSize;                    

    printf("SERVER waiting to receive\n");
    if((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE-1, 0)) < 0){ //Perform RCVBUFSIZE-1 to prevent garbage data at the end
        DieWithError("recv() failed");
    }
    printf("SERVER received: %s\n", echoBuffer);

    while(recvMsgSize > 0){
        if(send(clntSocket, echoBuffer, recvMsgSize, 0) != recvMsgSize)
            DieWithError("send() failed");

        printf("SERVER waiting to receive\n");
        if((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE-1, 0)) < 0){
            DieWithError("recv() failed");
        }
        printf("SERVER received: %s\n", echoBuffer);
    }

    close(clntSocket);  
}

int main(int argc, char *argv[]){
    int welcomeSock;
    int acceptSock;
    struct sockaddr_in servAddr;
    struct sockaddr_in clntAddr;
    unsigned short servPort;
    socklen_t clntLen;
    pid_t pid;

    if(argc != 2){
        fprintf(stderr, "Usage %s <Server Port>\n", argv[0]);
        exit(1);
    }

    if((welcomeSock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        DieWithError("socket() creation failed in SERVER");
    }

    servPort = atoi(argv[1]);
    bzero((void*) &servAddr, sizeof(struct sockaddr_in));

    // memset(&servAddr, 0, sizeof(struct sockaddr_in));

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);

    printf("Server address according to SERVER\n");
    printAddress(&servAddr);

    if(bind(welcomeSock, (struct sockaddr *)&servAddr, sizeof(struct sockaddr)) < 0){
        DieWithError("bind() failed in SERVER");
    }

    if(listen(welcomeSock, MAXPENDING) < 0){
        DieWithError("listen() failed in SERVER");
    }

    for(;;){
        fprintf(stdout, "LISTENING\n");
        fprintf(stdout, "At port: %d ;; IP: %s\n", ntohs(servAddr.sin_port), inet_ntoa(servAddr.sin_addr));

        clntLen = sizeof(struct sockaddr);

        if((acceptSock = accept(welcomeSock, (struct sockaddr*)&clntAddr, &clntLen)) < 0){
            DieWithError("accept() server failed");
        }

        if((pid = fork()) == 0){
            close(welcomeSock);
            HandleTCPClient(acceptSock);
            close(acceptSock);
            exit(0);
        }
        else{
            close(acceptSock);
        }
    }
}