#include "RDPMessage.h"

#include <cstring>

std::string RDPMessage::toString(){ return " "; }
const char* RDPMessage::toCString(char* wholeStr){ 
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
	
