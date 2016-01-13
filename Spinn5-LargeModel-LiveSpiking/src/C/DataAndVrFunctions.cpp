#ifndef DATA_AND_VR_FUNCTIONS_CPP
#define DATA_AND_VR_FUNCTIONS_CPP

#include "utilities.cc"
#include "NeuralGas.cpp"
#include "experiment.h"

using namespace std;

/*
 * calc the proboablity of a Poisson spike occuring on any timestep, given a required net spike rate in Hz
 */
double perTimestepPoissonSpikeProbability(double inputRateHz, double timestepMs) {
	double prob = inputRateHz  * timestepMs/1000.0;
	if (prob > 1.0)  prob = 1.0;
	return prob;
}

bool generatePoissonSpikeTimes(double inputRateHz, double startTimeMs, double timestepMs, int durationMs, vector<int> & spikeTimes) {
	double maxRate = 1000.0/timestepMs;
	if (inputRateHz > maxRate) {
		cerr << "Cannot generate spike rate  > " << maxRate << "Hz using a timestep of " << timestepMs << "ms" << endl;
		return false;
	}
	double probSpike = perTimestepPoissonSpikeProbability(inputRateHz,timestepMs);
	for (int t = 0; t <= durationMs; ++t) {
		if(getRand0to1() <= probSpike) {
			spikeTimes.push_back(startTimeMs + t);
		}
	}
	return true;
}

bool generateRegularSpikeTimes(double inputRateHz, double startTimeMs, double timestepMs, int durationMs, vector<int> & spikeTimes) {
	double maxRate = 1000.0/timestepMs;
	if (inputRateHz > maxRate) {
		cerr << "Cannot generate spike rate  > " << maxRate << "Hz using a timestep of " << timestepMs << "ms" << endl;
		return false;
	}
	int intervalMs = (int) (1000.0 / inputRateHz);
	int lastTimeMs = largest(durationMs,intervalMs); //add at least 2 spike times even if outside durationMs
	for (int t = 0; t <= lastTimeMs; t+=intervalMs) {
		spikeTimes.push_back(startTimeMs + t);
	}
	//cout << "Generated " << spikeTimes.size() << " regular spikes at frequency " << inputRateHz << "Hz" << endl;
	return true;
}


void rateCodeVrResponse(float vrResponse , float & freq, float & intervalMs) {
	float scale = 2.4f; //tuned to trigger an avg frequency = freq in RN cluster populations
	float maxFreq = MAX_FIRING_RATE_HZ * scale;
	freq = vrResponse * maxFreq + 0.0001; //don't want div by zero if response is zero
	intervalMs = 1000.0 / freq;
}

float * generateVrResponseRateCoding(float * vrResponseSet,int numObservations, int numVRs) {
	float * vrRateCodes = new float[numVRs * numObservations];
	float intervalNotUsed;
	for (int ob = 0; ob < numObservations; ++ob) {
		for (int vr = 0; vr < numVRs; ++vr) {
			int index  = ob * numVRs + vr;
			rateCodeVrResponse(vrResponseSet[index],vrRateCodes[index],intervalNotUsed); //is this going work can vrRateCodes[i] be modified by passing by ref? - (yes, it can - Ed.)
		}
	}
	//checkContents("vrResponseSet",vrResponseSet,numObservations * numVRs,numVRs,data_type_float,2);
	//checkContents("vrRateCodes",vrRateCodes,numObservations * numVRs,numVRs,data_type_float,2);
	return vrRateCodes;
}


/*
 * creates PyNN-format SpikeSource data file
 * The file comprises a sequence of responses to a set of observations, each response of duration g_ObservationsExposureMs
 * Each response comprises g_NumVR rate coded bands, one per VR response (0..1), corresponding to each row in the data file
 * The spiking rates generated have been tuned to trigger the correct avg frequency in RN cluster populations in the Spinnaker model
 */
int generateVrResponseSpikeSourceData(string path, float * vrResponseSet, int numObservations) {
	int spikeCount  = 0;
	int exposureDurationMs = g_ObservationsExposureMs * CLASS_ACTIVATION_EXPOSURE_FRACTION;
	ofstream out(path.c_str());
	for (int vr = 0; vr < g_NumVR; vr++) {
		vector<int> spikeTimes; //reset for each VR/row
		for (int observation = 0; observation < numObservations; ++observation) {
			double startTimeMs = observation * g_ObservationsExposureMs;
			float vrResponse = vrResponseSet[vr + g_NumVR*observation];
			float freq,intervalMs;
			rateCodeVrResponse(vrResponse, freq, intervalMs);
			if (intervalMs < exposureDurationMs) {//freq is not so low that the period is larger than the available time
				startTimeMs = startTimeMs   +  getRand0to1() * (intervalMs/2.0);
				generateRegularSpikeTimes(freq,startTimeMs,DT,exposureDurationMs,spikeTimes);
			}
		}
		//if this row was inactive for the whole presentation, create 2 random spikes during this period (spike source reader doesnt like zero or 2 entries on a line)
		if (spikeTimes.size()< 2) {
			int randomSpikeTimeMs = getRand0to1() * (numObservations*g_ObservationsExposureMs/2.0);
			spikeTimes.push_back(randomSpikeTimeMs);
			spikeTimes.push_back(randomSpikeTimeMs * 2);
		}
		spikeCount += spikeTimes.size();
		out << toString(spikeTimes,COMMA);
		if (vr < (g_NumVR -1)) out << endl;
	}
	out.close();
	cout << "Created PyNN-format SpikeSource data file " << path << endl;
	return spikeCount;
}

/*
 * creates csv data file to pass to the Pynn model
 * The file comprises the specification for sequence of responses to a set of observations, each response to be of duration g_ObservationsExposureMs
 * Each observation response (row) comprises g_NumVR rate coded bands (columns), one per VR response (0..1)
 * The spiking rates generated have been tuned to trigger the correct avg frequency in RN cluster populations in the Spinnaker model
 */
int generateVrResponseFreqIntervalData(string path, float * vrResponseSet, int numObservations) {
	int spikeCount  = 0;
	ofstream out(path.c_str());
	float scale = 2.4; //tuned to trigger an avg frequency = freq in RN cluster populations
	float maxFreq = MAX_FIRING_RATE_HZ * scale;
	for (int observation = 0; observation < numObservations; ++observation) {
		vector<int> intervals;
		intervals.reserve(g_NumVR);
		for (int vr = 0; vr < g_NumVR; vr++) {
			float vrResponse = vrResponseSet[vr + g_NumVR*observation];
			double freq = vrResponse * maxFreq + 0.0001; //don't want div by zero if response is zero
			int intervalMs = 1000.0 / freq;
			intervals.push_back(intervalMs);
		}
		out << toString(intervals,COMMA);
		if (observation < (numObservations -1)) out << endl;
	}
	out.close();
	cout << "Created PyNN-format SpikeSource data file " << path << endl;
	return spikeCount;
}

/*
 * creates PyNN-format SpikeSource data file
 * The file comprises a sequence of responses to a set of observations, each response of duration g_ObservationsExposureMs
 * Each response comprises g_NumVR rate coded bands, one per VR response (0..1), corresponding to each row in the data file
 * The spiking rates generated have been tuned to trigger the correct avg frequency in RN cluster populations in the Spinnaker model
 */
int generateClassActivationSpikeSourceData(vector<int> & classSequence,string path) {
	int spikeCount  = 0;
	int exposureDurationMs = g_ObservationsExposureMs * CLASS_ACTIVATION_EXPOSURE_FRACTION;
	ofstream out(path.c_str());
	for (int row = 0; row < NUM_CLASSES; row++) { //spike source output file will have one row for each class
		vector<int> spikeTimes; //reset for each class/row
		int numObservations = classSequence.size();
		for (int observation = 0; observation < numObservations; ++observation) {
			double startTimeMs = observation * g_ObservationsExposureMs;
			int activeClass = classSequence[observation]; //get class of this observation
			if (row==activeClass) {//this row will be the active one for this observation
				double freq = CLASS_ACTIVATION_SIGNAL_FREQ_HZ;
				int intervalMs = 1000.0 / freq;
				startTimeMs = startTimeMs  + (intervalMs/2);
				generateRegularSpikeTimes(freq,startTimeMs,DT,exposureDurationMs,spikeTimes);
			}
		}
		//if this row was inactive for the whole presentation, create 2 random spikes during this period (spike source reader doesnt like zero or 2 entries on a line)
		if (spikeTimes.size()< 2) {
			int spikeTimeMs = getRand0to1() * (g_ObservationsExposureMs/2.0);
			spikeTimes.push_back(spikeTimeMs);
			spikeTimeMs = spikeTimeMs  + (getRand0to1() * (g_ObservationsExposureMs/2.0));
			spikeTimes.push_back(spikeTimeMs);
		}
		spikeCount += spikeTimes.size();
		out << toString(spikeTimes,COMMA);
		if (row < (NUM_CLASSES - 1)) out << endl;
	}
	out.close();
	cout << "Created PyNN-format SpikeSource data file " << path << endl;
	return spikeCount;
}

/*
 * Create test data to trial Spinnaker classifier model
 * creates file of rate coded bands, one per VR response
 */
void generateTestVrRateCodeData(string path, int durationMs, int numVR) {
	ofstream out(path.c_str());
	float scale = 2.4;
	float maxFreq = 70 * scale;
	float freqStep  = maxFreq / (float)numVR;
	double dt = 1.0;//ms
	for (int vr = 1; vr <= numVR; vr++) {
		double freq = vr * freqStep;
		int intervalMs = 1000.0 / freq;
		vector<int> spikeTimes;
		//generatePoissonSpikeTimes(freq,0.0,dt,durationMs,spikeTimes);
		double startTimeMs = intervalMs / 2;
		generateRegularSpikeTimes(freq,startTimeMs,dt,durationMs,spikeTimes);
		out << toString(spikeTimes,COMMA);
		if (freq < maxFreq + 1) out << endl;
	}
	out.close();
	cout << "Created test data file " << path << endl;
	return;
}

/* --------------------------------------------------------------------------
load set of classes labelling the recordings
-------------------------------------------------------------------------- */
bool loadMNISTclassLabels(string path, int numObservations, vector<int> & classLabel)
{
	if (!fileExists(path)) {
		cerr << "Unable to locate labels file " << path << endl;
		exit(1);
	}

	classLabel.clear();
	classLabel.reserve(numObservations);

	//interrogate MNIST labels file
	ifstream file (path.c_str());
	if (file.is_open())
	{
		cout << "Opened MNIST labels file: " << path << endl;
		int magic_number=0;
		int numLabels=0;
		file.read((char*)&magic_number,sizeof(magic_number));
		magic_number= reverseInt(magic_number);
		cout << "Magic Number: " << magic_number << endl;
		file.read((char*)&numLabels,sizeof(numLabels));
		numLabels= reverseInt(numLabels);
		cout << "Number Of labels: " << numLabels << endl;

		if (NUM_CLASSES > numLabels) {
			cerr << "Expecting " << NUM_CLASSES << " Labels." << endl;
			exit(1);
		}

		for(int i=0;i<numObservations;++i)
		{
			unsigned char temp=0;
			file.read((char*)&temp,sizeof(temp));
			classLabel.push_back((int)temp);
			//cout << classLabel[i] << COMMA;
		}
		cout << endl;

		printf( "Class Labels loaded from file: %s\n",path.c_str());
		//checkContents("Class Labels", classLabel,numObservations,OBSERVATIONS_USED,data_type_uint,0);
	}
	return true;
}

//get a matching set of classes id's for the set of passed observations
void getClassesForObservations(vector<int>& observations, vector<int> & correspondingClasses, vector<int> & classLabels) {
	int numObservations = observations.size();
	correspondingClasses.clear();
	correspondingClasses.reserve(numObservations);

	for (int i = 0; i < numObservations; ++i) {
		correspondingClasses.push_back(classLabels[observations[i]]);
	}
}

float * generateClassActivationRateCoding(vector<int> & observations, vector<int> & classLabels) {
	unsigned int numRateCodes = observations.size() * NUM_CLASSES;
	float * activationRateCodes = new float[numRateCodes];
	initArray(activationRateCodes,numRateCodes,1.0f); //initalise all with with "zero" rate code 1Hz
	vector<int> activeClasses;
	getClassesForObservations(observations, activeClasses, classLabels);
	for (int ob = 0; ob < observations.size(); ++ob) {
		//Just fill in the single class entry for each observation with a high firing rate
		activationRateCodes[ob*NUM_CLASSES + activeClasses[ob]] = CLASS_ACTIVATION_SIGNAL_FREQ_HZ;
	}
	return activationRateCodes;

}

int generateClassActivationSpikeSourceData(string classSpikeSourcePath,vector<int> & observations, vector<int> & classLabels) {
	vector<int> activeClasses;
	getClassesForObservations(observations, activeClasses, classLabels);
	cout << "Classes for observation set:" << endl;
	cout << toString(activeClasses,SPACE) << endl;
	return generateClassActivationSpikeSourceData(activeClasses,classSpikeSourcePath);

}

void createObservationsDataFile(vector<int> & observationsSet, float * fullObservationsData, string targetDir, string targetFilename) {

	string trainingDataRecordingsFilePath = targetDir + SLASH + targetFilename;
	if(fileExists(trainingDataRecordingsFilePath)) {
		wildcardFileDelete(targetDir , targetFilename,false);
	}

	FILE * dataFile = fopen(trainingDataRecordingsFilePath.c_str(),"a");

	int numObservations = observationsSet.size();

	cout << toString(observationsSet,SPACE) << endl;

	for (int i=0; i< numObservations; i++ ) {

		//get the id of the recording to be inlcuded in the training data
		UINT observationIdx = observationsSet[i];

		//get pointer to start of cached array entry for that recording
		float * recordArray = & fullObservationsData[observationIdx*NUM_FEATURES];
		//add that sample to the training set
		writeArrayToTextFile(dataFile,recordArray,1,NUM_FEATURES,data_type_float,5,false,COMMA,false);
		fprintf(dataFile, "\n");

	}

	fclose(dataFile);

}

float * loadMNISTdata(string dataPath, int numImagesToLoad) {
	//interrogate MNIST data file
	ifstream file (dataPath.c_str());
	if (!file.is_open()) {
		cerr << "Failed to open MNIST file: " << dataPath << endl;
		exit(1);
	} else {
		cout << "Opened MNIST data file: " << dataPath << endl;
		int magic_number=0;
		int number_of_images=0;
		int n_rows=0;
		int n_cols=0;
		file.read((char*)&magic_number,sizeof(magic_number));
		magic_number= reverseInt(magic_number);
		cout << "Magic Number: " << magic_number << endl;
		file.read((char*)&number_of_images,sizeof(number_of_images));
		number_of_images= reverseInt(number_of_images);
		cout << "Number Of Images: " << number_of_images << endl;
		file.read((char*)&n_rows,sizeof(n_rows));
		n_rows= reverseInt(n_rows);
		file.read((char*)&n_cols,sizeof(n_cols));
		n_cols= reverseInt(n_cols);
		cout << "Image dimensions: " << n_rows << " x " << n_cols << endl;

		int pixels = n_rows * n_cols;
		if (NUM_FEATURES != (pixels)) {
			cerr << "Expecting " << NUM_FEATURES << " features (pixels). This file has " << n_rows << " x " << n_cols  << " = " << n_rows * n_cols << endl;
			exit(1);
		}

		if (numImagesToLoad > number_of_images) {
			cerr << "Expecting " << numImagesToLoad << " data samples (images). This file has only " << number_of_images << endl;
			exit(1);
		}
		if (numImagesToLoad < number_of_images) {
			cerr << "WARNING: Using " << numImagesToLoad << " data samples (images). This file has " << number_of_images << " available."<< endl;
		}

		//allocate data to load ALL recordings into
		float * loadedImages = new float [numImagesToLoad * NUM_FEATURES];

		for(int i=0;i<numImagesToLoad;++i)
		{
			for(int r=0;r<n_rows;++r)
			{
				for(int c=0;c<n_cols;++c)
				{
					long dataIndex = i * pixels + r * n_cols + c;

					unsigned char temp=0;
					file.read((char*)&temp,sizeof(temp));
					//translate to floats. This is wasteful in storage,
					//but retains compatibiltiy with all exisiting code wich assumes non-int, real number data
					loadedImages[dataIndex] = (float)temp;
				}
			}
			//cout << "Loaded image " << i << endl;
			//if (i<10) displayMNISTdigit(&loadedImages[i * pixels]);
		}
		cout << "Loaded all images ok." << endl;
		return loadedImages;
	}

}

struct ObservationDistanceLimits {
	float max,min,avg;
};

/* --------------------------------------------------------------------------
Calculate the response of a given VR to a single sample of sensor data (vector in feature space)
-------------------------------------------------------------------------- */
float calculateVrResponse(float * samplePoint, float * vrPoint, ObservationDistanceLimits distanceLimits)
{
	//get the Manhattan distance metric
	float distance = getManhattanDistance(samplePoint, vrPoint, NUM_FEATURES);

	//normalise response to a number in range 0..1

#ifdef SCALE_WITH_MAX_DISTANCE
	float scaleDistance  = distanceLimits.max;
#else
	float scaleDistance  = distanceLimits.avg;
#endif

	//compress (param_VR_RESPONSE_SIGMA_SCALING < 1) or expand (param_VR_RESPONSE_SIGMA_SCALING > 1) the distance axis
	float sigma = VR_RESPONSE_SIGMA_SCALING * scaleDistance;


#ifdef USE_NON_LINEAR_VR_RESPONSE
	//use parameterised e^(-x) function to get an customised response curve
	float power = VR_RESPONSE_POWER;
	float exponent = -powf(distance/sigma,power);
	float response  = expf(exponent);
	//cout << param_VR_RESPONSE_SIGMA_SCALING << COMMA << distance << COMMA << response << endl;

#else
	//use linear response scaled by the average or max distance between samples
	float response  =  1 - (distance /sigma ) ;
#endif

	return response < 0 ? 0: response; //if further than scale distance, cluster points (VR's) contribute no response

}

/* --------------------------------------------------------------------------
Go through the all pairs in the observation data set to find  the max and minimum absolute "distance" existing between 2
-------------------------------------------------------------------------- */
void findMaxMinObservationDistances(float  * allObservationData, UINT startAtObservation, UINT totalObservations, ObservationDistanceLimits & distanceLimits,float & totalledDistance, UINT & observationsCompared) {

	//we need to do an all vs all distance measurement to find the two closest and the two furthest points

	float * sampleA  = &allObservationData[startAtObservation * NUM_FEATURES]; //remember each observation comprises NUM_FEATURES floats

	for (UINT i = startAtObservation+1; i < totalObservations; i++) {
		float * sampleB  = &allObservationData[i * NUM_FEATURES];
		//printf("Sample B (%f,%f,%f,%f)\n",sampleB[0],sampleB[1],sampleB[2],sampleB[3]);
		float distanceAB = getManhattanDistance(sampleA,sampleB,NUM_FEATURES);
		if (distanceAB > distanceLimits.max || distanceLimits.max<0.0) {
			distanceLimits.max = distanceAB ; //update max
			printf("Max updated! sample:%d vs sample:%d    max:%f min:%f\n",startAtObservation,i,distanceLimits.max,distanceLimits.min);
		} else if (distanceAB < distanceLimits.min|| distanceLimits.min<0.0) {
			distanceLimits.min = distanceAB; //update min
			printf("Min updated! sample:%d vs sample:%d    max:%f min:%f\n",startAtObservation,i,distanceLimits.max,distanceLimits.min);
		}
		totalledDistance = totalledDistance  + distanceAB;
		observationsCompared++;

	}

}



/* --------------------------------------------------------------------------
Load full data set to find  the max and minimum absolute "distance" existing between 2 points in the sensor recording data set
-------------------------------------------------------------------------- */
ObservationDistanceLimits calculateDistanceLimits(float * allObservationData, UINT totalObservations) {

	ObservationDistanceLimits distanceLimits {-1.0,-1.0,-1.0};

	//load all sample data into one giant array (because we need to compare all vs all)
	printf( "Interrogating the full data set to find  the max and minimum distances between observation points..\n");

	float totalledDistance = 0;
	UINT observationsCompared = 0;

	for (int startAtObservation = 0; startAtObservation < (totalObservations-1); startAtObservation++) {
		//cout << "Checking sample " << startAtObservation << " of " << totalObservations << " against " << (totalObservations-startAtObservation) << " samples remaining." << endl;
		findMaxMinObservationDistances(allObservationData,startAtObservation,totalObservations,distanceLimits,totalledDistance,observationsCompared);
	}

	distanceLimits.avg = totalledDistance /((float)observationsCompared);

	printf( "Completed Max/min search.\n");

	return distanceLimits;

}

string getMaxMinDistanceFilepath(string srcDir,UINT totalObservations) {
	return srcDir + "/MaxMinAvgSampleDistances_"+ toString(totalObservations) + "Observations.csv";
}

/* --------------------------------------------------------------------------
Get the max or minimum absolute "distance" existing between any 2 points in the full observation data set
If no cached version exists then calculate by interrogating the full data set
-------------------------------------------------------------------------- */
ObservationDistanceLimits  loadDistances(string srcDir, float * allObservationData, UINT totalObservations)
{
	string path = getMaxMinDistanceFilepath(srcDir,totalObservations);

	ObservationDistanceLimits distanceLimits;

	if (fileExists(path)) {
		float distance[3]; //will hold the loaded max, min and avg values
		loadArrayFromTextFile(path,distance,",",3,data_type_float);
		distanceLimits.max = distance[0];
		distanceLimits.min = distance[1];
		distanceLimits.avg = distance[2];
		printf( "Max (%f), Min (%f) and Average (%f) Sample Distances loaded from file %s.\n",distanceLimits.max,distanceLimits.min,distanceLimits.avg,path.c_str());

	} else {
		//file doesn't exist yet, so need to generate values
		distanceLimits = calculateDistanceLimits(allObservationData,totalObservations);

		//now write to ache file
		FILE *f= fopen(path.c_str(),"w");
		fprintf(f,"%f,%f,%f",distanceLimits.max,distanceLimits.min,distanceLimits.avg);
		fclose(f);
		printf( "Max, Min and Average Sample Distances written to file: %s.\n",path.c_str());

	}
	return distanceLimits;
}

string getCacheSubDir(UINT observationIdx, string cacheDir) {
	createDirIfNotExists(cacheDir);
	int numSubDir = 250;
	string subDir = toString(observationIdx / numSubDir);
	string dir  = cacheDir + SLASH + subDir;
	createDirIfNotExists(dir);
	return dir;
}


string getVrResponseFilePath(UINT observationIdx, string cacheDir, int numVR)
{
	stringstream vrPath;
	vrPath << getCacheSubDir(observationIdx,cacheDir) << SLASH << "VRResponse_" << "_Observation" << observationIdx << "_" << numVR <<   "VRs.csv";
	return vrPath.str();

}

void normaliseToMaxEntry(float * vrResponse, UINT numVRs) {
	float max = vrResponse[getIndexOfHighestEntry(vrResponse,numVRs)];
	//max = clip(dither(max,0.1),0.0f,1.0f);

	if (max==0.0) return; //all zero response (shouldnt really happen) avoid division by zero
	for (UINT vr = 0; vr < numVRs; ++vr) {
		vrResponse[vr] = vrResponse[vr] / max;
	}
}

/*
 * Complete a 1D array of VR responses to a specified observation
 */
void getVrResponse(float * vrResponse,int observationIdx , UINT numVRs, float * vrData, float * allObservationData, ObservationDistanceLimits distanceLimits,string responseCacheDir) {

	string cachedResponsePath = getVrResponseFilePath(observationIdx,responseCacheDir,numVRs);
	if (fileExists(cachedResponsePath)) {
		loadArrayFromTextFile(cachedResponsePath,vrResponse,COMMA,numVRs,data_type_float);
	} else {
		float * observationDataPoint = &allObservationData[observationIdx * NUM_FEATURES];

		for (UINT vr=0 ; vr < numVRs; vr++) {//step through the set of VRs

			float * vrPoint = &vrData[vr * NUM_FEATURES]; //get a ptr to start of the next VR vector in the set

			//Calculate the response of the VR to the sample point
			float singleVrResponse  = calculateVrResponse(observationDataPoint,vrPoint,distanceLimits);
			vrResponse[vr] = singleVrResponse;
		}
		normaliseToMaxEntry(vrResponse, numVRs);
		FILE * file = fopen(cachedResponsePath.c_str(),"w");
		writeArrayToTextFile(file,vrResponse,1,numVRs,data_type_float,5, false,COMMA,true);

	}
}
/*
 * Allocate amd complete an array of VR responses to each of a set of observations
 * There is a row in the result for each of the specified set of observations
 * The nth column in the result refers to the 0..1 response of the nth VR to that observation
 */
float * getVrResponses(vector<int> targetObservations, UINT numVRs, string vrDataPath, float * allObservationData, string dataDir, string responseCacheDir) {

	float * vrData = new float [NUM_FEATURES * numVRs];
	loadArrayFromTextFile(vrDataPath,vrData,COMMA,NUM_FEATURES * numVRs,data_type_float);

	//get the distance limits for the data set, used to normalise the VR responses
	ObservationDistanceLimits distanceLimits = loadDistances(dataDir,allObservationData,targetObservations.size());

	UINT numTargetObservations = targetObservations.size();
	float * vrResponses = new float[numVRs * numTargetObservations];

	//for each observation, get a distance metric to each VR point, these will become the input rate levels to the network
	for (UINT i=0 ; i < numTargetObservations; i++) {
		int observationIdx = targetObservations[i];
		//fill in the correect row in the VR response array
		getVrResponse(&vrResponses[i*numVRs],observationIdx,numVRs,vrData,allObservationData,distanceLimits,responseCacheDir);
	}
	return vrResponses;
}

//return the first N observation id's of each class stipulated in the requiredClasses vector
void getFirstNInstancesPerClass(UINT n, vector<int> & classLabels,vector<int> & requiredClasses,vector<int> & observationsSet) {

	UINT numRequiredClasses = requiredClasses.size();
	observationsSet.clear();
	UINT numRequiredObservations = numRequiredClasses*n;
	observationsSet.reserve(numRequiredObservations);
	UINT classCount[NUM_CLASSES];
	zeroArray(classCount,NUM_CLASSES);

	UINT i = 0;
	while (observationsSet.size()< numRequiredObservations && i < classLabels.size()) {
		int observationClass = classLabels[i];
		if (vectorContains(requiredClasses,observationClass) && classCount[observationClass] < n) {
			classCount[observationClass]++;
			observationsSet.push_back(i);
			//cout << "Added observation " << i << " of class " << observationClass << endl;
		}
		i++;
	}
	cout << "Completed observation set with " << observationsSet.size() << " entries.";
	cout << "Class counts:-" << endl;
	for (int j = 0; j < NUM_CLASSES; ++j) cout << j << ": " << classCount[j] << endl;


}

//use this function instead to get observations ordered/stratified by class e.g. 0,1,2,3,4..
void getFirstNPerClassInOrder(UINT n, vector<int> & classLabels,vector<int> & orderedClasses,vector<int> & observationsSet) {

	UINT numRequiredClasses = orderedClasses.size();
	observationsSet.clear();
	UINT numRequiredObservations = numRequiredClasses*n;
	if (classLabels.size() < numRequiredObservations) {
		cerr << "Insufficient observations available to provide " << n << " per class for " << numRequiredClasses << " classes." << endl;
		exit(-1);
	}
	observationsSet.reserve(numRequiredObservations);

	//separate into vectors indexing the memebers of each class
	deque<int> classMembers[NUM_CLASSES]; //should be iniitalised with a emtpy vector in
	for (int ob = 0; ob < classLabels.size(); ++ob) {
		int classId = classLabels[ob];
		classMembers[classId].push_back(ob);
	}

	cout << "Class separation:" << endl;
	for (int cls = 0; cls < NUM_CLASSES; ++cls) {
		cout << "Class " << cls << ": " << classMembers[cls].size();
	}

	//now take one entry from each in turn according to ordered classes set provides

	UINT i = 0;
	while (observationsSet.size()< numRequiredObservations) {
		int requiredClass = orderedClasses[i];
		deque<int> & requiredSet = classMembers[requiredClass];
		if (requiredSet.size() > 0) {//still some of this class left to use
			int obIdx = requiredSet[0];
			requiredSet.pop_front();
			observationsSet.push_back(obIdx);
		}
		i++;
		if (i==orderedClasses.size()) i=0;
	}
	cout << "Completed observation set with " << observationsSet.size() << " entries.";

}

#endif
