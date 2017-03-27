#include <iomanip>
#include <sstream>
#include <string>
#include <cstring>
#include <iostream>

#include "RDPMessage.h"




// void RDPMessage::updateLength(){
// 	char messageString[MAX_MESS_LEN];
//     memset(messageString, '\0', sizeof(messageString));
//     toCString(messageString);
// 	setLength(strlen(messageString));
// }

std::string RDPMessage::toString(bool doPrint){
	// std::cout << "ACK val is " << ACK() << std::endl;
	std::ostringstream strStream;
	// Create StringStream so I can do std::setw() and such
	strStream << "Magic: " << magic() << std::endl;
	// Set Type
	strStream << "Type: " << std::setw(5) << std::setfill('0') << type() << std::endl;
	// Set Sequence Number
	strStream << "SeqNum: " << std::setw(7) << std::setfill('0') << seqNum() << std::endl;
	// Set Ack Number
	strStream << "AckNum: " <<  std::setw(7) << std::setfill('0') << ackNum() << std::endl;
	// Set Payload Len
	strStream << "Payload Len: " << std::setw(4) << std::setfill('0') << length() << std::endl;
	// Set Window Size
	strStream << "Window Size: " << std::setw(6) << std::setfill('0') << size() << std::endl;
	// Set Message Contents
	strStream << "Message Contents: " << message() << std::endl;
	// // ----- ToDo: Print Message Contents here
	if (doPrint)
		std::cout << strStream.str() << std::endl;
	return strStream.str();
}

void RDPMessage::toCString(char* wholeStr){
	std::string strMessage = magic();
	// Create StringStream so I can do std::setw() and such
	std::ostringstream strStream;
	// Set Type
	strStream << std::setw(5) << std::setfill('0') << type();
	strMessage += strStream.str();
	// std::cout << "type val is " << type() << std::endl;
	// std::cout << "type val is " << SYN << std::endl;
	// std::cout << "type is " << strMessage << std::endl;
	strStream.str(std::string()); 	// Clear the StringStream for next var
	// Set Sequence Number
	strStream << std::setw(7) << std::setfill('0') << seqNum();
	strMessage += strStream.str();
	strStream.str(std::string()); 	// Clear the StringStream for next var
	// Set Ack Number
	strStream << std::setw(7) << std::setfill('0') << ackNum();
	strMessage += strStream.str();
	strStream.str(std::string()); 	// Clear the StringStream for next var
	// Set Payload Len
	strStream << std::setw(4) << std::setfill('0') << length();
	strMessage += strStream.str();
	strStream.str(std::string()); 	// Clear the StringStream for next var
	// Set Window Size
	strStream << std::setw(6) << std::setfill('0') << size();
	strMessage += strStream.str();
	strStream.str(std::string()); 	// Clear the StringStream for next var
	strMessage += '\n';
	strStream << message();
	strMessage += strStream.str();
	strStream.str(std::string()); 	// Clear the StringStream for next var
	// ----- ToDo: Set Message Contents here
	// std::cout << "Full message size: " << strMessage.length() << std::endl;
	// 128 long for the 16 bit seq num
	strcat(wholeStr, strMessage.c_str());
}

void RDPMessage::unpackCString(char* wholeStr){
	// std::cout << "Message whole length is " <<  << std::endl;
	char copyBuffer[16];
	// Set Magic
    memset(copyBuffer, '\0', sizeof(copyBuffer));
	std::copy(wholeStr + 0, wholeStr + 6, copyBuffer);
	setMagic(std::string(copyBuffer));
	// Set Type
    memset(copyBuffer, '\0', sizeof(copyBuffer));
	std::copy(wholeStr + 6, wholeStr + 11, copyBuffer);
	setType(std::stoi(std::string(copyBuffer)));
	// Set Seq Num
    memset(copyBuffer, '\0', sizeof(copyBuffer));
	std::copy(wholeStr + 11, wholeStr + 18, copyBuffer);
	setSeqNum(std::stoi(std::string(copyBuffer)));
	// Set Ack Num
    memset(copyBuffer, '\0', sizeof(copyBuffer));
	std::copy(wholeStr + 18, wholeStr + 25, copyBuffer);
	setAckNum(std::stoi(std::string(copyBuffer)));
	// Set Payload Len
	memset(copyBuffer, '\0', sizeof(copyBuffer));
	std::copy(wholeStr + 25, wholeStr + 29, copyBuffer);
	setLength(std::stoi(std::string(copyBuffer)));
	// Set Window Size
	memset(copyBuffer, '\0', sizeof(copyBuffer));
	std::copy(wholeStr + 29, wholeStr + 35, copyBuffer);
	setSize(std::stoi(std::string(copyBuffer)));
	// std::cout << "Made it to end ?? setting" << std::endl;
	// ----- ToDo: Set Message Contents here
	char messageCopyBuff[MAX_MESS_LEN];
	memset(messageCopyBuff, '\0', sizeof(messageCopyBuff));
	// Should be the first char after the newline. Needs verification
	// std::cout << "----------------- WholeStr is ----------------- \n" <<
	// 		wholeStr << std::endl;
	if (length() > 0)
	{
		// std::cout << "length is " << length() << std::endl;
		// std::cout << "Copying from " << 34 << " to " << (34 + length()) << std::endl;
		// 40 Is the arbitrary header length
		// ToDo: Tentative solution, how come this isn't too far? But alright
		std::copy(wholeStr + 36, wholeStr + MAX_MESS_LEN, messageCopyBuff);
		setMessage(std::string(messageCopyBuff));
		// std::cout << "MessageCopyBuff contents are " << messageCopyBuff << std:
		// std::cout << "Copying from " << 34 << " to " << (34 + l:endl;
	}
	// std::cout << "copyBuffer contents " << copyBuffer << std::endl;
}

RDPMessage::RDPMessage(){}

RDPMessage::~RDPMessage(){}
