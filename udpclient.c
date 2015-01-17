#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <stdbool.h>

#define ECHOMAX 255     /* Longest string to echo */

int main(int argc, char *argv[]) {
	int sock;                        /* Socket descriptor */
    	struct sockaddr_in echoServAddr; /* Echo server address */
    	struct sockaddr_in fromAddr;     /* Source address of echo */
    	unsigned short echoServPort;     /* Echo server port */
    	unsigned int fromSize;           /* In-out of address size for recvfrom() */
    	char *servIP;                    /* IP address of server */
    	char *echoString;                /* String to send to echo server */
    	char echoBuffer[ECHOMAX+1];      /* Buffer for receiving echoed string */
    	int echoStringLen;               /* Length of string to echo */
    	int respStringLen;               /* Length of received response */
	char *parserString;
	char *tok;
	bool expectValue;
	

    	if (argc != 4) {   /* Test for correct number of arguments */
		printf("error");
        	exit(1);
    	}

    	servIP = argv[1];           /* First arg: server IP address (dotted quad) */
        echoServPort = atoi(argv[2]);  /* Use given port, if any */
    	echoString = argv[3];       /* Second arg: string to echo */

    	if ((echoStringLen = strlen(echoString)) > ECHOMAX) { /* Check input length */
		//print error that echostring is too long
	}

	parserString = malloc(strlen(echoString)+1);
	memcpy(parserString, echoString, strlen(echoString));
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

    	/* Construct the server address structure */
    	memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
    	echoServAddr.sin_family = AF_INET;                 /* Internet addr family */
    	echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
    	echoServAddr.sin_port   = htons(echoServPort);     /* Server port */

    	/* Send the string to the server */
    	if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) != echoStringLen) {
		//print error that sendto sent an unexpected number of bytes
	}
  
    	/* Recv a response */
    	fromSize = sizeof(fromAddr);
    	if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &fromAddr, &fromSize)) != echoStringLen) {
		//print error that recvfrom failed
	}

    	/* null-terminate the received data */
    	echoBuffer[respStringLen] = '\0';
    	printf("Received: %s\n", echoBuffer);    /* Print the echoed arg */

	if(expectValue) {
		respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &fromAddr, &fromSize);
		/* null-terminate the received data */
    		echoBuffer[respStringLen] = '\0';
    		printf("Received: %s\n", echoBuffer);    /* Print the echoed arg */
	}

    	if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr) {
        	fprintf(stderr,"Error: received a packet from unknown source.\n");
        	exit(1);
   	}
    
    	close(sock);
    	exit(0);
}
