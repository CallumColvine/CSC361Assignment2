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

#include <iostream>
#include <cstdlib>
#include <fstream>

#include "RDPMessage.h"


int sendSock;
int recvSock;

RDPMessage prepareMessage(){
	RDPMessage message;
	message.setSYN(true);
	message.setSeqNum(rand() % message.seqNumLen());
	message.setAckNum(0);
	message.setSize(0);
	message.setMessage("");
	message.setLength(0);
	// std::string cppString = message.toString(false);
	// message.setLength(cppString.length());
	message.updateLength();
	return message;
}

void initSendSock(){
	sendSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (-1 == sendSock) {
	    printf("Error Creating Socket");
	    exit(EXIT_FAILURE);
	}
}

struct sockaddr_in initSa(std::string recvIP, std::string recvPort){
	struct sockaddr_in sa;
	/* Zero out socket address */
	memset(&sa, 0, sizeof sa);
	/* The address is IPv4 */
	sa.sin_family = AF_INET;
	/* IPv4 adresses is a uint32_t, convert a string representation of the 
	octets to the appropriate value */
	// sa.sin_addr.s_addr = inet_addr("10.10.1.100");
	// sa.sin_addr.s_addr = inet_addr("10.0.2.255");
	sa.sin_addr.s_addr = inet_addr(recvIP.c_str());
	/* sockets are unsigned shorts, htons(x) ensures x is in network byte order, 
	set the port to 7654 */
	sa.sin_port = htons(stoi(recvPort));
	return sa;
}

void sendInitSyn(RDPMessage messageOut, std::string recvIP, std::string recvPort){
	int bytes_sent;
	initSendSock();
	struct sockaddr_in sa = initSa(recvIP, recvPort);
	// char buffer[1024];
	// ToDo: Edit copy here so it copies the whole message into the buffer
	char messageString[1024];
    memset(messageString, '\0', sizeof(messageString));
	messageOut.toCString(messageString);
	// std::cout << "messageString is " << messageString << std::endl;
	// std::cout << "Now unpackCString: " << std::endl;
	RDPMessage newMessage;
	newMessage.unpackCString(messageString);
	newMessage.toString(true);
	bytes_sent = sendto(sendSock, messageString, strlen(messageString), 0,
			(struct sockaddr*)&sa, sizeof sa);
	if (bytes_sent < 0) {
		printf("Error sending packet: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
}

struct sockaddr_in createRecvSocket(){
	struct sockaddr_in sa; 
	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(8080);
	if (-1 == bind(recvSock, (struct sockaddr *)&sa, sizeof sa)) {
	    perror("error bind failed");
	    close(recvSock);
	    exit(EXIT_FAILURE);
	}
	return sa;
}

void recvInitAck(){
	recvSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	char buffer[1024];
	ssize_t recsize;
	socklen_t fromlen;
	struct sockaddr_in sa = createRecvSocket();
	fromlen = sizeof(sa);

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

int establishConnection(RDPMessage messageOut, std::string sendIP, 
						 std::string sendPort, std::string recvIP, 
						 std::string recvPort){
	sendInitSyn(messageOut, recvIP, recvPort);
	recvInitAck();
	// close(sock); /* close the socket */
	// Should return the total recv window size
	return 10240;
}

int getFileLen(std::string filename){
	std::ifstream inFile(filename, std::ios::binary | std::ios::ate);
    int fileLen = inFile.tellg();
    inFile.close();
    return fileLen;
}

void openFile(std::string filename, char* fileContents, int fileLen){
	// Get file length
    // Read in file
    FILE * inFileC = fopen(filename.c_str(), "r");
    int bytes_read = fread(fileContents, sizeof(char), fileLen, inFileC);
    if (bytes_read == 0)
    std::cout << "Input file is empty" << std::endl;
	
}

void sendFile(std::string filename, int winSize){
	// Max RDP packet size = 1024 bytes
	int fileLen = getFileLen(filename);
    char fileContents[fileLen + 1];
    memset(fileContents, '\0', sizeof(fileContents));
	openFile(filename, fileContents, fileLen);
}

int main(int argc, char const *argv[])
{
	// std::cout << sizeof "CSC361" << std::endl;
	RDPMessage message = prepareMessage();
	// Initial 2-way handshake
	// Sender IP, Sender Port, Recv IP, Recv Port 
	int winSize = establishConnection(message, argv[1], argv[2], argv[3], argv[4]);
	// Use known window size from handshake to send file
	sendFile(argv[5], winSize);
	return 0;
}