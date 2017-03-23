extern "C" {
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>
	#include <string.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <arpa/inet.h>
}

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <thread>
#include <mutex>
#include <vector>
#include <future>

#include "RDPMessage.h"


int sendSock;
struct sockaddr_in saIn;
int recvSock;
struct sockaddr_in saOut;
// Seems to be 36 atm (or 35, going with 36 though)
int HEADER_LENGTH = 0;

RDPMessage prepSynMessage(){
	RDPMessage message;
	message.setSYN(true);
	message.setSeqNum(rand() % message.seqNumLen());
	message.setAckNum(0);
	message.setSize(0);
	message.setMessage("");
	// Not important for no-data messages
	message.setLength(0);
	// std::string cppString = message.toString(false);
	// message.setLength(cppString.length());
	// message.updateLength();
	return message;
}

RDPMessage prepFileMessage(int seqNum, int fileLen, std::string contents){
	RDPMessage message;
	message.setDAT(true);
	message.setSeqNum(seqNum);
	message.setAckNum(0);
	message.setSize(0);
	message.setMessage(contents);
	message.setLength(fileLen);
	// std::string cppString = message.toString(false);
	// message.setLength(cppString.length());
	// message.updateLength();
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
	saOut = initSa(recvIP, recvPort);
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
			(struct sockaddr*)&saOut, sizeof saOut);
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
	saIn = createRecvSocket();
	fromlen = sizeof(saIn);

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
}

int establishConnection(RDPMessage messageOut, std::string sendIP, 
						 std::string sendPort, std::string recvIP, 
						 std::string recvPort){
	sendInitSyn(messageOut, recvIP, recvPort);
	// recvInitAck();
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

std::string openFile(std::string filename, char* fileContents, int fileLen){
	// Get file length
    // Read in file
    FILE * inFileC = fopen(filename.c_str(), "r");
    int bytes_read = fread(fileContents, sizeof(char), fileLen, inFileC);
    if (bytes_read == 0)
    	std::cout << "Input file is empty" << std::endl;

	return std::string(fileContents);
}

// int waitForInput(){

// }

template<typename R>
  bool is_ready(std::future<R> const& f)
  { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }


std::mutex listEdit;
// Could be winSize / maxPackSize, ie. 10
// volatile std::vector<RDPMessage> packWaitAckList;
std::vector<RDPMessage> packWaitAckList;
// returns -1 on thread ACK
// returns seqNum on timed out message
int sendAndWaitThread(RDPMessage messageObj){
	// --- Send Packet ---
    char fullReply[MAX_MESS_LEN];
    memset(fullReply, '\0', sizeof(fullReply));
    messageObj.toCString(fullReply);
	// while (fileSent < fileLen){
    listEdit.lock();
    int bytesSent = sendto(sendSock, fullReply, strlen(fullReply), 0,
                    (struct sockaddr*)&saOut, sizeof saOut);
	packWaitAckList.push_back(messageObj);
	listEdit.unlock();
    // --- Wait for Reply Packet here ---
    std::cout << "Packet is sent " << bytesSent << " now waiting for ACK reply" 
    		<< std::endl;
    // Set a timer with timeout == 3 seconds
    struct timeval timeout;
    timeout.tv_sec = 3; 
    timeout.tv_usec = 0; 
    fd_set fdRead;
    FD_ZERO(&fdRead); 
    FD_SET(recvSock, &fdRead); 
    int retval = select(0, &fdRead, NULL, NULL, &timeout); 
    // Recursive error check sending
    if (retval <= 0){
	    std::cout << "Response timed out. Re-send..." << std::endl;
	    // ToDo: Check if it's still in the list, it might have been already covered
	    return sendAndWaitThread(messageObj);
	} else { 
    	char buffer[1024];
		socklen_t fromlen = sizeof(saIn);
		ssize_t recsize = recvfrom(recvSock, (void*)buffer, sizeof buffer, 0, 
    			(struct sockaddr*)&saIn, &fromlen);
	    if (recsize < 0) {
	        fprintf(stderr, "%s\n", strerror(errno));
	        exit(EXIT_FAILURE);
    	}
    	// RDPMessage temp;
    	// temp.unpackCString(buffer);
    	// ToDo: Remove self from list 
    	return bytesSent;
    } 
    return messageObj.seqNum();
}

// winSize is the TOTAL receiver window
void sendFile(std::string filename, int winSize, int seqNum){
	// Max RDP packet size = 1024 bytes
	int fileLen = getFileLen(filename);
    char fileContents[fileLen + 1];
    memset(fileContents, '\0', sizeof(fileContents));
	std::string wholeFile = openFile(filename, fileContents, fileLen);
	// Loop through file sending parts until their expected buffer is full
	// ToDo: Necessary
	// int fileSent = 0;


    int dataReplySize = fileLen;
    if (fileLen > (MAX_MESS_LEN - HEADER_LENGTH))
    	dataReplySize = MAX_MESS_LEN - HEADER_LENGTH;
    // --- Loop starts here ---
    // int i = 0;
    for (int i = 0; i < fileLen; i += dataReplySize){
    	std::cout << "Looping. File len is " << fileLen << " data reply size " 
    			<< dataReplySize << std::endl;
	    std::string sendFilePart = wholeFile.substr(i, dataReplySize);
	    RDPMessage messageObj = prepFileMessage(seqNum, dataReplySize, sendFilePart);
		// std::promise<int> p;
	    std::future<int> p = std::async(sendAndWaitThread, messageObj);
	    int retAck = p.get();
	    std::cout << "My thread promise is " << retAck << std::endl;
    }
    // ToDo: Try making identical loop to re-join all the threads 



    // This means we need to re-send the package with the 
    // Change to while loop for infinite fix?
    // listEdit.lock();
    // Error is from calling get() too many times
    // You can ONLY call get() once
    // Why don't I default it to the ackNum so it only falls into the loop once	
    // std::cout << "Is ready result is " << is_ready(p) << std::endl;
 //    if (retAck != -1)
 //    {
 //    	for (uint i = 0; i < packWaitAckList.size(); ++i)
 //    	{
 //    		if (packWaitAckList[i].seqNum() == retAck)
 //    		{
 //    			std::cout << "Resending packWaitAckList[i]" << std::endl;
	// 		    // std::future<int> p = std::async(sendAndWaitThread, messageObj);
 //    			// p.get();
	// 		    // std::cout << "My thread promise is " << retAck << std::endl;
 //    		}
 //    	}
 //    }
	// listEdit.unlock();
    // -- Copied to thread --
	// fileSent += bytesSent;
    // ToDo: Neccessary

	// }
}

int main(int argc, char const *argv[])
{
	// std::cout << sizeof "CSC361" << std::endl;
	RDPMessage message = prepSynMessage();
    char header[200];
    memset(header, '\0', sizeof(header));
    message.toCString(header);
    HEADER_LENGTH = strlen(header);
    std::cout << "HEADER_LENGTH was " << HEADER_LENGTH << std::endl;
	// Initial 2-way handshake
	// Sender IP, Sender Port, Recv IP, Recv Port 
	int winSize = establishConnection(message, argv[1], argv[2], argv[3], argv[4]);
	// Use known window size from handshake to send file
	sendFile(argv[5], winSize, message.seqNum() + 1);
	return 0;
}