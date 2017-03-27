CXX=g++

CXXFLAGS = -Wall -g -std=c++11
LDFLAGS = -lpthread

OBJS = RDPClient.o RDPServer.o

all: a2

a2: $(OBJS)
	$(CXX) $(CXXFLAGS) -o rdpc RDPClient.cpp RDPMessage.cpp RDPMessage.h $(LDFLAGS)
	$(CXX) $(CXXFLAGS) -o rdps RDPServer.cpp RDPMessage.cpp RDPMessage.h $(LDFLAGS)

clean:
	rm -rf $(OBJS) rdpc rdps
