/*
  ==============================================================================

    AudioProcessorGraphMultiThreaded.cpp
    Created: 26 Nov 2012 11:35:49am
    Author:  Christian

  ==============================================================================
*/

#include "AudioProcessorGraphMultiThreaded.h"


const int AudioProcessorGraphMultiThreaded::midiChannelIndex = 0x1000;

//==============================================================================
namespace GraphRenderingOpsMultiThreaded
{


//==============================================================================
struct ConnectionSorter
{
    static int compareElements (const AudioProcessorGraphMultiThreaded::Connection* const first,
                                const AudioProcessorGraphMultiThreaded::Connection* const second) noexcept
    {
        if      (first->sourceNodeId < second->sourceNodeId)                return -1;
        else if (first->sourceNodeId > second->sourceNodeId)                return 1;
        else if (first->destNodeId < second->destNodeId)                    return -1;
        else if (first->destNodeId > second->destNodeId)                    return 1;
        else if (first->sourceChannelIndex < second->sourceChannelIndex)    return -1;
        else if (first->sourceChannelIndex > second->sourceChannelIndex)    return 1;
        else if (first->destChannelIndex < second->destChannelIndex)        return -1;
        else if (first->destChannelIndex > second->destChannelIndex)        return 1;

        return 0;
    }
};

}

//==============================================================================
AudioProcessorGraphMultiThreaded::Connection::Connection (const uint32 sourceNodeId_, const int sourceChannelIndex_,
                                             const uint32 destNodeId_, const int destChannelIndex_) noexcept
    : sourceNodeId (sourceNodeId_), sourceChannelIndex (sourceChannelIndex_),
      destNodeId (destNodeId_), destChannelIndex (destChannelIndex_)
{
}

//==============================================================================
AudioProcessorGraphMultiThreaded::Node::Node (const uint32 nodeId_, AudioProcessor* const processor_, AudioProcessorGraphMultiThreaded& graph_) noexcept
    :   nodeId (nodeId_),
        processingDone(false),
        processor (processor_),
        isPrepared (false),
        graph(graph_)

{
	AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor* const ioProc
		= dynamic_cast <AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor*> (processor.get());

	if (ioProc != nullptr)
	{
		ioProc->setParentGraph (&graph);
	
	} ;



};

void AudioProcessorGraphMultiThreaded::Node::prepare (const double sampleRate, const int blockSize              )
{
    if (! isPrepared)
    {
        isPrepared = true;

		 setParentGraph (&graph); // setParentGraph also sets input and outputchannels 

		buffer= new AudioSampleBuffer(jmax(processor->getNumInputChannels(),processor->getNumOutputChannels()),blockSize);
		buffer->clear();

	
	

        processor->setPlayConfigDetails (processor->getNumInputChannels(),
                                         processor->getNumOutputChannels(),
                                         sampleRate, blockSize);

        processor->prepareToPlay (sampleRate, blockSize);

		
    }
}

void AudioProcessorGraphMultiThreaded::Node::unprepare()
{
    if (isPrepared)
    {
        isPrepared = false;
        processor->releaseResources();
    }
}



bool AudioProcessorGraphMultiThreaded::Node::process()
{
	// A node can only by processed by one thread at one time
	ScopedTryLock sl(nodeLock);
	if (!sl.isLocked())
	{
		return false;
	}

	if (!graph.nodeProcessingActive)
	{
		return false;
	}


	if (processingDone)
	{
		// Already processed
		return false;
	}

	if (outputNodesToInform.size()==0 && requiredInputs.size()==0)
	{
		// Has no inputs and no outputs, so do nothing 
		return false;
	}

	if (!hasReachedNumberOfRequiredInputs())
	{
		// waiting for other nodes input
		return false;
	};

	if (!isPrepared)
	{
		jassertfalse;	
		return false;
	}

//	DBG("ProcessNode "+ getProcessor()->getName());

	for (int o=0; o<buffer->getNumChannels(); o++)
	{
		bool unwritten=true;

		for (int i=0; i<requiredInputs.size();i++)
		{
			NodeChannel* ri=requiredInputs[i];
			if (ri->inputConnection->destChannelIndex==o)
			{
				if (ri->inputConnection->sourceChannelIndex<ri->node->buffer->getNumChannels())
				{
					if (unwritten)
					{
						buffer->copyFrom(o,0,*ri->node->buffer,ri->inputConnection->sourceChannelIndex,0,buffer->getNumSamples());		
						unwritten=false;
					} else
					{
						buffer->addFrom(o,0,*ri->node->buffer,ri->inputConnection->sourceChannelIndex,0,buffer->getNumSamples());		
					};
				} else
				{
					DBG("Impossible Connection: Source Channel "+String(ri->inputConnection->sourceChannelIndex)+" is higher than available channels "+String(buffer->getNumChannels()));
				}
			};
		}

		if (unwritten)
		{
			buffer->clear(o,0,buffer->getNumSamples());
		};
	};

	

	processor->processBlock(*buffer, midiFakeBufferNotImplemented);

	for (int i=0; i<outputNodesToInform.size();i++)
	{
		NodeChannel* ri=outputNodesToInform[i];
		++(ri->node->numberOfProcessedInputs);
	}


	processingDone=true;

	return true;

}


bool AudioProcessorGraphMultiThreaded::Node::hasReachedNumberOfRequiredInputs()
{
	return numberOfProcessedInputs.get()>=requiredInputs.size();
}


void AudioProcessorGraphMultiThreaded::Node::reset()
{
	ScopedLock sl(nodeLock);
	numberOfProcessedInputs=0;
	processingDone=false;
}

void AudioProcessorGraphMultiThreaded::Node::updateMyConnections()
{
	ScopedLock sl(graph.getConfigurationLock());

	requiredInputs.clear();
	outputNodesToInform.clear();
	
	

	for (int i = graph.connections.size(); --i >= 0;)
	{
		if (graph.connections.getUnchecked(i)->destNodeId==nodeId)
		{	
			
			NodeChannel* ri = new NodeChannel();
			ri->inputConnection=new Connection(*graph.connections.getUnchecked(i));
			ri->node=graph.getNodeRefPtrForId(graph.connections.getUnchecked(i)->sourceNodeId);
			requiredInputs.add(ri);
		};

		if (graph.connections.getUnchecked(i)->sourceNodeId==nodeId)
		{
			NodeChannel* ri = new NodeChannel();
			ri->inputConnection=new Connection(*graph.connections.getUnchecked(i));
			ri->node=graph.getNodeRefPtrForId(graph.connections.getUnchecked(i)->destNodeId);
			outputNodesToInform.add(ri);
		};
		

	};

}

void AudioProcessorGraphMultiThreaded::Node::setParentGraph( AudioProcessorGraphMultiThreaded*  graph ) const
{
	AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor* const ioProc
		= dynamic_cast <AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor*> (processor.get());

	if (ioProc != nullptr)
		ioProc->setParentGraph (graph);
}

int AudioProcessorGraphMultiThreaded::Node::getNumRequiredInputs()
{
	return requiredInputs.size();
}

bool AudioProcessorGraphMultiThreaded::Node::isProcessingDone()
{
	return processingDone;
}

//==============================================================================
AudioProcessorGraphMultiThreaded::AudioProcessorGraphMultiThreaded()
    :   nodeProcessingActive(false),
        waitEvent(true),
		lastNodeId (0),
      renderingBuffers (1, 1),
      currentAudioOutputBuffer (1, 1)

{
	nothingHappenedCounter=0;
	nextNodeIteritater=0;

	for (int i=0; i<jmax(1,SystemStats::getNumCpus()-1); i++)	
	{
		workerThreads.add(new WorkerThread(*this));
	};

};


void AudioProcessorGraphMultiThreaded::startAllThreads()
{
	for (int i=0; i<workerThreads.size();i++)
	{
		workerThreads[i]->startThread();
	} 
};


void AudioProcessorGraphMultiThreaded::stopAllThreads()
{
	for (int i=0; i<workerThreads.size();i++)
	{
		workerThreads[i]->signalThreadShouldExit();
	};

	for (int i=0; i<workerThreads.size();i++)
	{
		workerThreads[i]->stopThread(1000);
	};
}




AudioProcessorGraphMultiThreaded::~AudioProcessorGraphMultiThreaded()
{
	stopAllThreads();

	// Remove Circular References
	// yes, the use of ReferenceCountedPointers may not the ideal solution, because nodes refer each other, which result in Circular References 
	for (int i=0; i<nodes_original.size();i++)
	{
		nodes_original[i]->clearRef();
	}
	nodes_original.clear();

	// Remove Circular References 
	for (int i=0; i<nodes_workingCopy.size();i++)
	{
		nodes_workingCopy[i]->clearRef();
	}
	nodes_workingCopy.clear();


	connections.clear();
	outputNode=nullptr;
}

void AudioProcessorGraphMultiThreaded::clear()
{
	ScopedLock sl(reconfigurationLock);
	stopAllThreads();
	
	// Remove Circular References 
	for (int i=0; i<nodes_original.size();i++)
	{
		nodes_original[i]->clearRef();
	}
	nodes_original.clear();


	connections.clear();
	triggerAsyncUpdate();
}

const String AudioProcessorGraphMultiThreaded::getName() const
{
    return "Audio Graph";
}

//==============================================================================


AudioProcessorGraphMultiThreaded::Node* AudioProcessorGraphMultiThreaded::getNodeForId (const uint32 nodeId) const
{
	ScopedLock sl(reconfigurationLock);

	for (int i = nodes_original.size(); --i >= 0;)
        if (nodes_original.getUnchecked(i)->nodeId == nodeId)
            return nodes_original.getUnchecked(i);

    return nullptr;
}

AudioProcessorGraphMultiThreaded::Node::Ptr AudioProcessorGraphMultiThreaded::getNodeRefPtrForId (const uint32 nodeId) const
{
	ScopedLock sl(reconfigurationLock);

	for (int i = nodes_original.size(); --i >= 0;)
		if (nodes_original.getUnchecked(i)->nodeId == nodeId)
			return nodes_original.getUnchecked(i);

	return nullptr;
}



AudioProcessorGraphMultiThreaded::Node* AudioProcessorGraphMultiThreaded::addNode (AudioProcessor* const newProcessor, uint32 nodeId)
{
	ScopedLock sl(reconfigurationLock);


	
    if (newProcessor == nullptr)
    {
        jassertfalse;
        return nullptr;
    }

    if (nodeId == 0)
    {
        nodeId = ++lastNodeId;
    }
    else
    {
        // you can't add a node with an id that already exists in the graph..
        jassert (getNodeForId (nodeId) == nullptr);
        
		removeNode (nodeId);

        if (nodeId > lastNodeId)
            lastNodeId = nodeId;
    }

    Node* const n = new Node (nodeId, newProcessor,*this);
    nodes_original.add (n);
    triggerAsyncUpdate();

	

    return n;
}

bool AudioProcessorGraphMultiThreaded::removeNode (const uint32 nodeId)
{
	ScopedLock sl(reconfigurationLock);

    disconnectNode (nodeId);

	buildRenderingSequence();	// important to remove circular references

    for (int i = nodes_original.size(); --i >= 0;)
    {
        if (nodes_original.getUnchecked(i)->nodeId == nodeId)
        {
			nodes_original.remove (i);
            triggerAsyncUpdate();
            return true;
        }
    }

    return false;
}

//==============================================================================
const AudioProcessorGraphMultiThreaded::Connection* AudioProcessorGraphMultiThreaded::getConnectionBetween (const uint32 sourceNodeId,
                                                                                  const int sourceChannelIndex,
                                                                                  const uint32 destNodeId,
                                                                                  const int destChannelIndex) const
{
	ScopedLock sl(reconfigurationLock);

    const Connection c (sourceNodeId, sourceChannelIndex, destNodeId, destChannelIndex);
    GraphRenderingOpsMultiThreaded::ConnectionSorter sorter;
    return connections [connections.indexOfSorted (sorter, &c)];
}

bool AudioProcessorGraphMultiThreaded::isConnected (const uint32 possibleSourceNodeId,
                                       const uint32 possibleDestNodeId) const
{
	ScopedLock sl(reconfigurationLock);

    for (int i = connections.size(); --i >= 0;)
    {
        const Connection* const c = connections.getUnchecked(i);

        if (c->sourceNodeId == possibleSourceNodeId
             && c->destNodeId == possibleDestNodeId)
        {
            return true;
        }
    }

    return false;
}

bool AudioProcessorGraphMultiThreaded::canConnect (const uint32 sourceNodeId,
                                      const int sourceChannelIndex,
                                      const uint32 destNodeId,
                                      const int destChannelIndex) const
{
	ScopedLock sl(reconfigurationLock);

	if (sourceChannelIndex < 0
         || destChannelIndex < 0
         || sourceNodeId == destNodeId
         || (destChannelIndex == midiChannelIndex) != (sourceChannelIndex == midiChannelIndex))
        return false;

    const Node* const source = getNodeForId (sourceNodeId);

    if (source == nullptr
         || (sourceChannelIndex != midiChannelIndex && sourceChannelIndex >= source->getProcessor()->getNumOutputChannels())
         || (sourceChannelIndex == midiChannelIndex && ! source->getProcessor()->producesMidi()))
        return false;

    const Node* const dest = getNodeForId (destNodeId);

    if (dest == nullptr
         || (destChannelIndex != midiChannelIndex && destChannelIndex >= dest->getProcessor()->getNumInputChannels())
         || (destChannelIndex == midiChannelIndex && ! dest->getProcessor()->acceptsMidi()))
        return false;

    return getConnectionBetween (sourceNodeId, sourceChannelIndex,
                                 destNodeId, destChannelIndex) == nullptr;
}

bool AudioProcessorGraphMultiThreaded::addConnection (const uint32 sourceNodeId,
                                         const int sourceChannelIndex,
                                         const uint32 destNodeId,
                                         const int destChannelIndex)
{
	ScopedLock sl(reconfigurationLock);

    if (! canConnect (sourceNodeId, sourceChannelIndex, destNodeId, destChannelIndex))
        return false;

    GraphRenderingOpsMultiThreaded::ConnectionSorter sorter;
    connections.addSorted (sorter, new Connection (sourceNodeId, sourceChannelIndex,
                                                   destNodeId, destChannelIndex));
    triggerAsyncUpdate();
    return true;
}

void AudioProcessorGraphMultiThreaded::removeConnection (const int index)
{
	ScopedLock sl(reconfigurationLock);

	connections.remove (index);
    triggerAsyncUpdate();
}

bool AudioProcessorGraphMultiThreaded::removeConnection (const uint32 sourceNodeId, const int sourceChannelIndex,
                                            const uint32 destNodeId, const int destChannelIndex)
{
	ScopedLock sl(reconfigurationLock);

    bool doneAnything = false;

    for (int i = connections.size(); --i >= 0;)
    {
        const Connection* const c = connections.getUnchecked(i);

        if (c->sourceNodeId == sourceNodeId
             && c->destNodeId == destNodeId
             && c->sourceChannelIndex == sourceChannelIndex
             && c->destChannelIndex == destChannelIndex)
        {
            removeConnection (i);
            doneAnything = true;
        }
    }

    return doneAnything;
}

bool AudioProcessorGraphMultiThreaded::disconnectNode (const uint32 nodeId)
{
	ScopedLock sl(reconfigurationLock);

    bool doneAnything = false;

    for (int i = connections.size(); --i >= 0;)
    {
        const Connection* const c = connections.getUnchecked(i);

        if (c->sourceNodeId == nodeId || c->destNodeId == nodeId)
        {
            removeConnection (i);
            doneAnything = true;
        }
    }

    return doneAnything;
}

bool AudioProcessorGraphMultiThreaded::isConnectionLegal (const Connection* const c) const
{
	ScopedLock sl(reconfigurationLock);

    jassert (c != nullptr);

    const Node* const source = getNodeForId (c->sourceNodeId);
    const Node* const dest   = getNodeForId (c->destNodeId);

    return source != nullptr
        && dest != nullptr
        && (c->sourceChannelIndex != midiChannelIndex ? isPositiveAndBelow (c->sourceChannelIndex, source->getProcessor()->getNumOutputChannels())
                                                      : source->getProcessor()->producesMidi())
        && (c->destChannelIndex   != midiChannelIndex ? isPositiveAndBelow (c->destChannelIndex, dest->getProcessor()->getNumInputChannels())
                                                      : dest->getProcessor()->acceptsMidi());
}

bool AudioProcessorGraphMultiThreaded::removeIllegalConnections()
{
	ScopedLock sl(reconfigurationLock);

    bool doneAnything = false;

    for (int i = connections.size(); --i >= 0;)
    {
        if (! isConnectionLegal (connections.getUnchecked(i)))
        {
            removeConnection (i);
            doneAnything = true;
        }
    }

    return doneAnything;
}




//==============================================================================




/*
bool AudioProcessorGraphMultiProcessor::isAnInputTo (const uint32 possibleInputId,
                                       const uint32 possibleDestinationId,
                                       const int recursionCheck) const
{ 
	ScopedLock sl(reconfigurationLock);

    if (recursionCheck > 0)
    {
        for (int i = connections.size(); --i >= 0;)
        {
            const AudioProcessorGraphMultiProcessor::Connection* const c = connections.getUnchecked (i);

            if (c->destNodeId == possibleDestinationId
                 && (c->sourceNodeId == possibleInputId
                      || isAnInputTo (possibleInputId, c->sourceNodeId, recursionCheck - 1)))
                return true;
        }
    }

    return false;
}*/

void AudioProcessorGraphMultiThreaded::buildRenderingSequence()
{
    ScopedLock sl(reconfigurationLock);

	suspendProcessing(true);
	stopAllThreads();

	outputNode=nullptr;
	for (int i = 0; i < nodes_original.size(); ++i)
	{
		Node::Ptr n=nodes_original.getUnchecked(i);
		AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor* const ioProc
			= dynamic_cast <AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor*> (n->getProcessor());

		if (ioProc!=nullptr)
		{
			if (ioProc->isAudioOutputNode())
			{
				outputNode=n;
				break;
			}
		}
	};


	// Remove Cirucal References 
	for (int i=0; i<nodes_workingCopy.size();i++)
	{
		nodes_workingCopy[i]->clearRef();
	}


	nodes_workingCopy=nodes_original;
    
    for (int i = 0; i < nodes_original.size(); ++i)
    {
         Node* const node = nodes_original.getUnchecked(i);
         node->prepare (getSampleRate(), getBlockSize());
    }
    
   for (int i = 0; i < nodes_original.size(); ++i)
   {
	   Node* const node = nodes_original.getUnchecked(i);

	   node->updateMyConnections();
	   node->reset();
   };
		
   if (outputNode==nullptr)
   {
	   jassertfalse;
	   //No output node found
   } else
   {
	   

	   startAllThreads();
	   suspendProcessing(false);
   }

  
    
   // delete the old ones..
};

void AudioProcessorGraphMultiThreaded::handleAsyncUpdate()
{
    buildRenderingSequence();
}

//==============================================================================
void AudioProcessorGraphMultiThreaded::prepareToPlay (double /*sampleRate*/, int estimatedSamplesPerBlock)
{
    currentAudioInputBuffer = nullptr;
    currentAudioOutputBuffer.setSize (jmax (1, getNumOutputChannels()), estimatedSamplesPerBlock);
    currentMidiInputBuffer = nullptr;
    currentMidiOutputBuffer.clear();
   
    buildRenderingSequence();
}

void AudioProcessorGraphMultiThreaded::releaseResources()
{
	stopAllThreads();
	suspendProcessing(true);

	ScopedLock sl(reconfigurationLock);

    for (int i = 0; i < nodes_original.size(); ++i)
        nodes_original.getUnchecked(i)->unprepare();

    renderingBuffers.setSize (1, 1);
    midiBuffers.clear();

    currentAudioInputBuffer = nullptr;
    currentAudioOutputBuffer.setSize (1, 1);
    currentMidiInputBuffer = nullptr;
    currentMidiOutputBuffer.clear();

	suspendProcessing(false);
}

void AudioProcessorGraphMultiThreaded::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	ScopedLock sl (getCallbackLock()); // Can I trust that my parent has locked this?

	if (isSuspended())
	{
		buffer.clear();
		return;
	};

	const int numSamples = buffer.getNumSamples();

	const ScopedLock rl (renderLock);

	currentAudioInputBuffer = &buffer;
	currentAudioOutputBuffer.setSize (jmax (1, buffer.getNumChannels()), numSamples);
	currentAudioOutputBuffer.clear();
	currentMidiInputBuffer = &midiMessages;
	currentMidiOutputBuffer.clear();

	

	//DBG("Start Processing");
	// Process
	nodeProcessingActive=true;

	// Using this Callback for processing too
	if (outputNode->getNumRequiredInputs()>0)
	{
		while (!outputNode->isProcessingDone())
		{
			processOneNode(true);
		}
	} else
	{
		buffer.clear();
	}

	// All important Nodes have Processing Done
	// Set a flag

	nodeProcessingActive=false;

	// After calling reset, the input-node will immediatly begin processing, nodeProcessingActive=false will prevent this.
	// reset locks the node, to be sure 

	//DBG("Stop Processing");

	{
		
		for (int i = 0; i < nodes_workingCopy.size(); ++i)
		{
			Node* const node = nodes_workingCopy.getUnchecked(i);
			
			
//			jassert(outputNode->getNumRequiredInputs()==0 || node->isProcessingDone());
			
			node->reset();
		};
	}



	for (int i = 0; i < buffer.getNumChannels(); ++i)
		buffer.copyFrom (i, 0, currentAudioOutputBuffer, i, 0, numSamples);

	midiMessages.clear();
	midiMessages.addEvents (currentMidiOutputBuffer, 0, buffer.getNumSamples(), 0);

}

const String AudioProcessorGraphMultiThreaded::getInputChannelName (int channelIndex) const
{
    return "Input " + String (channelIndex + 1);
}

const String AudioProcessorGraphMultiThreaded::getOutputChannelName (int channelIndex) const
{
    return "Output " + String (channelIndex + 1);
}

bool AudioProcessorGraphMultiThreaded::isInputChannelStereoPair (int /*index*/) const    { return true; }
bool AudioProcessorGraphMultiThreaded::isOutputChannelStereoPair (int /*index*/) const   { return true; }
bool AudioProcessorGraphMultiThreaded::silenceInProducesSilenceOut() const               { return false; }
bool AudioProcessorGraphMultiThreaded::acceptsMidi() const   { return true; }
bool AudioProcessorGraphMultiThreaded::producesMidi() const  { return true; }
void AudioProcessorGraphMultiThreaded::getStateInformation (juce::MemoryBlock& /*destData*/)   {}
void AudioProcessorGraphMultiThreaded::setStateInformation (const void* /*data*/, int /*sizeInBytes*/)   {}

void AudioProcessorGraphMultiThreaded::processOneNode(bool thisIsAudioCallbackThread)
{
	
	AudioProcessorGraphMultiThreaded::Node::Ptr node(nullptr);

	
	
	// the node Pointer is safe because reference counted
	if (nodes_workingCopy.size()!=0)
	{
		node=nodes_workingCopy[ (++nextNodeIteritater) % nodes_workingCopy.size() ];
	}

	if (node!=nullptr)
	{
		waitEvent.reset();
		bool doneAnything=node->process();
		if (!doneAnything)
		{
			nothingHappenedCounter++;
		} else
		{
			waitEvent.signal();
			nothingHappenedCounter=0;
		}
	} else
	{
		nothingHappenedCounter++;
	};

	if (nothingHappenedCounter>=nodes_workingCopy.size())	
	{
		if (!thisIsAudioCallbackThread)
		{
			waitEvent.wait(1);// Don't waste CPU Power, when nothing is to do - wait a little bit unless another thread has something finished
		};
		nothingHappenedCounter=0;
	}
	
}

int AudioProcessorGraphMultiThreaded::getNumNodes() const
{
	ScopedLock sl(reconfigurationLock);
	return nodes_original.size();
};

AudioProcessorGraphMultiThreaded::Node* AudioProcessorGraphMultiThreaded::getNode( const int index ) const
{
	 ScopedLock sl(reconfigurationLock);
	return nodes_original [index];
}

double AudioProcessorGraphMultiThreaded::getTailLengthSeconds() const
{
	return 0.;
}





//==============================================================================
AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::AudioGraphIOProcessor (const IODeviceType type_)
    : type (type_),
      graph (nullptr)
{

	
	
}

AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::~AudioGraphIOProcessor()
{
}

const String AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getName() const
{
    switch (type)
    {
        case audioOutputNode:   return "Audio Output";
        case audioInputNode:    return "Audio Input";
        case midiOutputNode:    return "Midi Output";
        case midiInputNode:     return "Midi Input";
        default:                break;
    }

    return String::empty;
}

void AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::fillInPluginDescription (PluginDescription& d) const
{
    d.name = getName();
    d.uid = d.name.hashCode();
    d.category = "I/O devices";
    d.pluginFormatName = "Internal";
    d.manufacturerName = "Raw Material Software";
    d.version = "1.0";
    d.isInstrument = false;

    d.numInputChannels = getNumInputChannels();
    if (type == audioOutputNode && graph != nullptr)
        d.numInputChannels = graph->getNumInputChannels();

    d.numOutputChannels = getNumOutputChannels();
    if (type == audioInputNode && graph != nullptr)
        d.numOutputChannels = graph->getNumOutputChannels();
}

void AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::prepareToPlay (double, int)
{
    jassert (graph != nullptr);
}

void AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::releaseResources()
{
}

void AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::processBlock (AudioSampleBuffer& buffer,
                                                               MidiBuffer& midiMessages)
{
    jassert (graph != nullptr);

    switch (type)
    {
        case audioOutputNode:
        {
            for (int i = jmin (graph->currentAudioOutputBuffer.getNumChannels(),
                               buffer.getNumChannels()); --i >= 0;)
            {
                graph->currentAudioOutputBuffer.addFrom (i, 0, buffer, i, 0, buffer.getNumSamples());
            }

            break;
        }

        case audioInputNode:
        {
            for (int i = jmin (graph->currentAudioInputBuffer->getNumChannels(),
                               buffer.getNumChannels()); --i >= 0;)
            {
                buffer.copyFrom (i, 0, *graph->currentAudioInputBuffer, i, 0, buffer.getNumSamples());
            }

            break;
        }

        case midiOutputNode:
            graph->currentMidiOutputBuffer.addEvents (midiMessages, 0, buffer.getNumSamples(), 0);
            break;

        case midiInputNode:
            midiMessages.addEvents (*graph->currentMidiInputBuffer, 0, buffer.getNumSamples(), 0);
            break;

        default:
            break;
    }
}

bool AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::silenceInProducesSilenceOut() const
{
    return isOutput();
}

bool AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::acceptsMidi() const
{
    return type == midiOutputNode;
}

bool AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::producesMidi() const
{
    return type == midiInputNode;
}

const String AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getInputChannelName (int channelIndex) const
{
    switch (type)
    {
        case audioOutputNode:   return "Output " + String (channelIndex + 1);
        case midiOutputNode:    return "Midi Output";
        default:                break;
    }

    return String::empty;
}

const String AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getOutputChannelName (int channelIndex) const
{
    switch (type)
    {
        case audioInputNode:    return "Input " + String (channelIndex + 1);
        case midiInputNode:     return "Midi Input";
        default:                break;
    }

    return String::empty;
}

bool AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::isInputChannelStereoPair (int /*index*/) const
{
    return type == audioInputNode || type == audioOutputNode;
}

bool AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::isOutputChannelStereoPair (int index) const
{
    return isInputChannelStereoPair (index);
}

bool AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::isInput() const   { return type == audioInputNode  || type == midiInputNode; }
bool AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::isOutput() const  { return type == audioOutputNode || type == midiOutputNode; }
bool AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::isAudioOutputNode() const  { return type == audioOutputNode; };

bool AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::hasEditor() const                  { return false; }
AudioProcessorEditor* AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::createEditor()    { return nullptr; }

int AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getNumParameters()                  { return 0; }
const String AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getParameterName (int)     { return String::empty; }

float AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getParameter (int)                { return 0.0f; }
const String AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getParameterText (int)     { return String::empty; }
void AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::setParameter (int, float)          { }

int AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getNumPrograms()                    { return 0; }
int AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getCurrentProgram()                 { return 0; }
void AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::setCurrentProgram (int)            { }

const String AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getProgramName (int)       { return String::empty; }
void AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::changeProgramName (int, const String&) {}

void AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getStateInformation (juce::MemoryBlock&) {}
void AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::setStateInformation (const void*, int) {}

void AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::setParentGraph (AudioProcessorGraphMultiThreaded* const newGraph)
{
    graph = newGraph;

    if (graph != nullptr)
    {
        setPlayConfigDetails (type == audioOutputNode ? graph->getNumOutputChannels() : 0,
                              type == audioInputNode ? graph->getNumInputChannels() : 0,
                              getSampleRate(),
                              getBlockSize());

        updateHostDisplay();
    }
}

double AudioProcessorGraphMultiThreaded::AudioGraphIOProcessor::getTailLengthSeconds() const
{
	return 0.;
}





