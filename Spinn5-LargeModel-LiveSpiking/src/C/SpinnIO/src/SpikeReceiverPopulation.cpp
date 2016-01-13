/*
 * SpikeReceiverPopulation.cpp
 *
 *  Created on: 9 Nov 2015
 *      Author: ad246
 */

#include "SpikeReceiverPopulation.h"
#include <iostream>

namespace spinnio {

SpikeReceiverPopulation::SpikeReceiverPopulation(int _spinnakerSpikeReceiverPort , string _spinnPopulationLabel):
		isMaster(false), spinnakerSpikeReceiverPort(_spinnakerSpikeReceiverPort),spinnPopulationLabel(_spinnPopulationLabel),receiver(NULL)
{

}

void SpikeReceiverPopulation::stopReceiver() {
	if (isMaster) {
		receiver->disable();
	}
}

void SpikeReceiverPopulation::setupKeyIdMaps(DatabaseIF * db) {
	if (db->isReady()) {
		//find out the key of the first and last neurons, we may need to reject spikes coming from other populations
		map<int, int> * idKeyMap = db->getIdKeyMap(this->spinnPopulationLabel);
		this->firstNeuronKey = idKeyMap->at(0);
		this->lastNeuronKey = idKeyMap->at(idKeyMap->size()-1);

		this->keyIdMap = db->getKeyIdMap(this->spinnPopulationLabel);

	} else {
		cerr << "SpikeReceiverPopulation::initialiseReceiver() failed - spinnaker database was not in ready state" << endl;
		exit(-1);
	}
}

void SpikeReceiverPopulation::setupNewReceiver() {
	isMaster = true;
	receiver = new spinnio::EIEIOReceiver(this->spinnakerSpikeReceiverPort, (char *)this->spinnBoardIpAddress.c_str(), keyIdMap);
	// start the receiverthread
	receiver->start();

}

spinnio::EIEIOReceiver * SpikeReceiverPopulation::getReceiver()
{
	return this->receiver ;
}

void SpikeReceiverPopulation::setReceiver(spinnio::EIEIOReceiver * _receiver){
	this->receiver = _receiver;
}

void SpikeReceiverPopulation::extractSpikesReceived(vector<pair<int,int> > & spikes ) {

	deque<pair<int, int> > rejectedEvents;
	int spikesAccepted = 0;
	while (receiver->hasQueuedSpikes() > 0) {
		deque<pair<int, int> > spikeEvents = receiver->getNextSpikePacket();
		// get the individual spikes from the packet
		for (int i = 0; i < spikeEvents.size(); ++i) {
			int key = spikeEvents[i].second;
			int time = spikeEvents[i].first ;
			//cout << "Spike received at time(ms) "  << time << " with key " << key << endl;
			//check if this spike is intended for this population
			if (key >= this->firstNeuronKey && key <= this->lastNeuronKey) {
				spikesAccepted ++;
				int neuronId = this->keyIdMap->at(key);
				pair<int, int> decodedSpikeEvent (time, neuronId);
				spikes.push_back(decodedSpikeEvent);
				//cout << "key decoded to neuron id " << neuronId << endl;
			} else {
				//its for another population, so put it back on the queue
				rejectedEvents.push_back(spikeEvents[i]);
			}
		}
	}
	if (rejectedEvents.size() > 0) {
		receiver->queueSpikePacket(rejectedEvents);
	}
	//cout << spikesAccepted << " spikes accepted by receiver population " << spinnPopulationLabel << " (" << rejectedEvents.size()<< "passed over) " <<  endl;

}

const string& SpikeReceiverPopulation::getSpinnBoardIpAddress() const {
	return this->spinnBoardIpAddress;
}

void SpikeReceiverPopulation::setSpinnBoardIpAddress(
		const string& _spinnBoardIpAddress) {
	this->spinnBoardIpAddress = _spinnBoardIpAddress;
}

SpikeReceiverPopulation::~SpikeReceiverPopulation() {
	if (isMaster && receiver!=NULL) {
		stopReceiver();
		delete receiver;
		receiver = NULL;
	}
}

} /* namespace spinnio */
