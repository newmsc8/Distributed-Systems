#include <stdio.h>      // printf(), fprintf()
#include <sys/socket.h> // socket(), connect(), sendto(), recvfrom()
#include <arpa/inet.h>  // sockaddr_in, inet_addr()
#include <stdlib.h>     // atoi(), exit()
#include <string.h>     // memset()
#include <unistd.h>     // close()
#include <stdbool.h>	// bool
#include <sys/time.h>	// gettimeofday()

#define MSGMAX 255		// longest datagram client can/will send
bool expectValue = false;	// Boolean indicating whether client expects value from server

bool validDatagram(char *datagram) {
	struct timeval tv;
	double time_in_mill;
	char *parserString;		// Copy of message to parse
	char *tok;			// Tokens of parsed message

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
			fprintf(stderr,"%f: NULL key provided,",time_in_mill);	
			return false;		
		}	
		tok = strtok(NULL,",");
		if(tok == NULL) {
			gettimeofday(&tv, NULL);
			time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
			fprintf(stderr,"%f: NULL value provided.",time_in_mill);
			return false;
		}
		tok = strtok(NULL,",");
		if(tok != NULL) {
			gettimeofday(&tv, NULL);
			time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
			fprintf(stderr,"%f: Too many values provided.",time_in_mill);
			return false;
		}
	} else {
		// If action is get, ensure one more token and set expectValue to true
		if(!strcmp(tok,"GET")) {
			tok = strtok(NULL,",");
			if(tok == NULL) {
				gettimeofday(&tv, NULL);
				time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
				fprintf(stderr,"%f: NULL key provided.",time_in_mill);	
				return false;		
			}
			tok = strtok(NULL,",");
			if(tok != NULL) {
				gettimeofday(&tv, NULL);	
				time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
				fprintf(stderr,"%f: Too many values provided.",time_in_mill);
				return false;
			}
			expectValue = true;
		} else {
			// If action is delete, ensure one more token
			if(!strcmp(tok,"DELETE")) {
				tok = strtok(NULL,",");
				if(tok == NULL) {
					gettimeofday(&tv, NULL);
					time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
					fprintf(stderr,"%f: NULL key provided.",time_in_mill);
					return false;	
				}
				tok = strtok(NULL,",");
				if(tok != NULL) {
					gettimeofday(&tv, NULL);
					time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
					fprintf(stderr,"%f: NULL too many values provided.",time_in_mill);
					return false;
				}
			} else {
				// Unacceptable action
				gettimeofday(&tv, NULL);
				time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
				fprintf(stderr,"%f: Unacceptable action requested.",time_in_mill);
				return false;
			}
		}
	}
	return true;
}

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
	struct timeval tv;		// Timeval to set timeout and gettimeofday
	double time_in_mill;		// Holds time in milliseconds for print

	// Ensure correct number of parameters
    	if (argc != 4) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		fprintf(stderr,"%f: Wrong number of parameters provided.",time_in_mill);
        	exit(1);
    	}

	fprintf(stderr,"test");

    	servIP = argv[1];		// Set server IP address as the first argument
        serverPort = atoi(argv[2]);	// Set server port as the second argument
    	datagram = argv[3];       	// Set the datagram to send as the third argument

	// Ensure datagram isn't too long
    	if ((datagramLen = strlen(datagram)) > MSGMAX) {	
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		fprintf(stderr,"%f: Datagram too long,",time_in_mill);
		exit(1);
	}
	
	if(!validDatagram(datagram)) {
		exit(1);
	}

	

    	// Create datagram socket
    	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		fprintf(stderr,"%f: Socket creation failed.",time_in_mill);
		exit(1);	
	}

	tv.tv_sec = 30;	// 30 Secs Timeout
	tv.tv_usec = 0;

	// Set socket timeout
	if((setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval))) < 0) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		fprintf(stderr,"%f: Setting timeout failed.",time_in_mill);
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
		fprintf(stderr,"%f: Sent an unexpected number of bytes.",time_in_mill);
		exit(1);
	}
  
    	// Receive response
    	receiveSize = sizeof(receiveAddr);
    	if ((respStringLen = recvfrom(sock, receivedMsg, MSGMAX, 0, (struct sockaddr *) &receiveAddr, &receiveSize)) != datagramLen) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		fprintf(stderr,"%f: Error in recvfrom().",time_in_mill);
		exit(1);
	}

    	// Null-terminate received message
    	receivedMsg[respStringLen] = '\0';
	gettimeofday(&tv, NULL);
	time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    	fprintf(stdout,"%f: Received: %s.\n",time_in_mill, receivedMsg);	// Print the received message

	if(expectValue) {
		respStringLen = recvfrom(sock, receivedMsg, MSGMAX, 0, (struct sockaddr *) &receiveAddr, &receiveSize);
		// Null-terminate received message
    		receivedMsg[respStringLen] = '\0';
    		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    		fprintf(stdout,"%f: Received: %s.\n",time_in_mill, receivedMsg);	// Print the received message
	}

	// Check that the response came from expected source
    	if (serverAddr.sin_addr.s_addr != receiveAddr.sin_addr.s_addr) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
        	fprintf(stderr,"%f: Error: received a packet from unknown source.\n",time_in_mill);
        	exit(1);
   	}
    
    	close(sock);
    	exit(0);
}
