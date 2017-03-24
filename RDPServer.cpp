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
#include <vector>
#include <thread>
#include <ctime>
// #include <string>

// #define WINDOW_SIZE      1024

int sendSock;
struct sockaddr_in saIn;
int recvSock;

int mostRecentSeq;

int amountDataWaiting = 0;
std::vector<RDPMessage> inMessages;


RDPMessage establishConnection(std::string recvIP, std::string recvPort){
    recvSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    char buffer[MAX_MESS_LEN];
    memset(buffer, '\0', sizeof(buffer));
    ssize_t recsize;
    socklen_t fromlen;

    memset(&saIn, 0, sizeof saIn);
    saIn.sin_family = AF_INET;
    std::cout << "Setting my recv IP to " << recvIP << " and recv port to " <<
            recvPort << std::endl;
    saIn.sin_addr.s_addr = inet_addr(recvIP.c_str());
    saIn.sin_port = htons(std::stoi(recvPort));
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
    mostRecentSeq = messageIn.seqNum();
    // Since we increment by 1 from the 2-way handshake
    mostRecentSeq += 1;
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
    // struct sockaddr_in sa;
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
        // if socket failed to initialize, exit
        printf("Error Creating Socket");
        exit(EXIT_FAILURE);
    }
    bytes_sent = sendto(recvSock, messageString, strlen(messageString), 0,
            (struct sockaddr*)&saIn, sizeof saIn);
    if (bytes_sent < 0) {
        printf("Error sending packet: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int inputData(){
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
    // printf("datagram: %.*s\n", (int)recsize, buffer);
    RDPMessage messageIn;
    messageIn.unpackCString(buffer);
    inMessages.push_back(messageIn);
    return (int) recsize;
}

void sendAck(int outAckNum){
    RDPMessage messageOut;
    messageOut.setACK(true);
    messageOut.setSeqNum(outAckNum);
    char fullReply[MAX_MESS_LEN];
    memset(fullReply, '\0', sizeof(fullReply));
    messageOut.toCString(fullReply);
    int bytesSent = sendto(recvSock, fullReply, strlen(fullReply), 0,
            (struct sockaddr*)&saIn, sizeof saIn);
    // messageOut.setSeqNum
    std::cout << "Replied with num bytes: " << bytesSent << std::endl;
}

void inputLoop(char* fullWindow, std::string filenameOut){
    // Here is where I loop forever until I get a FIN packet
    // FILE* pFile = fopen(filenameOut.c_str(), "w");
    // Open file for writing
    std::ofstream out;
    out.open(filenameOut, std::ofstream::out | std::ofstream::app);
    bool receivedFIN = false;
    while(receivedFIN == false){
        int recvSize = inputData();
        RDPMessage messageIn = inMessages.back();
        std::cout << "Expected SEQ num " << mostRecentSeq;
        std::cout << " Received SEQ num " << messageIn.seqNum()<< std::endl;
        // Case where we caught the next package in the sequence
        if (mostRecentSeq + recvSize == messageIn.seqNum() + recvSize)
        {

            std::cout << "Received the next expected message " << std::endl;
            out << messageIn.message();
            out.flush();
            mostRecentSeq = mostRecentSeq + recvSize;
            sendAck(mostRecentSeq);
        }
        // Case where we caught a later than expected package
        else if (mostRecentSeq + recvSize < messageIn.seqNum() + recvSize){
            std::cout << "Received a message that's further than expected " << std::endl;
            sendAck(mostRecentSeq);
        }
        //
        // writeInputToFile(messageIn, out);
    }
    out.close();
}

int main(int argc, char const *argv[])
{
    char fullWindow[FULL_WINDOW_SIZE];
    RDPMessage messageIn = establishConnection(argv[1], argv[2]);
    RDPMessage messageOut;
    // Wait before reply, just for testing
    std::this_thread::sleep_for(std::chrono::seconds(3));
    sendReply(messageIn, messageOut);
    inputLoop(fullWindow, argv[3]);
    return 0;
}
