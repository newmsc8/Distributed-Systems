#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include "kvm.m"
#define PACKETSIZE 128

void handleErr(char *errorMessage){
    printf("%s\n",errorMessage );
    exit(1);}
int validateServer(char *server, char* ip){
    struct hostent *he;
    struct in_addr **addr_list;
    
    if ( (he = gethostbyname(server) ) == NULL)
    {
        handleErr("Invalid Server");
        return 0;
    }
    
    addr_list = (struct in_addr **) he->h_addr_list;
    
    if( addr_list[0] != NULL)
    {
        strcpy(ip , inet_ntoa(*addr_list[0]) );
        return 1;
    }
    
    return 0;
}
long send_packet(char* ip, unsigned short port, char* packet,char* response){
    int sock;
    struct sockaddr_in servAddr;
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        handleErr("Socket Failed");
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(ip);
    servAddr.sin_port        = htons(port);
    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        handleErr("Connection Failed");
    long packetSize = strlen(packet);
    send(sock,packet,packetSize,0);
    char buffer[PACKETSIZE];
    long bytesRcvd = recv(sock, buffer, PACKETSIZE - 1, 0);
    close(sock);
    strcpy(response,buffer);
    return bytesRcvd;
}
int main(int arg, char* args[]){
    int port = atoi(args[2]);
    char* server = args[1];
    char ip[100];
    int val_srv = validateServer(server,ip);
    if (val_srv == 1) {
        char response[PACKETSIZE];
        send_packet(ip,port,args[3],response);
        printf("Server(%s): %s\n", ip,response);
    }
}