import pyNN.spiNNaker as spynnaker
import matplotlib.pyplot as plt
import numpy as np
import random
import json
import os.path
from random import randint
from scipy import rand
from math import floor 
from math import ceil
#----------------------------------------------------------------------------------------------------------------------------
def readSpikeSourceDataFile(path):
    for line in open(path):
        x = eval(line)
        #print x
    
    spikeLists = {'spike_times': [list(eval(line)) for line in open(path)]}
    return spikeLists
    
#----------------------------------------------------------------------------------------------------------------------------
def printSeparator():
    print'--------------------------------------------------------------------------'

#----------------------------------------------------------------------------------------------------------------------------
def coresRequired(popList,maxNeuronsPerCore):
    cores = 0
    for pop in popList:
        cores += coresRequiredPerPop(pop, maxNeuronsPerCore)
    return cores

#----------------------------------------------------------------------------------------------------------------------------
def coresRequiredPerPop(pop,maxNeuronsPerCore):
    return int(ceil(float(pop._size) / float(maxNeuronsPerCore)))

#----------------------------------------------------------------------------------------------------------------------------
def createInterPopulationWTA(populations,weight,delay,connectivity):
    probConnector = spynnaker.FixedProbabilityConnector(connectivity, weights=weight, delays=delay, allow_self_connections=True)
    for i in range(len(populations)):
        for j in range(len(populations)):
            if i != j:
                projInhibitory = spynnaker.Projection(populations[i], populations[j], probConnector, target='inhibitory')
    #print ' WTA connections', projInhibitory.getWeights()
    
#----------------------------------------------------------------------------------------------------------------------------
# Use a from list connector to generate internal connections that implement a WTA inhibition between N sub-population "clusters"
def createIntraPopulationWTA(popn,numClusters,weight,delay,connectivity,createSingleProjection):
    allConnections = []
    clusterSize = popn._size / numClusters
    for i in range(numClusters):
        for j in range(numClusters):
            if i != j:
                srcFirstIdx = i * clusterSize
                srcLastIdx = srcFirstIdx + clusterSize - 1               
                targFirstIdx = j * clusterSize
                targLastIdx = targFirstIdx + clusterSize -1
                connections = fromList_fromRangeToRange(srcFirstIdx, srcLastIdx, targFirstIdx, targLastIdx, weight, delay, delay, connectivity)
                if (createSingleProjection):
                    allConnections += connections
                else:
                    projInhibitory = spynnaker.Projection(popn, popn, spynnaker.FromListConnector(connections), target='inhibitory')

    #print ' WTA connections', projInhibitory.getWeights()
    if (createSingleProjection):
        projInhibitory = spynnaker.Projection(popn, popn, spynnaker.FromListConnector(allConnections), target='inhibitory')

#----------------------------------------------------------------------------------------------------------------------------
#return a list of neuron to neuron connections from the specfied index to every entry entry in target popn
def fromList_SpecificNeuronToAll(fromIdx,targetPopSize,weight,delayMin, delayMax):
    return fromList_SpecificNeuronToRange(fromIdx,0,targetPopSize-1,weight,delayMin, delayMax)

#----------------------------------------------------------------------------------------------------------------------------
#return a list of random connections from the specified range in both the src and target populations
def fromList_fromRangeToRange(srcFirstIdx,srcLastIdx,targFirstIdx,targLastIdx,weight,delayMin, delayMax,probability=1.0):
    connections = list()
    for srcIdx in range(srcFirstIdx,srcLastIdx+1):
        connections += fromList_SpecificNeuronToRange(srcIdx, targFirstIdx, targLastIdx, weight, delayMin, delayMax,probability)
    return connections

#----------------------------------------------------------------------------------------------------------------------------
#return a list of one to one connections from the specified ranges in src and target populations (must be same size ranges)
def fromList_OneToOne_fromRangeToRange(srcFirstIdx,srcLastIdx,targFirstIdx,targLastIdx,weight,delayMin, delayMax):
    fromIdx = srcFirstIdx
    toIdx  = targFirstIdx
    connections = list()
    while fromIdx <= srcLastIdx:
        delay =  randint(delayMin,delayMax)
        singleConnection = (fromIdx, toIdx, weight, delay)
        connections.append(singleConnection)
        fromIdx += 1
        toIdx += 1
    
    return connections

#----------------------------------------------------------------------------------------------------------------------------
#return a list of neuron to neuron connections from the specfied index to every entry in target popn between firstIdx and lastIdx inclusive
def fromList_SpecificNeuronToRange(fromIdx,toFirstIdx,toLastIdx,weight,delayMin, delayMax,probability=1.0):
    connections = list()
    for targetIdx in range(toFirstIdx,toLastIdx + 1):
        if random.random() < probability:
            delay =  randint(delayMin,delayMax)
            singleConnection = (fromIdx, targetIdx, weight, delay)
            connections.append(singleConnection)
            
    return connections
       
#----------------------------------------------------------------------------------------------------------------------------
#return a list of connections where each target has a single randomly chosen src neuron
def fromList_OneRandomSrcForEachTarget(srcPopSize,targetPopSize,weight,delay):
    connections = list()
    for targetIdx in range(targetPopSize):
        fromIdx = randint(0, srcPopSize -1 )
        singleConnection = (fromIdx, targetIdx, weight, delay)
        connections.append(singleConnection)
    return connections

#----------------------------------------------------------------------------------------------------------------------------
def recordPopulations(populationList,enabled):
    if enabled:
        for popn in populationList:
            popn.record()

#----------------------------------------------------------------------------------------------------------------------------
def plotAllSpikes(populationList, totalSimulationTime , indexOffset, enabled):
    if enabled:
        for popn in populationList: 
            indexOffset = 1 + plotSpikes(popn,totalSimulationTime, indexOffset)
        lastIndexUsed = indexOffset - 1
        return lastIndexUsed
    else:
        return (indexOffset - 1)
    
#----------------------------------------------------------------------------------------------------------------------------
def plotSpikes(popn,totalSimulationTime, indexOffset):
    
    spikes = popn.getSpikes(compatible_output=True)
    spikeCount  = spikes.shape[0]
    neuronCount = popn._size
    timeWindowMs = totalSimulationTime - 20.0; #it takes around 20ms for any spikes to start in RN custers
    freqHz = (float(spikeCount)/float(neuronCount)) * 1000.0/timeWindowMs #numspikes per second
    #print 'population', popn.getLabel(),': ', neuronCount,'neurons produced', spikeCount,'spikes in ', timeWindowMs, 'ms. Spike Freq (Hz) implied:', freqHz 

    #For raster plot of all together, we need to convert neuron ids to be global not local to each pop
    for j in spikes:
        j[0]=j[0] + indexOffset
    
    #print spikes
    #plt.plot([i[1] for i in spikes], [i[0] for i in spikes], ".")
    plt.plot([i[1] for i in spikes], [i[0] for i in spikes], "k,") #black pixels
    #plt.plot([i[1] for i in spikes], [i[0] for i in spikes], markersize=1,marker='.',color='black')
    
    
    lastIndexUsed = indexOffset + len(popn) - 1 
    return lastIndexUsed

#----------------------------------------------------------------------------------------------------------------------------
def fromList_convertWeightMatrix(matrix, delay, weight_scale = 1.0):
    def build_list(indices):
        # Extract weights from matrix using indices
        weights = np.multiply(matrix[indices], weight_scale)

        # Build numpy array of delays
        delays = np.repeat(1, len(weights))

        # Zip x-y coordinates of non-zero weights with weights and delays
        return zip(indices[0], indices[1], weights, delays)

    # Get indices of non-zero weights
    non_zero_weights = np.where(matrix != 0.0)

    # Return connection lists
    return build_list(non_zero_weights)

#----------------------------------------------------------------------------------------------------------------------------
def loadListFromFile(filename):
    if fileExists(filename):
        with open(filename, 'r') as f:
            myList = json.load(f)
            f.close()
            return myList
    else:
        raise Exception('loadListFromFile(): Failed to located specified file to load List:' + filename)

#----------------------------------------------------------------------------------------------------------------------------
def saveListToFile(myList, filename):
    with open(filename, 'w') as f:
        json.dump(myList,f)
        f.close()

#----------------------------------------------------------------------------------------------------------------------------
def saveListAsCsvFile(myList, filename):
    with open(filename, 'w') as f:
        f.write(str(myList)[1:-1])
        f.close()
    
#----------------------------------------------------------------------------------------------------------------------------
def loadListFromCsvFile(filename,flatten):
    data = np.loadtxt(filename, delimiter=',')
    if flatten:
        return data.flatten().tolist()
    else:
        return data.toList()
    
#----------------------------------------------------------------------------------------------------------------------------
def saveSpikesToFile(popn,filename):    
    spikes = popn.getSpikes(compatible_output=True)
    np.savetxt(filename, spikes, delimiter=',',fmt='%i,%i')
    
#----------------------------------------------------------------------------------------------------------------------------
def fileExists(file_path):
    return os.path.isfile(file_path)  

#----------------------------------------------------------------------------------------------------------------------------
# return numpy array giving neuron id and spike time falling in a specified time range
def getSpikesBetween(startMs,endMs,spikes):
    
    if spikes.shape[0] == 0:
        #no spikes
        return np.array([])
     
    matchingIndices = np.where((spikes[:,1] >= startMs) & (spikes[:,1] < endMs))
    result  = spikes[matchingIndices]
    return result
    
      
    