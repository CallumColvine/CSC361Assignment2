#include <iostream>
#include <cstdlib>

#include "RDPMessage.h"

extern "C" {
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>
	#include <string.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <arpa/inet.h>
}

int sendSock;
int recvSock;

RDPMessage prepareMessage(){
	RDPMessage message;
	// do I need to fill these up to the expected max len?
	message.setSYN(true);
	message.setSeqNum(rand() % message.seqNumLen());
	message.setAckNum(0);
	message.setLength(0);
	message.setSize(0);
	message.setMessage("");
	return message;
}

void establishConnection(RDPMessage message){
  	struct sockaddr_in sa;
	int bytes_sent;
	char buffer[200];
	// ToDo: Edit copy here so it copies the whole message into the buffer
	char messageString[2048];
    memset(messageString, '\0', sizeof(messageString));
	message.toCString(messageString);
	std::cout << "messageString is " << messageString << std::endl;
	strcpy(buffer, messageString);
	/* create an Internet, datagram, socket using UDP */
	sendSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (-1 == sendSock) {
	    /* if socket failed to initialize, exit */
	    printf("Error Creating Socket");
	    exit(EXIT_FAILURE);
	}
	/* Zero out socket address */
	memset(&sa, 0, sizeof sa);
	/* The address is IPv4 */
	sa.sin_family = AF_INET;
	/* IPv4 adresses is a uint32_t, convert a string representation of the 
	octets to the appropriate value */
	// sa.sin_addr.s_addr = inet_addr("10.10.1.100");
	// sa.sin_addr.s_addr = inet_addr("10.0.2.255");
	sa.sin_addr.s_addr = inet_addr("10.0.2.15");
	/* sockets are unsigned shorts, htons(x) ensures x is in network byte order, 
	set the port to 7654 */
	sa.sin_port = htons(8080);
	bytes_sent = sendto(sendSock, buffer, strlen(buffer), 0,
			(struct sockaddr*)&sa, sizeof sa);
	if (bytes_sent < 0) {
		printf("Error sending packet: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
	// close(sock); /* close the socket */
}

void awaitReply(){
	recvSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in sa; 
	char buffer[1024];
	ssize_t recsize;
	socklen_t fromlen;

	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(8080);
	fromlen = sizeof(sa);

	if (-1 == bind(recvSock, (struct sockaddr *)&sa, sizeof sa)) {
	    perror("error bind failed");
	    close(recvSock);
	    exit(EXIT_FAILURE);
	}

	for (;;) {
	    recsize = recvfrom(recvSock, (void*)buffer, sizeof buffer, 0, 
	    		(struct sockaddr*)&sa, &fromlen);
	    if (recsize < 0) {
	        fprintf(stderr, "%s\n", strerror(errno));
	        exit(EXIT_FAILURE);
	    }
	    printf("datagram: %.*s\n", (int)recsize, buffer);
	    break;
	}
}

int main(int argc, char const *argv[])
{
	// std::cout << sizeof "CSC361" << std::endl;
	RDPMessage message = prepareMessage();
	establishConnection(message);
	// awaitReply();
	return 0;
}