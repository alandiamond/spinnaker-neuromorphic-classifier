
import pylab
import pyNN.spiNNaker as spynnaker
import spynnaker_external_devices_plugin.pyNN as ExternalDevices
import numpy as np
from random import randint
import ModellingUtils as utils
from math import ceil
from array import array

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
    rnSpikeInjectionPort = settings['RN_SPIKE_INJECTION_PORT']
    rnSpikeInjectionPopLabel = settings['RN_SPIKE_INJECTION_POP_LABEL']
    classActivationSpikeInjectionPort = settings['CLASS_ACTIVATION_SPIKE_INJECTION_PORT']
    classActivationSpikeInjectionPopLabel = settings['CLASS_ACTIVATION_SPIKE_INJECTION_POP_LABEL']
    
    learning = settings['LEARNING']
    
    print 'Setting up input layer..'
    numInjectionPops = setupLayerInput(params,rnSpikeInjectionPort,rnSpikeInjectionPopLabel, classActivationSpikeInjectionPort,classActivationSpikeInjectionPopLabel, populationsInput,learning)
    print numInjectionPops , ' spikeInjection populations were needed to accomodate ', params['NUM_VR'], " VRs"
    print 'Setting up noise source layer..'
    setupLayerNoiseSource(params, simTime, populationsNoiseSource)
    print 'Setting up RN layer..'
    numInputPops = len(populationsInput)
    injectionPops = populationsInput[1:len(populationsInput)]
    numInjectionPops = len(injectionPops)
    setupLayerRN(params,neuronModel, cell_params, injectionPops, populationsNoiseSource[0], populationsRN)
    #setupLayerRN(params,neuronModel, cell_params, injectionPops, populationsRN)
    print 'Setting up PN layer..'
    setupLayerPN(params, neuronModel, cell_params, populationsRN, populationsPN)
    print 'Setting up AN layer..'
    setupLayerAN(params, settings, neuronModel, cell_params, populationsInput[0], populationsNoiseSource[0], populationsPN, populationsAN,learning,projectionsPNAN)
    
    printModelConfigurationSummary(params, populationsInput, populationsNoiseSource, populationsRN, populationsPN, populationsAN)

#----------------------------------------------------------------------------------------------------------------------------
def setupLayerInput(params,rnSpikeInjectionPort,rnSpikeInjectionPopLabel, classActivationSpikeInjectionPort,classActivationSpikeInjectionPopLabel, populationsInput,learning):
    
    #Create a spike injection population, one neuron per VR, 
    #where each neuron wil be externally spiked in realtime according to the rate code for the VR response over the training and/or test set
    #Returns number of spike injection populations that were needed (max size applies)
     
    if learning:
        
        #Create a population, one neuron per class, 
        #During training the neuron representing the current class will be actively spiking, the others will be quiet
        #The purpose is to innervate the relevant ouptut class cluster/population so that fire-together-wire-together hebbian learning (via STDP) stregthens synapses from active PN clusters
        #During testing all these neurons will be silent, leaving the strengthened synapses to trigger activity direct from PN layer in the correct ouptpu cluster
         
        numNeurons = params['NUM_CLASSES']
        #popClassActivationSpikes = spynnaker.Population(numNeurons, spynnaker.SpikeSourceArray, spikeData, label='popClassActivationSpikes')
        popClassActivationSpikeInjection = spynnaker.Population(numNeurons,ExternalDevices.SpikeInjector,{'port': classActivationSpikeInjectionPort},label=classActivationSpikeInjectionPopLabel)
        populationsInput.append(popClassActivationSpikeInjection)
        
    else:
        #create an orphan dummy popn of 1 neuron to take the place of the now unused class activation input used in learning
        #This is to ensure that the freed up core does not get co-opted by the PN layer config routine
        # as this would makae the learning and testing configurations different in PN which would likely make the saved PNAN weight arrays incorrect
        popClassActivationSpikes = spynnaker.Population(1, neuronModel, cell_params, label='dummy_popClassActivationSpikes') 
        populationsInput.append(popClassActivationSpikes)
    
    sizes = list()    
    numVR = params['NUM_VR']
    max = params['MAX_SIZE_SPIKE_INJECTION_POP']
    size = 0
    for vr in range(numVR):
        size = size + 1
        if size == max:
            sizes.append(size)
            size = 0    
    if (size > 0):
        #put remiander in last one
        sizes.append(size)
    
    popIdx = 0
    for sz in sizes:
        popRnSpikeInjection = spynnaker.Population(sz,ExternalDevices.SpikeInjector,{'port': rnSpikeInjectionPort+popIdx},label=(rnSpikeInjectionPopLabel+str(popIdx)))
        #spynnaker.set_number_of_neurons_per_core(SpikeInjector, numRatecodeNeurons)
        populationsInput.append(popRnSpikeInjection)
        popIdx = popIdx + 1
    
    return len(sizes) #number of spike injection populations needed
#----------------------------------------------------------------------------------------------------------------------------
def setupLayerNoiseSource(params, simTime, populationsNoiseSource):
    
    #create a single "noise" population that with be used to to generate rate-coded RN populations
    
    noiseRateHz = params['RN_NOISE_RATE_HZ']
    params_poisson_noise= {'rate': noiseRateHz,'start':0,'duration':simTime}
    #numPoissonNeurons = params['RN_NOISE_SOURCE_POP_SIZE'] * params['NETWORK_SCALE']
    numPoissonNeurons = params['MAX_NEURONS_PER_CORE']
    popPoissionNoiseSource  = spynnaker.Population(numPoissonNeurons, spynnaker.SpikeSourcePoisson, params_poisson_noise , label='popPoissionNoiseSource')  
    populationsNoiseSource.append(popPoissionNoiseSource)
    

#----------------------------------------------------------------------------------------------------------------------------
    #Setup RN,PN and AN layers 
    #This model uses:-
    #RN: one large RN pop, 
    #PN: as many PN pop as needed to fit max neurons per core (e.g. 200VR split into 8 popn of 240 neurons, 30 assigned per VR)
    #AN: One popn per class. Only 32 neurons with incoming STDP can be used per core/popn     
#----------------------------------------------------------------------------------------------------------------------------

def setupLayerRN(params, neuronModel, cell_params, injectionPopulations, popPoissionNoiseSource, populationsRN):
    
    #create a single RN population divided into virtual clusters one per VR
    #this will be fed by the noise population and modulated by the relevant ratecoded neuron 
    #to create a rate coded population
    
    numVR = params['NUM_VR']
    rnClusterSize = int(params['CLUSTER_SIZE']) #* params['NETWORK_SCALE']
    rnPopSize = rnClusterSize * numVR
    popName = 'popRN'
    popRN = spynnaker.Population(rnPopSize, neuronModel, cell_params, label=popName)
    populationsRN.append(popRN)

    #connect one random poisson neuron to each RN neuron
    weight = params['WEIGHT_POISSON_TO_CLUSTER_RN']
    delay =  params['DELAY_POISSON_TO_CLUSTER_RN']
    connections = utils.fromList_OneRandomSrcForEachTarget(popPoissionNoiseSource._size,popRN._size,weight,delay)
    projPoissonToClusterRN = spynnaker.Projection(popPoissionNoiseSource, popRN, spynnaker.FromListConnector(connections), target='excitatory')
    
    vr = 0
    for injectionPopn in injectionPopulations:
        connections = list()
        for fromNeuronIdx in range(injectionPopn._size):
            #connect the correct VR ratecode neuron in popRateCodeSpikes to corresponding subsection (cluster) of the RN population
            weight = params['WEIGHT_RATECODE_TO_CLUSTER_RN']
            firstIndex = vr * rnClusterSize
            lastIndex = firstIndex + rnClusterSize - 1
            connections += utils.fromList_SpecificNeuronToRange(fromNeuronIdx,firstIndex,lastIndex,weight,params['MIN_DELAY_RATECODE_TO_CLUSTER_RN'],params['MAX_DELAY_RATECODE_TO_CLUSTER_RN'])
            vr  = vr + 1
        #after the last neuron in the current injection pop, create a projection to the RN  
        projRateToClusterRN = spynnaker.Projection(injectionPopn, popRN, spynnaker.FromListConnector(connections), target='excitatory')
        print 'Added projection to RN of ', len(connections), " connections from injection pop ", injectionPopn.label, "(size ", injectionPopn._size,")"
        #print "Connections:", connections  
        

#----------------------------------------------------------------------------------------------------------------------------            
def setupLayerPN(params, neuronModel, cell_params, populationsRN, populationsPN):
    
    #create an projection neuron PN cluster population per VR
    #this will be fed by the equivalent RN population and will laterally inhibit between clusters 
    

    numVR = int(params['NUM_VR'])
    rnClusterSize = int(params['CLUSTER_SIZE']) #* params['NETWORK_SCALE']
    rnPopSize = rnClusterSize * numVR

    #print 'PN layer, no. VR: ' , numVR
    pnClusterSize = int(params['CLUSTER_SIZE']) #* params['NETWORK_SCALE'])
    maxNeuronsPerCore = int(params['MAX_NEURONS_PER_CORE'])
    
    maxVrPerPop = maxNeuronsPerCore / pnClusterSize
    
    #how many cores were needed to accomodate RN layer (1 pynn pop in this case)
    numCoresRN = utils.coresRequired(populationsRN,maxNeuronsPerCore)
    #print 'The RN layer is taking up ', numCoresRN, ' cores'
     
    coresOnBoard = int(params['PROCESSOR_NODES_ON_BOARD']) * 16
    coresAvailablePN = int(coresOnBoard - params['NUM_CLASSES'] - numCoresRN - 3) # 2 x input, 1 x noise source
    #print 'PN layer, no. cores available:' , coresAvailablePN
    
    if bool(params['USE_MIN_CORES']):
        #try to fit as many as VR into each core as poss
        vrPerPop = maxVrPerPop
        #vrPerPop = 1
    else:
        vrPerPop = int(ceil(float(numVR) / float(coresAvailablePN)))
        if vrPerPop > maxVrPerPop:
            print 'The number of VR and/or cluster size stipulated for this model are too a large for the capacity of this board.'
            quit
    
    print 'PN layer, no. VRs per population will be: ', vrPerPop 
    pnPopSize = pnClusterSize * vrPerPop 
    print 'PN layer, neurons per population will be: ', pnPopSize
    numPopPN  = int( ceil(float(numVR) / float(vrPerPop)))
    print 'PN layer, number of populations(cores) used will be: ', numPopPN
    print 'PN layer, spare (unused) cores : ', coresAvailablePN - numPopPN
    
    weightPNPN = float(params['WEIGHT_WTA_PN_PN'])
    delayPNPN = int(params['DELAY_WTA_PN_PN'])
    connectivityPNPN = float(params['CONNECTIVITY_WTA_PN_PN'])
    
    for p in range(numPopPN):
        popName = 'popPN_' + str(p)
        popPN = spynnaker.Population(pnPopSize, neuronModel, cell_params, label=popName)
        print 'created population ', popName
        populationsPN.append(popPN)
        
        #create a FromList to feed each PN neuron in this popn from its corresponding RN neuron in the single monolithic RN popn  
        weightRNPN = float(params['WEIGHT_RN_PN'])
        delayRNPN =  int(params['DELAY_RN_PN'])
        rnStartIdx = p * pnPopSize
        rnEndIdx = rnStartIdx + pnPopSize - 1
        
        # The last PN popn will often have unneeded 'ghost' clusters at the end due to imperfect dstribution of VRs among cores
        # As there is no RN cluster that feeds these (RN is one pop of the correct total size) so the connections must stop at the end of RN
        #rnPopSize = populationsRN[0]._size
        rnMaxIdx = rnPopSize - 1
        if rnEndIdx > rnMaxIdx:
             rnEndIdx = rnMaxIdx #clamp to the end of the RN population
             
        pnEndIdx = rnEndIdx - rnStartIdx
 
        connections = utils.fromList_OneToOne_fromRangeToRange(rnStartIdx,rnEndIdx,0,pnEndIdx,weightRNPN,delayRNPN, delayRNPN)
        print 'Creating ' ,len(connections) , ' connections RN to PN..'
        projClusterRNToClusterPN = spynnaker.Projection(populationsRN[0], popPN,spynnaker.FromListConnector(connections), target='excitatory')
            
        
        #within this popn only, connect each PN sub-population VR "cluster" to inhibit every other
        if vrPerPop > 1:
            print 'Creating intra-pop connections PN to PN..'
            utils.createIntraPopulationWTA(popPN,vrPerPop,weightPNPN,delayPNPN,connectivityPNPN,True)

    #Also connect each PN cluster to inhibit every other cluster
    print 'Creating inter-pop connections PN to PN..'
    utils.createInterPopulationWTA(populationsPN,weightPNPN,delayPNPN,connectivityPNPN)
        
#----------------------------------------------------------------------------------------------------------------------------            
def setupLayerAN(params, settings, neuronModel, cell_params, popClassActivation, popPoissionNoiseSource, populationsPN, populationsAN,learning,projectionsPNAN):
    
    #create an Association Neuron AN cluster population per class
    #this will be fed by:
    #1) PN clusters via plastic synapses
    #2) Class activation to innervate the correct AN cluster for a given input  
    #3) laterally inhibit between AN clusters 
    

    numClasses = params['NUM_CLASSES']
    
    anClusterSize = int(params['CLUSTER_SIZE']) #* params['NETWORK_SCALE']
    
    for an in range(numClasses):
        popName = 'popClusterAN_'  + str(an) ;
        popClusterAN = spynnaker.Population(anClusterSize, neuronModel, cell_params, label=popName)
        populationsAN.append(popClusterAN)
        
        #connect neurons in every PN popn to x% (e.g 50%) neurons in this AN cluster 
        for pn in range(len(populationsPN)):
            if learning:
                projLabel = 'Proj_PN' + str(pn) + '_AN' + str(an)
                projClusterPNToClusterAN = connectClusterPNtoAN(params,populationsPN[pn],popClusterAN,float(settings['OBSERVATION_EXPOSURE_TIME_MS']),projLabel)
                projectionsPNAN.append(projClusterPNToClusterAN) #keep handle to use later for saving off weights at end of learning
            else:
                #Without plasticity, create PNAN FromList connectors using weights saved during learning stage
                connections = utils.loadListFromFile(getWeightsFilename(settings,'PNAN',pn,an))
                #print 'Loaded weightsList[',pn,',',an,']',connections
                tupleList = utils.createListOfTuples(connections) #new version only accepts list of tuples not list of lists
                #print 'tupleList[',pn,',',an,']',tupleList
                conn = spynnaker.FromListConnector(tupleList)
                projClusterPNToClusterAN = spynnaker.Projection(populationsPN[pn], popClusterAN,conn, target='excitatory')

        if learning:
            #use the class activity input neurons to create correlated activity during learining in the corresponding class cluster
            weight = params['WEIGHT_CLASS_EXCITATION_TO_CLUSTER_AN']
            connections = utils.fromList_SpecificNeuronToAll(an,anClusterSize,weight,params['MIN_DELAY_CLASS_ACTIVITY_TO_CLUSTER_AN'],params['MAX_DELAY_CLASS_ACTIVITY_TO_CLUSTER_AN'])
            projClassActivityToClusterAN = spynnaker.Projection(popClassActivation, popClusterAN, spynnaker.FromListConnector(connections), target='excitatory')
        
        else: #testing  
            #send spikes on these outputs back to correct host port , these will be used to determine winner etc
            anHostReceivePort = int(settings['AN_HOST_RECEIVE_PORT']) 
            ExternalDevices.activate_live_output_for(popClusterAN,port=anHostReceivePort)
            
    #connect each AN cluster to inhibit every other AN cluster
    utils.createInterPopulationWTA(populationsAN,params['WEIGHT_WTA_AN_AN'],params['DELAY_WTA_AN_AN'],float(params['CONNECTIVITY_WTA_AN_AN']))
    
    #inhibit other non-corresponding class clusters
    if learning:
        weight = params['WEIGHT_CLASS_INHIBITION_TO_CLUSTER_AN']
        for activeCls in range(numClasses):
            connections = utils.fromList_SpecificNeuronToAll(activeCls,anClusterSize,weight,params['MIN_DELAY_CLASS_ACTIVITY_TO_CLUSTER_AN'],params['MAX_DELAY_CLASS_ACTIVITY_TO_CLUSTER_AN'])
            for an in range(numClasses):
                if an != activeCls:
                    projClassActivityToClusterAN = spynnaker.Projection(popClassActivation, populationsAN[an], spynnaker.FromListConnector(connections), target='inhibitory')

#---------------------------------------------------------------------------------------------------------------------------- 
def connectClusterPNtoAN(params,popClusterPN,popClusterAN, observationExposureTimeMs, projLabel=''):
    
    #Using custom Hebbian-style plasticity, connect neurons in specfied PN cluster to x% neurons in specified AN cluster 
    
    startWeightPNAN = float(params['STARTING_WEIGHT_PN_AN'])
    delayPNAN =  int(params['DELAY_PN_AN'])
    connectivity = float(params['CONNECTIVITY_PN_AN'])
    
    #STDP curve parameters
    tau = float(params['STDP_TAU_PN_AN']) 
    wMin = float(params['STDP_WMIN_PN_AN']) 
    wMax = float(params['STDP_WMAX_PN_AN']) 
    
    gainScaling = float(params['STDP_SCALING_PN_AN'])
    
    '''
    #this setting was tuned for a 120ms learning window
    #rescale it according to actual window used. ie for longer window, slow down learning rate
    tweak = 120.0/float(observationExposureTimeMs)
    gainScaling = gainScaling * tweak
    print "Weight gain scaled by factor of ", tweak  
    '''
    
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
            #print 'weightsMatrix with NaN',weightsMatrix
            weightsMatrix = np.nan_to_num(weightsMatrix) #sets NaN to 0.0 , no connection from x to y is specified as a NaN entry, may cause problem on imports
            #print 'weightsMatrix without NaN',weightsMatrix
            weightsList = utils.fromList_convertWeightMatrix(weightsMatrix, delayPNAN) 
            utils.printSeparator()
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
    observationExposureTimeMs = float(settings['OBSERVATION_EXPOSURE_TIME_MS'])
    #classActivationExposureFraction = settings['CLASS_ACTIVATION_EXPOSURE_FRACTION']
       
    #set up lists to hold highest spike count and current winning class so far for each observation
    winningSpikeCount = [0] * numObservations
    winningClass = [-1] * numObservations

    spikeInputStartMs = 10000.0
    #set spike count window to most , not all the exposure time
    allSpikesByClass = list()
    
    '''
    This approach is faling during Test. There are much earlier ghost spikes in AN
    Try setting up mini pop fed by all of spike injection neurons with strong weigting.
    Any true injected spikes will trigger this pop, use it to mark start of true input   
    '''
    
    #we don't know when first live input spikes will appear as the model takes a varying amount of time to startup 
    #assume that first spike in whole AN layer is very close to the start of input from the spike sender
    #Go thorugh all AN spikes and mark the earliest
    for cls in range(numClasses):
        allSpikes = populationsAN[cls].getSpikes(compatible_output=True)
        allSpikesByClass.append(allSpikes)
        spikeTimeMs = utils.getFirstSpikeTime(allSpikes)
        if spikeTimeMs < spikeInputStartMs:
            spikeInputStartMs = spikeTimeMs
            print 'class ' , cls, 'spikeInputStartMs updated to ' , spikeInputStartMs
        
    
    presentationStartTimes = utils.loadListFromFile("PresentationTimes.txt")
    print 'Loaded presentation times from file:', presentationStartTimes
    
    for cls in range(numClasses):
        
        allSpikes = allSpikesByClass[cls] #get ptr to spikes extracted for this class over whole duration
           
        for observation in range(numObservations):
            observationStartMs = presentationStartTimes[observation]
 
            '''
            #don't set to whole observationExposureTimeMs because first spike may not be right at the start therefore could catch spikes from next observation 
            observationWindowMs = float(0.9 * observationExposureTimeMs)
            offsetMs = spikeInputStartMs + 0.5 * (observationExposureTimeMs-observationWindowMs) 
            startMs = offsetMs + (observation * observationExposureTimeMs)
            '''
            startMs = spikeInputStartMs + observationStartMs
            endMs = startMs + observationExposureTimeMs
            observationSpikes = utils.getSpikesBetween(startMs,endMs,allSpikes)
            spikeCount= observationSpikes.shape[0]
            print 'Observation:', observation, 'StartMs:', startMs, 'EndMs:', endMs, 'Class:' , cls, 'Spikes:' , spikeCount
            if spikeCount > winningSpikeCount[observation]:
                winningSpikeCount[observation] = spikeCount
                winningClass[observation] = cls
            
    print 'Winning Class for each observation:'
    print winningClass  
    return winningClass

#----------------------------------------------------------------------------------------------------------------------------
# compare the list of most active class cluster for each observation
# with the supplied list of correct answers (class labels) to obtain a classification score

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

#----------------------------------------------------------------------------------------------------------------------------
# switch on recording for the populations specified in the Settings file, these can differ in learning or training

def recordSpecifiedPopulations(settings,populationsInput,populationsNoiseSource,populationsRN,populationsPN,populationsAN): 
    recordPopnInput = settings['RECORD_POP_INPUT_LEARNING']
    recordPopnNoise = settings['RECORD_POP_NOISE_SOURCE_LEARNING']
    recordPopnRN = settings['RECORD_POP_RN_LEARNING']
    recordPopnPN = settings['RECORD_POP_PN_LEARNING']
    recordPopnAN = settings['RECORD_POP_AN_LEARNING']
    
    if not settings['LEARNING']:
        recordPopnInput = settings['RECORD_POP_INPUT_TESTING']
        recordPopnNoise = settings['RECORD_POP_NOISE_SOURCE_TESTING']
        recordPopnRN = settings['RECORD_POP_RN_TESTING']
        recordPopnPN = settings['RECORD_POP_PN_TESTING']
        recordPopnAN = settings['RECORD_POP_AN_TESTING']
    
    utils.recordPopulations(populationsInput,recordPopnInput)
    utils.recordPopulations(populationsNoiseSource,recordPopnNoise)
    utils.recordPopulations(populationsRN,recordPopnRN)
    utils.recordPopulations(populationsPN,recordPopnPN)
    utils.recordPopulations(populationsAN,recordPopnAN)
    
    return (recordPopnInput,recordPopnNoise,recordPopnRN,recordPopnPN,recordPopnAN)
