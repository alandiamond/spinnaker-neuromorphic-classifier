SPINN5- LARGE MODEL - LIVE SPIKING 
This experiment demonstrates classification of MNIST using a much larger model (tested up to 30K neurons) running on the 48-chip SPiNN5 board. 

To achieve the throughput of input data for this large dataset, rather than spike source files, this experiment uses a C/C++ front end program to send more than 500 channels of live spikes to the board via UDP. Similary, to quantify the performance , spikes on the output layer of the model are sent back live to receivers in the C front-end.

The C/C++ program was developed and tested on Ubuntu Linux using the g++ compiler. There is a dependency on the sqlite library to be installed and linked. The program can be compiled using the makefile in the Release subdirectory or using any IDE of choice (Eclipse was used for the development).

The SpiNNaker side was controlled using the "Arbitrary" release of the SpyNNaker software libraries (see https://github.com/SpiNNakerManchester for installation instructions).

The settings for the experiment can be adjusted to fit the user's platform and requirements by editing the following files:-

1) The testLimitedClasses_SeparateTrainingAndTestDataset() method in the experiment.cpp file controls the flow of the experiment and sets up which parameters are to be explored, e.g. for how long each MNIST digit best presented to the classifer network?

2) The experiment.h file defines the static settings for the C++ front end e.g. directory paths, IP address of the SPiNNaker board. 

3) The python/Settings-MNISTClassifier.txt file performs a simlar job for the Python/PyNN code.
4) The python/ModelParams--MNISTClassifier.txt file controls the settings for the neural model, these should require minimal adjustment.

