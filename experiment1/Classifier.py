
import pylab
import pyNN.spiNNaker as spynnaker
import numpy as np
from random import randint
import ModellingUtils as utils
from math import ceil
from array import array
#import BuildModel_1_25_10_populations as mybuilder

# +-------------------------------------------------------------------+
# | General Parameters                                                |
# +-------------------------------------------------------------------+

# Population parameters
neuronModel = spynnaker.IF_curr_exp

cell_params = {'cm': 0.25,
               'i_offset': 0.0,
               'tau_m': 20.0,
               'tau_refrac': 2.0,
               'tau_syn_E': 5.0,
               'tau_syn_I': 5.0,
               'v_reset': -70.0,
               'v_rest': -65.0,
               'v_thresh': -50.0
               }

#----------------------------------------------------------------------------------------------------------------------------
def run(simTime):
    print 'Model run starting at sim time ', spynnaker.get_current_time()
    spynnaker.run(simTime)
    print 'Model run ended at sim time ', spynnaker.get_current_time()
    
#----------------------------------------------------------------------------------------------------------------------------
def end():
    spynnaker.end();
    
#----------------------------------------------------------------------------------------------------------------------------
def setupModel(settings,params, dt,simTime,populationsInput, populationsNoiseSource, populationsRN,populationsPN,populationsAN,projectionsPNAN):
    
    maxDelay = max([params['MAX_DELAY_RATECODE_TO_CLUSTER_RN'],params['MAX_DELAY_CLASS_ACTIVITY_TO_CLUSTER_AN']])
    spynnaker.setup(timestep=dt,min_delay=1,max_delay=maxDelay)        
    spikeSourceVrResponsePath = settings['SPIKE_SOURCE_VR_RESPONSE_PATH']
    spikeSourceActiveClassPath = settings['SPIKE_SOURCE_ACTIVE_CLASS_PATH']
    learning = settings['LEARNING']
    
    setupLayerInput(params, spikeSourceVrResponsePath, spikeSourceActiveClassPath, populationsInput,learning)
    setupLayerNoiseSource(params, simTime, populationsNoiseSource)
    setupLayerRN(params,neuronModel, cell_params, populationsInput[0], populationsNoiseSource[0], populationsRN)
    setupLayerPN(params, neuronModel, cell_params, populationsRN, populationsPN)
    setupLayerAN(params, settings, neuronModel, cell_params, populationsInput[1], populationsNoiseSource[0], populationsPN, populationsAN,learning,projectionsPNAN)
    
    printModelConfigurationSummary(params, populationsInput, populationsNoiseSource, populationsRN, populationsPN, populationsAN)

#----------------------------------------------------------------------------------------------------------------------------
def setupLayerInput(params, spikeSourceVrResponsePath, spikeSourceActiveClassPath, populationsInput,learning):
    
    
    #Create a population, one neuron per VR, 
    #where each neuron wil be loaded with the rate code spikes for the VR response over the training and/or test set
     
    spikeData = utils.readSpikeSourceDataFile(spikeSourceVrResponsePath)
    numVR = params['NUM_VR']
    numRatecodeNeurons = numVR
    popRateCodeSpikes = spynnaker.Population(numRatecodeNeurons, spynnaker.SpikeSourceArray, spikeData, label='popRateCodeSpikes')
    populationsInput.append(popRateCodeSpikes)

    if learning:
        
        #Create a population, one neuron per class, 
        #During training the neuron representing the current class will be active with significant spikes, the others will be quiet
        #The purpose is to innervate the relevant ouptut class cluster/population so that fire-together-wire-together hebbian learning (via STDP) stregthens synapses from active PN clusters
        #During testing all these neurons will be silent, leaving the strengthened synapses to trigger activity direct from PN layer in the correct ouptpu cluster
         
        spikeData = utils.readSpikeSourceDataFile(spikeSourceActiveClassPath)
        numNeurons = params['NUM_CLASSES']
        popClassActivationSpikes = spynnaker.Population(numNeurons, spynnaker.SpikeSourceArray, spikeData, label='popClassActivationSpikes')
        populationsInput.append(popClassActivationSpikes)
        
    else:
        #create an orphan dummy popn of 1 neuron to take the place of the now unused spike source pop used in learning
        #This is to ensure that the freed up core does not get co-opted by the PN layer config routine
        # as this would makae the learning and testing configurations different in PN which would likely make the saved PNAN weight arrays incorrect
        popClassActivationSpikes = spynnaker.Population(1, neuronModel, cell_params, label='dummy_popClassActivationSpikes') 
        populationsInput.append(popClassActivationSpikes)

    
#----------------------------------------------------------------------------------------------------------------------------
def setupLayerNoiseSource(params, simTime, populationsNoiseSource):
    
    #create a single "noise" population that with be used to to generate rate-coded RN populations
    
    noiseRateHz = params['RN_NOISE_RATE_HZ']
    params_poisson_noise= {'rate': noiseRateHz,'start':0,'duration':simTime}
    numPoissonNeurons = params['RN_NOISE_SOURCE_POP_SIZE'] * params['NETWORK_SCALE']
    popPoissionNoiseSource  = spynnaker.Population(numPoissonNeurons, spynnaker.SpikeSourcePoisson, params_poisson_noise , label='popPoissionNoiseSource')  
    populationsNoiseSource.append(popPoissionNoiseSource)
    

#----------------------------------------------------------------------------------------------------------------------------
    #Setup RN,PN and AN layers 
    #This model uses:-
    #RN: one large RN pop, 
    #PN: as many PN pop as needed to fit max neurons per core (e.g. 200VR split into 8 popn of 240 neurons, 30 assigned per VR)
    #AN: One popn per class. Only 32 neurons with incoming STDP can be used per core/popn     
#----------------------------------------------------------------------------------------------------------------------------

def setupLayerRN(params, neuronModel, cell_params, popRateCodeSpikes, popPoissionNoiseSource, populationsRN):
    
    #create a single RN population divided into virtual clusters one per VR
    #this will be fed by the noise population and modulated by the relevant ratecoded neuron 
    #to create a rate coded population
    
    numVR = params['NUM_VR']
    rnClusterSize = params['CLUSTER_SIZE'] * params['NETWORK_SCALE']
    rnPopSize = rnClusterSize * numVR
    popName = 'popRN'
    
    popRN = spynnaker.Population(rnPopSize, neuronModel, cell_params, label=popName)
    populationsRN.append(popRN)
        
    #connect one random poisson neuron to each RN neuron
    weight = params['WEIGHT_POISSON_TO_CLUSTER_RN']
    delay =  params['DELAY_POISSON_TO_CLUSTER_RN']
    connections = utils.fromList_OneRandomSrcForEachTarget(popPoissionNoiseSource._size,popRN._size,weight,delay)
    projPoissonToClusterRN = spynnaker.Projection(popPoissionNoiseSource, popRN, spynnaker.FromListConnector(connections), target='excitatory')

    connections = list()
    for vr in range(numVR):
        #connect the correct VR ratecode neuron in popRateCodeSpikes to corresponding subsection (cluster) of the RN population
        weight = params['WEIGHT_RATECODE_TO_CLUSTER_RN']
        firstIndex = vr * rnClusterSize
        lastIndex = firstIndex + rnClusterSize - 1
        connections += utils.fromList_SpecificNeuronToRange(vr,firstIndex,lastIndex,weight,params['MIN_DELAY_RATECODE_TO_CLUSTER_RN'],params['MAX_DELAY_RATECODE_TO_CLUSTER_RN'])
        
    projRateToClusterRN = spynnaker.Projection(popRateCodeSpikes, popRN, spynnaker.FromListConnector(connections), target='excitatory')
        

#----------------------------------------------------------------------------------------------------------------------------            
def setupLayerPN(params, neuronModel, cell_params, populationsRN, populationsPN):
    
    #create an projection neuron PN cluster population per VR
    #this will be fed by the equivalent RN population and will laterally inhibit between clusters 
    

    numVR = int(params['NUM_VR'])
    #print 'PN layer, no. VR: ' , numVR
    pnClusterSize = int(params['CLUSTER_SIZE'] * params['NETWORK_SCALE'])
    maxNeuronsPerCore = int(params['MAX_NEURONS_PER_CORE'])
    
    maxVrPerPop = maxNeuronsPerCore / pnClusterSize
    
    #how many cores were needed to accomodate RN layer (1 pynn pop in this case)
    numCoresRN = utils.coresRequired(populationsRN,maxNeuronsPerCore)
    #print 'The RN layer is taking up ', numCoresRN, ' cores'
     
    coresAvailablePN = int(params['CORES_ON_BOARD'] - params['NUM_CLASSES'] - numCoresRN - 3) # 2 x input, 1 x noise source
    #print 'PN layer, no. cores available:' , coresAvailablePN
    
    vrPerPop = int(ceil(float(numVR) / float(coresAvailablePN)))
    if vrPerPop > maxVrPerPop:
        print 'The number of VR and/or cluster size stipulated for this model are too a large for the capacity of this board.'
        quit
    
    #print 'PN layer, no. VRs per population will be: ', vrPerPop 
    pnPopSize = pnClusterSize * vrPerPop 
    #print 'PN layer, neurons per population will be: ', pnPopSize
    numPopPN  = int( ceil(float(numVR) / float(vrPerPop)))
    #print 'PN layer, number of populations(cores) used will be: ', numPopPN
    #print 'PN layer, spare (unused) cores : ', coresAvailablePN - numPopPN
    
    weightPNPN = float(params['WEIGHT_WTA_PN_PN'])
    delayPNPN = int(params['DELAY_WTA_PN_PN'])
    connectivityPNPN = float(params['CONNECTIVITY_WTA_PN_PN'])
    
    for p in range(numPopPN):
        popName = 'popPN_' + str(p)
        popPN = spynnaker.Population(pnPopSize, neuronModel, cell_params, label=popName)
        #print 'created population ', popName
        populationsPN.append(popPN)
        
        #create a FromList to feed each PN neuron in this popn from its corresponding RN neuron in the single monolithic RN popn  
        weightRNPN = float(params['WEIGHT_RN_PN'])
        delayRNPN =  int(params['DELAY_RN_PN'])
        rnStartIdx = p * pnPopSize
        rnEndIdx = rnStartIdx + pnPopSize - 1
        
        # The last PN popn will often have unneeded 'ghost' clusters at the end due to imperfect dstribution of VRs among cores
        # As there is no RN cluster that feeds these (RN is one pop of the correct total size) so the connections must stop at the end of RN
        rnMaxIdx = populationsRN[0]._size - 1
        if rnEndIdx > rnMaxIdx:
             rnEndIdx = rnMaxIdx #clamp to the end of the RN population
             
        pnEndIdx = rnEndIdx - rnStartIdx
 
        connections = utils.fromList_OneToOne_fromRangeToRange(rnStartIdx,rnEndIdx,0,pnEndIdx,weightRNPN,delayRNPN, delayRNPN)
        #print connections
        projClusterRNToClusterPN = spynnaker.Projection(populationsRN[0], popPN,spynnaker.FromListConnector(connections), target='excitatory')
            
        #within this popn only, connect each PN sub-population VR "cluster" to inhibit every other
        if vrPerPop > 1:
            utils.createIntraPopulationWTA(popPN,vrPerPop,weightPNPN,delayPNPN,connectivityPNPN,True)
    
    #Also connect each PN cluster to inhibit every other cluster
    utils.createInterPopulationWTA(populationsPN,weightPNPN,delayPNPN,connectivityPNPN)
    
#----------------------------------------------------------------------------------------------------------------------------            
def setupLayerAN(params, settings, neuronModel, cell_params, popClassActivation, popPoissionNoiseSource, populationsPN, populationsAN,learning,projectionsPNAN):
    
    #create an Association Neuron AN cluster population per class
    #this will be fed by:
    #1) PN clusters via plastic synapses
    #2) Class activation to innervate the correct AN cluster for a given input  
    #3) laterally inhibit between AN clusters 
    

    numClasses = params['NUM_CLASSES']
    
    anClusterSize = params['CLUSTER_SIZE'] * params['NETWORK_SCALE']
    
    for an in range(numClasses):
        popName = 'popClusterAN_'  + str(an) ;
        popClusterAN = spynnaker.Population(anClusterSize, neuronModel, cell_params, label=popName)
        populationsAN.append(popClusterAN)
        
        #connect neurons in every PN popn to x% (e.g 50%) neurons in this AN cluster 
        for pn in range(len(populationsPN)):
            if learning:
                projLabel = 'Proj_PN' + str(pn) + '_AN' + str(an)
                projClusterPNToClusterAN = connectClusterPNtoAN(params,populationsPN[pn],popClusterAN,projLabel)
                projectionsPNAN.append(projClusterPNToClusterAN) #keep handle to use later for saving off weights at end of learning
            else:
                #Without plasticity, create PNAN FromList connectors using weights saved during learning stage
                connections = utils.loadListFromFile(getWeightsFilename(settings,'PNAN',pn,an))
                #print 'Loaded weightsList[',pn,',',an,']',connections
                projClusterPNToClusterAN = spynnaker.Projection(populationsPN[pn], popClusterAN,spynnaker.FromListConnector(connections), target='excitatory')

        if learning:
            #use the class activity input neurons to create correlated activity during learining in the corresponding class cluster
            weight = params['WEIGHT_CLASS_ACTIVITY_TO_CLUSTER_AN']
            connections = utils.fromList_SpecificNeuronToAll(an,anClusterSize,weight,params['MIN_DELAY_CLASS_ACTIVITY_TO_CLUSTER_AN'],params['MAX_DELAY_CLASS_ACTIVITY_TO_CLUSTER_AN'])
            projClassActivityToClusterAN = spynnaker.Projection(popClassActivation, popClusterAN, spynnaker.FromListConnector(connections), target='excitatory')
        
    #connect each AN cluster to inhibit every other AN cluster
    utils.createInterPopulationWTA(populationsAN,params['WEIGHT_WTA_AN_AN'],params['DELAY_WTA_AN_AN'],float(params['CONNECTIVITY_WTA_AN_AN']))


#---------------------------------------------------------------------------------------------------------------------------- 
def connectClusterPNtoAN(params,popClusterPN,popClusterAN, projLabel=''):
    
    #Using custom Hebbian-style plasticity, connect neurons in specfied PN cluster to x% neurons in specified AN cluster 
    
    startWeightPNAN = float(params['STARTING_WEIGHT_PN_AN'])
    delayPNAN =  int(params['DELAY_PN_AN'])
    connectivity = float(params['CONNECTIVITY_PN_AN'])
    
    #STDP curve parameters
    tau = float(params['STDP_TAU_PN_AN']) 
    wMin = float(params['STDP_WMIN_PN_AN']) 
    wMax = float(params['STDP_WMAX_PN_AN']) 
    gainScaling = float(params['STDP_SCALING_PN_AN']) 
    
    timingDependence = spynnaker.SpikePairRule(tau_plus=tau, tau_minus=tau, nearest=True)
    weightDependence = spynnaker.AdditiveWeightDependence(w_min=wMin, w_max=wMax, A_plus=gainScaling, A_minus=-gainScaling)
    stdp_model = spynnaker.STDPMechanism(timing_dependence = timingDependence, weight_dependence = weightDependence)
    probConnector = spynnaker.FixedProbabilityConnector(connectivity, weights=startWeightPNAN, delays=delayPNAN, allow_self_connections=True)
    projClusterPNToClusterAN = spynnaker.Projection(popClusterPN, popClusterAN,probConnector,synapse_dynamics = spynnaker.SynapseDynamics(slow = stdp_model), target='excitatory', label=projLabel)
    return projClusterPNToClusterAN

#----------------------------------------------------------------------------------------------------------------------------
def printParameters(title,params):
    utils.printSeparator()
    print title
    utils.printSeparator()
    for param in params:
        print param, '=', params[param]
    utils.printSeparator()

#----------------------------------------------------------------------------------------------------------------------------
def printModelConfigurationSummary(params, populationsInput, populationsNoiseSource, populationsRN, populationsPN, populationsAN):
    totalPops = len(populationsInput) + len(populationsNoiseSource) + len(populationsRN) + len(populationsPN) + len(populationsAN)
    stdMaxNeuronsPerCore = params['MAX_NEURONS_PER_CORE']
    stdpMaxNeuronsPerCore = params['MAX_STDP_NEURONS_PER_CORE']
    inputCores = utils.coresRequired(populationsInput, stdMaxNeuronsPerCore)
    noiseCores = utils.coresRequired(populationsNoiseSource, stdMaxNeuronsPerCore)
    rnCores = utils.coresRequired(populationsRN, stdMaxNeuronsPerCore)
    pnCores = utils.coresRequired(populationsPN, stdMaxNeuronsPerCore)
    anCores = utils.coresRequired(populationsAN, stdpMaxNeuronsPerCore)
    utils.printSeparator()
    print 'Population(Cores) Summary'
    utils.printSeparator()
    print 'Input: ', len(populationsInput), '(', inputCores, ' cores)'
    print 'Noise: ', len(populationsNoiseSource), '(', noiseCores, ' cores)'
    print 'RN:    ', len(populationsRN), '(', rnCores, ' cores)'
    print 'PN:    ', len(populationsPN), '(', pnCores, ' cores)'
    print 'AN:    ', len(populationsAN), '(', anCores, ' cores)'
    print 'TOTAL: ', totalPops, '(', inputCores + noiseCores + rnCores + pnCores + anCores, ' cores)'
    utils.printSeparator()

#----------------------------------------------------------------------------------------------------------------------------    
# generate the agreed file name for storing list of connection weights between two sets of populations
def getWeightsFilename(settings,prefix,prePopIdx,postPopIdx):
    path  = settings['CACHE_DIR']  + '/weights_' + prefix + '_' + str(prePopIdx) + '_' + str(postPopIdx) + '.txt'
    return path
    
#----------------------------------------------------------------------------------------------------------------------------    
def saveLearntWeightsPNAN(settings,params,projectionsPNAN,numPopsPN,numPopsAN):
    delayPNAN =  int(params['DELAY_PN_AN'])
    projections = iter(projectionsPNAN)
    for an in range(numPopsAN):
        for pn in range(numPopsPN):
            weightsMatrix = projections.next().getWeights(format="array")
            weightsList = utils.fromList_convertWeightMatrix(weightsMatrix, delayPNAN) 
            #utils.printSeparator()
            #print 'weightsList[',pn,',',an,']',weightsList
            utils.saveListToFile(weightsList, getWeightsFilename(settings,'PNAN',pn, an))
            
#----------------------------------------------------------------------------------------------------------------------------            
def saveSpikesAN(settings,populationsAN):
        
    for i in range(len(populationsAN)):
        path  = settings['CACHE_DIR']  + '/Spikes_Class' + str(i) + '.csv'
        utils.saveSpikesToFile(populationsAN[i],path)              
        
        
#----------------------------------------------------------------------------------------------------------------------------
# uses the spike counts in AN clusters to return a list of classifier "winners", one for each observation presentedt in the set. 
# the winner for each is judged from the highest spike cluster during the time window allocated to that observation     
        
def calculateWinnersAN(settings,populationsAN):
    
    numClasses = len(populationsAN)
    numObservations = settings['NUM_OBSERVATIONS']
    observationExposureTimeMs = settings['OBSERVATION_EXPOSURE_TIME_MS']
   
    #set up lists to hold highest spike count and current winning class so far for each observation
    winningSpikeCount = [0] * numObservations
    winningClass = [-1] * numObservations

    
    for cls in range(numClasses):

        allSpikes = populationsAN[cls].getSpikes(compatible_output=True)
        for observation in range(numObservations):
 
            startMs = observation * observationExposureTimeMs
            endMs = startMs + observationExposureTimeMs
            observationSpikes = utils.getSpikesBetween(startMs,endMs,allSpikes)
            spikeCount= observationSpikes.shape[0]
            #print 'StartMs:', startMs, 'EndMs:', endMs, 'Observation:' , observation, 'Class:' , cls, 'Spikes:' , spikeCount
            if spikeCount > winningSpikeCount[observation]:
                winningSpikeCount[observation] = spikeCount
                winningClass[observation] = cls
            
    return winningClass

def calculateScore(winningClassesByObservation,classLabels):
    utils.printSeparator()
    print 'Correct Answers', classLabels
    print 'Classifier Responses', winningClassesByObservation
    numObservations = len(winningClassesByObservation)
    score = 0.0
    for i in range (numObservations):
        if winningClassesByObservation[i] == classLabels[i]:
            score = score + 1.0
    scorePercent  = 100.0 * score/float(numObservations)
    print 'Score: ', int(score), 'out of ', numObservations, '(', scorePercent, '%)'
    utils.printSeparator() 
    return scorePercent