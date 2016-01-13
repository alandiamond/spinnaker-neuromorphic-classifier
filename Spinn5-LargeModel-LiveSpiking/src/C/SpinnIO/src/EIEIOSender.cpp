//####################################################################//
//SpinnIO - An Open-Source library for spike io to/from a spinnaker board																													
//
//																																																				//
//Copyright (c) <2015>, BABEL project, Plymouth University in collaboration with Manchester University

#include "EIEIOSender.h"
#include "spinnio_utilities.cc"
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <string.h>
#include <unistd.h>



using namespace std;

namespace spinnio
{


void EIEIOSender::flipQueueBuffers(){
	if (bufferStateA) {
		collectedSpikes = & spikeListB;
		sendQueue = & spikeListA;
		bufferStateA = false;
	} else {
		collectedSpikes = & spikeListA;
		sendQueue = & spikeListB;
		bufferStateA = true;
	}
}

//override / extend threadble start
bool EIEIOSender::start() {
	if (started) {
		cerr << "EIEIOSender  - Warning: called start when already started" << endl;
		return true;
	} else {
		Threadable::start(); //call super class
		started  = true;
	}
}

void EIEIOSender::initialise(int sendPort,string ip, std::map<int, int>* _idKeyMap) {


	this->idKeyMap = _idKeyMap;
	//cout << toString(*idKeyMap, NL) << endl;


	flipQueueBuffers();//initialise double buffering


	cout << "Creating EIEIO sender to spiNNaker with port " << sendPort << " and IP  " <<  ip << endl;

	if (pthread_mutex_init(&this->send_mutex, NULL) == -1) {
		fprintf(stderr, "Error initializing send mutex!\n");
		exit(-1);
	}
	pthread_cond_init(&this->cond, 0);


	// set up socket to send packets
	sendSocket = new SocketIF(sendPort);
	this->sockfd_input = sendSocket->getSocket();
	// setup socket for send
	memset(&this->hints,0,sizeof(this->hints));
	this->hints.ai_family = AF_UNSPEC;
	this->hints.ai_socktype = SOCK_DGRAM;
	this->hints.ai_protocol = 0;
	this->hints.ai_flags = 0;
	this->addr_info = 0;

	if (getaddrinfo(ip.c_str(), NULL, &(this->hints), &(this->addr_info)) != 0) {
		fprintf(stderr, "Could not resolve hostname, exiting\n");
		exit(-1);
	}
	((struct sockaddr_in *) this->addr_info->ai_addr)->sin_port = htons(sendPort);

	this->sendingEnabled = false;
	started = false;

}

// Standard constructor where socket is created for send
// and toolchain handshake is done to get back key-id mappings
EIEIOSender::EIEIOSender(int dbListenPort, int sendPort, string injectionPopLabel, string ip,  string dbPath):
				 sendingEnabled(false), bufferStateA(true), multiThreadWriting(false)
{
	DatabaseIF dbConnection(dbListenPort, (char * )dbPath.c_str());
	if (dbConnection.isReady()){
		// get key-id and id-key mappings
		map<int,int> *  idKeyMap = dbConnection.getIdKeyMap(injectionPopLabel);
		dbConnection.finish();
		cout << "Obtained key-id mapping from database succesfully." << endl;
		initialise(sendPort,ip, idKeyMap);
		cout << "Initialised EIEIOSender." << endl;
	} else {
		cerr << "Failed to complete database read" << endl;
		exit(-1);
	}

}



// Alternate constructor without DB handshake for when this has already been done
// elsewhere. Accepts the ID-Key map as an argument

EIEIOSender::EIEIOSender(int sendPort,string ip, std::map<int, int>* idKeyMap):
			sendingEnabled(false), bufferStateA(true), multiThreadWriting(false)
{
	initialise(sendPort,ip, idKeyMap);
}

// Generate and send a packet
int EIEIOSender::dispatchEIEIOMessage(spike * outputSpikes , unsigned char spikesInPacket) {
	EIEIOMessage* tmpMsg = new EIEIOMessage(uint8_t(spikesInPacket), BASIC, TYPE_32_K, 0, false, false, false);
	int byteCount = tmpMsg->writeMessage(outputSpikes, spikesInPacket);
	int result = this->sendMessage(tmpMsg->getMessageBuffer(),byteCount);
	//cout << "Dispatched EIEIOMessage containing " << ((int)spikesInPacket) << " spikes" << endl;
	delete tmpMsg; //surely
}


void EIEIOSender::InternalThreadEntry(){

	spike outputSpikes[MAX_SPIKE_COUNT];

	while (1) {

		// check if the queue is in use and has something in it
		// before attempting to take spikes from it
		if (sendingEnabled) {
			unsigned int numSpikes  = collectedSpikes->size();
			if (numSpikes > 0){
				//cout << "Found " << numSpikes << " spikes to send" << endl;
				int spikesInPacket = 0;
				// keep adding spikes to the UDP container up to max or until no spikes left to send
				for (unsigned int i = 0; i < numSpikes; ++i) {
					// get the next spike from the queue and convert to a key
					int id = (* collectedSpikes)[i];
					outputSpikes[spikesInPacket].key = (*(this->idKeyMap))[id] & OUT_MASK;
					spikesInPacket++;
					if (spikesInPacket==MAX_SPIKE_COUNT) { //packet full, send it out
						int result  = dispatchEIEIOMessage(outputSpikes , spikesInPacket);
						spikesInPacket=0;
					}
				}
				//send remaining spikes if any
				if (spikesInPacket > 0) dispatchEIEIOMessage(outputSpikes , spikesInPacket);
				collectedSpikes->clear();
			}
		//flip queue buffers
		flipQueueBuffers();
		}
	}
}


int EIEIOSender::getSendQueueSize(){
	int size = 0;
	pthread_mutex_lock(&this->send_mutex);
	size = this->sendQueue->size();
	pthread_cond_signal(&this->cond);
	pthread_mutex_unlock(&this->send_mutex);

	return size;

}

SocketIF* EIEIOSender::getSocketPtr(){

	return this->sendSocket;


}



std::map<int, int> *EIEIOSender::getIDKeyMap() {

	return this->idKeyMap;

}


void EIEIOSender::closeSendSocket(){

	this->sendSocket->closeSocket();
	cout <<  "Closed send socket " << sendSocket->getSocket() << endl;
}

void EIEIOSender::addSpikeToSendQueue(int neuronID){

	if (multiThreadWriting) { //synchronise access to queue
		pthread_mutex_lock(&this->send_mutex);
		this->sendQueue->push_back(neuronID);
		pthread_cond_signal(&this->cond);
		pthread_mutex_unlock(&this->send_mutex);
	} else {
		this->sendQueue->push_back(neuronID);
	}


}

void EIEIOSender::enableSendQueue(bool enable){

	this->sendingEnabled = enable;
	//If disabling then wait a little for thread to exit current iteration( see InternalThreadEntry())
	if (!enable) usleep(1000);



}

int EIEIOSender::sendMessage(uint8_t* data, int dataSize) {

	int result  = sendto(this->sockfd_input, data, dataSize, 0,
			this->addr_info->ai_addr, this->addr_info->ai_addrlen);
		if (result < 0) {
			cerr <<  "Failed to send packet (sendto() return value " << result << ")" << endl;
		}
	return result;
}



EIEIOSender::~EIEIOSender() {
	enableSendQueue(false);
	closeSendSocket();
	//stop(); //stop socket thread
	delete idKeyMap;
}
}
