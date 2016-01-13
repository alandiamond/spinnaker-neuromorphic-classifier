/*
 * SpikeInjectionMultiPopulation.h
 *
 *  Created on: 22 Sep 2015
 *      Author: ad246
 */

#ifndef EXTERNALRATECODER_H_
#define EXTERNALRATECODER_H_

#include <vector>
#include <string>
#include "StopWatch.h"
#include "EIEIOSender.h"
#include "SpikeInjectionPopulation.h"
#include "SpikeReceiverPopulation.h"

using namespace std;

#define SPINNAKER_MAX_LIVE_OUTPUT_PORTS 7

namespace spinnio
{

static const char * SPACE = " ";

class SpikeInjectionMultiPopulation {
public:
	SpikeInjectionMultiPopulation(int dbListenPort, string spinnBoardIPaddr, string dbPath, int _rateResolutionMs, vector<SpikeInjectionPopulation*> & _spikeInjectionPopulations,vector<SpikeReceiverPopulation *> & _spikeReceivePopulations);

	virtual ~SpikeInjectionMultiPopulation();
	void endDatabaseCommunication();
	void waitUntilModelReady();
	void run(int runtimeMs);

private:
	vector<SpikeInjectionPopulation*> spikeInjectionPopulations;
	vector<SpikeReceiverPopulation*> spikeReceiverPopulations;
	spinnio::StopWatch stopwatch1,stopwatch2,stopwatch3;
	int rateResolutionMs;
	DatabaseIF  * db;

};
}
#endif /* EXTERNALRATECODER_H_ */
