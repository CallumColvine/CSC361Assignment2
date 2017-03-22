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
#include <fstream>
// #include <string>

// #define WINDOW_SIZE 		1024
#define FULL_WINDOW_SIZE 	10240

int sendSock;
struct sockaddr_in saIn; 
int recvSock;

RDPMessage establishConnection(){
	recvSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	char buffer[MAX_MESS_LEN];
    memset(buffer, '\0', sizeof(buffer));
	ssize_t recsize;
	socklen_t fromlen;

	memset(&saIn, 0, sizeof saIn);
	saIn.sin_family = AF_INET;
	saIn.sin_addr.s_addr = htonl(INADDR_ANY);
	saIn.sin_port = htons(8080);
	fromlen = sizeof(saIn);

	if (-1 == bind(recvSock, (struct sockaddr *)&saIn, sizeof saIn)) {
	    perror("error bind failed");
	    close(recvSock);
	    exit(EXIT_FAILURE);
	}

	// for (;;) {
    recsize = recvfrom(recvSock, (void*)buffer, sizeof buffer, 0, 
    		(struct sockaddr*)&saIn, &fromlen);
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
	// messageOut.updateLength();
	return messageOut;
}

// Send ACK 
void sendReply(RDPMessage messageIn, RDPMessage messageOut){
	messageOut = prepareMessageOut(messageOut, messageIn);
  	struct sockaddr_in sa;
	int bytes_sent;
	// char buffer[WINDOW_SIZE];
	// ToDo: Edit copy here so it copies the whole message into the buffer
	char messageString[MAX_MESS_LEN];
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

RDPMessage inputData(){
	ssize_t recsize;
	socklen_t fromlen = sizeof(saIn);
	char buffer[MAX_MESS_LEN];
    memset(buffer, '\0', sizeof(buffer));
    recsize = recvfrom(recvSock, (void*)buffer, sizeof buffer, 0, 
    		(struct sockaddr*)&saIn, &fromlen);
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

// void writeInputToFile(RDPMessage messageIn, std::ofstream out){
// 	// std::cin >> messageIn;
// 	out << messageIn;
// 	// std::cout << "Writing to file... " << std::endl;
// 	// char buff[messageIn.message().size()];
// 	// buff = messageIn.message().c_str();
// 	// // std::cout << "Write out buff is " << buff << std::endl;
// 	// fwrite (buff, sizeof(char), sizeof(buff), pFile);
// }

void sendAck(){

}

void inputLoop(char* fullWindow, std::string filenameOut){
	// Here is where I loop forever until I get a FIN packet
	// FILE* pFile = fopen(filenameOut.c_str(), "w");
	std::ofstream out;
	out.open(filenameOut, std::ofstream::out | std::ofstream::app);
	bool receivedFIN = false;
	while(receivedFIN == false){
		RDPMessage messageIn = inputData();
		out << messageIn.message();
		out.flush();
		// writeInputToFile(messageIn, out);
		sendAck();
	}
	out.close();
}

int main(int argc, char const *argv[])
{
	char fullWindow[FULL_WINDOW_SIZE];
	RDPMessage messageIn = establishConnection();
	RDPMessage messageOut;
	// sendReply(messageIn, messageOut);
	inputLoop(fullWindow, argv[3]);
	return 0;
}