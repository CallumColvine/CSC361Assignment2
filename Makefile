CXX=g++

CXXFLAGS = -Wall -g -std=c++11
LDFLAGS = -lpthread

OBJS = RDPClient.o RDPServer.o

all: a2

a2: $(OBJS)
	$(CXX) $(CXXFLAGS) -o RDPClient RDPClient.cpp RDPMessage.cpp RDPMessage.h $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -o RDPServer RDPServer.cpp RDPMessage.cpp RDPMessage.h $(LDFLAGS)

clean: 
	rm -rf $(OBJS) RDPClient RDPServer
