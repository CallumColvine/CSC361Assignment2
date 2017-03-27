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

#include <sstream>
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

RDPMessage lastMess;

std::mutex listEdit;
std::mutex ackNumEdit;
std::mutex editNumSend;
std::mutex logging;
// std::mutex lastSeqNumEdit;
std::vector<RDPMessage> messToSend;
std::vector<RDPMessage> prioritySend;
// volatile int lastSeqNum;
volatile int numSending = 0;
volatile bool rollOver = false;
int lastSeq = 0;

std::string sourceIP;
std::string sourcePort;
std::string destIP;
std::string destPort;

// total data bytes sent: 1165152
std::mutex totalDataEdit;
volatile uint totalData = 0;
// unique data bytes sent: 1048576
std::mutex uniqueDataEdit;
volatile uint uniqueData = 0;
// total data packets sent: 1166
std::mutex totalPacketsEdit;
volatile uint totalPackets = 0;
// unique data packets sent: 1049
std::mutex uniquePacketsEdit;
volatile uint uniquePackets = 0;
// SYN packets sent: 1
uint synPackets = 0;
// FIN packets sent: 1
uint finPackets = 0;
// RST packets sent: 0
uint rstPacketsSent = 0;
// ACK packets received: 1051
std::mutex ackPacketsEdit;
volatile uint ackPackets = 0;
// RST packets received: 0
uint rstPacketsRecv = 0;
// total time duration (second): 0.093
struct timeval startTime;
struct timeval endTime;
// struct timeval totalTime;

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
    logging.lock();
    char type = getType(typeIn);
    std::string timePart = getTime();
    std::cout << timePart << ' ' << sendOrRecv << ' ' << sourceIP << ':' << sourcePort <<
            ' ' << destIP << ':' << destPort << ' ' << type << ' ' <<
            seqOrAck << ' ' << payloadOrWinSize << std::endl;
    logging.unlock();
}

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

void finalPrint(){
    gettimeofday(&endTime, NULL);
    std::cout << "total data bytes sent: " << totalData << std::endl;
    std::cout << "unique data bytes sent: " << uniqueData << std::endl;
    std::cout << "total data packets sent: " << totalPackets << std::endl;
    std::cout << "unique data packets sent: " << uniquePackets << std::endl;
    std::cout << "SYN packets sent: " << synPackets << std::endl;
    std::cout << "FIN packets sent: " << finPackets << std::endl;
    std::cout << "RST packets sent: " << rstPacketsSent << std::endl;
    std::cout << "ACK packets received: " << ackPackets << std::endl;
    std::cout << "RST packets received: " << rstPacketsRecv << std::endl;
    std::cout << "total time duration (second): " <<
            endTime.tv_sec - startTime.tv_sec << std::endl;
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

void incrAcks(){
    ackPacketsEdit.lock();
    ackPackets ++;
    ackPacketsEdit.unlock();
}

struct sockaddr_in initSa(std::string recvIP, std::string recvPort){
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(recvIP.c_str());
    sa.sin_port = htons(stoi(recvPort));
    return sa;
}

void incrTotal(int bytesSent){
    totalDataEdit.lock();
    totalData += bytesSent;
    totalDataEdit.unlock();
    totalPacketsEdit.lock();
    totalPackets ++;
    totalPacketsEdit.unlock();
}

void incrUnique(int bytesSent){
    uniqueDataEdit.lock();
    uniqueData += bytesSent;
    uniqueDataEdit.unlock();
    uniquePacketsEdit.lock();
    uniquePackets ++;
    uniquePacketsEdit.unlock();
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
    logAction(messageOut.type(), messageOut.seqNum(), messageOut.length(),
              messageOut.timesSent(), 's');
    incrTotal(bytes_sent);
    // incrUnique(bytes_sent);
    synPackets ++;
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
        RDPMessage messageIn;
        messageIn.unpackCString(buffer);
        logAction(messageIn.type(), messageIn.ackNum(), messageIn.size(),
                  messageIn.timesSent(), 'r');
        if (messageIn.type() == 10) {
            incrAcks();
        }
        if (messageIn.type() == 10)
            ackPackets ++;
        lastAck = messageIn.ackNum();
        return messageIn.size();
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
    // Read in file
    FILE * inFileC = fopen(filename.c_str(), "r");
    int bytes_read = fread(fileContents, sizeof(char), fileLen, inFileC);
    if (bytes_read == 0)
        std::cout << "Input file is empty" << std::endl;
    // std::cout << "Read in num bytes " << bytes_read << std::endl;
    // std::cout << "Filecontents is " << fileContents << std::endl;
    return std::string(fileContents);
}

template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }



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
    // std::cout << "Setting thread to wait for ACK " << messageOut.seqNum()
    //         << std::endl;
    ssize_t recsize = recvfrom(sendSock, (void*)buffer, sizeof buffer, 0,
            (struct sockaddr*)&saOut, &fromlen);
    if (recsize < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(EXIT_FAILURE);
    }
    RDPMessage messageIn;
    messageIn.unpackCString(buffer);
    logAction(messageIn.type(), messageIn.ackNum(), messageIn.size(),
              messageIn.timesSent(), 'r');
    if (messageIn.type() == 10) {
        incrAcks();
    }
    ackNumEdit.lock();
    if ((int)lastAck + MAX_MESS_LEN > 10000000)
        rollOver = true;
    if (!rollOver)
        lastAck = std::max((int)lastAck % 10000000, messageIn.ackNum() % 10000000);
    else {
        lastAck = std::min((int)lastAck % 10000000, messageIn.ackNum() % 10000000);
        rollOver = false;
    }
    listEdit.lock();
    // std::cout << "Received ackNum " << messageIn.ackNum() << std::endl;
    if (messageOut.seqNum() > lastAck){
        // std::cout << "This packet requeue. SEQ # is > than last ACK: " <<
                // lastAck << std::endl;
        messToSend.insert(messToSend.begin(), messageOut);
    }
    // This is the next message that should send.
    else if (messageOut.seqNum() == lastAck) {
        // std::cout << "This packet's SEQ # is THE NEXT one to send. Queue as priority"
                // << std::endl;
        prioritySend.insert(prioritySend.begin(), messageOut);
    } else {
        // std::cout << "This packet was less than the last ACK. Let it go."
        //         << std::endl;
        ;
    }
    // ToDo: Remove self from list
    listEdit.unlock();
    ackNumEdit.unlock();
    editNumSend.lock();
    numSending --;
    editNumSend.unlock();
}

void threadTimedOut(RDPMessage messageOut){
    // std::cout << "Response timed out. Replace in queue " <<
    //         messageOut.seqNum() << " last acked SEQ is " << lastAck <<  std::endl;
    // Only add if it's not an alrwady acked seq
    if (!messageOut.seqNum() < lastAck) {
        listEdit.lock();
        prioritySend.insert(prioritySend.begin(), messageOut);
        listEdit.unlock();
    }
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
    logAction(messageOut.type(), messageOut.seqNum(), messageOut.length(),
              messageOut.timesSent(), 's');
    incrTotal(bytesSent);
    if (messageOut.timesSent() == 0) {
        incrUnique(bytesSent);
    }
    messageOut.incrTimesSent();
    // --- Wait for Reply Packet here ---
    // std::cout << "Packet sent. Bytes: " << bytesSent << " SEQ num " <<
    //         messageOut.seqNum() << std::endl;
    // Set a timer with timeout == 3 seconds
    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(sendSock, &set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 30 * 1000;
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

void mainSendLoop(){
    while (((!messToSend.empty()) || (!prioritySend.empty())) &&
    numSending < (FULL_WINDOW_SIZE / MAX_MESS_LEN)){
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
            // std::cout << "Sending the expected next packet" << std::endl;
            listEdit.lock();
            RDPMessage sendNext = messToSend.front();
            messToSend.erase(messToSend.begin());
            // ToDo stuff about rollover?
            // if (sendNext.seqNum() == 1000)
            listEdit.unlock();
            std::thread(sendAndWaitThread, sendNext).detach();
        }
        numSending ++;
    }
}


int fillMessageSendVec(std::string filename, int seqNum){
    // std::cout << "Commencing file send from seqNum " << seqNum << std::endl;
    // Max RDP packet size = 1024 bytes
    std::ifstream t(filename);
    std::string wholeFile;
    t.seekg(0, std::ios::end);
    wholeFile.reserve(t.tellg());
    t.seekg(0, std::ios::beg);
    wholeFile.assign((std::istreambuf_iterator<char>(t)),
    std::istreambuf_iterator<char>());
    int fileLen = wholeFile.length();
    // Loop through file sending parts until their expected buffer is full
    int dataReplySize = fileLen;
    if (fileLen > (MAX_MESS_LEN - HEADER_LENGTH))
    dataReplySize = MAX_MESS_LEN - HEADER_LENGTH;
    // std::cout << "Data reply size is " << dataReplySize << std::endl;
    for (int i = 0; i < fileLen; i += dataReplySize){
        std::string sendFilePart;
        if (!(i + dataReplySize > fileLen))
        sendFilePart = wholeFile.substr(i, dataReplySize);
        else {
            // std::cout << "!!! fileLen - i is " << fileLen - i << "the pack is " << seqNum << std::endl;
            sendFilePart = wholeFile.substr(i, fileLen - i);
            // std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        RDPMessage messageObj = prepFileMessage(seqNum, dataReplySize, sendFilePart);
        messToSend.push_back(messageObj);
        lastMess = messageObj;
        lastSeq = seqNum;
        seqNum += MAX_MESS_LEN;
    }
    lastMess.incrTimesSent();
    return fileLen;
}

void addFinPacket(int lastAck){
    RDPMessage finMessage;
    finMessage.setFIN(true);
    finMessage.setSeqNum(lastAck);
    messToSend.push_back(finMessage);
}

// winSize is the TOTAL receiver window
void sendFile(std::string filename, int winSize, int seqNum){
    int fileLen = fillMessageSendVec(filename, seqNum);
    // int totalToSend = messToSend.size();
    std::cout << "Full file length is " << fileLen << std::endl;
    // std::cout << "Sending " << totalToSend << " packets " << std::endl;
    // std::cout << "numSending " << numSending << " messToSend empty " <<
            // messToSend.empty() << " prioritySend empty " <<
            // prioritySend.empty() << std::endl;
    for(;;){
        // std::cout << "Looping main send loop " << messToSend.empty() <<
                // prioritySend.empty() << std::endl;
        // std::cout << "lastSeq is " << lastSeq << " lastAck is " << lastAck <<
        //         " uniqueSent is " << uniquePackets << " totalLen is " <<
        //         totalToSend << std::endl;
        mainSendLoop();
        // std::cout << "Unique packets " << uniquePackets - synPackets <<
        //         " totalToSend " << totalToSend << std::endl;
        if (lastSeq < lastAck) {
            // We think both lists are empty. May be, but might drop last packet
            break;
        }
        if (messToSend.empty() && prioritySend.empty()) {
            messToSend.push_back(lastMess);
            // break;
        }
    }
    std::cout << "Last few packets sending. Slow down to ensure accuracy... " <<
            std::endl;
    // May be actually empty, but might also could have lost last packet
    for (;;){
        // Wait enough time for the last ack to come if the packet is not lost
        std::this_thread::sleep_for(std::chrono::seconds(1));
        mainSendLoop();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        if (messToSend.empty() && prioritySend.empty()) {
            break;
        }
    }
    std::cout << "Added FIN packet to list, preparing to send" << std::endl;
    addFinPacket(lastAck);
    // for (;;){
    // Wait enough time for the last ack to come if the packet is not lost
    std::this_thread::sleep_for(std::chrono::seconds(1));
    mainSendLoop();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    finalPrint();
}

int main(int argc, char const *argv[])
{
    gettimeofday(&startTime, NULL);
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
    sourceIP = argv[1];
    sourcePort = argv[2];
    destIP = argv[3];
    destPort = argv[4];
    // Use known window size from handshake to send file
    sendFile(argv[5], winSize, message.seqNum() + 1);
    return 0;
}
