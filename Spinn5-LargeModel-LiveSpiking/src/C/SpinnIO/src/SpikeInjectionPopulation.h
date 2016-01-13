
#ifndef SPIKEINJECTIONPOPULATION_H_
#define SPIKEINJECTIONPOPULATION_H_

#include <vector>
#include <string>
#include "StopWatch.h"
#include "EIEIOSender.h"

#define USLEEP_ADJUSTMENT_FACTOR  0.93f //tries to approximately correct error in usleep wait time on this box

using namespace std;

namespace spinnio
{


class SpikeInjectionPopulation  {
public:
	bool spikeRatesSet;
	string spinnPopulationLabel;

	SpikeInjectionPopulation(int dbListenPort, string spinnBoardIPaddr, string dbPath, int spinnakerSpikeInjectorPort , string _spinnPopulationLabel, int _totalNeurons, int _rateResolutionMs);
	SpikeInjectionPopulation(int spinnakerSpikeInjectorPort , string _spinnPopulationLabel, int _totalNeurons);
	virtual ~SpikeInjectionPopulation();

	void run(double runtimeMs);
	void initialiseSender(DatabaseIF * db);
	void setCustomNeuronIds(int * neuronIds);
	void setSpikeRates(float * spikeRatesHz);
	void resetRun();
	void step();

	void wakeup();

	int getRateResolutionMs() const ;

	void setRateResolutionMs(int rateResolutionMs) ;

	const string& getSpinnBoardIpAddress() const ;

	void setSpinnBoardIpAddress(const string& spinnBoardIpAddress) ;

	int getTotalNeurons() const {
		return totalNeurons;
	}

private:
	void initialise();
	void clearTimeslots();
	void generateFirstSpikes();
	float getRand0to1();
	int * neuronIds;
	int  spinnakerSpikeInjectorPort, totalNeurons, rateResolutionMs,numTimeStepsInBuffer;
	string spinnBoardIpAddress;
	vector<int> ** timeSlots; //ptr to array of ptr each pointing at a vwctor of neuronIds that will spike on that timestep
	//we use vector because can iterate much faster than deque (see http://www.gotw.ca/gotw/054.htm and )

	int * spikeIntervalTimesteps;
	bool usingCustomIds;
	int realTimeStepMicroSecs;
	spinnio::EIEIOSender * sender;
	spinnio::StopWatch stopwatch1,stopwatch2,stopwatch3;
	bool ticked;

	//These track progress thorugh a run()
	double timeElapsedMs;
	int currTimeSlot;

};
}
#endif /* SPIKEINJECTIONPOPULATION_H_ */
