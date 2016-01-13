import matplotlib.pyplot as plt
import Classifier_LiveSpikingInput as classifier
import ModellingUtils as utils
import sys
import os.path
import os
import time

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
    settings['RN_SPIKE_INJECTION_PORT'] = int(sys.argv[4])
if numArgumentsProvided >=5 :
    settings['RN_SPIKE_INJECTION_POP_LABEL'] = sys.argv[5]
if numArgumentsProvided >=6 :
    settings['CLASS_ACTIVATION_SPIKE_INJECTION_PORT'] = int(sys.argv[6])
if numArgumentsProvided >=7 :
    settings['CLASS_ACTIVATION_SPIKE_INJECTION_POP_LABEL'] = sys.argv[7]
if numArgumentsProvided >=8 :
    settings['NUM_OBSERVATIONS'] = int(sys.argv[8])
if numArgumentsProvided >=9 :
    settings['OBSERVATION_EXPOSURE_TIME_MS'] = int(sys.argv[9])
if numArgumentsProvided >=10 :
    settings['OUTPUT_DIR'] = sys.argv[10]    
if numArgumentsProvided >=11 :
    params['CLUSTER_SIZE'] = int(sys.argv[11])
if numArgumentsProvided >=12 :
    settings['STARTUP_WAIT_SECS'] = int(sys.argv[12])
    print 'updated STARTUP_WAIT_SECS  to ', settings['STARTUP_WAIT_SECS'] 

classifier.printParameters('Model Parameters',params)
classifier.printParameters('Classifier Settings',settings)
utils.writeObjectToFile(params,(settings['OUTPUT_DIR'] + '/Spynnaker-ModelParams.txt'))
utils.writeObjectToFile(settings,(settings['OUTPUT_DIR'] + '/Spynnaker-Settings.txt'))

populationsInput = list()
populationsNoiseSource = list()
populationsRN = list()
populationsPN = list()
populationsAN = list()
projectionsPNAN = list() #keep handle to these for saving learnt weights

totalSimulationTimeMs = settings['OBSERVATION_EXPOSURE_TIME_MS'] * settings['NUM_OBSERVATIONS']  + (1000 * settings['STARTUP_WAIT_SECS']) + 1000
print 'Total Simulation Time will be', totalSimulationTimeMs

startEverything = time.time()

DT = 1.0 #ms Integration timestep for simulation

classifier.setupModel(settings, params, DT,totalSimulationTimeMs, populationsInput, populationsNoiseSource, populationsRN,populationsPN,populationsAN,projectionsPNAN)


recordPopnInput,recordPopnNoise,recordPopnRN,recordPopnPN,recordPopnAN = classifier.recordSpecifiedPopulations(settings,populationsInput,populationsNoiseSource,populationsRN,populationsPN,populationsAN)
#always record AN during testing - we need the spike counts to determine the winner
'''
if not settings['LEARNING']:
    utils.recordPopulations(populationsAN,recordPopnAN)
'''

justStartModelThenStop = False    
#uncomment this section to get just a print of startup timing, use this number for the delay in the spike sender
#totalSimulationTimeMs = 10 #just run for 10ms
#justStartModelThenStop = True

#run the model for the whole learning or the whole testing period
start = time.time()
classifier.run(totalSimulationTimeMs)
runTime = (time.time() - start)

if justStartModelThenStop:
    print 'Time to complete setup and be ready to receive spikes ', runTime
    quit()


if settings['DISPLAY_RASTER_PLOT'] or settings['SAVE_RASTER_PLOT_AS_PNG']:
    title = 'Testing'
    if settings['LEARNING']:
        title = 'Training'
    title = title + ' - MNIST Classification - ' + str(params['NUM_VR']) + ' Virtual Receptors (VRs)'
    plt.figure(title,figsize=(50,5))
    plt.xlabel('Time/ms')
    plt.ylabel('Neurons')
    
    indexOffset = 0
    indexOffset = 1 + utils.plotAllSpikes(populationsInput, totalSimulationTimeMs, indexOffset,recordPopnInput)
    indexOffset = 1 + utils.plotAllSpikes(populationsNoiseSource, totalSimulationTimeMs, indexOffset,recordPopnNoise)
    indexOffset = 1 + utils.plotAllSpikes(populationsRN, totalSimulationTimeMs, indexOffset,recordPopnRN)
    indexOffset = 1 + utils.plotAllSpikes(populationsPN, totalSimulationTimeMs, indexOffset,recordPopnPN)
    indexOffset = 1 + utils.plotAllSpikes(populationsAN, totalSimulationTimeMs, indexOffset,recordPopnAN)

weightExtractionTime = 0.0
spikeExtractionTime = 0.0
start = time.time()

stageLabel = 'TESTING'
#if in the learning stage
if settings['LEARNING']:
    stageLabel = 'LEARNING'
    
    #tmp, to find out if winner functions are working , winners should go 0,1,2,3,4,5,6,7,8,9,0,1,2, etc
    #winningClassesByObservation = classifier.calculateWinnersAN(settings,populationsAN)
    
    #store the weight values learnt via plasticity, these will be reloaded as static weights for test stage
    #print 'WARNING - disabled weight saving for the moment!!'
    classifier.saveLearntWeightsPNAN(settings,params,projectionsPNAN,len(populationsPN),len(populationsAN))
    weightExtractionTime = (time.time() - start)
else:
    #save the AN layer spike data from the testing run. 
    #This data will be interrogated to find the winning class (most active AN pop) 
    #during the presentation of each test observation
    #classifier.saveSpikesAN(settings,populationsAN)
    '''
    winningClassesByObservation = classifier.calculateWinnersAN(settings,populationsAN)
    classLabels = utils.loadListFromCsvFile(settings['CLASS_LABELS_PATH'],True)
    scorePercent = classifier.calculateScore(winningClassesByObservation,classLabels)
    utils.saveListAsCsvFile(winningClassesByObservation,settings['CLASSIFICATION_RESULTS_PATH'])
    score = {'scorePercent':scorePercent}
    utils.writeObjectToFile(score,(settings['OUTPUT_DIR'] + '/Score.txt'))
    spikeExtractionTime = (time.time() - start)
    '''
        

if settings['SAVE_RASTER_PLOT_AS_PNG']:
    filename = '/RasterPlot-Testing.png'
    if settings['LEARNING']:
        filename = '/RasterPlot-Training.png'
    plt.savefig((settings['OUTPUT_DIR'] + filename), dpi=int(settings['SAVE_FIG_DPI']))
     

if settings['DISPLAY_RASTER_PLOT']:
    plt.show()

classifier.end()
print 'PyNN run completed.'

#write a marker file to allow invoking programs to know that the Python/Pynn run completed
utils.saveListToFile(['Pynn Run complete'],settings['RUN_COMPLETE_FILE']) 

