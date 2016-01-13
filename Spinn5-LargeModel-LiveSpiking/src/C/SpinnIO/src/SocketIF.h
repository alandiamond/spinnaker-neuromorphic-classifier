//####################################################################//
//SpinnIO - An Open-Source library for spike io to/from a spinnaker board																													
//
//																																																				//
//Copyright (c) <2015>, BABEL project, Plymouth University in collaboration with Manchester University																																								


#include <string>
#ifndef WIN32
#include <netdb.h>
#include <arpa/inet.h>
#else
#include <windows.h>
#include <ws2tcpip.h>
#include <winsock2.h>
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)
#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0)
#define close(sock)
typedef unsigned int uint;
typedef unsigned short ushort;
typedef int socklen_t;
#endif

#ifndef SPINNIO_SOCKETIF_H_
#define SPINNIO_SOCKETIF_H_

namespace spinnio
{

class SocketIF {
	int sockfd_input;
	struct sockaddr_in response_address;
	struct sockaddr_in si_other; // for incoming frames
	socklen_t addr_len_input;
public:
	virtual ~SocketIF();
	SocketIF(int);
	SocketIF(int, char*);
	int closeSocket();
	int getSocket();
	socklen_t getAddrLenInput();
	struct sockaddr_in getSiOther();
	void sendVoidMessage(char*, int);

private:
	void openSocket(int);
};
}
#endif /* SPINNIO_SOCKETIF_H_ */
