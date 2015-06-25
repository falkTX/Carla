/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2013 - Raw Material Software Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

const int AudioProcessorGraph::midiChannelIndex = 0x1000;

//==============================================================================
namespace GraphRenderingOps
{

//==============================================================================
struct AudioGraphRenderingOp
{
    AudioGraphRenderingOp() noexcept {}
    virtual ~AudioGraphRenderingOp() {}

    virtual void perform (AudioSampleBuffer& sharedBufferChans,
                          const OwnedArray<MidiBuffer>& sharedMidiBuffers,
                          const int numSamples) = 0;

    JUCE_LEAK_DETECTOR (AudioGraphRenderingOp)
};

//==============================================================================
struct ClearChannelOp  : public AudioGraphRenderingOp
{
    ClearChannelOp (const int channel) noexcept  : channelNum (channel)  {}

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray<MidiBuffer>&, const int numSamples)
    {
        sharedBufferChans.clear (channelNum, 0, numSamples);
    }

    const int channelNum;

    JUCE_DECLARE_NON_COPYABLE (ClearChannelOp)
};

//==============================================================================
struct CopyChannelOp  : public AudioGraphRenderingOp
{
    CopyChannelOp (const int srcChan, const int dstChan) noexcept
        : srcChannelNum (srcChan), dstChannelNum (dstChan)
    {}

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray<MidiBuffer>&, const int numSamples)
    {
        sharedBufferChans.copyFrom (dstChannelNum, 0, sharedBufferChans, srcChannelNum, 0, numSamples);
    }

    const int srcChannelNum, dstChannelNum;

    JUCE_DECLARE_NON_COPYABLE (CopyChannelOp)
};

//==============================================================================
struct AddChannelOp  : public AudioGraphRenderingOp
{
    AddChannelOp (const int srcChan, const int dstChan) noexcept
        : srcChannelNum (srcChan), dstChannelNum (dstChan)
    {}

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray<MidiBuffer>&, const int numSamples)
    {
        sharedBufferChans.addFrom (dstChannelNum, 0, sharedBufferChans, srcChannelNum, 0, numSamples);
    }

    const int srcChannelNum, dstChannelNum;

    JUCE_DECLARE_NON_COPYABLE (AddChannelOp)
};

//==============================================================================
struct ClearMidiBufferOp  : public AudioGraphRenderingOp
{
    ClearMidiBufferOp (const int buffer) noexcept  : bufferNum (buffer)  {}

    void perform (AudioSampleBuffer&, const OwnedArray<MidiBuffer>& sharedMidiBuffers, const int)
    {
        sharedMidiBuffers.getUnchecked (bufferNum)->clear();
    }

    const int bufferNum;

    JUCE_DECLARE_NON_COPYABLE (ClearMidiBufferOp)
};

//==============================================================================
struct CopyMidiBufferOp  : public AudioGraphRenderingOp
{
    CopyMidiBufferOp (const int srcBuffer, const int dstBuffer) noexcept
        : srcBufferNum (srcBuffer), dstBufferNum (dstBuffer)
    {}

    void perform (AudioSampleBuffer&, const OwnedArray<MidiBuffer>& sharedMidiBuffers, const int)
    {
        *sharedMidiBuffers.getUnchecked (dstBufferNum) = *sharedMidiBuffers.getUnchecked (srcBufferNum);
    }

    const int srcBufferNum, dstBufferNum;

    JUCE_DECLARE_NON_COPYABLE (CopyMidiBufferOp)
};

//==============================================================================
struct AddMidiBufferOp  : public AudioGraphRenderingOp
{
    AddMidiBufferOp (const int srcBuffer, const int dstBuffer) noexcept
        : srcBufferNum (srcBuffer), dstBufferNum (dstBuffer)
    {}

    void perform (AudioSampleBuffer&, const OwnedArray<MidiBuffer>& sharedMidiBuffers, const int numSamples)
    {
        sharedMidiBuffers.getUnchecked (dstBufferNum)
            ->addEvents (*sharedMidiBuffers.getUnchecked (srcBufferNum), 0, numSamples, 0);
    }

    const int srcBufferNum, dstBufferNum;

    JUCE_DECLARE_NON_COPYABLE (AddMidiBufferOp)
};

//==============================================================================
struct DelayChannelOp  : public AudioGraphRenderingOp
{
    DelayChannelOp (const int chan, const int delaySize) noexcept
        : channel (chan),
          bufferSize (delaySize + 1),
          readIndex (0), writeIndex (delaySize)
    {
        buffer.calloc ((size_t) bufferSize);
    }

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray<MidiBuffer>&, const int numSamples)
    {
        float* data = sharedBufferChans.getWritePointer (channel, 0);

        for (int i = numSamples; --i >= 0;)
        {
            buffer [writeIndex] = *data;
            *data++ = buffer [readIndex];

            if (++readIndex  >= bufferSize) readIndex = 0;
            if (++writeIndex >= bufferSize) writeIndex = 0;
        }
    }

    HeapBlock<float> buffer;
    const int channel, bufferSize;
    int readIndex, writeIndex;

    JUCE_DECLARE_NON_COPYABLE (DelayChannelOp)
};


//==============================================================================
struct ProcessBufferOp  : public AudioGraphRenderingOp
{
    ProcessBufferOp (const AudioProcessorGraph::Node::Ptr& n,
                     const Array<int>& audioChannels,
                     const int totalNumChans,
                     const int midiBuffer)
        : node (n),
          processor (n->getProcessor()),
          audioChannelsToUse (audioChannels),
          totalChans (jmax (1, totalNumChans)),
          midiBufferToUse (midiBuffer)
    {
        channels.calloc ((size_t) totalChans);

        while (audioChannelsToUse.size() < totalChans)
            audioChannelsToUse.add (0);
    }

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray<MidiBuffer>& sharedMidiBuffers, const int numSamples)
    {
        for (int i = totalChans; --i >= 0;)
            channels[i] = sharedBufferChans.getWritePointer (audioChannelsToUse.getUnchecked (i), 0);

        AudioSampleBuffer buffer (channels, totalChans, numSamples);

        processor->processBlock (buffer, *sharedMidiBuffers.getUnchecked (midiBufferToUse));
    }

    const AudioProcessorGraph::Node::Ptr node;
    AudioProcessor* const processor;

    Array<int> audioChannelsToUse;
    HeapBlock<float*> channels;
    const int totalChans;
    const int midiBufferToUse;

    JUCE_DECLARE_NON_COPYABLE (ProcessBufferOp)
};

class MapNode;

//==============================================================================
/** Represents a connection between two MapNodes.
    Both the source and destination node will have their own copy of this information.
 */
struct MapNodeConnection
{
    MapNodeConnection (MapNode* srcMapNode, MapNode* dstMapNode, int srcChannelIndex, int dstChannelIndex) noexcept
      : sourceMapNode (srcMapNode), destMapNode (dstMapNode),
        sourceChannelIndex (srcChannelIndex), destChannelIndex (dstChannelIndex)
    {}

    MapNode* sourceMapNode;
    MapNode* destMapNode;
    const int sourceChannelIndex, destChannelIndex;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MapNodeConnection)
};

//==============================================================================
/** Wraps an AudioProcessorGraph::Node providing information regarding source
    (input) and destination (output) connections, input latencies and sorting.
 */
class MapNode
{
public:
    MapNode (const uint32 nodeId, AudioProcessorGraph::Node* node) noexcept
      : feedbackLoopCheck (0), nodeId (nodeId), node (node),
        renderIndex (0), maxInputLatency (0), maxLatency (0)
    {}

    uint32 getNodeId() const noexcept                       { return nodeId; }
    int getRenderIndex() const noexcept                     { return renderIndex; }
    AudioProcessorGraph::Node* getNode() const noexcept     { return node; }
    int getMaxInputLatency() const noexcept                 { return maxInputLatency; }
    int getMaxLatency() const noexcept                      { return maxLatency; }

    void setRenderIndex (int theRenderIndex)
    {
        renderIndex = theRenderIndex;
    }

    const Array<MapNode*>& getUniqueSources() const noexcept                    { return uniqueSources; }
    const Array<MapNode*>& getUniqueDestinations() const noexcept               { return uniqueDestinations; }

    const Array<MapNodeConnection*>& getSourceConnections() const noexcept  { return sourceConnections; }
    const Array<MapNodeConnection*>& getDestConnections() const noexcept    { return destConnections; }

    void addInputConnection (MapNodeConnection* mnc)
    {
        sourceConnections.add (mnc);
    }
    
    void addOutputConnection (MapNodeConnection* mnc)
    {
        destConnections.add (mnc);
    }

    Array<MapNodeConnection*> getSourcesToChannel (int channel) const
    {
        Array<MapNodeConnection*> sourcesToChannel;

        for (int i = 0; i < sourceConnections.size(); ++i)
        {
            MapNodeConnection* ec = sourceConnections.getUnchecked (i);

            if (ec->destChannelIndex == channel)
                sourcesToChannel.add (ec);
        }

        return sourcesToChannel;
    }

    void calculateLatenciesWithInputLatency (int inputLatency)
    {
        maxInputLatency = inputLatency;
        maxLatency = maxInputLatency + node->getProcessor()->getLatencySamples();
    }

    void cacheUniqueNodeConnections()
    {
        //This should only ever be called once but in case things change
        uniqueSources.clear();
        uniqueDestinations.clear();

        for (int i = 0; i < sourceConnections.size(); ++i)
        {
            const MapNodeConnection* ec = sourceConnections.getUnchecked (i);

            if (! uniqueSources.contains(ec->sourceMapNode))
                uniqueSources.add(ec->sourceMapNode);
        }

        for (int i = 0; i < destConnections.size(); ++i)
        {
            const MapNodeConnection* ec = destConnections.getUnchecked (i);

            if (! uniqueDestinations.contains (ec->destMapNode))
                uniqueDestinations.add (ec->destMapNode);
        }

    }

    //#smell - ? Not really sure how to best handle feedback loops but this helps deal with them without incurring much performance penalty.
    uint32 feedbackLoopCheck;

private:
    const uint32 nodeId;
    AudioProcessorGraph::Node* node;
    int renderIndex, maxInputLatency, maxLatency;

    Array<MapNodeConnection*> sourceConnections, destConnections;
    Array<MapNode*> uniqueSources, uniqueDestinations;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MapNode)
};

//==============================================================================
/** Combines Node and Connection data into MapNodes and sorts them.
    Sorted MapNodes are provided to the RenderingOpsSequenceCalculator
 */
class GraphMap
{
public:
    GraphMap() : feedbackCheckLimit (50000) {}

    const Array<MapNode*>& getSortedMapNodes() const noexcept   { return sortedMapNodes; }

    void buildMap (const OwnedArray<AudioProcessorGraph::Connection>& connections,
                   const ReferenceCountedArray<AudioProcessorGraph::Node>& nodes)
    {
        mapNodes.ensureStorageAllocated (nodes.size());

        //Create MapNode for every node.
        for (int i = 0; i < nodes.size(); ++i)
        {
            AudioProcessorGraph::Node* node = nodes.getUnchecked (i);

            //#smell - just want to get the insert index, we don't care about the returned node.
            int index;
            MapNode* foundMapNode = findMapNode (node->nodeId, index);
            jassert (foundMapNode == nullptr);  //cannot have duplicate nodeIds.

            mapNodes.insert (index, new MapNode (node->nodeId, node));
        }

        //Add MapNodeConnections to MapNodes
        for (int i = 0; i < connections.size(); ++i)
        {
            const AudioProcessorGraph::Connection* c = connections.getUnchecked (i);

            //#smell - We don't care about index here but we do care about the returned node.
            int index;
            MapNode* srcMapNode = findMapNode (c->sourceNodeId, index);
            MapNode* destMapNode = findMapNode (c->destNodeId, index);

            jassert (srcMapNode != nullptr && destMapNode != nullptr); //somehow we have a connection that points to a non existant node.  Should never happen.
            
            MapNodeConnection * mnc = new MapNodeConnection (srcMapNode, destMapNode, c->sourceChannelIndex, c->destChannelIndex);
            mapNodeConnections.add (mnc);
            
            srcMapNode->addOutputConnection (mnc);
            destMapNode->addInputConnection (mnc);
            
        }

        //MapNodes have all their connections now, but for sorting we only care about connections to distinct nodes so
        //cache lists of the distinct source and destination nodes.
        for (int i = 0; i < mapNodes.size(); ++i)
            mapNodes.getUnchecked (i)->cacheUniqueNodeConnections();

        //Grab all the nodes that have no input connections (they are used as the starting points for the sort routine)
        Array<MapNode*> nodesToSort;
        for (int i = 0; i < mapNodes.size(); ++i)
        {
            MapNode* mapNode = mapNodes.getUnchecked (i);

            if (mapNode->getUniqueSources().size() == 0)
                nodesToSort.add (mapNode);
        }

        sortedMapNodes.ensureStorageAllocated (mapNodes.size());

        //Sort the nodes
        for (;;)
        {
            Array<MapNode*> nextNodes;
            sortNodes (nodesToSort, sortedMapNodes, nextNodes);

            if (nextNodes.size() == 0)
                break;

            nodesToSort.clear();
            nodesToSort.addArray (nextNodes);
        }
    }

private:
    MapNode* findMapNode (const uint32 destNodeId, int& insertIndex) const noexcept
    {
        int start = 0;
        int end = mapNodes.size();

        for (;;)
        {
            if (start >= end)
            {
                break;
            }
            else if (destNodeId == mapNodes.getUnchecked (start)->getNodeId())
            {
                insertIndex = start;
                return mapNodes.getUnchecked (start);
                break;
            }
            else
            {
                const int halfway = (start + end) / 2;

                if (halfway == start)
                {
                    if (destNodeId >= mapNodes.getUnchecked (halfway)->getNodeId())
                        ++start;

                    break;
                }
                else if (destNodeId >= mapNodes.getUnchecked (halfway)->getNodeId())
                {
                    start = halfway;
                }
                else
                {
                    end = halfway;
                }
            }
        }

        insertIndex = start;

        return nullptr;
    }

    /*
     * Iterates the given nodesToSort and if a node's sources have all been sorted then the node is inserted into the sort list.  If the node is still waiting
     * for a source to be sorted it is added to the nextNodes list for the next call.  If a node is sorted then any of it's destination nodes that are not yet queued are added to the nextNodes list.
     */
    void sortNodes (Array<MapNode*>& nodesToSort, Array<MapNode*>& sortedNodes, Array<MapNode*>& nextNodes) const
    {
        for (int i = 0; i < nodesToSort.size(); ++i)
        {
            MapNode* node = nodesToSort.getUnchecked (i);

            //all of the source nodes must have been sorted first
            const Array<MapNode*>& uniqueSources = node->getUniqueSources();

            //sanity checking
            //should never attempt to sort a node that has already been sorted
            jassert (! isNodeSorted (node));
            
#if JUCE_DEBUG
            //a node with a single source should never attempt to be sorted without it's source already sorted
            if (uniqueSources.size() == 1)
                jassert (isNodeSorted (uniqueSources.getUnchecked (0)));
#endif

            bool canBeSorted = true;
            
            for (int j = 0; j < uniqueSources.size(); ++j)
            {
                MapNode* sourceNode = uniqueSources.getUnchecked (j);

                if (! isNodeSorted (sourceNode))
                {
                    sourceNode->feedbackLoopCheck++;
                    
                    //50000 attempts waiting for this sourceNode to be sorted quite likely means there's a feedback loop.
                    //After 50000 we let the node proceed so that we don't remain stuck in an infinite loop.  Note the graph rendering
                    //may appear to be ok but it will be wrong in some fashion.  If you hit this you should definitely fix your code.
                    canBeSorted = sourceNode->feedbackLoopCheck > feedbackCheckLimit;
                    
                    jassert (sourceNode->feedbackLoopCheck <= feedbackCheckLimit); //Feedback loop detected.  Feedback loops are not supported, will not work, and sometimes mess up the sorting.
                    
                    if (! canBeSorted)
                        break;
                }
                
            }

            if (canBeSorted)
            {
                //Determine the latency information for this node.
                int maxInputLatency = 0;

                for (int j = 0; j < uniqueSources.size(); ++j)
                    maxInputLatency =  jmax (maxInputLatency, uniqueSources.getUnchecked (j)->getMaxLatency());

                node->calculateLatenciesWithInputLatency (maxInputLatency);

                //Add the node to the sorted list
                sortedNodes.add (node);
                node->setRenderIndex (sortedNodes.size());

                //Get destination nodes and add any that are not already added to the sorting routine.
                const Array<MapNode*>& uniqueDestinations = node->getUniqueDestinations();

                for (int j = 0; j < uniqueDestinations.size(); ++j)
                {
                    MapNode* nextNode = uniqueDestinations.getUnchecked (j);

                    //A destination node should never already be sorted but it could happen if a feedback loop is detected.
                    //If this assert fails without a previous feedback loop assertion failure then investigation is required.
                    jassert (! isNodeSorted (nextNode));
                    
                    if (   ! nodesToSort.contains (nextNode)  //the destination node is already trying to be sorted
                        && ! nextNodes.contains (nextNode)    //the destination node has already been queued for the next round of sorting
                        && ! isNodeSorted (nextNode))         //the destination node is already sorted (see assert above, should never be the case unless feedback loop is messing things up)
                        nextNodes.add (nextNode);
                }
            }
            else
            {
                jassert (! nextNodes.contains (node)); //Given the restrictions around adding destination nodes this should never be the case.
                
                //node is still waiting for at least 1 source to be sorted so add to nextNodes and try again on the next pass
                nextNodes.add (node);
                
            }
        }
    }

    bool isNodeSorted (const MapNode* mapNode) const
    {
        jassert (mapNode != nullptr);
        return mapNode->getRenderIndex() > 0;
    }

    const uint32 feedbackCheckLimit;
    OwnedArray<MapNode> mapNodes;
    Array<MapNode*> sortedMapNodes;
    OwnedArray<MapNodeConnection> mapNodeConnections;

    JUCE_DECLARE_NON_COPYABLE (GraphMap)
};

//==============================================================================
/** Used to calculate the correct sequence of rendering ops needed, based on
    the best re-use of shared buffers at each stage.
 */
struct RenderingOpSequenceCalculator
{
    RenderingOpSequenceCalculator (AudioProcessorGraph& g,
                                   const ReferenceCountedArray<AudioProcessorGraph::Node>& nodes,
                                   const OwnedArray<AudioProcessorGraph::Connection>& connections,
                                   Array<void*>& renderingOps)
        : graph (g),
          totalLatency (0)
    {
        audioChannelBuffers.add (new ChannelBufferInfo (0, (uint32) zeroNodeID));
        midiChannelBuffers.add (new ChannelBufferInfo (0, (uint32) zeroNodeID));

        GraphRenderingOps::GraphMap graphMap;
        graphMap.buildMap (connections, nodes);

        for (int i = 0; i < graphMap.getSortedMapNodes().size(); ++i)
        {
            const MapNode* mapNode = graphMap.getSortedMapNodes().getUnchecked (i);
            createRenderingOpsForNode (mapNode,renderingOps);
            markAnyUnusedBuffersAsFree (mapNode);
        }

        graph.setLatencySamples (totalLatency);
    }

    int getNumBuffersNeeded() const noexcept         { return audioChannelBuffers.size(); }
    int getNumMidiBuffersNeeded() const noexcept     { return midiChannelBuffers.size(); }

private:
    /**
     * A list of these are built up as needed by each call to the createRenderingOpsForNode method.  For each call the list is updated
     * by adding more to the list or marking existing ChannelBufferInfo's as free for use on the next pass.  When a ChannelBufferInfo is assigned to a node's output
     * channel, the connection data is analysed to determine at what point the channel will be free for use again.
     */
    struct ChannelBufferInfo
    {
        ChannelBufferInfo (int theSharedBufferChannelIndex, uint32 theNodeId)
        : mapNode (nullptr), nodeId (theNodeId), sharedBufferChannelIndex (theSharedBufferChannelIndex), channelIndex (0),
        freeAtRenderIndex (0)
        {}

        void markAsFree()
        {
            jassert(this->nodeId != (uint32) zeroNodeID);  //cannot reassign the zeroNodeID buffer.

            this->mapNode = nullptr;
            this->nodeId = (uint32) freeNodeID;
            this->channelIndex = 0;
            this->freeAtRenderIndex = 0;
        }

        void assignToNodeOutput(const MapNode * theMapNode, int theOutputChannelIndex)
        {
            jassert(this->nodeId != (uint32) zeroNodeID);  //cannot reassign the zeroNodeID buffer.
            jassert(theMapNode != nullptr);
            jassert(theOutputChannelIndex >= 0);

            this->mapNode = theMapNode;
            this->nodeId = theMapNode->getNodeId();
            this->channelIndex = theOutputChannelIndex;

            int channelFreeAtIndex = 0;

            for (int i = 0; i < mapNode->getDestConnections().size(); ++i)
            {
                const MapNodeConnection* ec = mapNode->getDestConnections().getUnchecked (i);

                if (ec->sourceChannelIndex == theOutputChannelIndex)
                    channelFreeAtIndex = jmax (channelFreeAtIndex, ec->destMapNode->getRenderIndex());
            }

            this->freeAtRenderIndex = channelFreeAtIndex;
        }

        const MapNode* mapNode;
        uint32 nodeId;
        const int sharedBufferChannelIndex;
        int channelIndex, freeAtRenderIndex;

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelBufferInfo)
    };

    //==============================================================================
    AudioProcessorGraph& graph;
    OwnedArray<ChannelBufferInfo> audioChannelBuffers, midiChannelBuffers;
    int totalLatency;

    enum { freeNodeID = 0xffffffff, zeroNodeID = 0xfffffffe };

    static bool isNodeBusy (uint32 nodeID) noexcept { return nodeID != freeNodeID && nodeID != zeroNodeID; }

    //==============================================================================
    void createRenderingOpsForNode (const MapNode* mapNode, Array<void*>& renderingOps)
    {
        AudioProcessor* processor = mapNode->getNode()->getProcessor();
        const int numIns = processor->getNumInputChannels();
        const int numOuts = processor->getNumOutputChannels();

        //The ProcessBufferOp requires an arrangement of indices refering to channels in the shared audio buffer.  It may look something like this:  1, 2, 1, 1, 13, 14, 0, 0.
        //The channels indices represent either channels from input connections, free channels or the read only channel (0) which is always silent.
        Array<int> processBufferOpSharedAudioChannelsToUse;

        const int maxInputLatency = mapNode->getMaxInputLatency();

        for (int inputChanIndex = 0; inputChanIndex < numIns; ++inputChanIndex)
        {
            ChannelBufferInfo* audioChannelBufferToUse = nullptr;

            const Array<GraphRenderingOps::MapNodeConnection*> srcConnectionsToChannel (mapNode->getSourcesToChannel (inputChanIndex));

            if (srcConnectionsToChannel.size() == 0)
            {
                // unconnected input channel

                if (inputChanIndex >= numOuts)
                {
                    audioChannelBufferToUse = getReadOnlyEmptyBuffer();
                }
                else
                {
                    audioChannelBufferToUse = getFreeBuffer (false);
                    renderingOps.add (new ClearChannelOp (audioChannelBufferToUse->sharedBufferChannelIndex));
                }
            }
            else if (srcConnectionsToChannel.size() == 1)
            {
                // channel with a straightforward single input..
                MapNodeConnection* sourceConnection = srcConnectionsToChannel.getUnchecked (0);

                //get the buffer index for the src node's channel
                audioChannelBufferToUse = getBufferContaining (sourceConnection->sourceMapNode->getNodeId(), sourceConnection->sourceChannelIndex);

                if (audioChannelBufferToUse == nullptr)
                {
                    // if not found, this is probably a feedback loop
                    audioChannelBufferToUse = getReadOnlyEmptyBuffer();
                }

                if (inputChanIndex < numOuts
                    && isBufferNeededLater (audioChannelBufferToUse, mapNode, inputChanIndex))
                {
                    // can't mess up this channel because it's needed later by another node, so we
                    // need to use a copy of it..
                    ChannelBufferInfo* newFreeBuffer = getFreeBuffer (false);

                    renderingOps.add (new CopyChannelOp (audioChannelBufferToUse->sharedBufferChannelIndex, newFreeBuffer->sharedBufferChannelIndex));

                    audioChannelBufferToUse = newFreeBuffer;
                }

                const int connectionInputLatency = sourceConnection->sourceMapNode->getMaxLatency();

                if (connectionInputLatency < maxInputLatency)
                    renderingOps.add (new DelayChannelOp (audioChannelBufferToUse->sharedBufferChannelIndex, maxInputLatency - connectionInputLatency));
            }
            else
            {
                // channel with a mix of several inputs..

                // try to find a re-usable channel from our inputs..
                int reusableInputIndex = -1;

                for (int i = 0; i < srcConnectionsToChannel.size(); ++i)
                {
                    MapNodeConnection* mapNodeConnection = srcConnectionsToChannel.getUnchecked (i);

                    const uint32 sourceNodeId = mapNodeConnection->sourceMapNode->getNodeId();
                    const int sourceChannelIndex = mapNodeConnection->sourceChannelIndex;

                    ChannelBufferInfo* sourceBuffer = getBufferContaining (sourceNodeId, sourceChannelIndex);

                    if (sourceBuffer != nullptr
                        && ! isBufferNeededLater (sourceBuffer, mapNode, inputChanIndex))
                    {
                        // we've found one of our input chans that can be re-used..
                        reusableInputIndex = i;
                        audioChannelBufferToUse = sourceBuffer;

                        const int connectionInputLatency = mapNodeConnection->sourceMapNode->getMaxLatency();

                        if (connectionInputLatency < maxInputLatency)
                            renderingOps.add (new DelayChannelOp (sourceBuffer->sharedBufferChannelIndex, maxInputLatency - connectionInputLatency));

                        break;
                    }
                }

                if (reusableInputIndex < 0)
                {
                    // can't re-use any of our input chans, so get a new one and copy everything into it..
                    audioChannelBufferToUse = getFreeBuffer (false);

                    MapNodeConnection* sourceConnection = srcConnectionsToChannel.getUnchecked (0);

                    const uint32 firstSourceNodeId = sourceConnection->sourceMapNode->getNodeId();

                    const ChannelBufferInfo* sourceBuffer = getBufferContaining (firstSourceNodeId, sourceConnection->sourceChannelIndex);

                    if (sourceBuffer == nullptr)
                    {
                        // if not found, this is probably a feedback loop
                        renderingOps.add (new ClearChannelOp (audioChannelBufferToUse->sharedBufferChannelIndex));
                    }
                    else
                    {
                        renderingOps.add (new CopyChannelOp (sourceBuffer->sharedBufferChannelIndex, audioChannelBufferToUse->sharedBufferChannelIndex));
                    }

                    reusableInputIndex = 0;
                    const int connectionInputLatency = sourceConnection->sourceMapNode->getMaxLatency();

                    if (connectionInputLatency < maxInputLatency)
                        renderingOps.add (new DelayChannelOp (audioChannelBufferToUse->sharedBufferChannelIndex, maxInputLatency - connectionInputLatency));
                }

                for (int j = 0; j < srcConnectionsToChannel.size(); ++j)
                {
                    GraphRenderingOps::MapNodeConnection* mapNodeConnection = srcConnectionsToChannel.getUnchecked (j);

                    const uint32 sourceNodeId = mapNodeConnection->sourceMapNode->getNodeId();
                    const int sourceChannelIndex = mapNodeConnection->sourceChannelIndex;

                    if (j != reusableInputIndex)
                    {
                        const ChannelBufferInfo* sourceBuffer = getBufferContaining (sourceNodeId,
                                                                                     sourceChannelIndex);
                        if (sourceBuffer != nullptr)
                        {
                            const int connectionInputLatency = mapNodeConnection->sourceMapNode->getMaxLatency();

                            if (connectionInputLatency < maxInputLatency)
                            {
                                if (! isBufferNeededLater (sourceBuffer, mapNode, inputChanIndex))
                                {
                                    renderingOps.add (new DelayChannelOp (sourceBuffer->sharedBufferChannelIndex, maxInputLatency - connectionInputLatency));
                                }
                                else // buffer is reused elsewhere, can't be delayed
                                {
                                    const ChannelBufferInfo* bufferToDelay = getFreeBuffer (false);
                                    renderingOps.add (new CopyChannelOp (sourceBuffer->sharedBufferChannelIndex, bufferToDelay->sharedBufferChannelIndex));
                                    renderingOps.add (new DelayChannelOp (bufferToDelay->sharedBufferChannelIndex, maxInputLatency - connectionInputLatency));
                                    sourceBuffer = bufferToDelay;
                                }
                            }

                            renderingOps.add (new AddChannelOp (sourceBuffer->sharedBufferChannelIndex, audioChannelBufferToUse->sharedBufferChannelIndex));
                        }
                    }
                }
            }

            jassert (audioChannelBufferToUse != nullptr);

            processBufferOpSharedAudioChannelsToUse.add (audioChannelBufferToUse->sharedBufferChannelIndex);

            if (inputChanIndex < numOuts)
            {
                //inputChanIndex here represents the corresponding output channel index!
                audioChannelBufferToUse->assignToNodeOutput(mapNode, inputChanIndex);
            }
        }

        //assign free buffers to any outputs beyond the number of inputs.
        for (int outputChan = numIns; outputChan < numOuts; ++outputChan)
        {
            ChannelBufferInfo* freeBuffer = getFreeBuffer (false);
            processBufferOpSharedAudioChannelsToUse.add (freeBuffer->sharedBufferChannelIndex);
            freeBuffer->assignToNodeOutput(mapNode, outputChan);
        }

        // Now the same thing for midi..

        ChannelBufferInfo* midiBufferToUse = nullptr;

        const Array<GraphRenderingOps::MapNodeConnection*> midiSourceConnections = mapNode->getSourcesToChannel(AudioProcessorGraph::midiChannelIndex);

        if (midiSourceConnections.size() == 0)
        {
            // No midi inputs..
            midiBufferToUse = getFreeBuffer (true); // need to pick a buffer even if the processor doesn't use midi

            if (mapNode->getNode()->getProcessor()->acceptsMidi() || mapNode->getNode()->getProcessor()->producesMidi())
                renderingOps.add (new ClearMidiBufferOp (midiBufferToUse->sharedBufferChannelIndex));
        }
        else if (midiSourceConnections.size() == 1)
        {
            const GraphRenderingOps::MapNodeConnection* mapNodeConnection = midiSourceConnections.getUnchecked (0);

            // One midi input..
            midiBufferToUse = getBufferContaining (mapNodeConnection->sourceMapNode->getNodeId(),
                                                   AudioProcessorGraph::midiChannelIndex);

            if (midiBufferToUse != nullptr)
            {
                if (isBufferNeededLater (midiBufferToUse, mapNode, AudioProcessorGraph::midiChannelIndex))
                {
                    // can't mess up this channel because it's needed later by another node, so we
                    // need to use a copy of it..
                    ChannelBufferInfo* newFreeBuffer = getFreeBuffer (true);
                    renderingOps.add (new CopyMidiBufferOp (midiBufferToUse->sharedBufferChannelIndex, newFreeBuffer->sharedBufferChannelIndex));
                    midiBufferToUse = newFreeBuffer;
                }
            }
            else
            {
                // probably a feedback loop, so just use an empty one..
                midiBufferToUse = getFreeBuffer (true); // need to pick a buffer even if the processor doesn't use midi
            }
        }
        else
        {
            // More than one midi input being mixed..
            int reusableInputIndex = -1;

            for (int i = 0; i < midiSourceConnections.size(); ++i)
            {

                const GraphRenderingOps::MapNodeConnection* midiConnection = midiSourceConnections.getUnchecked (i);

                ChannelBufferInfo* sourceBuffer = getBufferContaining (midiConnection->sourceMapNode->getNodeId(),
                                                                       AudioProcessorGraph::midiChannelIndex);

                if (sourceBuffer != nullptr
                    && ! isBufferNeededLater (sourceBuffer, mapNode, AudioProcessorGraph::midiChannelIndex))
                {
                    // we've found one of our input buffers that can be re-used..
                    reusableInputIndex = i;
                    midiBufferToUse = sourceBuffer;
                    break;
                }
            }

            if (reusableInputIndex < 0)
            {
                // can't re-use any of our input buffers, so get a new one and copy everything into it..
                midiBufferToUse = getFreeBuffer (true);
                jassert (midiBufferToUse >= 0);

                const GraphRenderingOps::MapNodeConnection* midiConnection = midiSourceConnections.getUnchecked (0);

                const ChannelBufferInfo* sourceBuffer = getBufferContaining (midiConnection->sourceMapNode->getNodeId(),
                                                                             AudioProcessorGraph::midiChannelIndex);
                if (sourceBuffer != nullptr)
                    renderingOps.add (new CopyMidiBufferOp (sourceBuffer->sharedBufferChannelIndex, midiBufferToUse->sharedBufferChannelIndex));
                else
                    renderingOps.add (new ClearMidiBufferOp (midiBufferToUse->sharedBufferChannelIndex));

                reusableInputIndex = 0;
            }

            for (int j = 0; j < midiSourceConnections.size(); ++j)
            {

                const GraphRenderingOps::MapNodeConnection* midiConnection = midiSourceConnections.getUnchecked (j);

                if (j != reusableInputIndex)
                {
                    const ChannelBufferInfo* sourceBuffer = getBufferContaining (midiConnection->sourceMapNode->getNodeId(),
                                                                                 AudioProcessorGraph::midiChannelIndex);
                    if (sourceBuffer != nullptr)
                        renderingOps.add (new AddMidiBufferOp (sourceBuffer->sharedBufferChannelIndex, midiBufferToUse->sharedBufferChannelIndex));
                }
            }
        }

        if (mapNode->getNode()->getProcessor()->producesMidi())
            midiBufferToUse->assignToNodeOutput(mapNode, AudioProcessorGraph::midiChannelIndex);

        if (numOuts == 0)
            totalLatency = maxInputLatency;

        const int totalChans = jmax (numIns, numOuts);

        renderingOps.add (new ProcessBufferOp (mapNode->getNode(), processBufferOpSharedAudioChannelsToUse,
                                               totalChans, midiBufferToUse->sharedBufferChannelIndex));
    }

    //==============================================================================
    ChannelBufferInfo* getFreeBuffer (const bool forMidi)
    {
        if (forMidi)
        {
            for (int i = 1; i < midiChannelBuffers.size(); ++i)
                if (midiChannelBuffers.getUnchecked (i)->nodeId == freeNodeID)
                    return midiChannelBuffers.getUnchecked (i);

            ChannelBufferInfo* cbi =  midiChannelBuffers.add (new ChannelBufferInfo (midiChannelBuffers.size(), (uint32) freeNodeID));
            return cbi;
        }

        for (int i = 1; i < audioChannelBuffers.size(); ++i)
        {
            ChannelBufferInfo* info = audioChannelBuffers.getUnchecked (i);
            if (info->nodeId == freeNodeID)
                return info;
        }

        ChannelBufferInfo* cbi = audioChannelBuffers.add (new ChannelBufferInfo (audioChannelBuffers.size(), (uint32) freeNodeID));
        return cbi;
    }
    
    ChannelBufferInfo* getReadOnlyEmptyBuffer() const noexcept
    {
        return audioChannelBuffers.getUnchecked (0);
    }
    
    ChannelBufferInfo* getBufferContaining (const uint32 nodeId, const int outputChannel) const noexcept
    {
        if (outputChannel == AudioProcessorGraph::midiChannelIndex)
        {
            for (int i = midiChannelBuffers.size(); --i >= 0;)
                if (midiChannelBuffers.getUnchecked (i)->nodeId == nodeId)
                    return midiChannelBuffers.getUnchecked (i);
        }
        else
        {
            for (int i = audioChannelBuffers.size(); --i >= 0;)
            {
                ChannelBufferInfo* info = audioChannelBuffers.getUnchecked (i);
                if (info->nodeId == nodeId && info->channelIndex == outputChannel)
                    return info;
            }
        }

        return nullptr;
    }

    bool isBufferNeededLater (const ChannelBufferInfo* channelBuffer,
                              const MapNode* currentMapNode,
                              int inputChannelOfCurrentMapNodeToIgnore) const
    {
        //Needed by another node later on, easy.
        if (channelBuffer->freeAtRenderIndex > currentMapNode->getRenderIndex())
            return true;

        //Could be connected to multiple inputs on the current node, see if it is connected to a channel other than
        //the specified one to ignore
        if (isNodeBusy (channelBuffer->nodeId))
        {
            jassert (channelBuffer->mapNode != nullptr);

            const MapNode* srcMapNode = channelBuffer->mapNode;
            const int outputChanIndex = channelBuffer->channelIndex;

            for (int i = 0; i < srcMapNode->getDestConnections().size(); ++i)
            {
                const MapNodeConnection* ec = srcMapNode->getDestConnections().getUnchecked (i);

                if (ec->destMapNode->getNodeId() == currentMapNode->getNodeId())
                {
                    if (outputChanIndex == AudioProcessorGraph::midiChannelIndex)
                    {
                        if (inputChannelOfCurrentMapNodeToIgnore != AudioProcessorGraph::midiChannelIndex
                            && ec->sourceChannelIndex == AudioProcessorGraph::midiChannelIndex
                            && ec->destChannelIndex == AudioProcessorGraph::midiChannelIndex)
                            return true;
                    }
                    else
                    {
                        if (ec->sourceChannelIndex == outputChanIndex
                            && ec->destChannelIndex != inputChannelOfCurrentMapNodeToIgnore)
                            return true;
                    }
                }
            }
        }

        return false;
    }
    
    void markAnyUnusedBuffersAsFree (const MapNode* currentMapNode)
    {
        for (int i = 0; i < audioChannelBuffers.size(); ++i)
        {
            ChannelBufferInfo* info = audioChannelBuffers.getUnchecked (i);

            if (isNodeBusy (info->nodeId)
                && currentMapNode->getRenderIndex() >= info->freeAtRenderIndex)
                info->markAsFree();
        }

        for (int i = 0; i < midiChannelBuffers.size(); ++i)
        {
            ChannelBufferInfo* info = midiChannelBuffers.getUnchecked (i);

            if (isNodeBusy (info->nodeId)
                && currentMapNode->getRenderIndex() >= info->freeAtRenderIndex)
                info->markAsFree();
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RenderingOpSequenceCalculator)
};


//==============================================================================
struct ConnectionSorter
{
    static int compareElements (const AudioProcessorGraph::Connection* const first,
                                const AudioProcessorGraph::Connection* const second) noexcept
    {
        if (first->sourceNodeId < second->sourceNodeId)                return -1;
        if (first->sourceNodeId > second->sourceNodeId)                return 1;
        if (first->destNodeId < second->destNodeId)                    return -1;
        if (first->destNodeId > second->destNodeId)                    return 1;
        if (first->sourceChannelIndex < second->sourceChannelIndex)    return -1;
        if (first->sourceChannelIndex > second->sourceChannelIndex)    return 1;
        if (first->destChannelIndex < second->destChannelIndex)        return -1;
        if (first->destChannelIndex > second->destChannelIndex)        return 1;

        return 0;
    }
};

}

//==============================================================================
AudioProcessorGraph::Connection::Connection (const uint32 sourceID, const int sourceChannel,
                                             const uint32 destID, const int destChannel) noexcept
    : sourceNodeId (sourceID), sourceChannelIndex (sourceChannel),
      destNodeId (destID), destChannelIndex (destChannel)
{
}

//==============================================================================
AudioProcessorGraph::Node::Node (const uint32 nodeID, AudioProcessor* const p) noexcept
    : nodeId (nodeID), processor (p), isPrepared (false)
{
    jassert (processor != nullptr);
}

void AudioProcessorGraph::Node::prepare (const double sampleRate, const int blockSize,
                                         AudioProcessorGraph* const graph)
{
    if (! isPrepared)
    {
        isPrepared = true;
        setParentGraph (graph);

        processor->setPlayConfigDetails (processor->getNumInputChannels(),
                                         processor->getNumOutputChannels(),
                                         sampleRate, blockSize);

        processor->prepareToPlay (sampleRate, blockSize);
    }
}

void AudioProcessorGraph::Node::unprepare()
{
    if (isPrepared)
    {
        isPrepared = false;
        processor->releaseResources();
    }
}

void AudioProcessorGraph::Node::setParentGraph (AudioProcessorGraph* const graph) const
{
    if (AudioProcessorGraph::AudioGraphIOProcessor* const ioProc
            = dynamic_cast<AudioProcessorGraph::AudioGraphIOProcessor*> (processor.get()))
        ioProc->setParentGraph (graph);
}

//==============================================================================
AudioProcessorGraph::AudioProcessorGraph()
    : lastNodeId (0),
      currentAudioInputBuffer (nullptr),
      currentMidiInputBuffer (nullptr)
{
}

AudioProcessorGraph::~AudioProcessorGraph()
{
    clearRenderingSequence();
    clear();
}

const String AudioProcessorGraph::getName() const
{
    return "Audio Graph";
}

//==============================================================================
void AudioProcessorGraph::clear()
{
    nodes.clear();
    connections.clear();
    triggerAsyncUpdate();
}

AudioProcessorGraph::Node* AudioProcessorGraph::getNodeForId (const uint32 nodeId) const
{
    for (int i = nodes.size(); --i >= 0;)
        if (nodes.getUnchecked(i)->nodeId == nodeId)
            return nodes.getUnchecked(i);

    return nullptr;
}

AudioProcessorGraph::Node* AudioProcessorGraph::addNode (AudioProcessor* const newProcessor, uint32 nodeId)
{
    if (newProcessor == nullptr || newProcessor == this)
    {
        jassertfalse;
        return nullptr;
    }

    for (int i = nodes.size(); --i >= 0;)
    {
        if (nodes.getUnchecked(i)->getProcessor() == newProcessor)
        {
            jassertfalse; // Cannot add the same object to the graph twice!
            return nullptr;
        }
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

    newProcessor->setPlayHead (getPlayHead());

    Node* const n = new Node (nodeId, newProcessor);
    nodes.add (n);
    triggerAsyncUpdate();

    n->setParentGraph (this);
    return n;
}

bool AudioProcessorGraph::removeNode (const uint32 nodeId)
{
    disconnectNode (nodeId);

    for (int i = nodes.size(); --i >= 0;)
    {
        if (nodes.getUnchecked(i)->nodeId == nodeId)
        {
            nodes.getUnchecked(i)->setParentGraph (nullptr);
            nodes.remove (i);
            triggerAsyncUpdate();

            return true;
        }
    }

    return false;
}

//==============================================================================
const AudioProcessorGraph::Connection* AudioProcessorGraph::getConnectionBetween (const uint32 sourceNodeId,
                                                                                  const int sourceChannelIndex,
                                                                                  const uint32 destNodeId,
                                                                                  const int destChannelIndex) const
{
    const Connection c (sourceNodeId, sourceChannelIndex, destNodeId, destChannelIndex);
    GraphRenderingOps::ConnectionSorter sorter;
    return connections [connections.indexOfSorted (sorter, &c)];
}

bool AudioProcessorGraph::isConnected (const uint32 possibleSourceNodeId,
                                       const uint32 possibleDestNodeId) const
{
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

bool AudioProcessorGraph::canConnect (const uint32 sourceNodeId,
                                      const int sourceChannelIndex,
                                      const uint32 destNodeId,
                                      const int destChannelIndex) const
{
    if (sourceChannelIndex < 0
         || destChannelIndex < 0
         || sourceNodeId == destNodeId
         || (destChannelIndex == midiChannelIndex) != (sourceChannelIndex == midiChannelIndex))
        return false;

    const Node* const source = getNodeForId (sourceNodeId);

    if (source == nullptr
         || (sourceChannelIndex != midiChannelIndex && sourceChannelIndex >= source->processor->getNumOutputChannels())
         || (sourceChannelIndex == midiChannelIndex && ! source->processor->producesMidi()))
        return false;

    const Node* const dest = getNodeForId (destNodeId);

    if (dest == nullptr
         || (destChannelIndex != midiChannelIndex && destChannelIndex >= dest->processor->getNumInputChannels())
         || (destChannelIndex == midiChannelIndex && ! dest->processor->acceptsMidi()))
        return false;

    return getConnectionBetween (sourceNodeId, sourceChannelIndex,
                                 destNodeId, destChannelIndex) == nullptr;
}

bool AudioProcessorGraph::addConnection (const uint32 sourceNodeId,
                                         const int sourceChannelIndex,
                                         const uint32 destNodeId,
                                         const int destChannelIndex)
{
    if (! canConnect (sourceNodeId, sourceChannelIndex, destNodeId, destChannelIndex))
        return false;

    GraphRenderingOps::ConnectionSorter sorter;
    connections.addSorted (sorter, new Connection (sourceNodeId, sourceChannelIndex,
                                                   destNodeId, destChannelIndex));
    triggerAsyncUpdate();
    return true;
}

void AudioProcessorGraph::removeConnection (const int index)
{
    connections.remove (index);
    triggerAsyncUpdate();
}

bool AudioProcessorGraph::removeConnection (const uint32 sourceNodeId, const int sourceChannelIndex,
                                            const uint32 destNodeId, const int destChannelIndex)
{
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

bool AudioProcessorGraph::disconnectNode (const uint32 nodeId)
{
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

bool AudioProcessorGraph::isConnectionLegal (const Connection* const c) const
{
    jassert (c != nullptr);

    const Node* const source = getNodeForId (c->sourceNodeId);
    const Node* const dest   = getNodeForId (c->destNodeId);

    return source != nullptr
        && dest != nullptr
        && (c->sourceChannelIndex != midiChannelIndex ? isPositiveAndBelow (c->sourceChannelIndex, source->processor->getNumOutputChannels())
                                                      : source->processor->producesMidi())
        && (c->destChannelIndex   != midiChannelIndex ? isPositiveAndBelow (c->destChannelIndex, dest->processor->getNumInputChannels())
                                                      : dest->processor->acceptsMidi());
}

bool AudioProcessorGraph::removeIllegalConnections()
{
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
static void deleteRenderOpArray (Array<void*>& ops)
{
    for (int i = ops.size(); --i >= 0;)
        delete static_cast<GraphRenderingOps::AudioGraphRenderingOp*> (ops.getUnchecked(i));
}

void AudioProcessorGraph::clearRenderingSequence()
{
    Array<void*> oldOps;

    {
        const ScopedLock sl (getCallbackLock());
        renderingOps.swapWith (oldOps);
    }

    deleteRenderOpArray (oldOps);
}

bool AudioProcessorGraph::isAnInputTo (const uint32 possibleInputId,
                                       const uint32 possibleDestinationId,
                                       const int recursionCheck) const
{
    if (recursionCheck > 0)
    {
        for (int i = connections.size(); --i >= 0;)
        {
            const AudioProcessorGraph::Connection* const c = connections.getUnchecked (i);

            if (c->destNodeId == possibleDestinationId
                 && (c->sourceNodeId == possibleInputId
                      || isAnInputTo (possibleInputId, c->sourceNodeId, recursionCheck - 1)))
                return true;
        }
    }

    return false;
}

void AudioProcessorGraph::buildRenderingSequence()
{
    Array<void*> newRenderingOps;
    int numRenderingBuffersNeeded = 2;
    int numMidiBuffersNeeded = 1;

    {
        MessageManagerLock mml;

        {
            for (int i = 0; i < nodes.size(); ++i)
            {
                Node* const node = nodes.getUnchecked(i);

                node->prepare (getSampleRate(), getBlockSize(), this);
            }
        }

        GraphRenderingOps::RenderingOpSequenceCalculator calculator (*this, nodes, connections, newRenderingOps);

        numRenderingBuffersNeeded = calculator.getNumBuffersNeeded();
        numMidiBuffersNeeded = calculator.getNumMidiBuffersNeeded();
    }

    {
        // swap over to the new rendering sequence..
        const ScopedLock sl (getCallbackLock());

        renderingBuffers.setSize (numRenderingBuffersNeeded, getBlockSize());
        renderingBuffers.clear();

        for (int i = midiBuffers.size(); --i >= 0;)
            midiBuffers.getUnchecked(i)->clear();

        while (midiBuffers.size() < numMidiBuffersNeeded)
            midiBuffers.add (new MidiBuffer());

        renderingOps.swapWith (newRenderingOps);
    }

    // delete the old ones..
    deleteRenderOpArray (newRenderingOps);
}

void AudioProcessorGraph::handleAsyncUpdate()
{
    buildRenderingSequence();
}

//==============================================================================
void AudioProcessorGraph::prepareToPlay (double /*sampleRate*/, int estimatedSamplesPerBlock)
{
    currentAudioInputBuffer = nullptr;
    currentAudioOutputBuffer.setSize (jmax (1, getNumOutputChannels()), estimatedSamplesPerBlock);
    currentMidiInputBuffer = nullptr;
    currentMidiOutputBuffer.clear();

    clearRenderingSequence();
    buildRenderingSequence();
}

void AudioProcessorGraph::releaseResources()
{
    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked(i)->unprepare();

    renderingBuffers.setSize (1, 1);
    midiBuffers.clear();

    currentAudioInputBuffer = nullptr;
    currentAudioOutputBuffer.setSize (1, 1);
    currentMidiInputBuffer = nullptr;
    currentMidiOutputBuffer.clear();
}

void AudioProcessorGraph::reset()
{
    const ScopedLock sl (getCallbackLock());

    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked(i)->getProcessor()->reset();
}

void AudioProcessorGraph::setNonRealtime (bool isProcessingNonRealtime) noexcept
{
    const ScopedLock sl (getCallbackLock());

    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked(i)->getProcessor()->setNonRealtime (isProcessingNonRealtime);
}

void AudioProcessorGraph::setPlayHead (AudioPlayHead* audioPlayHead)
{
    const ScopedLock sl (getCallbackLock());

    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked(i)->getProcessor()->setPlayHead (audioPlayHead);
}

void AudioProcessorGraph::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const int numSamples = buffer.getNumSamples();

    currentAudioInputBuffer = &buffer;
    currentAudioOutputBuffer.setSize (jmax (1, buffer.getNumChannels()), numSamples);
    currentAudioOutputBuffer.clear();
    currentMidiInputBuffer = &midiMessages;
    currentMidiOutputBuffer.clear();

    for (int i = 0; i < renderingOps.size(); ++i)
    {
        GraphRenderingOps::AudioGraphRenderingOp* const op
            = (GraphRenderingOps::AudioGraphRenderingOp*) renderingOps.getUnchecked(i);

        op->perform (renderingBuffers, midiBuffers, numSamples);
    }

    for (int i = 0; i < buffer.getNumChannels(); ++i)
        buffer.copyFrom (i, 0, currentAudioOutputBuffer, i, 0, numSamples);

    midiMessages.clear();
    midiMessages.addEvents (currentMidiOutputBuffer, 0, buffer.getNumSamples(), 0);
}

const String AudioProcessorGraph::getInputChannelName (int channelIndex) const
{
    return "Input " + String (channelIndex + 1);
}

const String AudioProcessorGraph::getOutputChannelName (int channelIndex) const
{
    return "Output " + String (channelIndex + 1);
}

bool AudioProcessorGraph::isInputChannelStereoPair (int) const      { return true; }
bool AudioProcessorGraph::isOutputChannelStereoPair (int) const     { return true; }
bool AudioProcessorGraph::silenceInProducesSilenceOut() const       { return false; }
double AudioProcessorGraph::getTailLengthSeconds() const            { return 0; }
bool AudioProcessorGraph::acceptsMidi() const                       { return true; }
bool AudioProcessorGraph::producesMidi() const                      { return true; }
void AudioProcessorGraph::getStateInformation (juce::MemoryBlock&)  {}
void AudioProcessorGraph::setStateInformation (const void*, int)    {}


//==============================================================================
AudioProcessorGraph::AudioGraphIOProcessor::AudioGraphIOProcessor (const IODeviceType deviceType)
    : type (deviceType), graph (nullptr)
{
}

AudioProcessorGraph::AudioGraphIOProcessor::~AudioGraphIOProcessor()
{
}

const String AudioProcessorGraph::AudioGraphIOProcessor::getName() const
{
    switch (type)
    {
        case audioOutputNode:   return "Audio Output";
        case audioInputNode:    return "Audio Input";
        case midiOutputNode:    return "Midi Output";
        case midiInputNode:     return "Midi Input";
        default:                break;
    }

    return String();
}

void AudioProcessorGraph::AudioGraphIOProcessor::fillInPluginDescription (PluginDescription& d) const
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

void AudioProcessorGraph::AudioGraphIOProcessor::prepareToPlay (double, int)
{
    jassert (graph != nullptr);
}

void AudioProcessorGraph::AudioGraphIOProcessor::releaseResources()
{
}

void AudioProcessorGraph::AudioGraphIOProcessor::processBlock (AudioSampleBuffer& buffer,
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

bool AudioProcessorGraph::AudioGraphIOProcessor::silenceInProducesSilenceOut() const
{
    return isOutput();
}

double AudioProcessorGraph::AudioGraphIOProcessor::getTailLengthSeconds() const
{
    return 0;
}

bool AudioProcessorGraph::AudioGraphIOProcessor::acceptsMidi() const
{
    return type == midiOutputNode;
}

bool AudioProcessorGraph::AudioGraphIOProcessor::producesMidi() const
{
    return type == midiInputNode;
}

const String AudioProcessorGraph::AudioGraphIOProcessor::getInputChannelName (int channelIndex) const
{
    switch (type)
    {
        case audioOutputNode:   return "Output " + String (channelIndex + 1);
        case midiOutputNode:    return "Midi Output";
        default:                break;
    }

    return String();
}

const String AudioProcessorGraph::AudioGraphIOProcessor::getOutputChannelName (int channelIndex) const
{
    switch (type)
    {
        case audioInputNode:    return "Input " + String (channelIndex + 1);
        case midiInputNode:     return "Midi Input";
        default:                break;
    }

    return String();
}

bool AudioProcessorGraph::AudioGraphIOProcessor::isInputChannelStereoPair (int /*index*/) const
{
    return type == audioInputNode || type == audioOutputNode;
}

bool AudioProcessorGraph::AudioGraphIOProcessor::isOutputChannelStereoPair (int index) const
{
    return isInputChannelStereoPair (index);
}

bool AudioProcessorGraph::AudioGraphIOProcessor::isInput() const noexcept           { return type == audioInputNode  || type == midiInputNode; }
bool AudioProcessorGraph::AudioGraphIOProcessor::isOutput() const noexcept          { return type == audioOutputNode || type == midiOutputNode; }

#if ! JUCE_AUDIO_PROCESSOR_NO_GUI
bool AudioProcessorGraph::AudioGraphIOProcessor::hasEditor() const                  { return false; }
AudioProcessorEditor* AudioProcessorGraph::AudioGraphIOProcessor::createEditor()    { return nullptr; }
#endif

int AudioProcessorGraph::AudioGraphIOProcessor::getNumPrograms()                    { return 0; }
int AudioProcessorGraph::AudioGraphIOProcessor::getCurrentProgram()                 { return 0; }
void AudioProcessorGraph::AudioGraphIOProcessor::setCurrentProgram (int)            { }

const String AudioProcessorGraph::AudioGraphIOProcessor::getProgramName (int)       { return String(); }
void AudioProcessorGraph::AudioGraphIOProcessor::changeProgramName (int, const String&) {}

void AudioProcessorGraph::AudioGraphIOProcessor::getStateInformation (juce::MemoryBlock&) {}
void AudioProcessorGraph::AudioGraphIOProcessor::setStateInformation (const void*, int) {}

void AudioProcessorGraph::AudioGraphIOProcessor::setParentGraph (AudioProcessorGraph* const newGraph)
{
    graph = newGraph;

    if (graph != nullptr)
    {
        setPlayConfigDetails (type == audioOutputNode ? graph->getNumOutputChannels() : 0,
                              type == audioInputNode  ? graph->getNumInputChannels()  : 0,
                              getSampleRate(),
                              getBlockSize());

        updateHostDisplay();
    }
}
