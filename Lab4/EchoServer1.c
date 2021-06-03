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
    struct sockaddr_in echoServAddr; 
    struct sockaddr_in echoClntAddr; 
    unsigned short echoServPort;     
    unsigned int clntLen;            

    if(argc != 2){
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  

    if((welcomeSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        DieWithError("socket() failed");

    memset(&echoServAddr, 0, sizeof(echoServAddr));   

    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    echoServAddr.sin_port = htons(echoServPort);      

    printf("Server address according to SERVER\n");
    printAddress(&echoServAddr);

    if (bind(welcomeSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");


    if(listen(welcomeSock, MAXPENDING) < 0)
        DieWithError("listen() failed");

    for(;;){
        fprintf(stdout, "LISTENING\n");
        fprintf(stdout, "At port: %d ;; IP: %s\n", echoServPort, inet_ntoa(echoServAddr.sin_addr));
        clntLen = sizeof(echoClntAddr);

        if((acceptSock = accept(welcomeSock, (struct sockaddr *)&echoClntAddr, &clntLen)) < 0){ //accept is a blocking call
            DieWithError("accept() failed");
        }
        
        //Without converting the port number to host format, arbitrary and wrong port number will be shown as output.
        
        printf("client address according to CLIENT\n");
        printAddress(&echoClntAddr);

        printf("Handling client IP: %s ;; port: %d\n", inet_ntoa(echoClntAddr.sin_addr), ntohs(echoClntAddr.sin_port));

        HandleTCPClient(acceptSock);
    }

}