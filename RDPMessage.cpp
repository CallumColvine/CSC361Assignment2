#include <iomanip>
#include <sstream>
#include <string> 
#include <cstring>
#include <iostream> 

#include "RDPMessage.h"


void RDPMessage::updateLength(){
	char messageString[1024];
    memset(messageString, '\0', sizeof(messageString));
    toCString(messageString);
	setLength(strlen(messageString));
}

std::string RDPMessage::toString(bool doPrint){ 
	// std::cout << "ACK val is " << ACK() << std::endl;
	std::ostringstream strStream;
	// Create StringStream so I can do std::setw() and such
	strStream << "Magic: " << magic() << std::endl;
	// Set Type
	strStream << "Type: " << std::setw(5) << std::setfill('0') << type() << std::endl;
	// Set Sequence Number
	strStream << "SeqNum: " << std::setw(6) << std::setfill('0') << seqNum() << std::endl;
	// Set Ack Number
	strStream << "AckNum: " <<  std::setw(6) << std::setfill('0') << ackNum() << std::endl;
	// Set Payload Len
	strStream << "Payload Len: " << std::setw(4) << std::setfill('0') << length() << std::endl;
	// Set Window Size
	strStream << "Window Size: " << std::setw(6) << std::setfill('0') << size() << std::endl;
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
	strStream << std::setw(6) << std::setfill('0') << seqNum();
	strMessage += strStream.str();
	strStream.str(std::string()); 	// Clear the StringStream for next var
	// Set Ack Number
	strStream << std::setw(6) << std::setfill('0') << ackNum();
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
	std::copy(wholeStr + 11, wholeStr + 17, copyBuffer);	
	setSeqNum(std::stoi(std::string(copyBuffer)));
	// Set Ack Num
    memset(copyBuffer, '\0', sizeof(copyBuffer));
	std::copy(wholeStr + 17, wholeStr + 23, copyBuffer);	
	setAckNum(std::stoi(std::string(copyBuffer)));
	// Set Payload Len
	memset(copyBuffer, '\0', sizeof(copyBuffer));
	std::copy(wholeStr + 23, wholeStr + 27, copyBuffer);	
	setLength(std::stoi(std::string(copyBuffer)));
	// Set Window Size
	memset(copyBuffer, '\0', sizeof(copyBuffer));
	std::copy(wholeStr + 27, wholeStr + 33, copyBuffer);	
	setSize(std::stoi(std::string(copyBuffer)));
	// std::cout << "Made it to end ?? setting" << std::endl;
	// ----- ToDo: Set Message Contents here
	// std::cout << "copyBuffer contents " << copyBuffer << std::endl;
}

RDPMessage::RDPMessage(){}

RDPMessage::~RDPMessage(){}
	
