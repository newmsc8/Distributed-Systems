#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <unistd.h>     /* for close() */
#include "kvm.h"
#include <time.h>
#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define RCVBUFSIZE 32   /* Size of receive buffer */
#define SOCKET_ERR -8
#define BIND_ERR -7
#define LISTEN_ERR -6
#define CLIENT_ERR -5
#define SERVER_ERR -1
#define PACKAGING_ERR -10
void log_request(char* message,char* client){
    FILE * fp;
    fp = fopen ("tcp.log", "a");
    time_t current_time;
    char* c_time_string;
    /* Obtain current time as seconds elapsed since the Epoch. */
    current_time = time(NULL);
    /* Convert to local time format. */
    c_time_string = ctime(&current_time);
    fprintf(fp, "%s:%s %s\n",c_time_string,client,message);
    fclose(fp);
}
void HandleError(char *errorMessage, int err){
    log_request(errorMessage,"");
    exit(0);
}
void HandleTCPClient(int clntSocket, struct sockaddr_in clntAddr, int clntLen){
    char log_msg[128] = "Invalid Request";
    char buffer[RCVBUFSIZE];
    for (int i = 0; i < RCVBUFSIZE; ++i)
    {
        buffer[i] = '\0';
    }
    recv(clntSocket, buffer, RCVBUFSIZE - 1, 0);
    char* bufferArray[3];
    bufferArray[0] = malloc(10);
    bufferArray[1] = malloc(10);
    bufferArray[2] = malloc(10);
    int package_result =  parse_package(buffer, bufferArray);
    char resmsg[128] = "ack";
    if(package_result == PUT){
        int v = put(bufferArray[1],bufferArray[2]);
        if(v == 0){
            strcpy(log_msg,  bufferArray[1]);
            strcat(log_msg," Key Insert Request");
            strcpy(resmsg,"Key Inserted");
        }else{
            strcpy(log_msg,"Error inserting key value pair");
            strcpy(resmsg,"Err:Key Value pair could not be inserted");
        }
    }else if(package_result == GET){
        int v = get(bufferArray[1],resmsg);
        if(v >= 0){
            strcpy(log_msg,  bufferArray[1]);
            strcat(log_msg," Key Request");
        }else{
            strcpy(log_msg,  bufferArray[1]);
            strcat(log_msg,  " is not a valid key");
            strcpy(resmsg,"Err: Could no locate key value pair");
            
        }
    }else if(package_result == DELETE){
        int v = delete_key(bufferArray[1]);
        strcpy(log_msg,  bufferArray[1]);
        if(v == 0){
            strcpy(log_msg,  bufferArray[1]);
            strcat(log_msg," Key Delete Request");
        }else{
            strcpy(log_msg, "Error deleting request");
            strcpy(resmsg,"Err:Could not delete key value pair");
            
        }
    }else{
        strcpy(resmsg,"Invalid Packet");
    }
    send(clntSocket,resmsg,5,0);
    log_request(log_msg,inet_ntoa(clntAddr.sin_addr));
    printf("%s\n", log_msg);
}

int run(int argc, char *argv[])
{
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned short echoServPort;     /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */
    
    if (argc != 2)     /* Test for correct number of arguments */
    {
        HandleError("Invalid Argument", SERVER_ERR);
    }
    
    echoServPort = atoi(argv[1]);  /* First arg:  local port */
    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        HandleError("socket() failed", SOCKET_ERR);
    
    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */
    
    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        HandleError("bind() failed",BIND_ERR);
    
    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        HandleError("listen() failed",LISTEN_ERR);
    
    while (1){
        clntLen = sizeof(echoClntAddr);
        clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen);
        if(clntSock < 0){
            HandleError("cleint connnection failed",CLIENT_ERR);
        }
        HandleTCPClient(clntSock,echoClntAddr,clntLen);
    }
    return 0;
}
int main(int arg, char* args[]){
    init_key_values();
    //char *argv[2] = {"","1024"};
    run(arg,args);
    
}