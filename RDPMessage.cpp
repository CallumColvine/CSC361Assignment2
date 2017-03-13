#include <iomanip>
#include <sstream>
#include <string> 
#include <cstring>
#include <iostream> 

#include "RDPMessage.h"


std::string RDPMessage::toString(){ return " "; }

const char* RDPMessage::toCString(char* wholeStr){ 
	std::string strMessage = magic(); 
	// Create StringStream so I can do std::setw() and such
	std::ostringstream strStream;
	// Set Type
	strStream << std::setw(5) << std::setfill('0') << type();
	strMessage += strStream.str();
	std::cout << "type val is " << type() << std::endl;
	// std::cout << "type val is " << SYN << std::endl;
	std::cout << "type is " << strMessage << std::endl;
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
	std::cout << "Sending full message: " << strMessage << std::endl;
	// 128 long for the 16 bit seq num
	char seqNumStr[128];
	sprintf(seqNumStr, "%d", seqNum());
	// char* wholeStr = magic().c_str() + 
	// char wholeStr[2048];
	strcat(wholeStr, magic().c_str());
	strcat(wholeStr, seqNumStr);
	return wholeStr;
}

RDPMessage::RDPMessage(){}

RDPMessage::~RDPMessage(){}
	
