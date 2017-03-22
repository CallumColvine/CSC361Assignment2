#ifndef RDPMESSAGE_H
#define RDPMESSAGE_H

#include <string>
#include <cmath>

// DAT = 00001
// ACK = 00010
// SYN = 00100
// FIN = 01000
// RST = 10000

#define MAX_MESS_LEN 1024


class RDPMessage
{
  public:
    // void updateLength();
  	std::string toString(bool doPrint);
    void toCString(char* wholeStr);
    void unpackCString(char* wholeStr);
  	// Getters
    int seqNumLen() { return _seqNumLen; }
  	std::string magic() { return _magic; }
    // User must 0-pad the int themselves since the interger 0 values disappear
  	int type(){ 
        if (DAT())
            return 1; 
        else if (ACK())
            return 10;
        else if (SYN())
            return 100;
        else if (FIN())
            return 1000;
        else if (RST())
            return 10000;
        else 
          return -1;
    }
  	int seqNum(){ return _seqNum; }
  	int ackNum(){ return _ackNum; }
  	int length(){ return _length; }              // Payload length
  	int size(){ return _size; }                  // Window size
  	std::string message() { return _message; }
    bool DAT() { return _dat; }
    bool ACK() { return _ack; }
    bool SYN() { return _syn; }
    bool FIN() { return _fin; }
    bool RST() { return _rst; }
  	// Setters
  	void setMagic(std::string magicIn) { _magic = magicIn; }
  	void setType(int typeIn) { 
        if (typeIn == 1)
            setDAT(true);
        else if (typeIn == 10)
            setACK(true);
        else if (typeIn == 100)
            setSYN(true);
        else if (typeIn == 1000)
            setFIN(true);
        else if (typeIn == 10000)
            setRST(true);
    }
  	void setSeqNum(int seqNumIn) { _seqNum = seqNumIn; }
  	void setAckNum(int ackNumIn) { _ackNum = ackNumIn; }
  	void setLength(int lengthIn) { _length = lengthIn; }
  	void setSize(int sizeIn) { _size = sizeIn; }
  	void setMessage(std::string messageIn) { _message = messageIn; }
    void setDAT(bool isTrue) { _dat = isTrue; }
    void setACK(bool isTrue) { _ack = isTrue; }
    void setSYN(bool isTrue) { _syn = isTrue; }
    void setFIN(bool isTrue) { _fin = isTrue; }
    void setRST(bool isTrue) { _rst = isTrue; }
  	// Contructor/destructor
	  RDPMessage();
	  ~RDPMessage();
  
  private:
    int _seqNumLen = pow(2, 16) - 1;
  	std::string _magic = "CSC361"; 	// "CSC361"
    bool _dat = false;
    bool _ack = false;
    bool _syn = false;
    bool _fin = false;
    bool _rst = false;
  	int _type;				// Type of packet. DAT, ACK, etc...
  	int _seqNum;			// Sequence Number
  	int _ackNum;			// Acknowledgement Number
  	int _length;			// Payload length
  	int _size;				// Window size
  	std::string _message;
};

#endif