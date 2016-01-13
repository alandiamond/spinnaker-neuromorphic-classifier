//####################################################################//
//SpinnIO - An Open-Source library for spike io to/from a spinnaker board																													
//
//																																																				//
//Copyright (c) <2015>, BABEL project, Plymouth University in collaboration with Manchester University																																								


#include "Threadable.h"
#include "DatabaseIF.h"
#include "SocketIF.h"
#include "EIEIOMessage.h"
#include <deque>
#include <list>
#include <map>
#include <pthread.h>

#ifndef SPINNIO_EIEIO_SENDER_H_
#define SPINNIO_EIEIO_SENDER_H_

using namespace std;

namespace spinnio
{

class EIEIOSender : public Threadable {
public:
	EIEIOSender(int dbListenPort, int sendPort, string injectionPopLabel, string ip, string dbPath);
	EIEIOSender(int sendPort,string ip, std::map<int, int>* idKeyMap);
	virtual ~EIEIOSender();
	int getSendQueueSize();
	SocketIF* getSocketPtr();
	std::map<int,int>* getIDKeyMap();
	void enableSendQueue(bool enable);
	void addSpikeToSendQueue(int);
	void closeSendSocket();
	bool start(); //override / extend
	bool started;

protected:
	void InternalThreadEntry();
	void initialise(int sendPort,string ip, std::map<int, int>* idKeyMap);
private:
	pthread_mutex_t send_mutex;
	pthread_cond_t cond;
	std::map<int, int> *idKeyMap;
	SocketIF *sendSocket;
	std::deque<int > * sendQueue;
	std::deque<int > * collectedSpikes;
	std::deque<int > spikeListA;
	std::deque<int > spikeListB;
	bool sendingEnabled;
	int sockfd_input;
	struct addrinfo hints;
	struct addrinfo* addr_info;
	bool bufferStateA;
	bool multiThreadWriting;

	int sendMessage(uint8_t*, int);
	int dispatchEIEIOMessage(spike * outputSpikes , unsigned char  spikesInPacket);
	void flipQueueBuffers();

};
}
#endif /* SPINNIO_EIEIO_SENDER_H_ */
