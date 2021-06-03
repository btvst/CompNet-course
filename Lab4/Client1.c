#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     

#define RCVBUFSIZE 32   

void DieWithError(char *errorMessage){
    perror(errorMessage);
    exit(1);
} 

void printAddress(struct sockaddr_in *addr){
    printf("--------------------------------------\n");
    printf("Addr family: %d\n", addr->sin_family);
    printf("Addr IP: %s\n", inet_ntoa(addr->sin_addr));
    printf("Addr Port: %d\n", ntohs(addr->sin_port));
    printf("--------------------------------------\n");
}

int main(int argc, char *argv[]){
    int sock;                        
    struct sockaddr_in echoServAddr; 
    unsigned short echoServPort;     
    char *servIP;                    
    char *echoString = (char*)malloc(sizeof(char)*100);                
    char echoBuffer[RCVBUFSIZE];     
    unsigned int echoStringLen;      
    int bytesRcvd, totalBytesRcvd;   

    if((argc < 2) || (argc > 3)){
        fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];             

    if (argc == 3)
        echoServPort = atoi(argv[2]); 
    else
        echoServPort = 7;

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        DieWithError("socket() failed");

    memset(&echoServAddr, 0, sizeof(echoServAddr));     

    echoServAddr.sin_family      = AF_INET;                     
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   
    echoServAddr.sin_port        = htons(echoServPort); 

    // fprintf(stdout, "Client port: %d ;; IP: %s\n", )

    printf("server address according to CLIENT\n");
    printAddress(&echoServAddr);

    if(connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() failed");

    for(int i=0; i<5; ++i){
        scanf("%[^\n]",echoString);
        getchar();
        echoStringLen = strlen(echoString);   

        if (send(sock, echoString, echoStringLen, 0) != echoStringLen){
            DieWithError("send() sent a different number of bytes than expected");
        }

        totalBytesRcvd = 0;

        printf("Received: ");                
        while(totalBytesRcvd < echoStringLen){ //As we might receive less amount of data id RCVMSGLEN > RCVBUFSIZE, hence continue receiving until the total lenght of the RECVMSGLEN does not exceed or become equal to the SNDMSGLEN
            if((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) < 0){ //Do not wan't to receive the NULL character at the end
                DieWithError("recv() failed or connection closed prematurely");
            }
            totalBytesRcvd += bytesRcvd;   
            echoBuffer[bytesRcvd] = '\0';  
            printf("%s", echoBuffer);
            fflush(stdout);     
        }

        printf("\n");
    }

    close(sock);
    exit(0);
}