#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define ECHOMAX 255     /* Longest string to echo */

int main(int argc, char *argv[]) {
	int sock;                        /* Socket */
	struct sockaddr_in echoServAddr; /* Local address */
    	struct sockaddr_in echoClntAddr; /* Client address */
    	unsigned int cliAddrLen;         /* Length of incoming message */
    	char echoBuffer[ECHOMAX];        /* Buffer for echo string */
    	unsigned short echoServPort;     /* Server port */
    	int recvMsgSize;                 /* Size of received message */
	char *parserString;
	char *tok;
	char *keyValue[256];

    	if (argc != 2) {         /* Test for correct number of parameters */
        	fprintf(stderr,"Usage:  %s <UDP SERVER PORT>\n", argv[0]);
        	exit(1);
    	}

    	echoServPort = atoi(argv[1]);  /* First arg:  local port */

    	/* Create socket for sending/receiving datagrams */
    	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		//print to file that socket creating failed
	}
	

    	/* Construct local address structure */
    	memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    	echoServAddr.sin_family = AF_INET;                /* Internet address family */
    	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    	echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    	/* Bind to the local address */
    	if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
		//print to file that binding failed
	}
  
    	for (;;) { /* Run forever */
        	/* Set the size of the in-out parameter */
        	cliAddrLen = sizeof(echoClntAddr);

        	/* Block until receive message from a client */
        	if ((recvMsgSize = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0) {
			//print to file that recvfrom failed
		}

        	printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

        	/* Send received datagram back to the client */
        	if (sendto(sock, echoBuffer, recvMsgSize, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != recvMsgSize) {
			//print to file that wrong # bytes sent
		} else {
			parserString = echoBuffer;
			printf("%s\n",parserString);
			tok = strtok(parserString,",");
			if(!strcmp(tok,"GET")) {
				printf("%s\n",tok);
				tok = strtok(NULL, ",");
				printf("%s\n",tok);
				printf("%d\n",atoi(tok));
				if(atoi(tok) < 0 || atoi(tok) > 255) {
					//printerror
				} else {
					printf("getting key value: %d\n",atoi(tok));
					printf("%s\n",tok);
					sendto(sock, keyValue[atoi(tok)], strlen(keyValue[atoi(tok)]), 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
				}
			}
			if(!strcmp(tok,"PUT")) {
				printf("%s\n",tok);
				tok = strtok(NULL, ",");
				printf("%s\n",tok);
				printf("%d\n",atoi(tok));
				if(atoi(tok) < 0 || atoi(tok) > 255) {
					//printerror
				} else {
					char* value = strtok(NULL,",");
					printf("%s\n",value);
					keyValue[atoi(tok)] = value;
					printf("%s\n",keyValue[atoi(tok)]);
				}
			}
			if(!strcmp(tok,"DELETE")) {
				tok = strtok(NULL,",");
				if(atoi(tok) < 0 || atoi(tok) > 255) {
					//printerror
				} else {
					keyValue[atoi(tok)] = "";
				}
			}
		}
	}
    /* NOT REACHED */
}
