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
	bool expectValue;		// Boolean indicating whether client expects value from server
	struct timeval tv;		// Timeval to set timeout and gettimeofday
	double time_in_mill;		// Holds time in milliseconds for print

	// Ensure correct number of parameters
    	if (argc != 4) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    		printf("%f",time_in_mill);
		printf("error");
        	exit(1);
    	}

    	servIP = argv[1];		// Set server IP address as the first argument
        serverPort = atoi(argv[2]);	// Set server port as the second argument
    	datagram = argv[3];       	// Set the datagram to send as the third argument

    	if ((datagramLen = strlen(datagram)) > MSGMAX) { /* Check input length */
		//print error that datagram is too long
	}

	parserString = malloc(strlen(datagram)+1);
	memcpy(parserString, datagram, strlen(datagram));
	tok = strtok(parserString,",");
	if(!strcmp(tok,"PUT")) {
		printf("PUT STATEMENT");
		tok = strtok(NULL,",");
		if(tok == NULL) {
			//NULL key			
			exit(1);
		}	
		tok = strtok(NULL,",");
		if(tok == NULL) {
			//NULL value
		}
		tok = strtok(NULL,",");
		if(tok != NULL) {
			//too many values
		}
	} else {
		if(!strcmp(tok,"GET")) {
			printf("GET STATEMEMNT");
			tok = strtok(NULL,",");
			if(tok == NULL) {
				//NULL key			
				exit(1);
			}
			tok = strtok(NULL,",");
			if(tok != NULL) {
				//too many values
			}
			expectValue = true;
		} else {
			if(!strcmp(tok,"DELETE")) {
				printf("DELETE STATEMENT");
				tok = strtok(NULL,",");
				if(tok == NULL) {
					//NULL key			
					exit(1);
				}
				tok = strtok(NULL,",");
				if(tok != NULL) {
					//too many values
				}
			} else {
				printf("NOPE!");
				exit(1);
			}
		}
	}

    	/* Create a datagram/UDP socket */
    	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		//print error that socket creation failed
	}
	tv.tv_sec = 30;  /* 30 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors

	if((setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval))) < 0) {
		//setting timeout failed
	}

    	/* Construct the server address structure */
    	memset(&serverAddr, 0, sizeof(serverAddr));    /* Zero out structure */
    	serverAddr.sin_family = AF_INET;                 /* Internet addr family */
    	serverAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
    	serverAddr.sin_port   = htons(serverPort);     /* Server port */

    	/* Send the string to the server */
    	if (sendto(sock, datagram, datagramLen, 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) != datagranLen) {
		//print error that sendto sent an unexpected number of bytes
	}
  
    	/* Recv a response */
    	fromSize = sizeof(receiveAddr);
    	if ((respStringLen = recvfrom(sock, receivedMsg, MSGMAX, 0, (struct sockaddr *) &receiveAddr, &fromSize)) != datagramLen) {
		//print error that recvfrom failed
	}

    	/* null-terminate the received data */
    	receivedMsg[respStringLen] = '\0';
	gettimeofday(&tv, NULL);
	time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    	printf("%f",time_in_mill);
    	printf("Received: %s\n", receivedMsg);    /* Print the echoed arg */

	if(expectValue) {
		respStringLen = recvfrom(sock, receivedMsg, MSGMAX, 0, (struct sockaddr *) &receiveAddr, &fromSize);
		/* null-terminate the received data */
    		receivedMsg[respStringLen] = '\0';
    		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    		printf("%f",time_in_mill);
    		printf("Received: %s\n", receivedMsg);    /* Print the echoed arg */
	}

    	if (serverAddr.sin_addr.s_addr != receiveAddr.sin_addr.s_addr) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    		printf("%f",time_in_mill);
        	fprintf(stderr,"Error: received a packet from unknown source.\n");
        	exit(1);
   	}
    
    	close(sock);
    	exit(0);
}
