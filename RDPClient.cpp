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


int sendSock;
struct sockaddr_in saIn;
struct sockaddr_in saOut;
// Seems to be 36 atm (or 35, going with 36 though)
int HEADER_LENGTH = 0;

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
    /* Zero out socket address */
    memset(&sa, 0, sizeof sa);
    /* The address is IPv4 */
    sa.sin_family = AF_INET;
    /* IPv4 adresses is a uint32_t, convert a string representation of the
    octets to the appropriate value */
    // sa.sin_addr.s_addr = inet_addr("10.10.1.100");
    // sa.sin_addr.s_addr = inet_addr("10.0.2.255");
    // std::cout << "Initializing the IP I'm sending TO to be " << recvIP <<
    //      " and port to " << recvPort << std::endl;
    sa.sin_addr.s_addr = inet_addr(recvIP.c_str());
    /* sockets are unsigned shorts, htons(x) ensures x is in network byte order,
    set the port to 7654 */
    sa.sin_port = htons(stoi(recvPort));
    return sa;
}

void sendInitSyn(RDPMessage messageOut){
    int bytes_sent;

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

struct sockaddr_in createRecvSocket(std::string sendIP, std::string sendPort){
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(sendIP.c_str());
    sa.sin_port = htons(std::stoi(sendPort));
    // if (-1 == bind(sendSock, (struct sockaddr *)&sa, sizeof sa)){
    //       perror("error bind failed");
    //       close(sendSock);
    //       exit(EXIT_FAILURE);
    // }
    return sa;
}

int recvInitAck(std::string sendIP, std::string sendPort){
    // recvSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    char buffer[1024];
    // saIn = createRecvSocket(sendIP, sendPort);
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
    return recvInitAck(recvIP, recvPort);
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

// Finds all occurances of a specific ACK num and removes it from messToSend
// and prioritySend.

// void removeDuplicates(){
//  listEdit.lock();
//  for (uint j = 0; j < messToSend.size(); j++)
//      if (messToSend[j].seqNum() == ackNum)
//          messToSend.erase(messToSend.begin() + j);
//  for (uint i = 0; i < prioritySend.size(); i++)
//      if (prioritySend[i].seqNum() == ackNum)
//          prioritySend.erase(prioritySend.begin() + i);
//  listEdit.unlock();
// }

template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

std::vector<RDPMessage> messToSend;
std::vector<RDPMessage> prioritySend;

std::mutex listEdit;
std::mutex winSizeEdit;
std::mutex ackNumEdit;
// Could be winSize / maxPackSize, ie. 10
// volatile std::vector<RDPMessage> packWaitAckList;
// std::vector<RDPMessage> packWaitAckList;
volatile int senderWindowSize = FULL_WINDOW_SIZE;
volatile int sendNext = -1;
volatile int expectedAckNum = 0;
volatile int guessSent = 0;
volatile int lastAck = 0;
volatile int lastSize = 0;

void filterList(int ackNum){
    listEdit.lock();
    for (uint j = 0; j < messToSend.size(); j++)
        if (messToSend[j].seqNum() == ackNum)
            messToSend.erase(messToSend.begin() + j);
    for (uint i = 0; i < prioritySend.size(); i++)
        if (prioritySend[i].seqNum() == ackNum)
            prioritySend.erase(prioritySend.begin() + i);
    listEdit.unlock();
}

// returns -1 on thread ACK
// returns seqNum on timed out message
int sendAndWaitThread(RDPMessage messageObj){
    // --- Send Packet ---
    char fullReply[MAX_MESS_LEN];
    memset(fullReply, '\0', sizeof(fullReply));
    messageObj.toCString(fullReply);
    int bytesSent = sendto(sendSock, fullReply, strlen(fullReply), 0,
                                    (struct sockaddr*)&saOut, sizeof saOut);
    // --- Wait for Reply Packet here ---
    std::cout << "Packet sent. Bytes: " << bytesSent << " SEQ num " << 
            messageObj.seqNum() << std::endl;
    // Set a timer with timeout == 3 seconds
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    fd_set fdRead;
    FD_ZERO(&fdRead);
    FD_SET(sendSock, &fdRead);
    int retval = select(sendSock + 1, &fdRead, NULL, NULL, &timeout);
    if (retval < 0){
        std::cout << "retval is " << retval << std::endl;
        std::cout << "Response timed out. Re-send by prioritizing " <<
                messageObj.seqNum() << std::endl;
        // Only add if not alread in list
        listEdit.lock();
        prioritySend.insert(prioritySend.begin(), messageObj);
        listEdit.unlock();
        return bytesSent;
    } else {
        std::cout << "retval is " << retval << std::endl;
        char buffer[1024];
        socklen_t fromlen = sizeof(saOut);
        std::cout << "Setting thread to wait for ACK " << messageObj.seqNum() << std::endl;
        ssize_t recsize = recvfrom(sendSock, (void*)buffer, sizeof buffer, 0,
                (struct sockaddr*)&saOut, &fromlen);
        if (recsize < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
        std::cout << "!!! RECEIVED REPLY FROM SERVER " << std::endl;
        RDPMessage temp;
        temp.unpackCString(buffer);

        ackNumEdit.lock();
        if (temp.seqNum() > lastAck)
        {
            lastAck = temp.seqNum();
            lastSize = bytesSent;
        }
        winSizeEdit.lock();
        senderWindowSize = temp.size();
        if (expectedAckNum == temp.seqNum())
        {
            std::cout << "- Packet was acknowledged with expected ACK num " << 
                    temp.ackNum() << std::endl;
            filterList(expectedAckNum - bytesSent);
            expectedAckNum += bytesSent;
        } else {
            std::cout << "- Packet unexpected ACK " << expectedAckNum <<
                    " it received " << temp.seqNum() << "Prioritizing" <<
                    " the re-send of the expected packet" << std::endl;
            // Only add if not alread in list
            prioritySend.insert(prioritySend.begin(), messageObj);
        }
        // ToDo: Remove self from list
        winSizeEdit.unlock();
        ackNumEdit.unlock();
    }
    // else {

    return bytesSent;

    // if (expectedAckNum == temp.seqNum())
    // {
    //  std::cout << "- Packet was acknowledged with expected ACK num" << std::endl;
    //  for (uint j = 0; j < messToSend.size(); j++)
    //  {
    //      if (messToSend[j].seqNum() == sendNext)
    //      {
    //          std::cout << "Removing ACK-ed packet from list " << std::endl;
    //          listEdit.lock();
 //                 guessSent -= MAX_MESS_LEN;
    //          messToSend.erase(messToSend.begin() + j);
    //          listEdit.unlock();
    //      }
    //  }
    //  expectedAckNum += bytesSent;
    // } else {
    //  std::cout << "Packet did NOT have expected ACK num " << expectedAckNum <<
    //          " it received " << temp.seqNum() << "Prioritizing" <<
    //          " the re-send of the expected packet" << std::endl;
    //  sendNext = temp.seqNum();
    // }
    // ackNumEdit.unlock();
    // // ToDo: Remove self from list
    // winSizeEdit.unlock();
    // return bytesSent;


        // return messageObj.seqNum();
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
        expectedAckNum = seqNum + dataReplySize + HEADER_LENGTH;
        for (int i = 0; i < fileLen; i += dataReplySize){
            std::string sendFilePart = wholeFile.substr(i, dataReplySize);
            RDPMessage messageObj = prepFileMessage(seqNum, dataReplySize, sendFilePart);
            messToSend.push_back(messageObj);
            seqNum += MAX_MESS_LEN;
        }
        int i = 0;
        for (;;){
        // While there's still file to go and while their buff is not full
            // while (i < fileLen && senderWindowSize <= FULL_WINDOW_SIZE && guessSent
                // < FULL_WINDOW_SIZE){
            // while (!messToSend.empty() && guessSent < winSize){
            while (((!messToSend.empty()) || (!prioritySend.empty())) && (senderWindowSize > 0)){
                senderWindowSize -= MAX_MESS_LEN;
                guessSent += MAX_MESS_LEN;
                std::cout << "Looping. File len " << fileLen << " data reply size "
                        << dataReplySize << " packet num " << i << std::endl;
                // Filter list
                ackNumEdit.lock();
                filterList(lastAck - lastSize);
                ackNumEdit.unlock();
                // Send item
                if (!prioritySend.empty())
                {
                    std::cout << "Sending a priority packet " << std::endl;
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
                std::this_thread::sleep_for(std::chrono::seconds(1));
                i ++;
                // packetNum ++;
            }
    }
    //       while (!messToSend.empty() && senderWindowSize > 0){
    //          guessSent += MAX_MESS_LEN;
    //          std::cout << "Looping. File len " << fileLen << " data reply size "
    //                  << dataReplySize << " packet num " << i << std::endl;
    //          if (sendNext != -1)
    //          {
    //              std::cout << "Received specific request for a packet. One " <<
    //                      "must have arrived out of order. Sending... " << std::endl;
    //          for (uint j = 0; j < messToSend.size(); j++)
    //          {
    //              if (messToSend[j].seqNum() == sendNext)
    //              {
    //                  std::cout << "Found the packet that was lost. " <<
    //                          "Resending " << j << std::endl;
    //                      std::thread(sendAndWaitThread, messToSend[j]).detach();
    //                      sendNext = -1;
    //              }
    //          }
    //          } else {
    //              std::cout << "Sending the expected next packet" << std::endl;
    //              std::thread(sendAndWaitThread, messToSend[i]).detach();
    //          }
    //          // Wait 1 second between sends so there's a smaller chance of unordered
    //          std::this_thread::sleep_for(std::chrono::seconds(1));
    //          i ++;
    //          // packetNum ++;
    //       }
    // }
        // ToDo: Try making identical loop to re-join all the threads
        // This means we need to re-send the package with the
        // Change to while loop for infinite fix?
        // listEdit.lock();
        // Error is from calling get() too many times
        // You can ONLY call get() once
        // Why don't I default it to the ackNum so it only falls into the loop once
        // std::cout << "Is ready result is " << is_ready(p) << std::endl;
 //     if (retAck != -1)
 //     {
 //         for (uint i = 0; i < packWaitAckList.size(); ++i)
 //         {
 //             if (packWaitAckList[i].seqNum() == retAck)
 //             {
 //                 std::cout << "Resending packWaitAckList[i]" << std::endl;
    //              // std::future<int> p = std::async(sendAndWaitThread, messageObj);
 //                 // p.get();
    //              // std::cout << "My thread promise is " << retAck << std::endl;
 //             }
 //         }
 //     }
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
