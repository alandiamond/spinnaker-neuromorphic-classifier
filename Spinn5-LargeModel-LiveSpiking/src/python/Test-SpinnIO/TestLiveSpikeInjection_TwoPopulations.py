import spynnaker.pyNN as spynn
import spynnaker_external_devices_plugin.pyNN as ExternalDevices
import sys
import pylab

spynn.setup(timestep=1.0, min_delay=1.0, max_delay=144.0)

print sys.argv

numArgumentsProvided =  len(sys.argv) - 1

if numArgumentsProvided == 7 :
    spikeInjectionPort1 = int(sys.argv[1])
    spikeInjectionPopLabel1 = sys.argv[2]
    nNeurons1 = int(sys.argv[3])
    spikeInjectionPort2 = int(sys.argv[4])
    spikeInjectionPopLabel2 = sys.argv[5]
    nNeurons2 = int(sys.argv[6])
    runTimeMs = int(sys.argv[7])
else:
    print 'usage: ', sys.argv[0] , ' <spikeInjectionPort1> <spikeInjectionPopLabel1> <nNeurons1> <spikeInjectionPort2> <spikeInjectionPopLabel2> <nNeurons2> <runTimeMs>'
    sys.exit(0)

#nNeurons = 784 # * 8; #MNIST 28 x 28 pixels , 8 neurons each
#runTimeMs = 1000 

cell_params_lif = {'cm'        : 0.25,  # nF
                   'i_offset'  : 0.0,
                   'tau_m'     : 20.0,
                   'tau_refrac': 2.0,
                   'tau_syn_E' : 5.0,
                   'tau_syn_I' : 5.0,
                   'v_reset'   : -70.0,
                   'v_rest'    : -65.0,
                   'v_thresh'  : -50.0
                  }


# The port on which the spiNNaker machine should listen for packets.
# Packets to be injected should be sent to this port on the spiNNaker machine
cell_params_spike_injector_1 = {'port': spikeInjectionPort1}
cell_params_spike_injector_2 = {'port': spikeInjectionPort2}


populations = list()
projections = list()

weight_to_spike = 2.0

pop_spikes_out_1 = spynn.Population(nNeurons1, spynn.IF_curr_exp, cell_params_lif, label='pop_spikes_out_1') 
populations.append(pop_spikes_out_1)
pop_spikes_out_2 = spynn.Population(nNeurons2, spynn.IF_curr_exp, cell_params_lif, label='pop_spikes_out_2') 
populations.append(pop_spikes_out_2)

pop_spikes_in_1 = spynn.Population(nNeurons1,ExternalDevices.SpikeInjector,cell_params_spike_injector_1,label=spikeInjectionPopLabel1)
populations.append(pop_spikes_in_1)
pop_spikes_in_2 = spynn.Population(nNeurons2,ExternalDevices.SpikeInjector,cell_params_spike_injector_2,label=spikeInjectionPopLabel2)
populations.append(pop_spikes_in_2)

pop_spikes_out_1.record()
#ExternalDevices.activate_live_output_for(pop_spikes_out_1)
pop_spikes_out_2.record()
#ExternalDevices.activate_live_output_for(pop_spikes_out_2)

projections.append(spynn.Projection(pop_spikes_in_1, pop_spikes_out_1,spynn.OneToOneConnector(weights=weight_to_spike)))
projections.append(spynn.Projection(pop_spikes_in_2, pop_spikes_out_2,spynn.OneToOneConnector(weights=weight_to_spike)))

spynn.run(runTimeMs + 1000) #add extra second to get all downstream spikes

spikes1 = pop_spikes_out_1.getSpikes(compatible_output=True)
spikes2 = pop_spikes_out_2.getSpikes(compatible_output=True)
#For raster plot of all together, we need to convert neuron ids to be global not local to each pop
for j in spikes2:
    j[0]=j[0] + nNeurons1

totalSpikes = len(spikes1) +  len(spikes1)
print 'Total spikes generated: ' , totalSpikes
if  totalSpikes > 0:
    print "Last spike pop 1: " , spikes1[len(spikes1) - 1]
    print "Last spike pop 2: " , spikes2[len(spikes2) - 1]
    #print spikes
    pylab.figure()
    pylab.plot([i[1] for i in spikes1], [i[0] for i in spikes1], "k,")  #black pixels
    pylab.plot([i[1] for i in spikes2], [i[0] for i in spikes2], "k,")  #black pixels
    pylab.ylabel('neuron id')
    pylab.xlabel('Time/ms')
    pylab.title('Raster Plot')
    pylab.show()
else:
    print "No spikes received"


spynn.end()
