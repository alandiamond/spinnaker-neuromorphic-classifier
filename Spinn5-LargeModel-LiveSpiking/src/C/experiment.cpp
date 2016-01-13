
#include <iostream>
#include "utilities.cc"
#include "experiment.h"


UINT g_NumVR = DEFAULT_NUM_VR;
UINT g_ObservationsUsedPerClass = DEFAULT_OBSERVATIONS_USED_PER_CLASS;
UINT g_clusterSize = DEFAULT_CLUSTER_SIZE;
UINT g_ObservationsExposureMs = DEFAULT_OBSERVATION_EXPOSURE_TIME_MS;

#include "SpinnIO/src/SpikeInjectionPopulation.h"
#include "SpinnIO/src/SpikeReceiverPopulation.h"
#include "SpinnIO/src/SpikeInjectionMultiPopulation.h"
#include "SpinnIO/src/EIEIOSender.h"

#include <stdio.h>
#include <string>
#include <time.h>
#include <unistd.h>

using namespace std;

#include "DataAndVrFunctions.cpp"
#include "hr_time.cpp"


void testSpikeInjectionMultiPopulation() {

	//Common parameters
	string spinn_ip = "192.168.240.1";
	if (!ping(spinn_ip)) {
		cerr << "No board available at " << spinn_ip << endl;
		exit(1);
	}
	string dbPath = "/home/ad246/cuda-workspace/Spinnaker_MNIST_LiveSpiking/src/python/Test-SpinnIO/application_generated_data_files/latest/input_output_database.db";
	int timestepMs = 1;//ms
	int SPINN_DATABASE_PORT = 19999;// For now the port for connection to spinnaker is fixed, but this will change in next release
	string pythonTestDir  = "/home/ad246/cuda-workspace/Spinnaker_MNIST_LiveSpiking/src/python/Test-SpinnIO";
	int spinnakerBoardRunTimeMs = 10000;//ms

	//setup up holder for population injections
	vector<spinnio::SpikeInjectionPopulation*> populations;
	vector<spinnio::SpikeReceiverPopulation*> receivePopulations;

	//Population specific parameters
	int spinnPopPort1 = 12345;
	string spinnPopLabel1  = "spikes_in_1";
	int totalNeuronsPop1 = 400;
	spinnio::SpikeInjectionPopulation * spikeInjectionPop1 = new spinnio::SpikeInjectionPopulation(spinnPopPort1,spinnPopLabel1, totalNeuronsPop1);
	populations.push_back(spikeInjectionPop1);

	int spinnPopPort2 = 12346;
	string spinnPopLabel2  = "spikes_in_2";
	int totalNeuronsPop2 = 200;
	spinnio::SpikeInjectionPopulation * spikeInjectionPop2 = new spinnio::SpikeInjectionPopulation(spinnPopPort2,spinnPopLabel2, totalNeuronsPop2);
	populations.push_back(spikeInjectionPop2);

	//Build and kick off PyNN model running on spinnaker board
	vector<string> args;
	args.push_back(toString(spinnPopPort1));
	args.push_back(toString(spinnPopLabel1));
	args.push_back(toString(totalNeuronsPop1));
	args.push_back(toString(spinnPopPort2));
	args.push_back(toString(spinnPopLabel2));
	args.push_back(toString(totalNeuronsPop2));
	args.push_back(toString(spinnakerBoardRunTimeMs));

	//wait 1 second before running asychronously to give spike sender chance to start up and invoke handshake
	launchPythonScript(pythonTestDir,"TestLiveSpikeInjection_TwoPopulations.py", args, 1, 0, true, PYTHON_USE_PROFILER);

	//create controller that will synchronise spike train inputs for both populations at once
	spinnio::SpikeInjectionMultiPopulation * spikeInjectionMultiPop = new spinnio::SpikeInjectionMultiPopulation(SPINN_DATABASE_PORT,spinn_ip,dbPath,timestepMs,populations,receivePopulations);

	spikeInjectionMultiPop->waitUntilModelReady();

	//sleep(1);//secs for PyNN/ SpyNNaker model to be up and ready to receive spikes, otherwise UDP packets get sent into the ether too early and get lost
	spikeInjectionMultiPop->endDatabaseCommunication();

	//Specify firing rates for each population
	int totalNeurons = totalNeuronsPop1;
	float * ratesHz = new float[totalNeurons];
	float maxRate = 100.0f;//Hz
	float stepHz = maxRate/(float)totalNeurons;
	for (int n = 0; n < totalNeurons; ++n) ratesHz[n] = ((float)n) * stepHz;
	spikeInjectionPop1->setSpikeRates(ratesHz);

	totalNeurons = totalNeuronsPop2;
	for (int n = 1; n <= totalNeurons; ++n) ratesHz[totalNeurons-n] = ((float)n) * stepHz;
	spikeInjectionPop2->setSpikeRates(ratesHz);

	spikeInjectionMultiPop->run(500);//ms

	delete spikeInjectionPop1;
	delete spikeInjectionPop2;
	delete spikeInjectionMultiPop;

}

void testSpikeInjectionAndReceiveMultiPopulation() {

	//Common parameters
	string spinn_ip = "192.168.240.1";
	if (!ping(spinn_ip)) {
		cerr << "No board available at " << spinn_ip << endl;
		exit(1);
	}
	int defaultSpinnSpikeReceivePort = 18250;
	string dbPath = "/home/ad246/cuda-workspace/Spinnaker_MNIST_LiveSpiking/src/python/Test-SpinnIO/application_generated_data_files/latest/input_output_database.db";
	int timestepMs = 1;//ms
	int SPINN_DATABASE_PORT = 19999;// For now the port for connection to spinnaker is fixed, but this will change in next release
	string pythonTestDir  = "/home/ad246/cuda-workspace/Spinnaker_MNIST_LiveSpiking/src/python/Test-SpinnIO";
	int spinnakerBoardRunTimeMs = 10000;//ms

	//setup up holder for population injections
	vector<spinnio::SpikeInjectionPopulation*> injectionPopulations;

	//setup up holder for population spike receivers
	vector<spinnio::SpikeReceiverPopulation*> receivePopulations;

	//Population specific parameters
	int spinnPopPort1 = 12345;
	string spinnPopLabel1  = "spikes_in_1";
	int totalNeuronsPop1 = 400;
	spinnio::SpikeInjectionPopulation * spikeInjectionPop1 = new spinnio::SpikeInjectionPopulation(spinnPopPort1,spinnPopLabel1, totalNeuronsPop1);
	injectionPopulations.push_back(spikeInjectionPop1);

	int spinnPopPort2 = 12346;
	string spinnPopLabel2  = "spikes_in_2";
	int totalNeuronsPop2 = 200;
	spinnio::SpikeInjectionPopulation * spikeInjectionPop2 = new spinnio::SpikeInjectionPopulation(spinnPopPort2,spinnPopLabel2, totalNeuronsPop2);
	injectionPopulations.push_back(spikeInjectionPop2);

	string spinnReceivePopLabel1  = "spikes_out_1";
	int hostReceivePortPop1 = 12347; //port on host where spikes for this pop will get sent
	spinnio::SpikeReceiverPopulation * spikeReceivePop1 = new spinnio::SpikeReceiverPopulation(hostReceivePortPop1,spinnReceivePopLabel1);
	receivePopulations.push_back(spikeReceivePop1);


	//Build and kick off PyNN model running on spinnaker board
	vector<string> args;
	args.push_back(toString(spinnPopPort1));
	args.push_back(toString(spinnPopLabel1));
	args.push_back(toString(totalNeuronsPop1));
	args.push_back(toString(spinnPopPort2));
	args.push_back(toString(spinnPopLabel2));
	args.push_back(toString(totalNeuronsPop2));
	args.push_back(toString(hostReceivePortPop1));
	args.push_back(toString(spinnReceivePopLabel1));
	args.push_back(toString(spinnakerBoardRunTimeMs));

	//wait 1 second before running asychronously to give spike sender chance to start up and invoke handshake
	launchPythonScript(pythonTestDir,"TestLiveSpikeInjection_TwoInjections_OneReceive.py", args, 1, 0, true, PYTHON_USE_PROFILER);

	//create controller that will synchronise spike train inputs for both populations at once
	spinnio::SpikeInjectionMultiPopulation * spikeInjectionMultiPop = new spinnio::SpikeInjectionMultiPopulation(SPINN_DATABASE_PORT,spinn_ip,dbPath,timestepMs,injectionPopulations,receivePopulations);

	spikeInjectionMultiPop->waitUntilModelReady();

	//sleep(1);//secs for PyNN/ SpyNNaker model to be up and ready to receive spikes, otherwise UDP packets get sent into the ether too early and get lost
	spikeInjectionMultiPop->endDatabaseCommunication();

	//Specify firing rates for each population
	int totalNeurons = totalNeuronsPop1 + totalNeuronsPop2;
	float * ratesHz = new float[totalNeurons];
	float maxRate = 100.0f;//Hz
	float stepHz = maxRate/(float)totalNeurons;
	for (int n = 0; n < totalNeurons; ++n) ratesHz[n] = ((float)n) * stepHz;
	spikeInjectionPop1->setSpikeRates(ratesHz);
	int offset = spikeInjectionPop1->getTotalNeurons();
	spikeInjectionPop2->setSpikeRates(& ratesHz[offset]); //point at the 2nd part of the block rate codes

	spikeInjectionMultiPop->run(500);//ms

	vector<pair<int,int> > spikesReceived;
	spikeReceivePop1->extractSpikesReceived(spikesReceived);
	cout << "Spikes received from population " << spikeReceivePop1->spinnPopulationLabel << ": " << endl;
	for (int i = 0; i < spikesReceived.size(); ++i) {
		cout << "(" << spikesReceived[i].first << COMMA << spikesReceived[i].second << ") " ;
		if (i % 30 == 0 ) cout << endl;
	}


	delete spikeInjectionPop1;
	delete spikeInjectionPop2;
	delete spikeInjectionMultiPop;

}
bool buildRunSpyNNakerSpikeSourceModel(bool learning, int numVR, int numClasses,
		string vrSpikeSourcePath, string classActivationSpikeSourcePath,
		int numObservations, int observationExposureTimeMs,
		string classLabelsPath, string classificationResultsPath,string outputDir) {
	stringstream cmd;
	cmd << "cd " << PYTHON_DIR << " && python " << PYNN_MODEL_SCRIPT_SPIKE_SOURCES;
	cmd << SPACE << (learning?"True":"False");
	cmd << SPACE << numVR;
	cmd << SPACE << numClasses;
	cmd << SPACE << vrSpikeSourcePath;
	cmd << SPACE << classActivationSpikeSourcePath;
	cmd << SPACE << numObservations;
	cmd << SPACE << observationExposureTimeMs;
	cmd << SPACE << outputDir;
	cmd << SPACE << classLabelsPath;
	cmd << SPACE << classificationResultsPath;
	cout << cmd.str() << endl;
	int result = system(cmd.str().c_str());
	cout << "Control has returned to calling C program" << endl;
	cout.flush();
	//string result;
	//invokeLinuxShellCommand(cmd.str().c_str(),result);
	//cout << result;
	string runCompleteMarkerFile = PYTHON_DIR + SLASH + PYTHON_RUN_COMPLETE_FILE;
	bool ok = fileExists(runCompleteMarkerFile);
	if (!ok) cerr << "Run of Spinnaker model did not complete" << endl;
	return ok;
}


bool buildRunSpyNNakerLiveSpikeInjectionModel(
		bool learning, int numVR, int numClasses,
		float * vrRateCodes, float * classActivationRateCodes,
		int numObservations, int observationExposureTimeMs,
		string outputDir,int clusterSize,
		vector<int> & classifierDecision) {

	//checkContents("vrRateCodes",vrRateCodes,numObservations * numVR,numVR,data_type_float,2);
	bool boardPresent = ping(SPINNAKER_BOARD_IP_ADDR);
	if (!boardPresent) {
		cerr << "Nothing appears to be connected on specified IP address of " << SPINNAKER_BOARD_IP_ADDR << endl;
		exit(-1);
	}

	//Build and kick off PyNN model running on spinnaker board
	vector<string> args;
	args.push_back((learning?"True":"False"));
	args.push_back(toString(numVR));
	args.push_back(toString(numClasses));
	args.push_back(toString(RN_FIRST_SPIKE_INJECTION_PORT)); //the (first) port where the spinnkaer RN poluation will listen for UDP spikes
	args.push_back(toString(RN_SPIKE_INJECTION_POP_LABEL)); //the name to be used for this pop. Needs to be matched as it is used to extract neuron id's from database
	args.push_back(toString(CLASS_ACTIVATION_SPIKE_INJECTION_PORT)); //the port where the spinnaker class activcation poluation will listen for UDP spikes during learning
	args.push_back(toString(CLASS_ACTIVATION_SPIKE_INJECTION_POP_LABEL));
	args.push_back(toString(numObservations));
	args.push_back(toString(observationExposureTimeMs));
	args.push_back(outputDir);
	args.push_back(toString(clusterSize));
	args.push_back(toString(PYNN_MODEL_EXTRA_STARTUP_WAIT_SECS));//start up wait should be added on to the run time to avoid finishing too early while spikes are still being sent

	//wait 1 second before running asychronously to give spike sender chance to start up and invoke handshake
	launchPythonScript(PYTHON_DIR,PYNN_MODEL_SCRIPT_LIVE_SPIKING, args, 1, 0, true, PYTHON_USE_PROFILER);


	//setup up holder for population injections
	vector<spinnio::SpikeInjectionPopulation*> sendPopulations;
	vector<spinnio::SpikeReceiverPopulation*> receivePopulations;

	//we may have to set up more than one sender popn as spinnaker seems to have a size limit
	vector<int>senderPopnSizes;
	separateAcross(numVR,MAX_SIZE_SPIKE_INJECTION_POP,senderPopnSizes);
	cout << senderPopnSizes.size() << " spike injection populations will be set up of sizes " << toString(senderPopnSizes,SPACE) << endl;
	for (int sendPop = 0; sendPop < senderPopnSizes.size(); ++sendPop) {
		string label = RN_SPIKE_INJECTION_POP_LABEL + toString(sendPop);
		int port  = RN_FIRST_SPIKE_INJECTION_PORT + sendPop;
		int size = senderPopnSizes[sendPop];
		spinnio::SpikeInjectionPopulation * spikeInjectionPopRN = new spinnio::SpikeInjectionPopulation(port,label, size);
		sendPopulations.push_back(spikeInjectionPopRN);
	}

	spinnio::SpikeInjectionPopulation * spikeInjectionPopClassActivation = NULL;
	if (learning) { //send activation also
		spikeInjectionPopClassActivation = new spinnio::SpikeInjectionPopulation(CLASS_ACTIVATION_SPIKE_INJECTION_PORT,CLASS_ACTIVATION_SPIKE_INJECTION_POP_LABEL, numClasses);
		sendPopulations.push_back(spikeInjectionPopClassActivation);

	} else { //testing
		//set up a receiver on one shared port for all the AN populations (i.e. one per class)
		string spinnReceivePopLabelTemplate = "popClusterAN_";
		int hostReceivePort = HOST_RECEIVE_PORT; //port on host where spikes for this pop will get sent
		for (unsigned int cls = 0; cls < numClasses; ++cls) {
			string label = spinnReceivePopLabelTemplate + toString(cls);
			spinnio::SpikeReceiverPopulation * spikeReceivePop = new spinnio::SpikeReceiverPopulation(hostReceivePort,label);
			receivePopulations.push_back(spikeReceivePop);
		}
	}


	string dbPath = PYTHON_DIR + toString("/application_generated_data_files/latest/input_output_database.db");

	//create controller that will synchronise spike train inputs for both populations at once
	spinnio::SpikeInjectionMultiPopulation * spikeInjectionMultiPop = new spinnio::SpikeInjectionMultiPopulation(SPINNAKER_DATABASE_PORT,SPINNAKER_BOARD_IP_ADDR,dbPath,DT,sendPopulations,receivePopulations);
	spikeInjectionMultiPop->waitUntilModelReady();//SpiNNaker will send another handshake to the database port whn model starts to run
	spikeInjectionMultiPop->endDatabaseCommunication();
	sleep(PYNN_MODEL_EXTRA_STARTUP_WAIT_SECS);

	vector<int> presentationTimesMs;
	CStopWatch timer;
	timer.startTimer();

	cout << "Sending observations: " << endl;
	for (int ob = 0; ob < numObservations; ++ob) {
		int offset = 0;
		for (int sendPop = 0; sendPop < senderPopnSizes.size(); ++sendPop) {
			spinnio::SpikeInjectionPopulation * spikeInjectionPopRN = sendPopulations[sendPop];
			int rateCodeIndex = (ob * numVR) + offset;
			spikeInjectionPopRN->setSpikeRates(& vrRateCodes[rateCodeIndex]);//point at the block of rate codes for this observation
			offset += spikeInjectionPopRN->getTotalNeurons();
		}
		if (learning) spikeInjectionPopClassActivation->setSpikeRates(& classActivationRateCodes[ob * numClasses]);

		//note the elapsed time (relative to the first observation)
		timer.stopTimer();
		double elapsedTimeMs = 1000.0 * timer.getElapsedTime();
		//these will be used as the time boundaries for evaluating the classifier output
		//presentationTimesMs.push_back(elapsedTimeMs);

		//tweak the time for next observation to keep on track with overall time. This helps with knowing how long the whole run will take. For ten thousand samples, a 1ms error puts it 10 sec away from the expected finish
		int expectedElapsedTimeMs = ob * observationExposureTimeMs; //this is where we should be
		int tweakMs = elapsedTimeMs - expectedElapsedTimeMs; //generally seems to jitter / fall behind around 1ms for every 200ms
		tweakMs = clip(tweakMs,-5,5);//Don't adjust by too much on any one observation.

		spikeInjectionMultiPop->run(observationExposureTimeMs * CLASS_ACTIVATION_EXPOSURE_FRACTION - tweakMs);
		usleep(1000 * observationExposureTimeMs * (1-CLASS_ACTIVATION_EXPOSURE_FRACTION)); //"quiet" gap for rest of exposure time


		if (learning) {
			cout <<  ob << " " ;
			cout.flush();
			if (ob % 30 == 0) cout << endl;

		} else { //testing

			//find which AN output population was most active during this obsevation presentation
			cout <<  endl << "Observation: " << ob << endl;
			int mostActiveOutputClass = -1;
			int mostSpikes = -1;
			for (int cls = 0; cls < numClasses; ++cls) {
				vector<pair<int,int> > spikesReceived;
				receivePopulations[cls]->extractSpikesReceived(spikesReceived);
				int spikeCount = spikesReceived.size();
				cout << spikeCount << SPACE;
				if (spikeCount > mostSpikes) {
					mostSpikes = spikeCount;
					mostActiveOutputClass = cls;
				}
			}
			classifierDecision.push_back(mostActiveOutputClass);
			int unprocessedSpikes  = receivePopulations[0]->getReceiver()->getRecvQueueSize();
			if (unprocessedSpikes >  0) {
				cerr << "After presentation, there are still " << unprocessedSpikes  << " unprocessed spikes in the queue. " << endl;
				cerr << "This suggests that in PyNN, live_output has been set on a population not expected by the host code" << endl;
				cerr << "Alternatively there are some late 'straggler' spikes coming in live" << endl;
			}

		}


	}
	cout << endl;

	/*
	cout << "presentation Times:" << endl;
	cout << toString(presentationTimesMs,SPACE) << endl;
	writeToPythonList(presentationTimesMs,PYTHON_DIR + SLASH + "PresentationTimes.txt");
	 */

	delete spikeInjectionMultiPop;
	//delete spikeInjectionPopRN;
	//delete spikeInjectionPopClassActivation;
	deleteHeapObjects(sendPopulations);
	if (!learning) deleteHeapObjects(receivePopulations);


	string runCompleteMarkerFile = PYTHON_DIR + SLASH + PYTHON_RUN_COMPLETE_FILE;
	cout << "Spike injections complete, waiting for Spinnaker to complete.." << endl;
	bool ok = waitForFileToAppear(3000,runCompleteMarkerFile);
	cout << "Detected Spinnaker completed." << endl;
	if (!ok) cerr << "Run of Spinnaker model did not complete" << endl;
	return ok;
}

void getTrialVrResponses(vector<int> & observations,float * vrResponse) {
	int numObservations = observations.size();
	vector<string> vrResponseFilenames;
	int trialObservations[] {34,8,25,12,20,35,32,38,41,22}; //these are examples of classes 0-9
	for (int i = 0; i < numObservations; ++i) {
		string path = getVrResponseFilePath(trialObservations[i],DATA_CACHE_DIR,g_NumVR);
		float * ptrVrResponse  = & vrResponse[g_NumVR * i];
		loadArrayFromTextFile(path,ptrVrResponse,COMMA, g_NumVR,data_type_float);
		normaliseToMaxEntry(ptrVrResponse,g_NumVR);
	}
}
//find the class of a given observation
int getClass(int observationId) {
	//dummy implementation
	return observationId;
}


int calculateClassificationScore(vector<int> & classifierDecision, vector<int> & observations, vector<int> & classLabel) {
	//compare decision with correc answers to obtain scores
	int numObservations = observations.size();
	vector<int> correctClasses;
	getClassesForObservations(observations, correctClasses, classLabel);
	int score = 0;
	for (int i = 0; i < numObservations; ++i) {
		if (classifierDecision[i] == correctClasses[i]) {
			score++;
		}
	}
	return score;
}


int calculateClassificationScore(const string& classificationResultsPath, vector<int> & observations, vector<int> & classLabel, vector<int> & classificationDetails) {
	//retrieve results of classification to obtain scores
	int numObservations = observations.size();
	classificationDetails.clear();
	classificationDetails.reserve(numObservations);
	int results[numObservations];
	loadArrayFromTextFile(classificationResultsPath, results, COMMA, numObservations, data_type_int);
	vector<int> correctClasses;
	getClassesForObservations(observations, correctClasses, classLabel);
	int score = 0;
	for (int i = 0; i < numObservations; ++i) {
		classificationDetails[i] = results[i];
		if (results[i] == correctClasses[i]) {
			score++;
		}
	}
	return score;
}



//generic parameter struct to allow generic testing and reporting of different parameter settings
typedef struct Parameter {
	string name;
	string value;
} Parameter;

/*-----------------------------------------------------------------
Uses a timestamp plus network parameters used to create an id string unique to this run
-----------------------------------------------------------------*/

string generateUniqueRunId()
{
	string timestamp = toString(time (NULL));
	string id = timestamp +
			"-" + DATASET_NAME;
	return id;
}



void presentObservationsToModel(bool useLiveSpiking, bool training, vector<int> & observations,vector<int> & classLabels, string vrDataPath, float * fullObservationsData, string classLabelsPath, string classificationResultsPath,string outputDir, vector<int> & classifierDecision) {
	int numObservations = observations.size();
	float * vrResponseSet = getVrResponses(observations, g_NumVR, vrDataPath, fullObservationsData, OBSERVATIONS_DATA_DIR, DATA_CACHE_DIR);

	if (useLiveSpiking) { //send in live spiking input using ExternalRateCodeGenerator
		float * vrRateCodes  = generateVrResponseRateCoding(vrResponseSet,numObservations, g_NumVR);
		float * classActivationRateCodes = NULL;
		if (training) { //only used in training stage
			classActivationRateCodes = generateClassActivationRateCoding(observations,classLabels);
		} else {
			classActivationRateCodes = new float[numObservations];
			zeroArray(classActivationRateCodes,numObservations);
		}
		cout << "Launching Live SPiking Classifier for " << (training? "training.." : "testing") << endl;
		bool ok = buildRunSpyNNakerLiveSpikeInjectionModel(training, g_NumVR,NUM_CLASSES,vrRateCodes, classActivationRateCodes, numObservations, g_ObservationsExposureMs,outputDir, g_clusterSize, classifierDecision);
		cout << "About to delete vrRateCodes.." <<endl;
		delete [] vrRateCodes;
		cout << "deleted vrRateCodes" <<endl;
		cout << "About to delete classActivationRateCodes.." <<endl;
		delete [] classActivationRateCodes;
		cout << "deleted classActivationRateCodes" <<endl;
		cout << "About to delete vrResponseSet.." <<endl;
		delete [] vrResponseSet;
		cout << "deleted vrResponseSet" <<endl;

		if (!ok) exit (1);

	} else { //Use spike source files
		string vrSpikeSourcePath = PYTHON_DIR + SLASH + VR_RESPONSE_SPIKE_SOURCE_FILENAME;
		string classSpikeSourcePath = PYTHON_DIR + SLASH + CLASS_ACTIVATION_SPIKE_SOURCE_FILENAME;
		int spikesGenerated = generateVrResponseSpikeSourceData(vrSpikeSourcePath,vrResponseSet,numObservations);
		if (training) { //only used in training stage
			spikesGenerated += generateClassActivationSpikeSourceData(classSpikeSourcePath,observations,classLabels);
		}
		cout << "Spike sources created totalling " << spikesGenerated << " spikes." << endl;
		delete [] vrResponseSet;
		cout << "Launching Spike Source Classifier for " << (training? "training.." : "testing") << endl;
		bool ok = buildRunSpyNNakerSpikeSourceModel( training, g_NumVR,NUM_CLASSES,vrSpikeSourcePath, classSpikeSourcePath, numObservations, g_ObservationsExposureMs,classLabelsPath,classificationResultsPath,outputDir);
		if (!ok) exit (1);
	}

}

/*
 * Overload for training stage ( class labels and classification results are not required)
 */
void presentObservationsToModel(bool useLiveSpiking, bool training, vector<int> & observations,vector<int> & classLabels, string vrDataPath, float * fullObservationsData,string outputDir){
	vector<int> emptyVector;
	return presentObservationsToModel(useLiveSpiking,training,observations,classLabels,vrDataPath,fullObservationsData,EMPTY_STRING,EMPTY_STRING,outputDir, emptyVector);
}

void writeObservationClassesToFile(vector<int> & observations,string classLabelsPath,vector<int> & classLabels) {
	vector<int> correctClasses;
	getClassesForObservations(observations, correctClasses, classLabels);
	FILE * file = fopen(classLabelsPath.c_str(),"w");
	writeArrayToTextFile(file,&correctClasses[0],1,observations.size(),data_type_int,0,false,COMMA,true);
}

unsigned int runTrainingAndTest(bool useLiveSpiking, vector<int> & observationsTrainingSet, vector<int> & observationsTestingSet, string vrPath, vector<int> & classLabels, float * fullObservationsData, vector<int> & classificationDetails, string outputDir) {

	//run training / learning
	presentObservationsToModel(useLiveSpiking, true,observationsTrainingSet,classLabels,vrPath, fullObservationsData,outputDir);

	//run testing
	string classificationResultsPath  = PYTHON_DIR + SLASH +  CLASSIFICATION_RESULTS_FILENAME;
	string classLabelsPath  = PYTHON_DIR + SLASH + CLASS_LABELS_FILENAME;
	writeObservationClassesToFile(observationsTestingSet,classLabelsPath,classLabels);
	vector<int> classifierDecision;
	presentObservationsToModel(useLiveSpiking, false,observationsTestingSet,classLabels,vrPath, fullObservationsData, classLabelsPath, classificationResultsPath,outputDir,classifierDecision);

	//retrieve results of classification to obtain scores
	//unsigned int score = calculateClassificationScore(classificationResultsPath, observationsTestingSet, classLabels, classificationDetails);
	classificationDetails = classifierDecision;
	unsigned int score = calculateClassificationScore(classifierDecision, observationsTestingSet, classLabels);
	cout << "Calculated classification scores." << endl;
	return score;
}

void testLimitedClasses_SeparateTrainingAndTestDataset(bool useLiveSpiking) {

	CStopWatch timer;

	createDirIfNotExists(OUTPUT_DIR);
	if (promptYN("Clear down output directory (" + toString(OUTPUT_DIR)  + ") ?")) clearDirectory(OUTPUT_DIR);

	vector<int> requiredClasses;
	addAllToVector(requiredClasses,"0 1 2 3 4 5 6 7 8 9");

	//vector<UINT> observationsPerClass;
	//addAllToVector(observationsPerClass,"500");

	vector<UINT> observationsExposureMs;
	addAllToVector(observationsExposureMs,"120");

	vector<UINT> vrCount;
	//addAllToVector(vrCount,"10 15 20 25 50 100 150 200");
	addAllToVector(vrCount,"100");

	vector<UINT> clusterSizes;
	addAllToVector(clusterSizes,"20");

	for (UINT cl = 0; cl < clusterSizes.size(); ++cl) {

		g_clusterSize = clusterSizes[cl];

		for (UINT j = 0; j < observationsExposureMs.size(); ++j) {

			g_ObservationsExposureMs = observationsExposureMs[j];

			for (UINT i = 0; i < vrCount.size(); ++i) {
				g_NumVR = vrCount[i];

				clearDirectory(DATA_CACHE_DIR);
				//ensure we work out the sample distances for the set in question
				fileDelete(getMaxMinDistanceFilepath(OBSERVATIONS_DATA_DIR,g_ObservationsUsedPerClass * requiredClasses.size()));

				string uniqueRunId = generateUniqueRunId();
				stringstream outputDir;
				outputDir << OUTPUT_DIR <<  SLASH <<  uniqueRunId <<  "[clust" <<  toString(g_clusterSize) <<  "][exp" <<g_ObservationsExposureMs << "ms]["  << g_NumVR << "VRs]";
				createDirIfNotExists(outputDir.str());

				stringstream summaryText;
				printSeparator();
				summaryText << "VRs: " << g_NumVR << endl;
				summaryText << "Observations per class: " << g_ObservationsUsedPerClass << endl;
				summaryText << "Observation exposure (ms): " << g_ObservationsExposureMs << endl;
				summaryText << "Cluster size: " << g_clusterSize << endl;

				printSeparator();
				string summaryPath = outputDir.str() + SLASH + uniqueRunId + ".txt";
				ofstream summary(summaryPath.c_str());
				summary << summaryText.str()  << endl;
				//summary.close();
				cout << summaryText.str()  << endl;
				cout << (useLiveSpiking?"Using LIVE spiking":"Using spike source files""Using spike source files") << " for input."  << endl;

				//load all the training observation data and class labelling
				string dataPath = OBSERVATIONS_DATA_DIR + SLASH + FILENAME_ALL_SAMPLES_TRAINING;
				float * fullObservationsData = loadMNISTdata(dataPath,TRAINING_DATASET_SIZE); //NB: this is going on the heap
				vector<int> classLabels;
				string labelspath = OBSERVATIONS_DATA_DIR + SLASH + FILENAME_CLASS_LABELS_TRAINING;
				loadMNISTclassLabels(labelspath,TRAINING_DATASET_SIZE,classLabels);


				vector<int> observationsTrainingSet;
				//getFirstNInstancesPerClass(g_ObservationsUsedPerClass,classLabels,requiredClasses,observationsTrainingSet);
				getFirstNPerClassInOrder(g_ObservationsUsedPerClass,classLabels,requiredClasses,observationsTrainingSet);


				string vrFilename =  toString(requiredClasses,"-") + "_" + toString(g_NumVR) + "VRs_" + toString(g_ObservationsUsedPerClass) + "ObsPerClass.dat";
				string vrPath = OBSERVATIONS_DATA_DIR + SLASH + vrFilename;
				if (!fileExists(vrPath) || !USE_VR_CACHE) {
					cout << endl << "Creating custom VR file " << vrFilename << endl;
					//create and save a VR set from the current training set only, using SOM/neural gas
					string trainingDataFilename = "tmpTrainingDataset.csv";
					createObservationsDataFile(observationsTrainingSet, fullObservationsData,OBSERVATIONS_DATA_DIR, trainingDataFilename);
					runStdNeuralGas(OBSERVATIONS_DATA_DIR, trainingDataFilename, vrFilename, COMMA, g_NumVR, NUM_EPOCHS);
				} else {
					cout << endl << "Located compatible VR file " << vrPath << endl;
				}

				//run training / learning
				timer.startTimer();
				presentObservationsToModel(useLiveSpiking, true,observationsTrainingSet,classLabels,vrPath, fullObservationsData,outputDir.str());
				timer.stopTimer();
				float realTimeElapsed = timer.getElapsedTime();
				summary << "Training time (sec): " << realTimeElapsed << endl;

				delete[] fullObservationsData;
				classLabels.clear();
				clearDirectory(DATA_CACHE_DIR);//don't want to reuse VR responses as id's will overlap with training set

				//run testing
				//load all the testing observation data and class labelling
				dataPath = OBSERVATIONS_DATA_DIR + SLASH + FILENAME_ALL_SAMPLES_TESTING;
				fullObservationsData = loadMNISTdata(dataPath,TESTING_DATASET_SIZE);
				labelspath = OBSERVATIONS_DATA_DIR + SLASH + FILENAME_CLASS_LABELS_TESTING;
				loadMNISTclassLabels(labelspath,TESTING_DATASET_SIZE,classLabels);

				vector<int> observationsTestingSet;
				//getFirstNInstancesPerClass(g_ObservationsUsedPerClass,classLabels,requiredClasses,observationsTestingSet);
				getFirstNPerClassInOrder(g_ObservationsUsedPerClass,classLabels,requiredClasses,observationsTestingSet);

				string classificationResultsPath  = PYTHON_DIR + SLASH +  CLASSIFICATION_RESULTS_FILENAME;
				string classLabelsPath  = PYTHON_DIR + SLASH + CLASS_LABELS_FILENAME;
				writeObservationClassesToFile(observationsTestingSet,classLabelsPath,classLabels);

				timer.startTimer();
				vector<int> classifierDecision; //holder for the detailed results from the classifier
				presentObservationsToModel(useLiveSpiking, false,observationsTestingSet,classLabels,vrPath, fullObservationsData, classLabelsPath, classificationResultsPath,outputDir.str(),classifierDecision);
				timer.stopTimer();
				realTimeElapsed = timer.getElapsedTime();
				summary << "Testing time (sec): " << realTimeElapsed << endl;


				//retrieve results of classification to obtain scores
				//vector<int> classificationDetails; //holder for the detailed results from the classifier , if needed
				unsigned int score = calculateClassificationScore(classifierDecision, observationsTestingSet, classLabels);

				unsigned int numObservations = observationsTestingSet.size();
				float percentCorrect   = 100*(float)score/(float)numObservations;
				printf( "Classifier Testing completed. Score:%u/%u (%f percent) \n",score,numObservations,percentCorrect);

				summary << "Classifier Testing completed. Score:" << score << " out of " << numObservations << "(" << percentCorrect << "%)" << endl;
				summary.close();

				delete[] fullObservationsData;

				cout << "End of Run  " << uniqueRunId << endl;
			}
		}
	}

}

int main() {

	//testSpikeInjectionMultiPopulation();
	//testSpikeInjectionAndReceiveMultiPopulation();
	testLimitedClasses_SeparateTrainingAndTestDataset(USE_LIVE_SPIKING);

	return 0;
}
