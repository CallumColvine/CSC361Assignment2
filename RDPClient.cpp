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
#include <chrono>
#include <algorithm>

#include "RDPMessage.h"


#define LOCAL_TESTING false

int sendSock;
struct sockaddr_in saIn;
struct sockaddr_in saOut;
// Seems to be 36 atm (or 35, going with 36 though)
int HEADER_LENGTH = 0;
volatile int lastAck = 0;


RDPMessage prepSynMessage(){
    RDPMessage message;
    message.setSYN(true);
    message.setSeqNum(rand() % 100);
    // message.setSeqNum(rand() % message.seqNumLen());
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

}

struct sockaddr_in initSa(std::string recvIP, std::string recvPort){
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(recvIP.c_str());
    sa.sin_port = htons(stoi(recvPort));
    return sa;
}

void sendInitSyn(RDPMessage messageOut){
    int bytes_sent;
    // ToDo: Edit copy here so it copies the whole message into the buffer
    char messageString[1024];
        memset(messageString, '\0', sizeof(messageString));
    messageOut.toCString(messageString);
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

struct sockaddr_in createRecvSocket(std::string sendIP, std::string sendPort){
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(sendIP.c_str());
    sa.sin_port = htons(std::stoi(sendPort));
    return sa;
}

int recvInitAck(std::string sendIP, std::string sendPort){
    // ToDo: check ACK num from SYN reply
    char buffer[1024];
    socklen_t fromlen = sizeof(saOut);
        ssize_t recsize = recvfrom(sendSock, (void*)buffer, sizeof buffer, 0,
                (struct sockaddr*)&saOut, &fromlen);
        if (recsize < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
        printf("datagram: %.*s\n", (int)recsize, buffer);
        // break;
        RDPMessage temp;
        temp.unpackCString(buffer);
        lastAck = temp.ackNum();
        return temp.size();
    // }
}

void bindPort(std::string sendIP, std::string sendPort, std::string recvIP,
                std::string recvPort){
    sendSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == sendSock) {
            printf("Error Creating Socket");
            exit(EXIT_FAILURE);
    }
    saIn = initSa(sendIP, sendPort);
    saOut = initSa(recvIP, recvPort);
    if (-1 == bind(sendSock, (struct sockaddr *)&saIn, sizeof saIn)) {
            perror("error bind failed");
            close(sendSock);
            exit(EXIT_FAILURE);
    }
}


int establishConnection(RDPMessage messageOut, std::string sendIP,
                         std::string sendPort, std::string recvIP,
                         std::string recvPort){
    bindPort(sendIP, sendPort, recvIP, recvPort);
    sendInitSyn(messageOut);
    std::cout << "Init SYN sent, waiting for response... " << std::endl;

    // close(sock); /* close the socket */
    // Should return the total recv window size
    int winSize = FULL_WINDOW_SIZE;
    if (! LOCAL_TESTING)
        winSize = recvInitAck(recvIP, recvPort);
    return winSize;
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

template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }


std::mutex listEdit;
// std::mutex winSizeEdit;
std::mutex ackNumEdit;
// std::mutex guessEdit;
std::mutex editNumSend;
// Could be winSize / maxPackSize, ie. 10
// volatile std::vector<RDPMessage> packWaitAckList;
// std::vector<RDPMessage> packWaitAckList;
// volatile int senderWindowSize = FULL_WINDOW_SIZE;
// volatile int sendNext = -1;
// volatile int expectedAck = 0;
std::vector<RDPMessage> messToSend;
std::vector<RDPMessage> prioritySend;
volatile int numSending = 0;

void filterList(int ackNum){
    listEdit.lock();
    for (uint j = 0; j < messToSend.size(); j++)
        if (messToSend[j].ackNum() == ackNum)
            messToSend.erase(messToSend.begin() + j);
    for (uint i = 0; i < prioritySend.size(); i++)
        if (prioritySend[i].ackNum() == ackNum)
            prioritySend.erase(prioritySend.begin() + i);
    listEdit.unlock();
}

void threadGetReply(RDPMessage messageOut){
    char buffer[1024];
    socklen_t fromlen = sizeof(saOut);
    std::cout << "Setting thread to wait for ACK " << messageOut.seqNum() 
            << std::endl;
    ssize_t recsize = recvfrom(sendSock, (void*)buffer, sizeof buffer, 0,
            (struct sockaddr*)&saOut, &fromlen);
    if (recsize < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(EXIT_FAILURE);
    }
    RDPMessage messageIn;
    messageIn.unpackCString(buffer);
    // std::cout << "just sent " << std::endl;
    // messageOut.toString(true);
    // std::cout << "and received " << std::endl;
    // messageIn.toString(true);
    ackNumEdit.lock();
    lastAck = std::max((int)lastAck, messageIn.ackNum());
    listEdit.lock();
    // senderWindowSize = messageIn.size();
    // std::cout << "Before if okay " << std::endl;
    if (messageOut.seqNum() > lastAck){
        std::cout << "This packet's SEQ # is > than last ACK. Re-queue" << std::endl;
        messToSend.insert(messToSend.begin(), messageOut);                   
    } 
    // This is the next message that should send. 
    else if (messageOut.seqNum() == lastAck) {
        std::cout << "This packet's SEQ # is THE NEXT one to send. Queue as priority" 
                << std::endl;
        prioritySend.insert(prioritySend.begin(), messageOut);
    } else {
        std::cout << "This packet was less than the last ACK. Let it go." 
                << std::endl;
    }
    // ToDo: Remove self from list
    listEdit.unlock();
    ackNumEdit.unlock();
    editNumSend.lock();
    numSending --;
    editNumSend.unlock();
}

void threadTimedOut(RDPMessage messageOut){
    std::cout << "Response timed out. Re-send by prioritizing " <<
            messageOut.seqNum() << std::endl;
    // Only add if not alread in list
    listEdit.lock();
    prioritySend.insert(prioritySend.begin(), messageOut);
    listEdit.unlock();
    editNumSend.lock();
    numSending --;
    editNumSend.unlock();
}

// returns -1 on thread ACK
// returns seqNum on timed out message
int sendAndWaitThread(RDPMessage messageOut){
    // --- Send Packet ---
    char fullReply[MAX_MESS_LEN];
    memset(fullReply, '\0', sizeof(fullReply));
    messageOut.toCString(fullReply);
    int bytesSent = sendto(sendSock, fullReply, strlen(fullReply), 0,
                                    (struct sockaddr*)&saOut, sizeof saOut);
    // --- Wait for Reply Packet here ---
    std::cout << "Packet sent. Bytes: " << bytesSent << " SEQ num " << 
            messageOut.seqNum() << std::endl;
    // Set a timer with timeout == 3 seconds
    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(sendSock, &set);
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    int retval = select(sendSock + 1, &set, NULL, NULL, &timeout);
    if (retval == 0){
        threadTimedOut(messageOut); 
    } 
    else if(retval < 0) {
        perror("Error with select()");
        editNumSend.lock();
        numSending --;
        editNumSend.unlock();
    } 
    // Socket ready to read. Set it to listen
    else {
        threadGetReply(messageOut);
    }
    return bytesSent; 
}


// winSize is the TOTAL receiver window
void sendFile(std::string filename, int winSize, int seqNum){
    std::cout << "Commencing file send from seqNum " << seqNum << std::endl;
    // Max RDP packet size = 1024 bytes
    int fileLen = getFileLen(filename);
    char fileContents[fileLen + 1];
    memset(fileContents, '\0', sizeof(fileContents));
    std::string wholeFile = openFile(filename, fileContents, fileLen);
    // Loop through file sending parts until their expected buffer is full
    int dataReplySize = fileLen;
    if (fileLen > (MAX_MESS_LEN - HEADER_LENGTH))
        dataReplySize = MAX_MESS_LEN - HEADER_LENGTH;
    // Set the first expected ACK num == the initial seq num + the length of pack
    // expectedAckNum = seqNum + dataReplySize + HEADER_LENGTH;
    for (int i = 0; i < fileLen; i += dataReplySize){
        std::string sendFilePart = wholeFile.substr(i, dataReplySize);
        RDPMessage messageObj = prepFileMessage(seqNum, dataReplySize, sendFilePart);
        messToSend.push_back(messageObj);
        seqNum += MAX_MESS_LEN;
    }
    int i = 0;
    for (;;){
        while (((!messToSend.empty()) || (!prioritySend.empty())) && 
               numSending < (FULL_WINDOW_SIZE / MAX_MESS_LEN)){
            std::cout << "Looping. File len " << fileLen << " data reply size "
                    << dataReplySize << " packet num " << i << std::endl;
            // Filter list
            ackNumEdit.lock();
            // filterList(lastAck - lastSize);
            ackNumEdit.unlock();
            // Send item
            if (!prioritySend.empty())
            {
                // std::cout << "Sending a priority packet " << std::endl;
                listEdit.lock();
                RDPMessage sendNext = prioritySend.front();
                prioritySend.erase(prioritySend.begin());
                listEdit.unlock();
                std::thread(sendAndWaitThread, sendNext).detach();
            } else {
                std::cout << "Sending the expected next packet" << std::endl;
                listEdit.lock();
                RDPMessage sendNext = messToSend.front();
                messToSend.erase(messToSend.begin());
                listEdit.unlock();
                std::thread(sendAndWaitThread, sendNext).detach();
            }
            // Wait 1 second between sends so there's a smaller chance of unordered
            // std::this_thread::sleep_for(std::chrono::seconds(1));
            i ++;
            numSending ++;
            // packetNum ++;
        }
    }

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
