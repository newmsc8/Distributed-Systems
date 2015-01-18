#include <stdio.h>      // printf(), fprintf()
#include <sys/socket.h> // socket(), connect(), sendto(), recvfrom()
#include <arpa/inet.h>  // sockaddr_in, inet_addr()
#include <stdlib.h>     // atoi(), exit()
#include <string.h>     // memset()
#include <unistd.h>     // close()
#include <stdbool.h>	// bool
#include <sys/time.h>	// gettimeofday()

#define MSGMAX 255	// longest datagram client can/will send

int main(int argc, char *argv[]) {
	int sock;                     	// Socket
    	struct sockaddr_in serverAddr;	// Server address structure
    	struct sockaddr_in receiveAddr;	// Address message received from
    	unsigned short serverPort;	// Server port
    	unsigned int receiveSize;   	// Received address size for recvfrom()
    	char *servIP;            	// Server IP address
    	char *datagram;     		// String to send to server
    	char receivedMsg[MSGMAX+1];	// Receiving message
    	int datagramLen;   		// Length of datagram
    	int respStringLen;   		// Length of received response
	char *parserString;		// Copy of message to parse
	char *tok;			// Tokens of parsed message
	bool expectValue = false;	// Boolean indicating whether client expects value from server
	struct timeval tv;		// Timeval to set timeout and gettimeofday
	double time_in_mill;		// Holds time in milliseconds for print

	// Ensure correct number of parameters
    	if (argc != 4) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		printf("error");
        	exit(1);
    	}

    	servIP = argv[1];		// Set server IP address as the first argument
        serverPort = atoi(argv[2]);	// Set server port as the second argument
    	datagram = argv[3];       	// Set the datagram to send as the third argument

	// Ensure datagram isn't too long
    	if ((datagramLen = strlen(datagram)) > MSGMAX) {	
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		printf("datagram too long");
		exit(1);
	}

	// Parse datagram by commas
	parserString = malloc(strlen(datagram)+1);
	memcpy(parserString, datagram, strlen(datagram));
	tok = strtok(parserString,",");
	// If action is put, ensure two more tokens
	if(!strcmp(tok,"PUT")) {
		tok = strtok(NULL,",");
		if(tok == NULL) {
			gettimeofday(&tv, NULL);
			time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
			printf("NULL key provided");			
			exit(1);
		}	
		tok = strtok(NULL,",");
		if(tok == NULL) {
			gettimeofday(&tv, NULL);
			time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
			printf("NULL value provided");
			exit(1);
		}
		tok = strtok(NULL,",");
		if(tok != NULL) {
			gettimeofday(&tv, NULL);
			time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
			printf("too many values provided");
			exit(1);
		}
	} else {
		// If action is get, ensure one more token and set expectValue to true
		if(!strcmp(tok,"GET")) {
			tok = strtok(NULL,",");
			if(tok == NULL) {
				gettimeofday(&tv, NULL);
				time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
				printf("NULL key provided");			
				exit(1);
			}
			tok = strtok(NULL,",");
			if(tok != NULL) {
				gettimeofday(&tv, NULL);
				time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
				printf("too many values provided");
				exit(1);	
			}
			expectValue = true;
		} else {
			// If action is delete, ensure one more token
			if(!strcmp(tok,"DELETE")) {
				tok = strtok(NULL,",");
				if(tok == NULL) {
					gettimeofday(&tv, NULL);
					time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
					printf("NULL key provided");	
					exit(1);
				}
				tok = strtok(NULL,",");
				if(tok != NULL) {
					gettimeofday(&tv, NULL);
					time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
					printf("NULL too many values provided");
					exit(1);	
				}
			} else {
				// Unacceptable action
				gettimeofday(&tv, NULL);
				time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
				printf("Unacceptable action requested");
				exit(1);
			}
		}
	}

    	// Create datagram socket
    	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		printf("socket creation failed");
		exit(1);	
	}

	tv.tv_sec = 30;  // 30 Secs Timeout
	tv.tv_usec = 0;

	// Set socket timeout
	if((setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval))) < 0) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		printf("setting timeout failed");
		exit(1);
	}

    	// Create server address structure
    	memset(&serverAddr, 0, sizeof(serverAddr));	// Empty address structure
    	serverAddr.sin_family = AF_INET;		// Set address family
    	serverAddr.sin_addr.s_addr = inet_addr(servIP);	// Set server IP
    	serverAddr.sin_port   = htons(serverPort);	// Set server port

    	// Send datagram to the server
    	if (sendto(sock, datagram, datagramLen, 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != datagramLen) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		printf("sent an unexpected number of bytes");
		exit(1);
	}
  
    	// Receive response
    	receiveSize = sizeof(receiveAddr);
    	if ((respStringLen = recvfrom(sock, receivedMsg, MSGMAX, 0, (struct sockaddr *) &receiveAddr, &receiveSize)) != datagramLen) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		printf("error in recvfrom()");
		exit(1);
	}

    	// Null-terminate received message
    	receivedMsg[respStringLen] = '\0';
	gettimeofday(&tv, NULL);
	time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    	printf("Received: %s\n", receivedMsg);    // Print the received message

	if(expectValue) {
		respStringLen = recvfrom(sock, receivedMsg, MSGMAX, 0, (struct sockaddr *) &receiveAddr, &receiveSize);
		// Null-terminate received message
    		receivedMsg[respStringLen] = '\0';
    		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    		printf("Received: %s\n", receivedMsg);    // Print the received message
	}

	// Check that the response came from expected source
    	if (serverAddr.sin_addr.s_addr != receiveAddr.sin_addr.s_addr) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
        	fprintf(stderr,"Error: received a packet from unknown source.\n");
        	exit(1);
   	}
    
    	close(sock);
    	exit(0);
}
