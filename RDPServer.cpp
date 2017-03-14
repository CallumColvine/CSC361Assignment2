extern "C" {
	#include <stdio.h>
	#include <errno.h>
	#include <string.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <unistd.h> /* for close() for socket */ 
	#include <stdlib.h>
	#include <arpa/inet.h>

}

#include <iostream>
#include "RDPMessage.h"

#define WINDOW_SIZE 		1024
#define FULL_WINDOW_SIZE 	10240

int sendSock;
int recvSock;

RDPMessage establishConnection(){
	recvSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in sa; 
	char buffer[WINDOW_SIZE];
    memset(buffer, '\0', sizeof(buffer));
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

	// for (;;) {
    recsize = recvfrom(recvSock, (void*)buffer, sizeof buffer, 0, 
    		(struct sockaddr*)&sa, &fromlen);
    if (recsize < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("datagram: %.*s\n", (int)recsize, buffer);
    	// break;
	// }
	RDPMessage messageIn;
	messageIn.unpackCString(buffer);
	return messageIn;
}

RDPMessage prepareMessageOut(RDPMessage messageOut, RDPMessage messageIn){
	std::cout << "message in was" << std::endl; 
	messageIn.toString(true);
	messageOut.setACK(true);
	messageOut.setSeqNum(messageIn.seqNum());
	int ackNum = messageIn.seqNum() + messageIn.length();
	messageOut.setAckNum(ackNum);
	messageOut.setSize(0);
	messageOut.setMessage("");
	messageOut.updateLength();
	return messageOut;
}

// Send ACK 
void sendReply(RDPMessage messageIn, RDPMessage messageOut){
	messageOut = prepareMessageOut(messageOut, messageIn);
  	struct sockaddr_in sa;
	int bytes_sent;
	// char buffer[WINDOW_SIZE];
	// ToDo: Edit copy here so it copies the whole message into the buffer
	char messageString[WINDOW_SIZE];
    memset(messageString, '\0', sizeof(messageString));
    messageOut.toString(true);
    messageOut.toCString(messageString);
    std::cout << "Replying with " << messageString << std::endl;

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
	bytes_sent = sendto(sendSock, messageString, strlen(messageString), 0,
			(struct sockaddr*)&sa, sizeof sa);
	if (bytes_sent < 0) {
		printf("Error sending packet: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
}

void inputData(){

}

void sendAck(){

}

void inputLoop(char* fullWindow){
	// Here is where I loop forever until I get a FIN packet
	bool receivedFIN = false;
	while(receivedFIN == false){
		inputData();
		sendAck();
	}
}

int main(int argc, char const *argv[])
{
	char fullWindow[FULL_WINDOW_SIZE];
	RDPMessage messageIn = establishConnection();
	RDPMessage messageOut;
	sendReply(messageIn, messageOut);
	inputLoop(fullWindow);
	return 0;
}