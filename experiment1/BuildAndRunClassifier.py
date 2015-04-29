import matplotlib.pyplot as plt
import Classifier as classifier
import ModellingUtils as utils
import sys
import os.path
import os

params = eval(open("ModelParams-MNISTClassifier.txt").read())
settings = eval(open("Settings-MNISTClassifier.txt").read())

#clear marker file
if utils.fileExists(settings['RUN_COMPLETE_FILE']):
    os.remove(settings['RUN_COMPLETE_FILE'])

#Override default params with any passed args
numArgumentsProvided =  len(sys.argv) - 1

if numArgumentsProvided >=1 :
    settings['LEARNING'] = eval(sys.argv[1])
if numArgumentsProvided >=2 :
    params['NUM_VR'] = int(sys.argv[2])
if numArgumentsProvided >=3 :
    params['NUM_CLASSES'] = int(sys.argv[3])
if numArgumentsProvided >=4 :
    settings['SPIKE_SOURCE_VR_RESPONSE_PATH'] = sys.argv[4]
if numArgumentsProvided >=5 :
    settings['SPIKE_SOURCE_ACTIVE_CLASS_PATH'] = sys.argv[5]
if numArgumentsProvided >=6 :
    settings['NUM_OBSERVATIONS'] = int(sys.argv[6])
if numArgumentsProvided >=7 :
    settings['OBSERVATION_EXPOSURE_TIME_MS'] = int(sys.argv[7])
if numArgumentsProvided >=8 :
    settings['CLASS_LABELS_PATH'] = sys.argv[8]
if numArgumentsProvided >=9 :
    settings['CLASSIFICATION_RESULTS_PATH'] = sys.argv[8]    

classifier.printParameters('Model Parameters',params)
classifier.printParameters('Classifier Settings',settings)

populationsInput = list()
populationsNoiseSource = list()
populationsRN = list()
populationsPN = list()
populationsAN = list()
projectionsPNAN = list() #keep handle to these for saving learnt weights

totalSimulationTime = settings['OBSERVATION_EXPOSURE_TIME_MS'] * settings['NUM_OBSERVATIONS']
print 'Total Simulation Time will be', totalSimulationTime

DT = 1.0 #ms Integration timestep for simulation

classifier.setupModel(settings, params, DT,totalSimulationTime, populationsInput, populationsNoiseSource, populationsRN,populationsPN,populationsAN,projectionsPNAN)

utils.recordPopulations(populationsInput,settings['RECORD_POP_INPUT'])
utils.recordPopulations(populationsNoiseSource,settings['RECORD_POP_NOISE_SOURCE'])
utils.recordPopulations(populationsRN,settings['RECORD_POP_RN'])
utils.recordPopulations(populationsPN,settings['RECORD_POP_PN'])
utils.recordPopulations(populationsAN,settings['RECORD_POP_AN'])

#run the model for the whole learning or the whole testing period
classifier.run(totalSimulationTime)

if settings['DISPLAY_RASTER_PLOT'] or settings['SAVE_RASTER_PLOT_AS_PNG']:
    plt.figure()
    plt.xlabel('Time/ms')
    plt.ylabel('Neurons')
    title = 'Testing'
    if settings['LEARNING']:
        title = 'Training'
    title = title + ' - MNIST Classification - ' + str(params['NUM_VR']) + ' Virtual Receptors (VRs)'
    plt.title(title)
    
    indexOffset = 0
    indexOffset = 1 + utils.plotAllSpikes(populationsInput, totalSimulationTime, indexOffset,settings['RECORD_POP_INPUT'])
    indexOffset = 1 + utils.plotAllSpikes(populationsNoiseSource, totalSimulationTime, indexOffset,settings['RECORD_POP_NOISE_SOURCE'])
    indexOffset = 1 + utils.plotAllSpikes(populationsRN, totalSimulationTime, indexOffset,settings['RECORD_POP_RN'])
    indexOffset = 1 + utils.plotAllSpikes(populationsPN, totalSimulationTime, indexOffset,settings['RECORD_POP_PN'])
    indexOffset = 1 + utils.plotAllSpikes(populationsAN, totalSimulationTime, indexOffset,settings['RECORD_POP_AN'])

#if in the learning stage
if settings['LEARNING']:
    #store the weight values learnt via plasticity, these will be reloaded as static weights for test stage
    classifier.saveLearntWeightsPNAN(settings,params,projectionsPNAN,len(populationsPN),len(populationsAN))
else:
    #save the AN layer spike data from the testing run. 
    #This data will be interrogated to find the winning class (most active AN pop) 
    #during the presentation of each test observation
    #classifier.saveSpikesAN(settings,populationsAN)
    winningClassesByObservation = classifier.calculateWinnersAN(settings,populationsAN)
    classLabels = utils.loadListFromCsvFile(settings['CLASS_LABELS_PATH'],True)
    scorePercent = classifier.calculateScore(winningClassesByObservation,classLabels)
    utils.saveListAsCsvFile(winningClassesByObservation,settings['CLASSIFICATION_RESULTS_PATH'])
        
#make sure spinnaker shut down before blocking figure is put up
classifier.end()

if settings['SAVE_RASTER_PLOT_AS_PNG']:
    filename = 'RasterPlot-Testing.png'
    if settings['LEARNING']:
        filename = 'RasterPlot-Training.png'
    plt.savefig(filename)
     
#write a marker file to allow invoking programs to know that the Python/Pynn run completed
utils.saveListToFile(['Pynn Run complete'],settings['RUN_COMPLETE_FILE']) 

if settings['DISPLAY_RASTER_PLOT']:
    plt.show()

print 'PyNN run completed.'
