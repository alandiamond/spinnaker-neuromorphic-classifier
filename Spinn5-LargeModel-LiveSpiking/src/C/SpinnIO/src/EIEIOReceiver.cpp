//####################################################################//
//SpinnIO - An Open-Source library for spike io to/from a spinnaker board																													
//
//																																																				//
//Copyright (c) <2015>, BABEL project, Plymouth University in collaboration with Manchester University

#include "EIEIOReceiver.h"
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <string.h>
#include <unistd.h>

using namespace std;

namespace spinnio
{
// Standard constructor where socket is created for receive
// and toolchain handshake is done to get back key-id mappings
EIEIOReceiver::EIEIOReceiver(int spinnPort, string spinnPopLabel, char* ip, bool dbConn, char* dbPath):
					convertKeysToNeuronId(false), enabled(true)
{

	cout << "Creating EIEIO receiver from spiNNaker with port " << spinnPort << " and ip " << ip << endl;

	if (pthread_mutex_init(&this->recvr_mutex, NULL) == -1) {
		cerr << "Error initializing recvr mutex!" << endl;
		exit(-1);
	}
	pthread_cond_init(&this->cond, 0);

	// Setup database connection and read if needed

	if (dbConn == true){
		int dbListenPort = 19999;
		// For now the port for connection to spinnaker is hardcoded
		// but this will change in next release
		dbConnection = new DatabaseIF(dbListenPort, (char*)dbPath);
		if (dbConnection->isReady()){
			// get key-id and id-key mappings
			this->keyIDMap = dbConnection->getKeyIdMap(spinnPopLabel);
			cout << "Cleaning up dbConnection" << endl;
			dbConnection = NULL;
		}

	}

	// set up socket to receive packets
	recvSocket = new SocketIF(spinnPort, (char*)ip);

	this->sockfd_input = recvSocket->getSocket();
	this->si_other = recvSocket->getSiOther();
	this->addr_len_input = recvSocket->getAddrLenInput();
}

// Alternate constructor without DB handshake for when this has already been done
// by a sender. Accepts the Key-ID map as an argument

EIEIOReceiver::EIEIOReceiver(int spinnPort, char* ip, std::map<int, int>* keymap) :
							convertKeysToNeuronId(false), enabled(true)
{

	cout << "Creating EIEIO receiver from spiNNaker with port " << spinnPort << " and ip " << ip << endl;

	if (pthread_mutex_init(&this->recvr_mutex, NULL) == -1) {
		cerr << "Error initializing recvr mutex!" << endl;
		exit(-1);
	}
	pthread_cond_init(&this->cond, 0);


	// set up socket to receive packets
	recvSocket = new SocketIF(spinnPort, (char*)ip);

	this->sockfd_input = recvSocket->getSocket();
	this->si_other = recvSocket->getSiOther();
	this->addr_len_input = recvSocket->getAddrLenInput();

	// As this is a Receiver we only use the Key-ID map
	this->keyIDMap = keymap;

}

void EIEIOReceiver::disable() {
	enabled = false;
	//now wait a little for thread to exit current iteration( see InternalThreadEntry())
	usleep(1000);

}


void EIEIOReceiver::InternalThreadEntry(){

	unsigned char buffer_input[3000];

	while (true) {

		if (enabled) {
			// read from socket and push onto recv queue

			int numbytes_input = recvfrom(this->sockfd_input, (char *) buffer_input,
					sizeof(buffer_input), 0, (sockaddr*) &this->si_other,
					(socklen_t*) &this->addr_len_input);
			if (numbytes_input == -1) {
				cerr << "Packet not received, exiting" << endl;
				exit(-1);
			} else if (numbytes_input < 9) {
				cerr << "Error - packet too short" << endl;
				continue;
			} else { // packet is OK
				// Decode eieio message
				struct eieio_message* new_message = new eieio_message();
				new_message->header.count = buffer_input[0];
				new_message->header.p = ((buffer_input[1] >> 7) & 1);
				new_message->header.f = (messageFormat)((buffer_input[1] >> 6) & 1);
				new_message->header.d = ((buffer_input[1] >> 5) & 1);
				new_message->header.t = ((buffer_input[1] >> 4) & 1);
				new_message->header.type = (messageType)((buffer_input[1] >> 2) & 3);
				new_message->header.tag = (buffer_input[1] & 3);
				new_message->data = (unsigned char *) malloc(numbytes_input - 2);
				memcpy(new_message->data, &buffer_input[2], numbytes_input - 2);

				// convert message to a list of (time, nrn id) pairs
				deque<pair<int, int> > data;
				this->convertEIEIOMessage(new_message, data);
				// clean up message data
				free(new_message->data);
				queueSpikePacket(data);
			}
		}
	}
}

//add new set of spike events to queue, we pass by value deliberately to make sure lists in queue remain valid data
void EIEIOReceiver::queueSpikePacket(deque<pair<int, int> > data){
	pthread_mutex_lock(&this->recvr_mutex);
	this->recvQueue.push_back(data);
	//cout << " Added packet of " << data.size() << " spikes to queue" << endl;
	pthread_cond_signal(&this->cond);
	pthread_mutex_unlock(&this->recvr_mutex);

}


void EIEIOReceiver::convertEIEIOMessage(eieio_message* message, deque<pair<int, int> > &points){


	//check that its a data message
	if (message->header.f != 0 or message->header.p != 0 or message->header.d != 1 
			or message->header.t != 1 or message->header.type != 2){
		cout <<"this packet was determined to be a "
				"command packet. therefore not processing it." << endl;
	} else {
		uint time = (message->data[3] << 24) |
				(message->data[2] << 16) |
				(message->data[1] << 8) |
				(message->data[0]);

		for (int position = 0; position < message->header.count; position++){
			int data_position = (position * SIZE_OF_KEY) + 4;
			int key = (message->data[data_position + 3] << 24) |
					(message->data[data_position + 2] << 16) |
					(message->data[data_position + 1] << 8) |
					(message->data[data_position]);

			if (convertKeysToNeuronId) {
				int neuron_id = (*(this->keyIDMap))[key];
				if (this->keyIDMap->find(key) == this->keyIDMap->end()) {
					cerr << "Missing neuron id for key " << key << endl;
					continue;
				}
				//fprintf(stderr, "time = %i, key = %i, neuron_id = %i\n", time, key, neuron_id);
				pair<int, int> point(time, neuron_id);
				points.push_back(point);

			} else {//keep raw spinnaker key, decode later within popn
				pair<int, int> point(time, key);
				points.push_back(point);
			}

		}
	}
}

bool EIEIOReceiver::hasQueuedSpikes(){
	return this->recvQueue.size() > 0;
}

int EIEIOReceiver::getRecvQueueSize(){
	int size = 0;
	pthread_mutex_lock(&this->recvr_mutex);
	size = this->recvQueue.size();
	pthread_cond_signal(&this->cond);
	pthread_mutex_unlock(&this->recvr_mutex);

	return size;

}


deque<pair<int, int> > EIEIOReceiver::getNextSpikePacket(){
	deque<pair<int, int> > spikePacket;
	pthread_mutex_lock(&this->recvr_mutex);
	spikePacket = this->recvQueue.front();
	this->recvQueue.pop_front();
	pthread_mutex_unlock(&this->recvr_mutex);
	return spikePacket;
}

SocketIF* EIEIOReceiver::getSocketPtr(){

	return this->recvSocket;


}

std::map<int, int> *EIEIOReceiver::getKeyIDMap() {

	return this->keyIDMap;

}

void EIEIOReceiver::closeRecvSocket(){

	this->recvSocket->closeSocket();
	cout << "closed receiver socket" << endl;
}

EIEIOReceiver::~EIEIOReceiver() {
	disable();
	stop(); //stop thread
	closeRecvSocket();
	delete recvSocket;
	recvQueue.clear();

}
}
