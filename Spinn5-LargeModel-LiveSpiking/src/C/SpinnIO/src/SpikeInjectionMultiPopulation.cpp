/*
 * SpikeInjectionMultiPopulation.cpp
 *
 *  Created on: 22 Sep 2015
 *      Author: ad246
 */

#include <iostream>
#include <cstdlib>
#include <math.h>
#include <cmath>
#include <unistd.h>
#include "SpikeInjectionMultiPopulation.h"
#include "DatabaseIF.h"


namespace spinnio
{


SpikeInjectionMultiPopulation::SpikeInjectionMultiPopulation(int dbListenPort, string spinnBoardIPaddr, string dbPath, int _rateResolutionMs, vector<SpikeInjectionPopulation*> & _spikeInjectionPopulations,vector<SpikeReceiverPopulation *> & _spikeReceivePopulations):
			spikeInjectionPopulations(_spikeInjectionPopulations), rateResolutionMs(_rateResolutionMs),spikeReceiverPopulations(_spikeReceivePopulations)
{
	this->db =  new DatabaseIF(dbListenPort,(char * )dbPath.c_str()); //handshake and connect to database
	for (int pop = 0; pop < spikeInjectionPopulations.size(); ++pop) {
		//set common parameters
		spikeInjectionPopulations[pop]->setSpinnBoardIpAddress(spinnBoardIPaddr);
		spikeInjectionPopulations[pop]->setRateResolutionMs(_rateResolutionMs);
		spikeInjectionPopulations[pop]->initialiseSender(db); //get popn to interrogate database for its own key-id map
	}

	//set up receiver populations.
	//NB There is a Spinnaker hardware limit of 7 receiving ports,
	//this is why we have to share ports if we nave more than 7 populations needing spikes returned
	//Only one receiever is set up per port number,
	//Master populations create and hold the actual EIEIOreceiver listening on the port,
	//other populations using the same port are slaves
	int masterReceiverCount = 0;
	for (int pop = 0; pop < spikeReceiverPopulations.size(); ++pop) {
		//set common parameters
		spikeReceiverPopulations[pop]->setSpinnBoardIpAddress(spinnBoardIPaddr);
		spikeReceiverPopulations[pop]->setupKeyIdMaps(db); //get popn to interrogate database for its own key-id map
		SpikeReceiverPopulation * masterPop = NULL;
		if (pop > 0)
			for (int i = 0; i < pop; ++i) {
				if (spikeReceiverPopulations[pop]->spinnakerSpikeReceiverPort == spikeReceiverPopulations[i]->spinnakerSpikeReceiverPort) {
					masterPop = spikeReceiverPopulations[i];
					break;
				}
			}
		if (masterPop !=NULL) {
			spikeReceiverPopulations[pop]->setReceiver(masterPop->getReceiver()); //link to master's receiver
			cout << spikeReceiverPopulations[pop]->spinnPopulationLabel << " population slaved to port from population " << masterPop->spinnPopulationLabel << endl;
		}
		else {//this will be a new master
			spikeReceiverPopulations[pop]->setupNewReceiver();
			masterReceiverCount++;
			cout << "New receiver set up for "<< spikeReceiverPopulations[pop]->spinnPopulationLabel << " population on port " << spikeReceiverPopulations[pop]->spinnakerSpikeReceiverPort << endl;

		}
	}

	if (masterReceiverCount > SPINNAKER_MAX_LIVE_OUTPUT_PORTS) {
		cerr << "You have specified to use " << masterReceiverCount << " different live output ports but the max supported by Spinnaker is " << SPINNAKER_MAX_LIVE_OUTPUT_PORTS << endl;
		exit(-1);
	}
	db->sendReadyNotification();

}

void SpikeInjectionMultiPopulation::endDatabaseCommunication() {
	db->finish();
}



void SpikeInjectionMultiPopulation::waitUntilModelReady() {
	cout << "Waiting for signal that model is ready.." << endl;
	db->waitForDatabaseReadyNotification(); //wait for a second msg on the same port
}

SpikeInjectionMultiPopulation::~SpikeInjectionMultiPopulation() {
	endDatabaseCommunication();
}

void SpikeInjectionMultiPopulation::run(int runtimeMs) {

	stopwatch3.startTimer();

	for (int pop = 0; pop < spikeInjectionPopulations.size(); ++pop) {
		if (!spikeInjectionPopulations[pop]->spikeRatesSet) {
			cerr << "SpikeInjectionPopulation " << spikeInjectionPopulations[pop]->spinnPopulationLabel <<  " cannot run as spike rates not provided" << endl;
			exit(-1);
		}
		spikeInjectionPopulations[pop]->resetRun();

	}
	//cout << spikeInjectionPopulations.size() << " x SpikeInjectionPopulation(s) set to run for " << runtimeMs << "ms" << endl;

	double timeElapsedMs = 0.0;
	int requiredTimestepMicrosecs = 1000 * rateResolutionMs; //how long must each iteration take in real time in microsecs
	double adjustmentMicroSecs = 0.0;

	while (timeElapsedMs < runtimeMs) {

		//enter real time zone
		// Each iteration of this loop should take exactly rateResolutionMs (e.g. 1ms)
		// If it takes less, then we have to wait the right amount of time
		// If it takes more then we need a diferent strategy to maintain realtime
		// e.g. raise rate resolution to 2ms instead of 1ms or run multiple ratecoders in separate threads to use more cores at once


		//start timer
		stopwatch1.startTimer(); //how long was spent in generating spikes and updating the spikes queues for this timestep
		stopwatch2.startTimer(); //time the whole iteration including the wait

		//Generate the spikes and resultant UDP packet(s) for current timestep
		for (int pop = 0; pop < spikeInjectionPopulations.size(); ++pop) {
			spikeInjectionPopulations[pop]->step();
		}

		//cout << timeElapsedMs << "ms" << endl;

		stopwatch1.stopTimer();
		int steppingTimeUsedMicrosecs = stopwatch1.getSubSecondElapsedTimeInMicroSeconds();

		if (steppingTimeUsedMicrosecs > requiredTimestepMicrosecs) {
			cerr << " WARNING: ratecoder falling behind realtime, iteration took " << steppingTimeUsedMicrosecs << "us instead of max " << requiredTimestepMicrosecs << "us" << endl;
		} else {
			//wait for remainder of timestep
			double remainingTimeMicrosecs = ((double)(requiredTimestepMicrosecs - steppingTimeUsedMicrosecs));

			double wait  = remainingTimeMicrosecs + adjustmentMicroSecs;
			wait  = wait < 0.0 ? 0.0 : wait; //if stepping was really slow on previous step ( e.g. more than entire timestep allowed) then could get a negative wait after adjustment.

			//cout << "About to sleep for " << wait << "us (remaining time " << remainingTimeMicrosecs << "us with an adjustment of " <<  adjustmentMicroSecs << "us)" << endl;
			usleep(wait  * USLEEP_ADJUSTMENT_FACTOR);
		}

		stopwatch2.stopTimer();
		double actualTimeOfIterationMicrosecs = stopwatch2.getSubSecondElapsedTimeInMicroSeconds();
		timeElapsedMs += (actualTimeOfIterationMicrosecs/1000.0); //use real time , don't assume rateResolutionMs was actually how long elapsed
		//cout << "The whole iteration inc wait took " << actualTimeOfIterationMicrosecs << "us" << endl;

		//adjust the next timestep to correect error in this one
		adjustmentMicroSecs = requiredTimestepMicrosecs - actualTimeOfIterationMicrosecs;
		//cout << "Adjustment usec set to " << adjustmentMicroSecs << endl;
	}

	stopwatch3.stopTimer();
	//cout << "Complete run timed at:" << (stopwatch3.getElapsedTime() * 1000) << "ms" << endl;
	//cout << "SpikeInjectionMultiPopulation run() completed." << endl;

}




}

