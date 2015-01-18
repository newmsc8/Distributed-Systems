#include <stdio.h>      // printf(), fprintf()
#include <sys/socket.h> // socket(), bind()
#include <arpa/inet.h>  // sockaddr_in, inet_ntoa()
#include <stdlib.h>     // atoi(), exit()
#include <string.h>     // memset()
#include <unistd.h>     // close()
#include <sys/time.h>	// gettimeofday()

#define MSGMAX 255     // Longest datagram can/will receive

int main(int argc, char *argv[]) {
	int sock;                        	// Socket
	struct sockaddr_in serverAddr; 		// Local address 
    	struct sockaddr_in clientAddr;		// Client address 
    	unsigned int clientAddrLen;		// Length of incoming message
    	char message[MSGMAX];			// Incoming message
    	unsigned short serverPort;		// Server port
    	int msgSize;				// Size of received message
	char *parserString;			// Copy of received message to parse
	char *tok;				// Tokens of parsed message
	char *keyValue[256];			// Key-Value Store
	struct timeval tv;			// Timeval for gettimeofday
	double time_in_mill;			// Holds time in milliseconds for print

	// Ensure correct number of parameters
    	if (argc != 2) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
        	fprintf(stderr,"Usage:  %s <UDP SERVER PORT>\n", argv[0]);
        	exit(1);
    	}

	// Set server port as the first argument
    	serverPort = atoi(argv[1]);

    	// Create datagram socket
    	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		printf("error creating socket");
	}
	

    	// Create local address structure
    	memset(&serverAddr, 0, sizeof(serverAddr));   	// Empty address structure
    	serverAddr.sin_family = AF_INET;           	// Set address family
    	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);	// Any incoming interface
    	serverAddr.sin_port = htons(serverPort);	// Set local port

    	// Bind the socket to the local address
    	if (bind(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
		printf("error binding the socket to the local address structure");		
	}
  
	// Run for loop forever
    	for (;;) {
        	// Set the length of incoming messages
        	clientAddrLen = sizeof(clientAddr);

        	// Eait to receive a message from a client
        	if ((msgSize = recvfrom(sock, message, MSGMAX, 0, (struct sockaddr *) &clientAddr, &clientAddrLen)) < 0) {
			gettimeofday(&tv, NULL);
			time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
			printf("recvfrom() failed");			
		}

		// Print address of client
		gettimeofday(&tv, NULL);
		time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
        	printf("Handling client %s\n", inet_ntoa(clientAddr.sin_addr));

        	// Print error if wrong size message sent, otherwise take appropriate acction
        	if (sendto(sock, message, msgSize, 0, (struct sockaddr *) &clientAddr, sizeof(clientAddr)) != msgSize) {
			gettimeofday(&tv, NULL);
			time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
			printf("wrong number of bytes sent");
		} else {
			parserString = message;
			tok = strtok(parserString,",");
			// GET action
			if(!strcmp(tok,"GET")) {
				tok = strtok(NULL, ",");
				// Ensure key is a valid key
				if(atoi(tok) < 0 || atoi(tok) > 255) {
					gettimeofday(&tv, NULL);
					time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
					printf("Invalid key provided");
				} else {
					// Send value at key location
					sendto(sock, keyValue[atoi(tok)], strlen(keyValue[atoi(tok)]), 0, (struct sockaddr *) &clientAddr, sizeof(clientAddr));
				}
			}
			// PUT action
			if(!strcmp(tok,"PUT")) {
				tok = strtok(NULL, ",");
				// Ensure key is a valid key
				if(atoi(tok) < 0 || atoi(tok) > 255) {	
					gettimeofday(&tv, NULL);
					time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
					printf("Invalid key provided");
				} else {
					// Set key location to provided value
					char* value = strtok(NULL,",");
					keyValue[atoi(tok)] = value;
				}
			}
			// DELETE action
			if(!strcmp(tok,"DELETE")) {
				tok = strtok(NULL,",");
				// Ensure key is a valid key
				if(atoi(tok) < 0 || atoi(tok) > 255) {	
					gettimeofday(&tv, NULL);
					time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
					printf("Invalid key provided");
				} else {
					// Set key location to empty
					keyValue[atoi(tok)] = "";
				}
			}
		}
	}
	// Never reach this point
}
