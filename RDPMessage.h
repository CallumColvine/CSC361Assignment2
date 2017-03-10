#ifndef RDPMESSAGE_H
#define RDPMESSAGE_H

#include <string>

class RDPMessage
{
  public:
  	std::string toString();
  	// Getters
  	std::string magic() { return _magic; }
  	int type(){ return _type; }
  	int seqNum(){ return _seqNum; }
  	int ackNum(){ return _ackNum; }
  	int length(){ return _length; }
  	int size(){ return _size; }
  	std::string message() { return _message; }
  	// Setters
  	void setMagic(std::string magicIn) { _magic = magicIn; }
  	void setType(int typeIn) { _type = typeIn; }
  	void setSeqNum(int seqNumIn) { _seqNum = seqNumIn; }
  	void setAckNum(int ackNumIn) { _ackNum = ackNumIn; }
  	void setLength(int lengthIn) { _length = lengthIn; }
  	void setSize(int sizeIn) { _size = sizeIn; }
  	void setMessage(std::string messageIn) { _message = messageIn; }
  	// Contructor/destructor
	RDPMessage();
	~RDPMessage();
  
  private:
  	std::string _magic; 	// "CSC361"
  	int _type;				// Type of packet. DAT, ACK, etc...
  	int _seqNum;			// Sequence Number
  	int _ackNum;			// Acknowledgement Number
  	int _length;			// Payload length
  	int _size;				// Window size
  	std::string _message;
};

#endif