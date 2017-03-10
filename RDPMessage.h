#ifndef RDPMESSAGE_H
#define RDPMESSAGE_H

#include <string>

class RDPMessage
{
  public:
  	std::string toString();
	RDPMessage();
	~RDPMessage();
  
  private:
  	std::string _magic; 	// "CSC361"
  	int _type;				// Type of packet. DAT, ACK, etc...
  	int _seqNum;			// Sequence Number
  	int _ackNum;			// Acknowledgement Number
  	int _length;			// Payload length
  	int _size;				// Window size
};

#endif