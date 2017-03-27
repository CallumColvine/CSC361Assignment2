extern "C" {
    #include <stdio.h>
    #include <errno.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <unistd.h> /* for close() for socket */
    #include <stdlib.h>
    #include <arpa/inet.h>

}

#include <sstream>
#include <iostream>
#include "RDPMessage.h"
#include <fstream>
#include <vector>
#include <thread>
#include <ctime>
// #include <string>

// #define WINDOW_SIZE      1024

// total data bytes received:
uint totalData = 0;
// unique data bytes received:
uint uniqueData = 0;
// total data packets received:
uint totalDataPackets = 0;
// unique data packets received:
uint uniqueDataPackets = 0;
// SYN packets received:
uint synPackets = 0;
// FIN packets received:
uint finPackets = 0;
// RST packets received:
uint rstPacketsRecv = 0;
// ACK packets sent:
uint ackPackets = 0;
// RST packets sent:
uint rstPacketsSent = 0;
// total time duration (second):
struct timeval startTime;
struct timeval endTime;

int sendSock;
struct sockaddr_in saIn;
int recvSock;

int mostRecentSeq;

int amountDataWaiting = 0;
std::vector<RDPMessage> inMessages;

int curWindowSize = FULL_WINDOW_SIZE;
int lastBytesRead = 0;

std::string sourceIP;
std::string sourcePort;
std::string destIP;
std::string destPort;


std::vector<std::string> splitString(std::string input)
{
    std::istringstream iss(input);
    std::vector<std::string> dateVector;
    do
    {
        std::string sub;
        iss >> sub;
        dateVector.push_back(sub);

    } while (iss);
    return dateVector;
}


std::string getTime(){
    time_t tobj;
    time(&tobj);
    // struct tm * now = localtime(&tobj);
    // ctime (&rawtime)
    std::string now = ctime(&tobj);
    // std::cout << now << std::endl;
    std::stringstream returnTimeS;
    // std::cout << "now is " << now << std::endl;
    std::vector<std::string> splitTime = splitString(now);
    returnTimeS << splitTime[3];
    std::string returnTime = returnTimeS.str();
    return returnTime;
}

char getType(int typeIn){
    if (typeIn == 1)
        return 'D';
    else if (typeIn == 10)
        return 'A';
    else if (typeIn == 100)
        return 'S';
    else if (typeIn == 1000)
        return 'F';
    else if (typeIn == 10000)
        return 'R';
    return 'X';
}

void logAction(int typeIn, int seqOrAck, int payloadOrWinSize,
               int numSentRecv, char sendOrRecv){
    if (sendOrRecv == 's') {
        if (numSentRecv > 0) {
            sendOrRecv = 'S';
        }
    } else {
        sendOrRecv = 'r';
    }
    char type = getType(typeIn);
    std::string timePart = getTime();
    std::cout << timePart << ' ' << sendOrRecv << ' ' << sourceIP << ':' << sourcePort <<
            ' ' << destIP << ':' << destPort << ' ' << type << ' ' <<
            seqOrAck << ' ' << payloadOrWinSize << std::endl;
}

void finalPrint(){
    gettimeofday(&endTime, NULL);
    std::cout << "total data bytes received: " << totalData << std::endl;
    std::cout << "unique data bytes received: " << uniqueData << std::endl;
    std::cout << "total data packets received: " << totalDataPackets << std::endl;
    std::cout << "unique data packets received: " << uniqueDataPackets << std::endl;
    std::cout << "SYN packets received: " << synPackets << std::endl;
    std::cout << "FIN packets received: " << finPackets << std::endl;
    std::cout << "RST packets received: " << rstPacketsRecv << std::endl;
    std::cout << "ACK packets sent: " << ackPackets << std::endl;
    std::cout << "RST packets sent: " << rstPacketsSent << std::endl;
    std::cout << "total time duration: " <<
            endTime.tv_sec - startTime.tv_sec << std::endl;
}

RDPMessage establishConnection(std::string recvIP, std::string recvPort){
    recvSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    char buffer[MAX_MESS_LEN];
    memset(buffer, '\0', sizeof(buffer));
    ssize_t recsize;
    socklen_t fromlen;

    memset(&saIn, 0, sizeof saIn);
    saIn.sin_family = AF_INET;
    // std::cout << "Setting my recv IP to " << recvIP << " and recv port to " <<
    //         recvPort << std::endl;
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
    logAction(messageIn.type(), messageIn.seqNum(), messageIn.length(),
              messageIn.timesSent(), 'r');
    if (messageIn.type() == 100)
        synPackets ++;
    gettimeofday(&startTime, NULL);
    mostRecentSeq = messageIn.seqNum();
    // Since we increment by 1 from the 2-way handshake
    mostRecentSeq += 1;
    lastBytesRead = int(recsize);
    return messageIn;
}

RDPMessage prepareMessageOut(RDPMessage messageOut, RDPMessage messageIn){
    // std::cout << "message in was" << std::endl;
    messageIn.toString(true);
    messageOut.setACK(true);
    messageOut.setSeqNum(messageIn.seqNum());
    int ackNum = messageIn.seqNum() + lastBytesRead;
    messageOut.setAckNum(ackNum);
    // std::cout << "Sending a reply with ACK == " << messageOut.ackNum();
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
    // std::cout << "Replying with " << messageString << std::endl;
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
    logAction(messageOut.type(), messageOut.ackNum(), messageOut.size(),
              messageOut.timesSent(), 's');
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
    totalData += recsize;
    totalDataPackets ++;
    // printf("datagram: %.*s\n", (int)recsize, buffer);
    RDPMessage messageIn;
    messageIn.unpackCString(buffer);
    logAction(messageIn.type(), messageIn.seqNum(), messageIn.length(),
              messageIn.timesSent(), 'r');
    inMessages.push_back(messageIn);
    return (int) recsize;
}

void sendAck(int outAckNum, int newWinSize, int type = 100){
    RDPMessage messageOut;
    messageOut.setType(type);
    messageOut.setAckNum(outAckNum);
    messageOut.setSize(newWinSize);
    char fullReply[MAX_MESS_LEN];
    memset(fullReply, '\0', sizeof(fullReply));
    messageOut.toCString(fullReply);
    int bytesSent = sendto(recvSock, fullReply, strlen(fullReply), 0,
            (struct sockaddr*)&saIn, sizeof saIn);
    if (bytesSent < 0) {
        printf("Error sending packet: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    logAction(messageOut.type(), messageOut.ackNum(), messageOut.size(),
              messageOut.timesSent(), 's');
    // messageOut.setSeqNum
    // std::cout << "Replied with num bytes: " << bytesSent << std::endl;
}

bool rollOver = false;

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
        // std::cout << "Expected SEQ num " << mostRecentSeq;
        // std::cout << " Received SEQ num " << messageIn.seqNum()<< std::endl;
        // FIN packet caught, reply with FIN
        if (messageIn.type() == 1000) {
            std::cout << "Received FIN packet" << std::endl;
            sendAck(mostRecentSeq, curWindowSize);
            receivedFIN = true;
        }
        // Case where we caught the next package in the sequence
        if(mostRecentSeq + recvSize > 10000000){
            rollOver = true;
        }
        ackPackets ++;
        if (mostRecentSeq == messageIn.seqNum())
        {
            // std::cout << "Received the next expected message " << std::endl;
            curWindowSize -= recvSize;
            sendAck(mostRecentSeq, curWindowSize);
            out << messageIn.message();
            out.flush();
            curWindowSize += recvSize;
            mostRecentSeq = (mostRecentSeq + recvSize) % 10000000;
            uniqueData += recvSize;
            uniqueDataPackets ++;
        }
        // Case where we caught a later than expected package
        else if (mostRecentSeq < messageIn.seqNum()){
            // std::cout << "Received that's further than expected asking for" << std::endl;
            sendAck(mostRecentSeq, curWindowSize);
        }
        // Case where the SEQ num rolled over, so this is backwards
        // SEQ num in will be like 400 when mostRecent is like 99900
        else if(mostRecentSeq > messageIn.seqNum()) {
            sendAck(mostRecentSeq, curWindowSize);
            rollOver = false;
        }
        else {
            // std::cout << "Receiving messages earlier than expected. Asking for " <<
            //         mostRecentSeq << std::endl;
            sendAck(mostRecentSeq, curWindowSize);
        }
        //
        // writeInputToFile(messageIn, out);
    }
    out.close();
    finalPrint();
}

int main(int argc, char const *argv[])
{
    char fullWindow[FULL_WINDOW_SIZE];
    RDPMessage messageIn = establishConnection(argv[1], argv[2]);
    RDPMessage messageOut;

    sourceIP = argv[1];
    sourcePort = argv[2];
    destIP = argv[1];
    destPort = argv[2];

    // Wait before reply, just for testing
    std::this_thread::sleep_for(std::chrono::seconds(3));
    sendReply(messageIn, messageOut);
    inputLoop(fullWindow, argv[3]);
    return 0;
}
