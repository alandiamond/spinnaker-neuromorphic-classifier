//####################################################################//
//SpinnIO - An Open-Source library for spike io to/from a spinnaker board																													
//
//																																																				//
//Copyright (c) <2015>, BABEL project, Plymouth University in collaboration with Manchester University																																								


#include <stdio.h>
#include <iostream>
#include <string.h>
#include <cstdlib>
#include <unistd.h>
#include "SocketIF.h"

using namespace std;


namespace spinnio
{

SocketIF::~SocketIF() {
}

// Constructor accepting just port for general 
// connection

SocketIF::SocketIF(int socketPort) {
	this->addr_len_input = sizeof(this->si_other);
    cout << "Creating socket with port " << socketPort << endl;

	openSocket(socketPort);

}

// Constructor with port and IP is for the
// board connection to receive from SpiNNaker.
// It does a sendVoidMessage on port 17893
// to ensure SpiNNaker can send


SocketIF::SocketIF(int socketPort, char *ip) {
	this->addr_len_input = sizeof(this->si_other);
        cout << "Creating socket with port " << socketPort << endl;
	openSocket(socketPort);
        cout << "Checking hostname " << ip << endl;
	if (ip != NULL) {
	    sendVoidMessage(ip, 17893);
	}

}

int SocketIF::getSocket(){

   return this->sockfd_input;

}

socklen_t SocketIF::getAddrLenInput(){

   return this->addr_len_input;
}

sockaddr_in SocketIF::getSiOther(){

   return this->si_other;

}

void SocketIF::openSocket(int port) {
    char portno_input[6];
    cout << "Opening socket on port " << port << endl;
    snprintf(portno_input, 6, "%d", port);
    struct addrinfo hints_input;
    bzero(&hints_input, sizeof(hints_input));
    hints_input.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints_input.ai_socktype = SOCK_DGRAM; // type UDP (socket datagram)
    hints_input.ai_flags = AI_PASSIVE; // use my IP

    //struct timeval tv;
    //tv.tv_sec = 10;  /* 10 Secs Timeout */
    //tv.tv_usec = 0;  // Not init'ing this can cause strange errors

    int rv_input;
    struct addrinfo *servinfo_input;
    if ((rv_input = getaddrinfo(NULL, portno_input, &hints_input,
            &servinfo_input)) != 0) {
        cerr << "getaddrinfo: " << gai_strerror(rv_input) << endl;
        exit(1);
    }

    // loop through all the results and bind to the first we can
    struct addrinfo *p_input;
    for (p_input = servinfo_input; p_input != NULL; p_input =
            p_input->ai_next) {
        if ((this->sockfd_input = socket(p_input->ai_family,
                                         p_input->ai_socktype,
                                         p_input->ai_protocol))
                == -1) {
            cout << "SocketIF: socket" << endl;
            perror("SocketIF: socket");
            continue;
        }

        //setsockopt(this->sockfd_input, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

        if (bind(this->sockfd_input, p_input->ai_addr,
                 p_input->ai_addrlen) == -1) {
            close(this->sockfd_input);
            cout << "SocketIF: bind" << endl;
            perror("SocketIF: bind");
            continue;
        }

        break;
    }

    if (p_input == NULL) {
        cerr << "SocketIF: failed to bind socket" << endl;
        exit(-1);
    }

    freeaddrinfo(servinfo_input);

    cout << "Socket opened." << endl;
}

void SocketIF::sendVoidMessage(char *hostname, int port) {
    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;
    struct addrinfo* addr_info = 0;
    if (getaddrinfo(hostname, NULL, &hints, &addr_info) != 0) {
        cerr << "Could not resolve hostname, exiting" << endl;
        exit(-1);
    }
    ((struct sockaddr_in *) addr_info->ai_addr)->sin_port = htons(port);

    char data [1];
    if (sendto(this->sockfd_input, data, 1, 0,
        addr_info->ai_addr, addr_info->ai_addrlen) == -1) {
        cerr << "Could not send packet" << endl;
        exit(-1);
    }
    cout << "IP check OK" << endl;
}



int  SocketIF::closeSocket(){
	int result  = close(this->sockfd_input);
	if (result < 0) {
		cerr << "WARNING: SocketIF failed to close socket" << endl;
	} else {
		cout << "SocketIF: socket closed ok." << endl;
	}
	return result;
}


}// namespace spinnio
