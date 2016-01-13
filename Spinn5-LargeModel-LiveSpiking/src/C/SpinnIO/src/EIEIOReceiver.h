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

#ifndef SPINNIO_EIEIO_RECEIVER_H_
#define SPINNIO_EIEIO_RECEIVER_H_

using namespace std;

namespace spinnio
{

class EIEIOReceiver : public Threadable {
public:
	EIEIOReceiver(int, string spinnPopLabel, char*, bool, char*);
	EIEIOReceiver(int, char*, std::map<int,int>*);
	virtual ~EIEIOReceiver();
        int getRecvQueueSize();
        bool hasQueuedSpikes();
        deque<pair<int, int> > getNextSpikePacket();
        void queueSpikePacket(deque<pair<int, int> > data);
        SocketIF* getSocketPtr();
        std::map<int,int>* getKeyIDMap();
        void closeRecvSocket();
        void disable();

protected:
	void InternalThreadEntry();
        void convertEIEIOMessage(eieio_message*, deque<pair<int, int> > &);
private:
	bool convertKeysToNeuronId;
	pthread_mutex_t recvr_mutex;
	pthread_cond_t cond;
        DatabaseIF *dbConnection;
        std::map<int, int> *keyIDMap;
        SocketIF *recvSocket;
        std::deque<deque<pair<int, int> > > recvQueue;
        int sockfd_input;
	struct sockaddr_in si_other;
	socklen_t addr_len_input;
	const static int SIZE_OF_KEY = 4;
	bool enabled;
        
};
}
#endif /* SPINNIO_EIEIO_RECEIVER_H_ */
