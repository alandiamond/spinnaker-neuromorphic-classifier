#ifndef neuralgas_cc
#define neuralgas_cc
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <math.h>
#include <algorithm> //for std:sort
#include "utilities.cc"

//definition for a set of GasNode objects . These will be held in a vector that must be repeatedly sorted according to distance from current observation
struct GasNode {
	float * pos; //ptr to an array containing the current poistion in data space of the Node (VR)
	//float * x;   //ptr to an array containing the current poistion in data space of the currnt observation being processed
	float * relVec;  //ptr to an array containing the relative vector going from pos to x
	float distance; //the distance from pos to x
};

bool compareNodeDistances(GasNode node1, GasNode node2) {
	return node1.distance < node2.distance;
}

/*
 * Uncover the best set of N nodes to describe the observation data set according to the neural gas algorithm
 * NB: the array to store the result of nodesRequired nodes must be preallocated and paased in by ptr
 * The parameter startFromExisting indicates wehther to assume this array is prepopulated with a starting node set or
 * whehter to overwrite with a random set of nodes
 */
bool runStdNeuralGas(float * observations, int numFeatures, int numObservations, float* resultNodes, int nodesRequired, bool startFromExisting, int epochs) {

	//checkContents("First 10 observation values ",observations,10*numFeatures,numFeatures,data_type_float,3);
	/*
    epsilon_i, epsilon_f
      initial and final values of epsilon. Fraction of the distance
      between the closest node and the presented data point by which the
      node moves towards the data point in an adaptation step. Epsilon
      decays during training by e(t) = e_i(e_f/e_i)^(t/t_max) with t
      being the epoch. */
    float epsilon_i=0.3; 	//initial epsilon
    float epsilon_f=0.05;  	//final epsilon

    /*
    lambda_i, lambda_f
      initial and final values of lambda. Lambda influences how the
      weight change of nodes in the ranking decreases with lower rank. It
      is sometimes called the "neighborhood factor". Lambda decays during
      training in the same manner as epsilon does.
      */
	float lambda_i=30.0;	//initial lambda
	float lambda_f=0.01;    //final lambda

	if (!startFromExisting) {
		//borrow N random points from the data set to start with
		for (int node = 0; node < nodesRequired; ++node) {
			int randIdx = rand() % numObservations;
			singleVectorCopy(& resultNodes[node * numFeatures], & observations[randIdx * numFeatures], numFeatures);
		}
	}
	//checkContents("Starting node values",resultNodes,nodesRequired*numFeatures,numFeatures,data_type_float,3);

	//preallocate space for the relative vec data held by each node
	float relVecs[numFeatures * nodesRequired];

	//load a vector to hold the Nodes. These will be repeatedly sorted in distance order
	vector<GasNode> rankedNodes;
	for (int j = 0; j < nodesRequired; ++j) {
		GasNode n;
		n.pos = & resultNodes[j * numFeatures]; //point pos to the relevant array entry
		n.relVec = & relVecs[j * numFeatures];  //point relVec to the preallocated space , ready to receve this data when calucated
		rankedNodes.push_back(n);
	}

	//create vector of indices of the observations (we will be shuffling this on every epoch, to present the data in random order)
	vector<int> observeIndices;
	for (int i = 0; i < numObservations; ++i) {
		observeIndices.push_back(i);
	}

	//create holder for the current distance rank metric (this only changes by epoch, so we can cache it)
	float distanceRankMetric[nodesRequired];

	cout << "Epoch: ";
	for (int epoch = 0; epoch < epochs; ++epoch) {

		if (epoch % 10 == 0) cout << epoch << SPACE;
		cout.flush();

		//printSeparator();
		//cout << "Starting epoch " << epoch << " of " << epochs << endl;
		//printSeparator();

		//shuffle the observations order for this epoch
		random_shuffle(observeIndices.begin(),observeIndices.end());
		//cout << "Shuffled observation indices" << endl;
		//cout << toString(observeIndices,COMMA) << endl;

		//adjust the epsilon and lambda params according to how far through we are
		float completion = (float)epoch/(float)epochs;
		//cout << "Completion " << completion << endl;
        float epsilon = epsilon_i * powf((epsilon_f/epsilon_i),completion);
        //cout << "epsilon updated to " << epsilon << endl;
        float lambda = lambda_i * powf((lambda_f/lambda_i),completion);
        //cout << "lambda updated to " << lambda << endl;

        //update distance metric once for this epoch.
        //Note that as an optimisation for performance,
        //everything below a near zero threshold is zeroed and the rank at which this point is reached is saved to use as a cutoff
        bool passedThreshold = false;
        int cutoffRankIdx = nodesRequired -1;
        float threshold = powf(10,-5);
        for (int rank = 0; rank < nodesRequired; ++rank) {
        	if (!passedThreshold) {
        		distanceRankMetric[rank] = epsilon * expf(-rank/lambda);
        		passedThreshold = distanceRankMetric[rank] < threshold;
        		cutoffRankIdx = rank; //save the rank where the cut off point was detected.
        	} else {
        		distanceRankMetric[rank] = 0.0f; //zero everything below the threshhold
        	}
		}
        //checkContents("Updated distance rank metrics", distanceRankMetric,nodesRequired,10,data_type_float,10,SPACE);
        //cout << "Cutoff set at " << cutoffRankIdx << " nearest nodes";

        //go thorugh the data points observations in random order
		//cout << "Observation:" << endl;
        for (int i = 0; i < numObservations; ++i) {
        		//cout << "Using observation " << observeIndices[i] << " as observation " << i << " of " << numObservations << endl;
        		//if (i % 200 == 0) cout << i << SPACE;
        		//cout.flush();

        		float * x = & observations[observeIndices[i] * numFeatures]; //get ptr to start of current observation

        		//cout << "Updated distances before sort:-" << endl;
        		//update the relative vector and distance from each node position
        		bool useManhattan = true; //use the manhattan distance not euclidean (saves on sqrt etc)
        		for (int j = 0; j < nodesRequired; ++j) {
        			vectorSubtract(rankedNodes[j].relVec,x,rankedNodes[j].pos,numFeatures);
        			rankedNodes[j].distance = vectorLength(rankedNodes[j].relVec,numFeatures,useManhattan);
        			//cout << rankedNodes[j].distance << SPACE;
        		}
        		//cout << endl;

        		//cout << "updated the relative vector and distance from each node position" << endl;

                //Step 1 rank nodes according to their distance to current random observation x
                //std::sort(rankedNodes.begin(),rankedNodes.end(),compareNodeDistances);
                //cout << "Sorted " << nodesRequired << " nodes according to their distance to current random observation x" << endl;

        		//version II : use partial_sort on just the top N closest nodes. As the rest will have a near-zero distance metric there is no point sorting them
        		std::partial_sort(rankedNodes.begin(),rankedNodes.begin()+cutoffRankIdx,rankedNodes.end(),compareNodeDistances);

                //cout << "Updated distances AFTER sort:-" << endl;
                //Step 2 move nodes towards x according to distance and rank
				for (int rank = 0; rank < cutoffRankIdx; ++rank) {
					GasNode & node = rankedNodes[rank]; //needs to be a reference or we create a copy and the original node wont get updated
					//delta_w = distanceRankMetric[rank] * (x - node.pos);
					float delta_w[numFeatures];
					singleVectorMultiply(delta_w,node.relVec,numFeatures,distanceRankMetric[rank]);
					//node.pos += delta_w;
					vectorAdd(node.pos,node.pos,delta_w,numFeatures); //move the node a weighted distance towards the current observation x
					//cout << rankedNodes[rank].distance << SPACE;
				}
				//cout << endl;
				//cout << "Moved closest " << cutoffRankIdx << " nodes towards x according to distance and rank" << endl;
        }
        //cout << endl; //end of observations
	} // end of epochs
	cout << endl;
	cout << "Completed neural gas process to obtain " << nodesRequired << " nodes from " << numObservations << " observations." << endl;
	return true;
}

/**
 * Overload using files for IO not arrays
 */

bool runStdNeuralGas(string srcDir, string srcFilename, string resultFilename, string fileDelim, int numResultNodesRequired, int epochs) {

	string srcPath = srcDir + SLASH + srcFilename;
	string resultPath  = srcDir + SLASH + resultFilename;
	int numFeatures = countColsInTextFile(srcPath,fileDelim);
	int numObservations = countRowsInTextFile(srcPath,false);

	cout << "Invoking Neural Gas algorithm." << endl;
	cout << "Data will be taken from file " << srcPath << endl;
	cout << "which contains " << numFeatures << " features and " << numObservations << " observations." << endl;
	cout << "Output will be written to file " << resultPath << endl;

	float * observations = new float[numFeatures * numObservations];
	bool ok = loadArrayFromTextFile(srcPath,observations,fileDelim,numFeatures*numObservations,data_type_float);

	//create holder for the result set
	float * resultNodes = new float[numFeatures * numResultNodesRequired];

	ok =  runStdNeuralGas(observations, numFeatures, numObservations, resultNodes, numResultNodesRequired, false, epochs);
	delete[] observations;

	if (ok) {
		FILE * resultFile = fopen(resultPath.c_str(),"w");
		ok =  writeArrayToTextFile(resultFile,resultNodes,numResultNodesRequired,numFeatures,data_type_float,10,false, fileDelim, true);
		cout << "Output written to file " << resultPath << endl;
	}
	delete[] resultNodes;
	return ok;

}
#endif
