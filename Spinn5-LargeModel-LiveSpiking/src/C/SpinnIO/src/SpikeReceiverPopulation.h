/*
 * SpikeReceiverPopulation.h
 *
 *  Created on: 9 Nov 2015
 *      Author: ad246
 */

#ifndef SPIKERECEIVERPOPULATION_H_
#define SPIKERECEIVERPOPULATION_H_

#include <string>
#include "EIEIOReceiver.h"
#include <vector>

namespace spinnio {

class SpikeReceiverPopulation {
public:
	SpikeReceiverPopulation(int _spinnakerSpikeReceiverPort , string _spinnPopulationLabel);
	virtual ~SpikeReceiverPopulation();
	void setupNewReceiver();
	void setupKeyIdMaps(DatabaseIF * db);
	const string& getSpinnBoardIpAddress() const ;
	void setSpinnBoardIpAddress(const string& spinnBoardIpAddress) ;
	void extractSpikesReceived(vector<pair<int,int> > & spikes );
	void setReceiver(spinnio::EIEIOReceiver * _receiver);
	spinnio::EIEIOReceiver * getReceiver();
	void stopReceiver();

	string spinnPopulationLabel;
	int spinnakerSpikeReceiverPort;

private:
	int  firstNeuronKey,lastNeuronKey;
	string spinnBoardIpAddress;
	spinnio::EIEIOReceiver * receiver;
	std::map<int, int> * keyIdMap;
	bool isMaster;

};

} /* namespace spinnio */
#endif /* SPIKERECEIVERPOPULATION_H_ */
