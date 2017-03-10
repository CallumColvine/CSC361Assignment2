extern "C" {
	#include <stdio.h>
	#include <errno.h>
	#include <string.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <unistd.h> /* for close() for socket */ 
	#include <stdlib.h>
	#include <arpa/inet.h>

}

int sendSock;
int recvSock;

void establishConnection(){
	recvSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in sa; 
	char buffer[1024];
	ssize_t recsize;
	socklen_t fromlen;

	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(8080);
	fromlen = sizeof(sa);

	if (-1 == bind(recvSock, (struct sockaddr *)&sa, sizeof sa)) {
	    perror("error bind failed");
	    close(recvSock);
	    exit(EXIT_FAILURE);
	}

	for (;;) {
	    recsize = recvfrom(recvSock, (void*)buffer, sizeof buffer, 0, 
	    		(struct sockaddr*)&sa, &fromlen);
	    if (recsize < 0) {
	        fprintf(stderr, "%s\n", strerror(errno));
	        exit(EXIT_FAILURE);
	    }
	    printf("datagram: %.*s\n", (int)recsize, buffer);
	    // break;
	}
}

void sendReply(){
  	struct sockaddr_in sa;
	int bytes_sent;
	char buffer[200];
	strcpy(buffer, "hello world!");
	/* create an Internet, datagram, socket using UDP */
	sendSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (-1 == sendSock) {
	    /* if socket failed to initialize, exit */
	    printf("Error Creating Socket");
	    exit(EXIT_FAILURE);
	}
	/* Zero out socket address */
	memset(&sa, 0, sizeof sa);
	/* The address is IPv4 */
	sa.sin_family = AF_INET;
	/* IPv4 adresses is a uint32_t, convert a string representation of the 
	octets to the appropriate value */
	// sa.sin_addr.s_addr = inet_addr("10.10.1.100");
	// sa.sin_addr.s_addr = inet_addr("10.0.2.255");
	sa.sin_addr.s_addr = inet_addr("10.0.2.15");
	// sa.sin_addr.s_addr = inet_addr("127.0.0.1");
	/* sockets are unsigned shorts, htons(x) ensures x is in network byte order, 
	set the port to 7654 */
	sa.sin_port = htons(8080);
	bytes_sent = sendto(sendSock, buffer, strlen(buffer), 0,
			(struct sockaddr*)&sa, sizeof sa);
	if (bytes_sent < 0) {
		printf("Error sending packet: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
	// close(sock); /* close the socket */
}

int main(int argc, char const *argv[])
{
	establishConnection();
	// sendReply();
	return 0;
}