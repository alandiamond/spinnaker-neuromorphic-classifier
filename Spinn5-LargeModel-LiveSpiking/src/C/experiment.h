/*--------------------------------------------------------------------------
   Author: Alan Diamond
  
   initial version: 2014-Mar-05
  
--------------------------------------------------------------------------

Header file containing global variables, constants and macros used in running the Schmuker_2014_classifier.

-------------------------------------------------------------------------- */

/*--------------------------------------------------------------------------
//Experiment constants
 --------------------------------------------------------------------------*/

#ifndef EXPERIMENT_H
#define EXPERIMENT_H
#define OUTPUT_DIR "/home/ad246/SpinnakerClassifier/output"

//Spinnaker simulation / Python parameters
#define PYTHON_USE_PROFILER false
#define USE_LIVE_SPIKING true
#define PYNN_MODEL_SCRIPT_LIVE_SPIKING "BuildAndRunClassifier_LiveSpikingInput.py"
#define PYNN_MODEL_SCRIPT_SPIKE_SOURCES "BuildAndRunClassifier.py"
#define PYNN_MODEL_EXTRA_STARTUP_WAIT_SECS 2//extra seconds to wait before pynn model is actually running. This can be used to know when to start sending live spikes
#define SPINNAKER_BOARD_IP_ADDR "192.168.240.1" //big board
//#define SPINNAKER_BOARD_IP_ADDR "192.168.240.252" //small board
#define SPINNAKER_DATABASE_PORT 19999// For now the port for connection to spinnaker is fixed, but this will change in next release
#define RN_FIRST_SPIKE_INJECTION_PORT  12347 //the first port where the spinnkaer RN poluation will listen for UDP spikes. More may get added if larger than max size by incrementing port number
#define MAX_SIZE_SPIKE_INJECTION_POP 256 //larger injection pop cause spinnkaer to crash with resource isssue. RPesumably they are being spread across more than one core , each then tries to plug a socket onto the same port.
#define RN_SPIKE_INJECTION_POP_LABEL "SpikeInjectionRN" // the name that will be assigned to the population in Spinnaker model
#define CLASS_ACTIVATION_SPIKE_INJECTION_PORT  12346 //the port where the spinnaker class activcation poluation will listen for UDP spikes during learning
#define CLASS_ACTIVATION_SPIKE_INJECTION_POP_LABEL "SpikeInjectionClassActivation" // the name that will be assigned to the population in Spinnaker model
#define HOST_RECEIVE_PORT 12345 //Host port to which live spikes wil be returned from spinnaker
#define PYTHON_DIR "/home/ad246/Dropbox/SpiNNaker/spinnaker_mnist_live_spiking/src/python" //where to find python scripts used by experiment
#define SPINNAKER_CACHE_DIR "/home/ad246/SpinnakerClassifier/cache"
#define VR_RESPONSE_SPIKE_SOURCE_FILENAME "VrResponse_SpikeSourceData.csv" //file where spike source created from ratecoded VR response is placed for spinnaker model to pick up
#define CLASS_ACTIVATION_SPIKE_SOURCE_FILENAME "ClassActivation_SpikeSourceData.csv" //file providing spikes to activate the relevant AN cluster as a teaching signal
#define CLASSIFICATION_RESULTS_FILENAME "WinningClassesByObservation.csv"
#define CLASS_LABELS_FILENAME "ClassLabels.csv"
#define PYTHON_RUN_COMPLETE_FILE "PynnRunCompleted.txt"
#define CLASS_ACTIVATION_EXPOSURE_FRACTION 0.7 //when using teaching signal what fraction (0..1) of the observation exposure time is used
//#define CLASS_ACTIVATION_SIGNAL_FREQ_HZ 300.0 //the spiking rate to use for the teaching signal input neuron to indicate/activate the correct class/AN
#define CLASS_ACTIVATION_SIGNAL_FREQ_HZ 300.0
#define DT 1.0 //integration timestep ms used in Spinnaker simulation

//Dataset parameters
#define DATASET_NAME "MNIST"
#define OBSERVATIONS_DATA_DIR "/home/ad246/SpinnakerClassifier/mnist_data"
//std name for file holding all the discrete samples or subsamples in the dataset
//This file is primarly used to drive the clustering algorthm and to locate max/imn/avg distances betweeen samples
#define FILENAME_ALL_SAMPLES_TRAINING "train-images.idx3-ubyte"
#define FILENAME_CLASS_LABELS_TRAINING "train-labels.idx1-ubyte"
#define FILENAME_ALL_SAMPLES_TESTING "t10k-images.idx3-ubyte"
#define FILENAME_CLASS_LABELS_TESTING "t10k-labels.idx1-ubyte"
#define DATA_CACHE_DIR "/home/ad246/SpinnakerClassifier/mnist_cached_data" //location for cached input data after transformation to set of VR responses and then rate code
//#define DATA_CACHE_DIR "/home/ad246/Dropbox/SpiNNaker/spinnaker_mnist_classifier/src/python/VrResponseCache" //use this to match the response set used for the github experiment1 demo
#define IMAGE_WIDTH_PIXELS 28
#define IMAGE_HEIGHT_PIXELS 28
#define NUM_FEATURES (IMAGE_WIDTH_PIXELS * IMAGE_HEIGHT_PIXELS) //dimensionality of data set
#define NUM_CLASSES 10 //number of classes to be classified = digits 0-9
#define DEFAULT_CLUSTER_SIZE 30

//VR parameters
#define DEFAULT_NUM_VR 200 //default number of VR generated to map the input space. Usually overriden in in main fn e.g. to exolore a range of num VR
#define VR_DATA_FILENAME_TEMPLATE "tmpTrainingDataVRSet.csv"
#define NUM_EPOCHS 100 //indicates how many re-exposures were used when invoking the clustering algorithm (neural gas)
#define VR_DIR "VRData" //subdir below recordings dir where VR files stored
#define USE_VR_CACHE true //whehter to always make new VR set from obsevation training set being used


//Control of statisitics gathering / scoring
//#define N_FOLDING 5 //e.g. five fold folding of training vs test data  -> 80:20 split
#define N_FOLDING 10  // SVM results were obtained with 10-fold
#define USE_FOLDINGS N_FOLDING //set this to less than N_FOLDING to move on quicker. e.g. get quick results with just 2 foldings
//#define USE_FOLDINGS 5

#define N_REPEAT_TRIAL_OF_PARAMETER 1 //Repeat each crossvalidation of a parameter how many times . e.g. N=50. performance variation (Std deviation) can be high for low no of repeats

//#define TRAINING_DATASET_SIZE  60000 //How many digits ("observations") are in the underlying dataset
#define TRAINING_DATASET_SIZE 25000
#define TESTING_DATASET_SIZE 10000
#define DEFAULT_OBSERVATIONS_USED_PER_CLASS 100
//#define DEFAULT_OBSERVATIONS_USED_PER_CLASS 10

#define DEFAULT_OBSERVATION_EXPOSURE_TIME_MS 120.0 //How long is each encoded digit presented for in ms

//Generating and displaying data during a run
//#define FLAG_GENERATE_RASTER_PLOT_DURING_TRAINING_REPEAT 0 //specifiy rpt in which to create rasters. not defined = none. -1 = all repeats. Use 0 for immediate raster plots. Use REPEAT_LEARNING_SET-1 for plots in last stage of training
//#define FLAG_GENERATE_RASTER_PLOT_DURING_TESTING 1

#define RASTER_FREQ 1 //generate a raster plot every N recordings.
#define DISPLAY_RASTER_IMMEDIATELY 1
#define RASTER_PLOT_EXTRA "ACT"; //As well as raster plot also show either ACTivation bars or HEATmap
//#define WRITE_CLUSTER_SPIKE_COUNTS //debug option to look at unususal changes in spiking rates across whole runs

//#define WRITE_VR_ACTIVATION_RESULTS 1 //write files tottting up VR activation
//#define FLAG_GENERATE_VR_ACTIVATION_DATA_PER_RECORDING 1 //write a file per recording showing VR response in RN and PN layers
//#define TRACK_PERFORMANCE_DETAIL 1  //write details of performance per recording and per class

//#define FLAG_OUTPUT_INDIVIDUAL_RESULTS 1 //write a file giving the result details of every classification decision
#define OUTPUT_MISTAKES_ONLY 1 //for the above file , include all results or only mistakes

//#define FLAG_OVERRIDE_RECORDING_SELECTED 1 // query user to select a specific recording

/*--------------------------------------------------------------------------
Data and extra network dimensions (see also Model file)
 --------------------------------------------------------------------------*/

//this range is used to scale the input data into firing rates (Hz)
#define MAX_FIRING_RATE_HZ 70 //Hz
#define MIN_FIRING_RATE_HZ 00 //Hz

//define VR response function
//#define SCALE_WITH_MAX_DISTANCE  //use either the Max or Avg distance to scale response
//#define USE_NON_LINEAR_VR_RESPONSE 1
//#define VR_RESPONSE_SIGMA_SCALING 1.25f //linear response setting used in GeNN MNIST
//#define VR_RESPONSE_SIGMA_SCALING 1.1f //linear response setting used in github experiment1
#define VR_RESPONSE_SIGMA_SCALING 0.8f
//#define VR_RESPONSE_SIGMA_SCALING 0.5f //non linear setting
#define VR_RESPONSE_POWER 0.7f //only used if non linear rsponse specified

#endif

