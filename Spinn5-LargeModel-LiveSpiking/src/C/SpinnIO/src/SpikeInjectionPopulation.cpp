
#include <iostream>
#include <cstdlib>
#include <math.h>
#include <cmath>
#include <unistd.h>
#include "SpikeInjectionPopulation.h"

namespace spinnio
{

static const char * SPACE = " ";

//construcutor where there is only one injection popn. Looks up its own details from spinakker database
SpikeInjectionPopulation::SpikeInjectionPopulation(int dbListenPort, string _spinnBoardIPaddr, string dbPath, int _spinnakerSpikeInjectorPort , string _spinnPopulationLabel, int _totalNeurons, int _rateResolutionMs):
			spinnBoardIpAddress(_spinnBoardIPaddr),totalNeurons(_totalNeurons), rateResolutionMs(_rateResolutionMs),spikeRatesSet(false),usingCustomIds(false), ticked(false), spinnakerSpikeInjectorPort(_spinnakerSpikeInjectorPort), spinnPopulationLabel(_spinnPopulationLabel)
{
	sender = new spinnio::EIEIOSender(dbListenPort, spinnakerSpikeInjectorPort, spinnPopulationLabel, spinnBoardIpAddress, dbPath);
	initialise();

}

//construcutor to use where there are two or more injection popns.
SpikeInjectionPopulation::SpikeInjectionPopulation(int _spinnakerSpikeInjectorPort , string _spinnPopulationLabel, int _totalNeurons):
			totalNeurons(_totalNeurons), spikeRatesSet(false),usingCustomIds(false), ticked(false), spinnakerSpikeInjectorPort(_spinnakerSpikeInjectorPort), spinnPopulationLabel(_spinnPopulationLabel)
{
}

void SpikeInjectionPopulation::initialiseSender(DatabaseIF * db) {
	if (db->isReady()) {
		sender = new spinnio::EIEIOSender(this->spinnakerSpikeInjectorPort, this->spinnBoardIpAddress, db->getIdKeyMap(this->spinnPopulationLabel));
		initialise();
	} else {
		cerr << "SpikeInjectionPopulation::initialiseSender() failed - spinnaker database was not in ready state" << endl;
		exit(-1);
	}

}

void SpikeInjectionPopulation::initialise() {
	//create default neuron ids 0-totalNeurons-1
	neuronIds = new int[totalNeurons];
	for (int i = 0; i < totalNeurons; ++i) neuronIds[i]=i;

	//set up array to hold spike intervals (i.e. rates )for each neruons
	this->spikeIntervalTimesteps = new int[totalNeurons];

	//set up one second's worth of buffer spike id's
	numTimeStepsInBuffer  = 1000 / rateResolutionMs; //eg. 1000 x 1ms slots or 500 x 2ms slots
	// allocate array of ptr each pointing at a set of neuronIds that will spike on that timestep
	timeSlots =  new vector<int> * [numTimeStepsInBuffer];
	for (int i = 0; i < numTimeStepsInBuffer; ++i) timeSlots[i] = new vector<int>; //declare ech on the heap

	realTimeStepMicroSecs = 1000 * rateResolutionMs; //how long must each iteration take in real time in microsecs

	// start the sender thread
	if (!sender->started) {
		sender->start();
	}
	sender->enableSendQueue(true);

}


void SpikeInjectionPopulation::setCustomNeuronIds(int * neuronIds) {
	//clear default values
	delete[] neuronIds;
	usingCustomIds = true;
}
void SpikeInjectionPopulation::setSpikeRates(float * spikeRatesHz) {
	spikeRatesSet = true;

	int spikeIntervalMs =  999;

	for (int i = 0; i < totalNeurons; ++i) {

		float rate  = spikeRatesHz[i];

		if (rate <= 1.0) {
			spikeIntervalMs = 999;// clip at max period of 1 sec (= buffer size) to allow ring buffer to function
		} else {
			spikeIntervalMs = (int) (1000.0f / rate);
		}
		spikeIntervalTimesteps[i] = spikeIntervalMs/(float)rateResolutionMs; //store as num of timesteps between spikes

		//cout << "Neuron " << i << ": Rate " <<  rate  << "Hz. Spike interval set to " << spikeIntervalTimesteps[i] << " x " << rateResolutionMs << "ms" << endl;


	}
}
void SpikeInjectionPopulation::generateFirstSpikes() {
	//add first spike for every neuron
	for (int n = 0; n < totalNeurons; ++n) {
		int neuronId = neuronIds[n];
		int neuronIntervalTimesteps = spikeIntervalTimesteps[neuronId];
		//dither start spikes by placing first spike randomly withing half its time interval
		// we don't want all spikes to start synchronised
		int firstSpikeTimesteps  = getRand0to1() * ((float)neuronIntervalTimesteps)/2.0f;
		//add a spike for that neuron into the chosen timeslot
		timeSlots[firstSpikeTimesteps]->push_back(neuronId);
	}

}

int SpikeInjectionPopulation::getRateResolutionMs() const {
	return this->rateResolutionMs;
}

void SpikeInjectionPopulation::setRateResolutionMs(int _rateResolutionMs) {
	this->rateResolutionMs = _rateResolutionMs;
}

const string& SpikeInjectionPopulation::getSpinnBoardIpAddress() const {
	return this->spinnBoardIpAddress;
}

void SpikeInjectionPopulation::setSpinnBoardIpAddress(
		const string& _spinnBoardIpAddress) {
	this->spinnBoardIpAddress = _spinnBoardIpAddress;
}

/* --------------------------------------------------------------------------
generate a random number between 0 and 1 inclusive
-------------------------------------------------------------------------- */
float SpikeInjectionPopulation::getRand0to1()
{
	return ((float)rand()) / ((float)RAND_MAX) ;
}


void SpikeInjectionPopulation::resetRun() {
	timeElapsedMs = 0;
	currTimeSlot = 0;
	clearTimeslots(); //clear out any spikes left in buffer from previous run.
	generateFirstSpikes(); //put back first spikes according to current/new rates
}


//send spikes for this tiomestep and place spikes ready for next one
void SpikeInjectionPopulation::step() {
	//get the spikes that need to happen on this timestep
	vector<int> & currSpikes = * timeSlots[currTimeSlot];
	//put them on the send queue (running in another thread)
	int numCurrSpikes = currSpikes.size();
	for (int spike = 0; spike < numCurrSpikes; ++spike) {
		int neuronId = currSpikes[spike];
		sender->addSpikeToSendQueue(neuronId);
		//work out the next spike slot for this neuron
		int nextTimeSlot = (currTimeSlot + spikeIntervalTimesteps[neuronId]) % numTimeStepsInBuffer;
		//add a spike for that neuron into the next timeslot
		timeSlots[nextTimeSlot]->push_back(neuronId);
	}
	currSpikes.clear();

	//cout << this->spinnPopulationLabel << " spikes sent this timestep: " << numCurrSpikes << endl;

	currTimeSlot  = (currTimeSlot+1) % numTimeStepsInBuffer; //increment timeslot with wraparound (ringbuffer)

}

void SpikeInjectionPopulation::run(double runtimeMs) {

	stopwatch3.startTimer();

	if (!spikeRatesSet) {
		cerr << "SpikeInjectionPopulation::Cannot run as spike rates not provided" << endl;
		exit(-1);
	}

	cout << "SpikeInjectionPopulation " << this->spinnPopulationLabel << " set to run for " << runtimeMs << "ms" << endl;
	resetRun();

	int requiredTimestepMicrosecs = 1000 * rateResolutionMs; //how long must each iteration take in real time in microsecs
	double adjustmentMicroSecs = 0.0;


	while (timeElapsedMs < runtimeMs) {

		//enter real time zone
		// Each iteration of this loop must take exactly rateResolutionMs (e.g. 1ms)
		// If it takes less, then we have to wait the right amount of time
		// If it takes more then we need a diferent strategy to maintain realtime
		// e.g. raise rate resolution to 2ms instead of 1ms or run multiple ratecoders in separate threads to use more cores at once


		//start timer
		stopwatch1.startTimer(); //time how much long was spent in updating the spikes queues for this timestep
		stopwatch2.startTimer(); //time the whole iteration including wait

		step();

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

//clear out any spikes left in buffer from previous run
//This will always be the case, otherwise new spikes will build up from these orphans in a positive feedback manner
void SpikeInjectionPopulation::clearTimeslots() {
	for (int i = 0; i < numTimeStepsInBuffer; ++i) {
		timeSlots[i]->clear();
	}
}

SpikeInjectionPopulation::~SpikeInjectionPopulation() {

	//sender->enableSendQueue(false);
	//sender->closeSendSocket();
	delete sender;

	if (!usingCustomIds) {
		delete[] neuronIds;
	}

	delete spikeIntervalTimesteps;

	//clear down buffer
	for (int i = 0; i < numTimeStepsInBuffer; ++i) delete timeSlots[i]; //delete each list/vector
	delete[] timeSlots; //delete array (of ptrs) itself
}


}
