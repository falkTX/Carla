/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2015 ROLI Ltd.
   Copyright (C) 2017-2020 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the GNU
   General Public License as published by the Free Software Foundation;
   either version 2 of the License, or any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

   For a full copy of the GNU General Public License see the doc/GPL.txt file.

  ==============================================================================
*/

#include "AudioProcessorGraph.h"
#include "../containers/SortedSet.h"

namespace water {

//==============================================================================
namespace GraphRenderingOps
{

struct AudioGraphRenderingOpBase
{
    AudioGraphRenderingOpBase() noexcept {}
    virtual ~AudioGraphRenderingOpBase() {}

    virtual void perform (AudioSampleBuffer& sharedAudioBufferChans,
                          AudioSampleBuffer& sharedCVBufferChans,
                          const OwnedArray<MidiBuffer>& sharedMidiBuffers,
                          const int numSamples) = 0;
};

// use CRTP
template <class Child>
struct AudioGraphRenderingOp  : public AudioGraphRenderingOpBase
{
    void perform (AudioSampleBuffer& sharedAudioBufferChans,
                  AudioSampleBuffer& sharedCVBufferChans,
                  const OwnedArray<MidiBuffer>& sharedMidiBuffers,
                  const int numSamples) override
    {
        static_cast<Child*> (this)->perform (sharedAudioBufferChans,
                                             sharedCVBufferChans,
                                             sharedMidiBuffers,
                                             numSamples);
    }
};

//==============================================================================
struct ClearChannelOp  : public AudioGraphRenderingOp<ClearChannelOp>
{
    ClearChannelOp (const int channel, const bool cv) noexcept
        : channelNum (channel), isCV (cv) {}

    void perform (AudioSampleBuffer& sharedAudioBufferChans,
                  AudioSampleBuffer& sharedCVBufferChans,
                  const OwnedArray<MidiBuffer>&,
                  const int numSamples)
    {
        if (isCV)
            sharedCVBufferChans.clear (channelNum, 0, numSamples);
        else
            sharedAudioBufferChans.clear (channelNum, 0, numSamples);
    }

    const int channelNum;
    const bool isCV;

    CARLA_DECLARE_NON_COPY_CLASS (ClearChannelOp)
};

//==============================================================================
struct CopyChannelOp  : public AudioGraphRenderingOp<CopyChannelOp>
{
    CopyChannelOp (const int srcChan, const int dstChan, const bool cv) noexcept
        : srcChannelNum (srcChan), dstChannelNum (dstChan), isCV (cv) {}

    void perform (AudioSampleBuffer& sharedAudioBufferChans,
                  AudioSampleBuffer& sharedCVBufferChans,
                  const OwnedArray<MidiBuffer>&,
                  const int numSamples)
    {
        if (isCV)
            sharedCVBufferChans.copyFrom (dstChannelNum, 0, sharedCVBufferChans, srcChannelNum, 0, numSamples);
        else
            sharedAudioBufferChans.copyFrom (dstChannelNum, 0, sharedAudioBufferChans, srcChannelNum, 0, numSamples);
    }

    const int srcChannelNum, dstChannelNum;
    const bool isCV;

    CARLA_DECLARE_NON_COPY_CLASS (CopyChannelOp)
};

//==============================================================================
struct AddChannelOp  : public AudioGraphRenderingOp<AddChannelOp>
{
    AddChannelOp (const int srcChan, const int dstChan, const bool cv) noexcept
        : srcChannelNum (srcChan), dstChannelNum (dstChan), isCV (cv) {}

    void perform (AudioSampleBuffer& sharedAudioBufferChans,
                  AudioSampleBuffer& sharedCVBufferChans,
                  const OwnedArray<MidiBuffer>&,
                  const int numSamples)
    {
        if (isCV)
            sharedCVBufferChans.addFrom (dstChannelNum, 0, sharedCVBufferChans, srcChannelNum, 0, numSamples);
        else
            sharedAudioBufferChans.addFrom (dstChannelNum, 0, sharedAudioBufferChans, srcChannelNum, 0, numSamples);
    }

    const int srcChannelNum, dstChannelNum;
    const bool isCV;

    CARLA_DECLARE_NON_COPY_CLASS (AddChannelOp)
};

//==============================================================================
struct ClearMidiBufferOp  : public AudioGraphRenderingOp<ClearMidiBufferOp>
{
    ClearMidiBufferOp (const int buffer) noexcept  : bufferNum (buffer)  {}

    void perform (AudioSampleBuffer&, AudioSampleBuffer&,
                  const OwnedArray<MidiBuffer>& sharedMidiBuffers,
                  const int)
    {
        sharedMidiBuffers.getUnchecked (bufferNum)->clear();
    }

    const int bufferNum;

    CARLA_DECLARE_NON_COPY_CLASS (ClearMidiBufferOp)
};

//==============================================================================
struct CopyMidiBufferOp  : public AudioGraphRenderingOp<CopyMidiBufferOp>
{
    CopyMidiBufferOp (const int srcBuffer, const int dstBuffer) noexcept
        : srcBufferNum (srcBuffer), dstBufferNum (dstBuffer)
    {}

    void perform (AudioSampleBuffer&, AudioSampleBuffer&,
                  const OwnedArray<MidiBuffer>& sharedMidiBuffers,
                  const int)
    {
        *sharedMidiBuffers.getUnchecked (dstBufferNum) = *sharedMidiBuffers.getUnchecked (srcBufferNum);
    }

    const int srcBufferNum, dstBufferNum;

    CARLA_DECLARE_NON_COPY_CLASS (CopyMidiBufferOp)
};

//==============================================================================
struct AddMidiBufferOp  : public AudioGraphRenderingOp<AddMidiBufferOp>
{
    AddMidiBufferOp (const int srcBuffer, const int dstBuffer)
        : srcBufferNum (srcBuffer), dstBufferNum (dstBuffer)
    {}

    void perform (AudioSampleBuffer&, AudioSampleBuffer&,
                  const OwnedArray<MidiBuffer>& sharedMidiBuffers,
                  const int numSamples)
    {
        sharedMidiBuffers.getUnchecked (dstBufferNum)
            ->addEvents (*sharedMidiBuffers.getUnchecked (srcBufferNum), 0, numSamples, 0);
    }

    const int srcBufferNum, dstBufferNum;

    CARLA_DECLARE_NON_COPY_CLASS (AddMidiBufferOp)
};

//==============================================================================
struct DelayChannelOp  : public AudioGraphRenderingOp<DelayChannelOp>
{
    DelayChannelOp (const int chan, const int delaySize, const bool cv)
        : channel (chan),
          bufferSize (delaySize + 1),
          readIndex (0), writeIndex (delaySize),
          isCV (cv)
    {
        buffer.calloc ((size_t) bufferSize);
    }

    void perform (AudioSampleBuffer& sharedAudioBufferChans,
                  AudioSampleBuffer& sharedCVBufferChans,
                  const OwnedArray<MidiBuffer>&,
                  const int numSamples)
    {
        float* data = isCV
                    ? sharedCVBufferChans.getWritePointer (channel, 0)
                    : sharedAudioBufferChans.getWritePointer (channel, 0);
        HeapBlock<float>& block = buffer;

        for (int i = numSamples; --i >= 0;)
        {
            block [writeIndex] = *data;
            *data++ = block [readIndex];

            if (++readIndex  >= bufferSize) readIndex = 0;
            if (++writeIndex >= bufferSize) writeIndex = 0;
        }
    }

private:
    HeapBlock<float> buffer;
    const int channel, bufferSize;
    int readIndex, writeIndex;
    const bool isCV;

    CARLA_DECLARE_NON_COPY_CLASS (DelayChannelOp)
};

//==============================================================================
struct ProcessBufferOp   : public AudioGraphRenderingOp<ProcessBufferOp>
{
    ProcessBufferOp (const AudioProcessorGraph::Node::Ptr& n,
                     const Array<uint>& audioChannelsUsed,
                     const uint totalNumChans,
                     const Array<uint>& cvInChannelsUsed,
                     const Array<uint>& cvOutChannelsUsed,
                     const int midiBuffer)
        : node (n),
          processor (n->getProcessor()),
          audioChannelsToUse (audioChannelsUsed),
          cvInChannelsToUse (cvInChannelsUsed),
          cvOutChannelsToUse (cvOutChannelsUsed),
          totalAudioChans (jmax (1U, totalNumChans)),
          totalCVIns (cvInChannelsUsed.size()),
          totalCVOuts (cvOutChannelsUsed.size()),
          midiBufferToUse (midiBuffer)
    {
        audioChannels.calloc (totalAudioChans);
        cvInChannels.calloc (totalCVIns);
        cvOutChannels.calloc (totalCVOuts);

        while (audioChannelsToUse.size() < static_cast<int>(totalAudioChans))
            audioChannelsToUse.add (0);
    }

    void perform (AudioSampleBuffer& sharedAudioBufferChans,
                  AudioSampleBuffer& sharedCVBufferChans,
                  const OwnedArray<MidiBuffer>& sharedMidiBuffers,
                  const int numSamples)
    {
        HeapBlock<float*>& audioChannelsCopy = audioChannels;
        HeapBlock<float*>& cvInChannelsCopy  = cvInChannels;
        HeapBlock<float*>& cvOutChannelsCopy = cvOutChannels;

        for (uint i = 0; i < totalAudioChans; ++i)
            audioChannelsCopy[i] = sharedAudioBufferChans.getWritePointer (audioChannelsToUse.getUnchecked (i), 0);

        for (uint i = 0; i < totalCVIns; ++i)
            cvInChannels[i] = sharedCVBufferChans.getWritePointer (cvInChannelsToUse.getUnchecked (i), 0);

        for (uint i = 0; i < totalCVOuts; ++i)
            cvOutChannels[i] = sharedCVBufferChans.getWritePointer (cvOutChannelsToUse.getUnchecked (i), 0);

        AudioSampleBuffer audioBuffer (audioChannelsCopy, totalAudioChans, numSamples);
        AudioSampleBuffer cvInBuffer  (cvInChannelsCopy, totalCVIns, numSamples);
        AudioSampleBuffer cvOutBuffer (cvOutChannelsCopy, totalCVOuts, numSamples);

        if (processor->isSuspended())
        {
            audioBuffer.clear();
            cvOutBuffer.clear();
        }
        else
        {
            const CarlaRecursiveMutexLocker cml (processor->getCallbackLock());

            callProcess (audioBuffer, cvInBuffer, cvOutBuffer, *sharedMidiBuffers.getUnchecked (midiBufferToUse));
        }
    }

    void callProcess (AudioSampleBuffer& audioBuffer,
                      AudioSampleBuffer& cvInBuffer,
                      AudioSampleBuffer& cvOutBuffer,
                      MidiBuffer& midiMessages)
    {
        processor->processBlockWithCV (audioBuffer, cvInBuffer, cvOutBuffer, midiMessages);
    }

    const AudioProcessorGraph::Node::Ptr node;
    AudioProcessor* const processor;

private:
    Array<uint> audioChannelsToUse;
    Array<uint> cvInChannelsToUse;
    Array<uint> cvOutChannelsToUse;
    HeapBlock<float*> audioChannels;
    HeapBlock<float*> cvInChannels;
    HeapBlock<float*> cvOutChannels;
    AudioSampleBuffer tempBuffer;
    const uint totalAudioChans;
    const uint totalCVIns;
    const uint totalCVOuts;
    const int midiBufferToUse;

    CARLA_DECLARE_NON_COPY_CLASS (ProcessBufferOp)
};

//==============================================================================
/** Used to calculate the correct sequence of rendering ops needed, based on
    the best re-use of shared buffers at each stage.
*/
struct RenderingOpSequenceCalculator
{
    RenderingOpSequenceCalculator (AudioProcessorGraph& g,
                                   const Array<AudioProcessorGraph::Node*>& nodes,
                                   Array<void*>& renderingOps)
        : graph (g),
          orderedNodes (nodes),
          totalLatency (0)
    {
        audioNodeIds.add ((uint32) zeroNodeID); // first buffer is read-only zeros
        audioChannels.add (0);

        cvNodeIds.add ((uint32) zeroNodeID);
        cvChannels.add (0);

        midiNodeIds.add ((uint32) zeroNodeID);

        for (int i = 0; i < orderedNodes.size(); ++i)
        {
            createRenderingOpsForNode (*orderedNodes.getUnchecked(i), renderingOps, i);
            markAnyUnusedBuffersAsFree (i);
        }

        graph.setLatencySamples (totalLatency);
    }

    int getNumAudioBuffersNeeded() const noexcept    { return audioNodeIds.size(); }
    int getNumCVBuffersNeeded() const noexcept       { return cvNodeIds.size(); }
    int getNumMidiBuffersNeeded() const noexcept     { return midiNodeIds.size(); }

private:
    //==============================================================================
    AudioProcessorGraph& graph;
    const Array<AudioProcessorGraph::Node*>& orderedNodes;
    Array<uint> audioChannels, cvChannels;
    Array<uint32> audioNodeIds, cvNodeIds, midiNodeIds;

    enum { freeNodeID = 0xffffffff, zeroNodeID = 0xfffffffe, anonymousNodeID = 0xfffffffd };

    static bool isNodeBusy (uint32 nodeID) noexcept     { return nodeID != freeNodeID; }

    Array<uint32> nodeDelayIDs;
    Array<int> nodeDelays;
    int totalLatency;

    int getNodeDelay (const uint32 nodeID) const        { return nodeDelays [nodeDelayIDs.indexOf (nodeID)]; }

    void setNodeDelay (const uint32 nodeID, const int latency)
    {
        const int index = nodeDelayIDs.indexOf (nodeID);

        if (index >= 0)
        {
            nodeDelays.set (index, latency);
        }
        else
        {
            nodeDelayIDs.add (nodeID);
            nodeDelays.add (latency);
        }
    }

    int getInputLatencyForNode (const uint32 nodeID) const
    {
        int maxLatency = 0;

        for (int i = graph.getNumConnections(); --i >= 0;)
        {
            const AudioProcessorGraph::Connection* const c = graph.getConnection (i);

            if (c->destNodeId == nodeID)
                maxLatency = jmax (maxLatency, getNodeDelay (c->sourceNodeId));
        }

        return maxLatency;
    }

    //==============================================================================
    void createRenderingOpsForNode (AudioProcessorGraph::Node& node,
                                    Array<void*>& renderingOps,
                                    const int ourRenderingIndex)
    {
        AudioProcessor& processor = *node.getProcessor();
        const uint numAudioIns = processor.getTotalNumInputChannels(AudioProcessor::ChannelTypeAudio);
        const uint numAudioOuts = processor.getTotalNumOutputChannels(AudioProcessor::ChannelTypeAudio);
        const uint numCVIns = processor.getTotalNumInputChannels(AudioProcessor::ChannelTypeCV);
        const uint numCVOuts = processor.getTotalNumOutputChannels(AudioProcessor::ChannelTypeCV);
        const uint totalAudioChans = jmax (numAudioIns, numAudioOuts);

        Array<uint> audioChannelsToUse, cvInChannelsToUse, cvOutChannelsToUse;
        int midiBufferToUse = -1;

        int maxLatency = getInputLatencyForNode (node.nodeId);

        for (uint inputChan = 0; inputChan < numAudioIns; ++inputChan)
        {
            // get a list of all the inputs to this node
            Array<uint32> sourceNodes;
            Array<uint> sourceOutputChans;

            for (int i = graph.getNumConnections(); --i >= 0;)
            {
                const AudioProcessorGraph::Connection* const c = graph.getConnection (i);

                if (c->destNodeId == node.nodeId
                    && c->destChannelIndex == inputChan
                    && c->channelType == AudioProcessor::ChannelTypeAudio)
                {
                    sourceNodes.add (c->sourceNodeId);
                    sourceOutputChans.add (c->sourceChannelIndex);
                }
            }

            int bufIndex = -1;

            if (sourceNodes.size() == 0)
            {
                // unconnected input channel
                bufIndex = getFreeBuffer (AudioProcessor::ChannelTypeAudio);
                renderingOps.add (new ClearChannelOp (bufIndex, false));
            }
            else if (sourceNodes.size() == 1)
            {
                // channel with a straightforward single input..
                const uint32 srcNode = sourceNodes.getUnchecked(0);
                const uint srcChan = sourceOutputChans.getUnchecked(0);

                bufIndex = getBufferContaining (AudioProcessor::ChannelTypeAudio, srcNode, srcChan);

                if (bufIndex < 0)
                {
                    // if not found, this is probably a feedback loop
                    bufIndex = getReadOnlyEmptyBuffer();
                    wassert (bufIndex >= 0);
                }

                if (inputChan < numAudioOuts
                     && isBufferNeededLater (AudioProcessor::ChannelTypeAudio,
                                             ourRenderingIndex,
                                             inputChan,
                                             srcNode, srcChan))
                {
                    // can't mess up this channel because it's needed later by another node, so we
                    // need to use a copy of it..
                    const int newFreeBuffer = getFreeBuffer (AudioProcessor::ChannelTypeAudio);

                    renderingOps.add (new CopyChannelOp (bufIndex, newFreeBuffer, false));

                    bufIndex = newFreeBuffer;
                }

                const int nodeDelay = getNodeDelay (srcNode);

                if (nodeDelay < maxLatency)
                    renderingOps.add (new DelayChannelOp (bufIndex, maxLatency - nodeDelay, false));
            }
            else
            {
                // channel with a mix of several inputs..

                // try to find a re-usable channel from our inputs..
                int reusableInputIndex = -1;

                for (int i = 0; i < sourceNodes.size(); ++i)
                {
                    const int sourceBufIndex = getBufferContaining (AudioProcessor::ChannelTypeAudio,
                                                                    sourceNodes.getUnchecked(i),
                                                                    sourceOutputChans.getUnchecked(i));

                    if (sourceBufIndex >= 0
                        && ! isBufferNeededLater (AudioProcessor::ChannelTypeAudio,
                                                  ourRenderingIndex,
                                                  inputChan,
                                                  sourceNodes.getUnchecked(i),
                                                  sourceOutputChans.getUnchecked(i)))
                    {
                        // we've found one of our input chans that can be re-used..
                        reusableInputIndex = i;
                        bufIndex = sourceBufIndex;

                        const int nodeDelay = getNodeDelay (sourceNodes.getUnchecked (i));
                        if (nodeDelay < maxLatency)
                            renderingOps.add (new DelayChannelOp (sourceBufIndex, maxLatency - nodeDelay, false));

                        break;
                    }
                }

                if (reusableInputIndex < 0)
                {
                    // can't re-use any of our input chans, so get a new one and copy everything into it..
                    bufIndex = getFreeBuffer (AudioProcessor::ChannelTypeAudio);
                    wassert (bufIndex != 0);

                    markBufferAsContaining (AudioProcessor::ChannelTypeAudio,
                                            bufIndex, static_cast<uint32> (anonymousNodeID), 0);

                    const int srcIndex = getBufferContaining (AudioProcessor::ChannelTypeAudio,
                                                              sourceNodes.getUnchecked (0),
                                                              sourceOutputChans.getUnchecked (0));
                    if (srcIndex < 0)
                    {
                        // if not found, this is probably a feedback loop
                        renderingOps.add (new ClearChannelOp (bufIndex, false));
                    }
                    else
                    {
                        renderingOps.add (new CopyChannelOp (srcIndex, bufIndex, false));
                    }

                    reusableInputIndex = 0;
                    const int nodeDelay = getNodeDelay (sourceNodes.getFirst());

                    if (nodeDelay < maxLatency)
                        renderingOps.add (new DelayChannelOp (bufIndex, maxLatency - nodeDelay, false));
                }

                for (int j = 0; j < sourceNodes.size(); ++j)
                {
                    if (j != reusableInputIndex)
                    {
                        int srcIndex = getBufferContaining (AudioProcessor::ChannelTypeAudio,
                                                            sourceNodes.getUnchecked(j),
                                                            sourceOutputChans.getUnchecked(j));
                        if (srcIndex >= 0)
                        {
                            const int nodeDelay = getNodeDelay (sourceNodes.getUnchecked (j));

                            if (nodeDelay < maxLatency)
                            {
                                if (! isBufferNeededLater (AudioProcessor::ChannelTypeAudio,
                                                           ourRenderingIndex, inputChan,
                                                           sourceNodes.getUnchecked(j),
                                                           sourceOutputChans.getUnchecked(j)))
                                {
                                    renderingOps.add (new DelayChannelOp (srcIndex, maxLatency - nodeDelay, false));
                                }
                                else // buffer is reused elsewhere, can't be delayed
                                {
                                    const int bufferToDelay = getFreeBuffer (AudioProcessor::ChannelTypeAudio);
                                    renderingOps.add (new CopyChannelOp (srcIndex, bufferToDelay, false));
                                    renderingOps.add (new DelayChannelOp (bufferToDelay, maxLatency - nodeDelay, false));
                                    srcIndex = bufferToDelay;
                                }
                            }

                            renderingOps.add (new AddChannelOp (srcIndex, bufIndex, false));
                        }
                    }
                }
            }

            CARLA_SAFE_ASSERT_CONTINUE (bufIndex >= 0);
            audioChannelsToUse.add (bufIndex);

            if (inputChan < numAudioOuts)
                markBufferAsContaining (AudioProcessor::ChannelTypeAudio, bufIndex, node.nodeId, inputChan);
        }

        for (uint outputChan = numAudioIns; outputChan < numAudioOuts; ++outputChan)
        {
            const int bufIndex = getFreeBuffer (AudioProcessor::ChannelTypeAudio);
            CARLA_SAFE_ASSERT_CONTINUE (bufIndex > 0);
            audioChannelsToUse.add (bufIndex);
            markBufferAsContaining (AudioProcessor::ChannelTypeAudio, bufIndex, node.nodeId, outputChan);
        }

        for (uint inputChan = 0; inputChan < numCVIns; ++inputChan)
        {
            // get a list of all the inputs to this node
            Array<uint32> sourceNodes;
            Array<uint> sourceOutputChans;

            for (int i = graph.getNumConnections(); --i >= 0;)
            {
                const AudioProcessorGraph::Connection* const c = graph.getConnection (i);

                if (c->destNodeId == node.nodeId
                    && c->destChannelIndex == inputChan
                    && c->channelType == AudioProcessor::ChannelTypeCV)
                {
                    sourceNodes.add (c->sourceNodeId);
                    sourceOutputChans.add (c->sourceChannelIndex);
                }
            }

            int bufIndex = -1;

            if (sourceNodes.size() == 0)
            {
                // unconnected input channel
                bufIndex = getFreeBuffer (AudioProcessor::ChannelTypeCV);
                renderingOps.add (new ClearChannelOp (bufIndex, true));
            }
            else if (sourceNodes.size() == 1)
            {
                // channel with a straightforward single input..
                const uint32 srcNode = sourceNodes.getUnchecked(0);
                const uint srcChan = sourceOutputChans.getUnchecked(0);

                bufIndex = getBufferContaining (AudioProcessor::ChannelTypeCV, srcNode, srcChan);

                if (bufIndex < 0)
                {
                    // if not found, this is probably a feedback loop
                    bufIndex = getReadOnlyEmptyBuffer();
                    wassert (bufIndex >= 0);
                }

                const int newFreeBuffer = getFreeBuffer (AudioProcessor::ChannelTypeCV);

                renderingOps.add (new CopyChannelOp (bufIndex, newFreeBuffer, true));

                bufIndex = newFreeBuffer;

                const int nodeDelay = getNodeDelay (srcNode);

                if (nodeDelay < maxLatency)
                    renderingOps.add (new DelayChannelOp (bufIndex, maxLatency - nodeDelay, true));
            }
            else
            {
                // channel with a mix of several inputs..

                {
                    bufIndex = getFreeBuffer (AudioProcessor::ChannelTypeCV);
                    wassert (bufIndex != 0);

                    const int srcIndex = getBufferContaining (AudioProcessor::ChannelTypeCV,
                                                              sourceNodes.getUnchecked (0),
                                                              sourceOutputChans.getUnchecked (0));
                    if (srcIndex < 0)
                    {
                        // if not found, this is probably a feedback loop
                        renderingOps.add (new ClearChannelOp (bufIndex, true));
                    }
                    else
                    {
                        renderingOps.add (new CopyChannelOp (srcIndex, bufIndex, true));
                    }

                    const int nodeDelay = getNodeDelay (sourceNodes.getFirst());

                    if (nodeDelay < maxLatency)
                        renderingOps.add (new DelayChannelOp (bufIndex, maxLatency - nodeDelay, true));
                }

                for (int j = 1; j < sourceNodes.size(); ++j)
                {
                    int srcIndex = getBufferContaining (AudioProcessor::ChannelTypeCV,
                                                        sourceNodes.getUnchecked(j),
                                                        sourceOutputChans.getUnchecked(j));
                    if (srcIndex >= 0)
                    {
                        const int nodeDelay = getNodeDelay (sourceNodes.getUnchecked (j));

                        if (nodeDelay < maxLatency)
                        {
                            const int bufferToDelay = getFreeBuffer (AudioProcessor::ChannelTypeCV);
                            renderingOps.add (new CopyChannelOp (srcIndex, bufferToDelay, true));
                            renderingOps.add (new DelayChannelOp (bufferToDelay, maxLatency - nodeDelay, true));
                            srcIndex = bufferToDelay;
                        }

                        renderingOps.add (new AddChannelOp (srcIndex, bufIndex, true));
                    }
                }
            }

            CARLA_SAFE_ASSERT_CONTINUE (bufIndex >= 0);
            cvInChannelsToUse.add (bufIndex);
            markBufferAsContaining (AudioProcessor::ChannelTypeCV, bufIndex, node.nodeId, inputChan);
        }

        for (uint outputChan = 0; outputChan < numCVOuts; ++outputChan)
        {
            const int bufIndex = getFreeBuffer (AudioProcessor::ChannelTypeCV);
            CARLA_SAFE_ASSERT_CONTINUE (bufIndex > 0);
            cvOutChannelsToUse.add (bufIndex);
            markBufferAsContaining (AudioProcessor::ChannelTypeCV, bufIndex, node.nodeId, outputChan);
        }

        // Now the same thing for midi..
        Array<uint32> midiSourceNodes;

        for (int i = graph.getNumConnections(); --i >= 0;)
        {
            const AudioProcessorGraph::Connection* const c = graph.getConnection (i);

            if (c->destNodeId == node.nodeId && c->channelType == AudioProcessor::ChannelTypeMIDI)
                midiSourceNodes.add (c->sourceNodeId);
        }

        if (midiSourceNodes.size() == 0)
        {
            // No midi inputs..
            midiBufferToUse = getFreeBuffer (AudioProcessor::ChannelTypeMIDI); // need to pick a buffer even if the processor doesn't use midi

            if (processor.acceptsMidi() || processor.producesMidi())
                renderingOps.add (new ClearMidiBufferOp (midiBufferToUse));
        }
        else if (midiSourceNodes.size() == 1)
        {
            // One midi input..
            midiBufferToUse = getBufferContaining (AudioProcessor::ChannelTypeMIDI,
                                                   midiSourceNodes.getUnchecked(0),
                                                   0);
            if (midiBufferToUse >= 0)
            {
                if (isBufferNeededLater (AudioProcessor::ChannelTypeMIDI,
                                         ourRenderingIndex, 0,
                                         midiSourceNodes.getUnchecked(0), 0))
                {
                    // can't mess up this channel because it's needed later by another node, so we
                    // need to use a copy of it..
                    const int newFreeBuffer = getFreeBuffer (AudioProcessor::ChannelTypeMIDI);
                    renderingOps.add (new CopyMidiBufferOp (midiBufferToUse, newFreeBuffer));
                    midiBufferToUse = newFreeBuffer;
                }
            }
            else
            {
                // probably a feedback loop, so just use an empty one..
                midiBufferToUse = getFreeBuffer (AudioProcessor::ChannelTypeMIDI); // need to pick a buffer even if the processor doesn't use midi
            }
        }
        else
        {
            // More than one midi input being mixed..
            int reusableInputIndex = -1;

            for (int i = 0; i < midiSourceNodes.size(); ++i)
            {
                const int sourceBufIndex = getBufferContaining (AudioProcessor::ChannelTypeMIDI,
                                                                midiSourceNodes.getUnchecked(i),
                                                                0);

                if (sourceBufIndex >= 0
                     && ! isBufferNeededLater (AudioProcessor::ChannelTypeMIDI,
                                               ourRenderingIndex, 0,
                                               midiSourceNodes.getUnchecked(i), 0))
                {
                    // we've found one of our input buffers that can be re-used..
                    reusableInputIndex = i;
                    midiBufferToUse = sourceBufIndex;
                    break;
                }
            }

            if (reusableInputIndex < 0)
            {
                // can't re-use any of our input buffers, so get a new one and copy everything into it..
                midiBufferToUse = getFreeBuffer (AudioProcessor::ChannelTypeMIDI);
                wassert (midiBufferToUse >= 0);

                const int srcIndex = getBufferContaining (AudioProcessor::ChannelTypeMIDI,
                                                          midiSourceNodes.getUnchecked(0),
                                                          0);
                if (srcIndex >= 0)
                    renderingOps.add (new CopyMidiBufferOp (srcIndex, midiBufferToUse));
                else
                    renderingOps.add (new ClearMidiBufferOp (midiBufferToUse));

                reusableInputIndex = 0;
            }

            for (int j = 0; j < midiSourceNodes.size(); ++j)
            {
                if (j != reusableInputIndex)
                {
                    const int srcIndex = getBufferContaining (AudioProcessor::ChannelTypeMIDI,
                                                              midiSourceNodes.getUnchecked(j),
                                                              0);
                    if (srcIndex >= 0)
                        renderingOps.add (new AddMidiBufferOp (srcIndex, midiBufferToUse));
                }
            }
        }

        if (processor.producesMidi())
            markBufferAsContaining (AudioProcessor::ChannelTypeMIDI,
                                    midiBufferToUse, node.nodeId,
                                    0);

        setNodeDelay (node.nodeId, maxLatency + processor.getLatencySamples());

        if (numAudioOuts == 0)
            totalLatency = maxLatency;

        renderingOps.add (new ProcessBufferOp (&node,
                                               audioChannelsToUse,
                                               totalAudioChans,
                                               cvInChannelsToUse,
                                               cvOutChannelsToUse,
                                               midiBufferToUse));
    }

    //==============================================================================
    int getFreeBuffer (const AudioProcessor::ChannelType channelType)
    {
        switch (channelType)
        {
        case AudioProcessor::ChannelTypeAudio:
            for (int i = 1; i < audioNodeIds.size(); ++i)
                if (audioNodeIds.getUnchecked(i) == freeNodeID)
                    return i;

            audioNodeIds.add ((uint32) freeNodeID);
            audioChannels.add (0);
            return audioNodeIds.size() - 1;

        case AudioProcessor::ChannelTypeCV:
            for (int i = 1; i < cvNodeIds.size(); ++i)
                if (cvNodeIds.getUnchecked(i) == freeNodeID)
                    return i;

            cvNodeIds.add ((uint32) freeNodeID);
            cvChannels.add (0);
            return cvNodeIds.size() - 1;

        case AudioProcessor::ChannelTypeMIDI:
            for (int i = 1; i < midiNodeIds.size(); ++i)
                if (midiNodeIds.getUnchecked(i) == freeNodeID)
                    return i;

            midiNodeIds.add ((uint32) freeNodeID);
            return midiNodeIds.size() - 1;
        }

        return -1;
    }

    int getReadOnlyEmptyBuffer() const noexcept
    {
        return 0;
    }

    int getBufferContaining (const AudioProcessor::ChannelType channelType,
                             const uint32 nodeId,
                             const uint outputChannel) const noexcept
    {
        switch (channelType)
        {
        case AudioProcessor::ChannelTypeAudio:
            for (int i = audioNodeIds.size(); --i >= 0;)
                if (audioNodeIds.getUnchecked(i) == nodeId && audioChannels.getUnchecked(i) == outputChannel)
                    return i;
            break;

        case AudioProcessor::ChannelTypeCV:
            for (int i = cvNodeIds.size(); --i >= 0;)
                if (cvNodeIds.getUnchecked(i) == nodeId && cvChannels.getUnchecked(i) == outputChannel)
                    return i;
            break;

        case AudioProcessor::ChannelTypeMIDI:
            for (int i = midiNodeIds.size(); --i >= 0;)
            {
                if (midiNodeIds.getUnchecked(i) == nodeId)
                    return i;
            }
            break;
        }

        return -1;
    }

    void markAnyUnusedBuffersAsFree (const int stepIndex)
    {
        for (int i = 0; i < audioNodeIds.size(); ++i)
        {
            if (isNodeBusy (audioNodeIds.getUnchecked(i))
                 && ! isBufferNeededLater (AudioProcessor::ChannelTypeAudio,
                                           stepIndex, -1,
                                           audioNodeIds.getUnchecked(i),
                                           audioChannels.getUnchecked(i)))
            {
                audioNodeIds.set (i, (uint32) freeNodeID);
            }
        }

        // NOTE: CV skipped on purpose

        for (int i = 0; i < midiNodeIds.size(); ++i)
        {
            if (isNodeBusy (midiNodeIds.getUnchecked(i))
                 && ! isBufferNeededLater (AudioProcessor::ChannelTypeMIDI,
                                           stepIndex, -1,
                                           midiNodeIds.getUnchecked(i), 0))
            {
                midiNodeIds.set (i, (uint32) freeNodeID);
            }
        }
    }

    bool isBufferNeededLater (const AudioProcessor::ChannelType channelType,
                              int stepIndexToSearchFrom,
                              uint inputChannelOfIndexToIgnore,
                              const uint32 nodeId,
                              const uint outputChanIndex) const
    {
        while (stepIndexToSearchFrom < orderedNodes.size())
        {
            const AudioProcessorGraph::Node* const node = (const AudioProcessorGraph::Node*) orderedNodes.getUnchecked (stepIndexToSearchFrom);

            for (uint i = 0; i < node->getProcessor()->getTotalNumInputChannels(channelType); ++i)
                if (i != inputChannelOfIndexToIgnore
                        && graph.getConnectionBetween (channelType,
                                                       nodeId, outputChanIndex,
                                                       node->nodeId, i) != nullptr)
                    return true;

            inputChannelOfIndexToIgnore = (uint)-1;
            ++stepIndexToSearchFrom;
        }

        return false;
    }

    void markBufferAsContaining (const AudioProcessor::ChannelType channelType,
                                 int bufferNum, uint32 nodeId, int outputIndex)
    {
        switch (channelType)
        {
        case AudioProcessor::ChannelTypeAudio:
            CARLA_SAFE_ASSERT_BREAK (bufferNum >= 0 && bufferNum < audioNodeIds.size());
            audioNodeIds.set (bufferNum, nodeId);
            audioChannels.set (bufferNum, outputIndex);
            break;

        case AudioProcessor::ChannelTypeCV:
            CARLA_SAFE_ASSERT_BREAK (bufferNum >= 0 && bufferNum < cvNodeIds.size());
            cvNodeIds.set (bufferNum, nodeId);
            cvChannels.set (bufferNum, outputIndex);
            break;

        case AudioProcessor::ChannelTypeMIDI:
            CARLA_SAFE_ASSERT_BREAK (bufferNum > 0 && bufferNum < midiNodeIds.size());
            midiNodeIds.set (bufferNum, nodeId);
            break;
        }
    }

    CARLA_DECLARE_NON_COPY_CLASS (RenderingOpSequenceCalculator)
};

//==============================================================================
// Holds a fast lookup table for checking which nodes are inputs to others.
class ConnectionLookupTable
{
public:
    explicit ConnectionLookupTable (const OwnedArray<AudioProcessorGraph::Connection>& connections)
    {
        for (int i = 0; i < static_cast<int>(connections.size()); ++i)
        {
            const AudioProcessorGraph::Connection* const c = connections.getUnchecked(i);

            int index;
            Entry* entry = findEntry (c->destNodeId, index);

            if (entry == nullptr)
            {
                entry = new Entry (c->destNodeId);
                entries.insert (index, entry);
            }

            entry->srcNodes.add (c->sourceNodeId);
        }
    }

    bool isAnInputTo (const uint32 possibleInputId,
                      const uint32 possibleDestinationId) const noexcept
    {
        return isAnInputToRecursive (possibleInputId, possibleDestinationId, entries.size());
    }

private:
    //==============================================================================
    struct Entry
    {
        explicit Entry (const uint32 destNodeId_) noexcept : destNodeId (destNodeId_) {}

        const uint32 destNodeId;
        SortedSet<uint32> srcNodes;

        CARLA_DECLARE_NON_COPY_CLASS (Entry)
    };

    OwnedArray<Entry> entries;

    bool isAnInputToRecursive (const uint32 possibleInputId,
                               const uint32 possibleDestinationId,
                               int recursionCheck) const noexcept
    {
        int index;

        if (const Entry* const entry = findEntry (possibleDestinationId, index))
        {
            const SortedSet<uint32>& srcNodes = entry->srcNodes;

            if (srcNodes.contains (possibleInputId))
                return true;

            if (--recursionCheck >= 0)
            {
                for (int i = 0; i < srcNodes.size(); ++i)
                    if (isAnInputToRecursive (possibleInputId, srcNodes.getUnchecked(i), recursionCheck))
                        return true;
            }
        }

        return false;
    }

    Entry* findEntry (const uint32 destNodeId, int& insertIndex) const noexcept
    {
        Entry* result = nullptr;

        int start = 0;
        int end = entries.size();

        for (;;)
        {
            if (start >= end)
            {
                break;
            }
            else if (destNodeId == entries.getUnchecked (start)->destNodeId)
            {
                result = entries.getUnchecked (start);
                break;
            }
            else
            {
                const int halfway = (start + end) / 2;

                if (halfway == start)
                {
                    if (destNodeId >= entries.getUnchecked (halfway)->destNodeId)
                        ++start;

                    break;
                }
                else if (destNodeId >= entries.getUnchecked (halfway)->destNodeId)
                    start = halfway;
                else
                    end = halfway;
            }
        }

        insertIndex = start;
        return result;
    }

    CARLA_DECLARE_NON_COPY_CLASS (ConnectionLookupTable)
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
AudioProcessorGraph::Connection::Connection (ChannelType ct,
                                             const uint32 sourceID, const uint sourceChannel,
                                             const uint32 destID, const uint destChannel) noexcept
    : channelType (ct),
      sourceNodeId (sourceID), sourceChannelIndex (sourceChannel),
      destNodeId (destID), destChannelIndex (destChannel)
{
}

//==============================================================================
AudioProcessorGraph::Node::Node (const uint32 nodeID, AudioProcessor* const p) noexcept
    : nodeId (nodeID), processor (p), isPrepared (false)
{
    wassert (processor != nullptr);
}

void AudioProcessorGraph::Node::prepare (const double newSampleRate, const int newBlockSize,
                                         AudioProcessorGraph* const graph)
{
    if (! isPrepared)
    {
        setParentGraph (graph);

        processor->setRateAndBufferSizeDetails (newSampleRate, newBlockSize);
        processor->prepareToPlay (newSampleRate, newBlockSize);
        isPrepared = true;
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
struct AudioProcessorGraph::AudioProcessorGraphBufferHelpers
{
    AudioProcessorGraphBufferHelpers() noexcept
        : currentAudioInputBuffer (nullptr),
          currentCVInputBuffer (nullptr) {}

    void setRenderingBufferSize (int newNumAudioChannels, int newNumCVChannels, int newNumSamples) noexcept
    {
        renderingAudioBuffers.setSize (newNumAudioChannels, newNumSamples);
        renderingAudioBuffers.clear();

        renderingCVBuffers.setSize (newNumCVChannels, newNumSamples);
        renderingCVBuffers.clear();
    }

    void release() noexcept
    {
        renderingAudioBuffers.setSize (1, 1);
        currentAudioInputBuffer = nullptr;
        currentCVInputBuffer = nullptr;
        currentAudioOutputBuffer.setSize (1, 1);
        currentCVOutputBuffer.setSize (1, 1);

        renderingCVBuffers.setSize (1, 1);
    }

    void prepareInOutBuffers (int newNumAudioChannels, int newNumCVChannels, int newNumSamples) noexcept
    {
        currentAudioInputBuffer = nullptr;
        currentCVInputBuffer = nullptr;
        currentAudioOutputBuffer.setSize (newNumAudioChannels, newNumSamples);
        currentCVOutputBuffer.setSize (newNumCVChannels, newNumSamples);
    }

    AudioSampleBuffer        renderingAudioBuffers;
    AudioSampleBuffer        renderingCVBuffers;
    AudioSampleBuffer*       currentAudioInputBuffer;
    const AudioSampleBuffer* currentCVInputBuffer;
    AudioSampleBuffer        currentAudioOutputBuffer;
    AudioSampleBuffer        currentCVOutputBuffer;
};

//==============================================================================
AudioProcessorGraph::AudioProcessorGraph()
    : lastNodeId (0), audioAndCVBuffers (new AudioProcessorGraphBufferHelpers),
      currentMidiInputBuffer (nullptr), isPrepared (false), needsReorder (false)
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
    needsReorder = true;
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
    CARLA_SAFE_ASSERT_RETURN (newProcessor != nullptr && newProcessor != this, nullptr);

    for (int i = nodes.size(); --i >= 0;)
    {
        CARLA_SAFE_ASSERT_RETURN (nodes.getUnchecked(i)->getProcessor() != newProcessor, nullptr);
    }

    if (nodeId == 0)
    {
        nodeId = ++lastNodeId;
    }
    else
    {
        // you can't add a node with an id that already exists in the graph..
        CARLA_SAFE_ASSERT_RETURN (getNodeForId (nodeId) == nullptr, nullptr);
        removeNode (nodeId);

        if (nodeId > lastNodeId)
            lastNodeId = nodeId;
    }

    Node* const n = new Node (nodeId, newProcessor);
    nodes.add (n);

    if (isPrepared)
        needsReorder = true;

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
            nodes.remove (i);

            if (isPrepared)
                needsReorder = true;

            return true;
        }
    }

    return false;
}

bool AudioProcessorGraph::removeNode (Node* node)
{
    CARLA_SAFE_ASSERT_RETURN(node != nullptr, false);

    return removeNode (node->nodeId);
}

//==============================================================================
const AudioProcessorGraph::Connection* AudioProcessorGraph::getConnectionBetween (const ChannelType ct,
                                                                                  const uint32 sourceNodeId,
                                                                                  const uint sourceChannelIndex,
                                                                                  const uint32 destNodeId,
                                                                                  const uint destChannelIndex) const
{
    const Connection c (ct, sourceNodeId, sourceChannelIndex, destNodeId, destChannelIndex);
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

bool AudioProcessorGraph::canConnect (ChannelType ct,
                                      const uint32 sourceNodeId,
                                      const uint sourceChannelIndex,
                                      const uint32 destNodeId,
                                      const uint destChannelIndex) const
{
    if (sourceNodeId == destNodeId)
        return false;

    const Node* const source = getNodeForId (sourceNodeId);

    if (source == nullptr
         || (ct != ChannelTypeMIDI && sourceChannelIndex >= source->processor->getTotalNumOutputChannels(ct))
         || (ct == ChannelTypeMIDI && ! source->processor->producesMidi()))
        return false;

    const Node* const dest = getNodeForId (destNodeId);

    if (dest == nullptr
         || (ct != ChannelTypeMIDI && destChannelIndex >= dest->processor->getTotalNumInputChannels(ct))
         || (ct == ChannelTypeMIDI && ! dest->processor->acceptsMidi()))
        return false;

    return getConnectionBetween (ct,
                                 sourceNodeId, sourceChannelIndex,
                                 destNodeId, destChannelIndex) == nullptr;
}

bool AudioProcessorGraph::addConnection (const ChannelType ct,
                                         const uint32 sourceNodeId,
                                         const uint sourceChannelIndex,
                                         const uint32 destNodeId,
                                         const uint destChannelIndex)
{
    if (! canConnect (ct, sourceNodeId, sourceChannelIndex, destNodeId, destChannelIndex))
        return false;

    GraphRenderingOps::ConnectionSorter sorter;
    connections.addSorted (sorter, new Connection (ct,
                                                   sourceNodeId, sourceChannelIndex,
                                                   destNodeId, destChannelIndex));

    if (isPrepared)
        needsReorder = true;

    return true;
}

void AudioProcessorGraph::removeConnection (const int index)
{
    connections.remove (index);

    if (isPrepared)
        needsReorder = true;
}

bool AudioProcessorGraph::removeConnection (const ChannelType ct,
                                            const uint32 sourceNodeId, const uint sourceChannelIndex,
                                            const uint32 destNodeId, const uint destChannelIndex)
{
    bool doneAnything = false;

    for (int i = connections.size(); --i >= 0;)
    {
        const Connection* const c = connections.getUnchecked(i);

        if (c->channelType == ct
             && c->sourceNodeId == sourceNodeId
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
    CARLA_SAFE_ASSERT_RETURN (c != nullptr, false);

    const Node* const source = getNodeForId (c->sourceNodeId);
    const Node* const dest   = getNodeForId (c->destNodeId);

    return source != nullptr
        && dest != nullptr
        && (c->channelType != ChannelTypeMIDI ? (c->sourceChannelIndex < source->processor->getTotalNumOutputChannels(c->channelType))
                                              : source->processor->producesMidi())
        && (c->channelType != ChannelTypeMIDI ? (c->destChannelIndex < dest->processor->getTotalNumInputChannels(c->channelType))
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
        delete static_cast<GraphRenderingOps::AudioGraphRenderingOpBase*> (ops.getUnchecked(i));
}

void AudioProcessorGraph::clearRenderingSequence()
{
    Array<void*> oldOps;

    {
        const CarlaRecursiveMutexLocker cml (getCallbackLock());
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
    int numAudioRenderingBuffersNeeded = 2;
    int numCVRenderingBuffersNeeded = 0;
    int numMidiBuffersNeeded = 1;

    {
        const CarlaRecursiveMutexLocker cml (reorderMutex);

        Array<Node*> orderedNodes;

        {
            const GraphRenderingOps::ConnectionLookupTable table (connections);

            for (int i = 0; i < nodes.size(); ++i)
            {
                Node* const node = nodes.getUnchecked(i);

                node->prepare (getSampleRate(), getBlockSize(), this);

                int j = 0;
                for (; j < orderedNodes.size(); ++j)
                    if (table.isAnInputTo (node->nodeId, ((Node*) orderedNodes.getUnchecked(j))->nodeId))
                      break;

                orderedNodes.insert (j, node);
            }
        }

        GraphRenderingOps::RenderingOpSequenceCalculator calculator (*this, orderedNodes, newRenderingOps);

        numAudioRenderingBuffersNeeded = calculator.getNumAudioBuffersNeeded();
        numCVRenderingBuffersNeeded = calculator.getNumCVBuffersNeeded();
        numMidiBuffersNeeded = calculator.getNumMidiBuffersNeeded();
    }

    {
        // swap over to the new rendering sequence..
        const CarlaRecursiveMutexLocker cml (getCallbackLock());

        audioAndCVBuffers->setRenderingBufferSize (numAudioRenderingBuffersNeeded,
                                                   numCVRenderingBuffersNeeded,
                                                   getBlockSize());

        for (int i = static_cast<int>(midiBuffers.size()); --i >= 0;)
            midiBuffers.getUnchecked(i)->clear();

        while (static_cast<int>(midiBuffers.size()) < numMidiBuffersNeeded)
            midiBuffers.add (new MidiBuffer());

        renderingOps.swapWith (newRenderingOps);
    }

    // delete the old ones..
    deleteRenderOpArray (newRenderingOps);
}

//==============================================================================
void AudioProcessorGraph::prepareToPlay (double sampleRate, int estimatedSamplesPerBlock)
{
    setRateAndBufferSizeDetails(sampleRate, estimatedSamplesPerBlock);

    audioAndCVBuffers->prepareInOutBuffers(jmax(1U, getTotalNumOutputChannels(AudioProcessor::ChannelTypeAudio)),
                                           jmax(1U, getTotalNumOutputChannels(AudioProcessor::ChannelTypeCV)),
                                           estimatedSamplesPerBlock);

    currentMidiInputBuffer = nullptr;
    currentMidiOutputBuffer.clear();

    clearRenderingSequence();
    buildRenderingSequence();

    isPrepared = true;
}

void AudioProcessorGraph::releaseResources()
{
    isPrepared = false;

    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked(i)->unprepare();

    audioAndCVBuffers->release();
    midiBuffers.clear();

    currentMidiInputBuffer = nullptr;
    currentMidiOutputBuffer.clear();
}

void AudioProcessorGraph::reset()
{
    const CarlaRecursiveMutexLocker cml (getCallbackLock());

    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked(i)->getProcessor()->reset();
}

void AudioProcessorGraph::setNonRealtime (bool isProcessingNonRealtime) noexcept
{
    const CarlaRecursiveMutexLocker cml (getCallbackLock());

    AudioProcessor::setNonRealtime (isProcessingNonRealtime);

    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked(i)->getProcessor()->setNonRealtime (isProcessingNonRealtime);
}

/*
void AudioProcessorGraph::processAudio (AudioSampleBuffer& audioBuffer, MidiBuffer& midiMessages)
{
    AudioSampleBuffer*& currentAudioInputBuffer  = audioAndCVBuffers->currentAudioInputBuffer;
    AudioSampleBuffer&  currentAudioOutputBuffer = audioAndCVBuffers->currentAudioOutputBuffer;
    AudioSampleBuffer&  renderingAudioBuffers    = audioAndCVBuffers->renderingAudioBuffers;
    AudioSampleBuffer&  renderingCVBuffers       = audioAndCVBuffers->renderingCVBuffers;

    const int numSamples = audioBuffer.getNumSamples();

    if (! audioAndCVBuffers->currentAudioOutputBuffer.setSizeRT(numSamples))
        return;
    if (! audioAndCVBuffers->renderingAudioBuffers.setSizeRT(numSamples))
        return;
    if (! audioAndCVBuffers->renderingCVBuffers.setSizeRT(numSamples))
        return;

    currentAudioInputBuffer = &audioBuffer;
    currentAudioOutputBuffer.clear();
    currentMidiInputBuffer = &midiMessages;
    currentMidiOutputBuffer.clear();

    for (int i = 0; i < renderingOps.size(); ++i)
    {
        GraphRenderingOps::AudioGraphRenderingOpBase* const op
            = (GraphRenderingOps::AudioGraphRenderingOpBase*) renderingOps.getUnchecked(i);

        op->perform (renderingAudioBuffers, renderingCVBuffers, midiBuffers, numSamples);
    }

    for (uint32_t i = 0; i < audioBuffer.getNumChannels(); ++i)
        audioBuffer.copyFrom (i, 0, currentAudioOutputBuffer, i, 0, numSamples);

    midiMessages.clear();
    midiMessages.addEvents (currentMidiOutputBuffer, 0, audioBuffer.getNumSamples(), 0);
}
*/

void AudioProcessorGraph::processAudioAndCV (AudioSampleBuffer& audioBuffer,
                                             const AudioSampleBuffer& cvInBuffer,
                                             AudioSampleBuffer& cvOutBuffer,
                                             MidiBuffer& midiMessages)
{
    AudioSampleBuffer*&       currentAudioInputBuffer  = audioAndCVBuffers->currentAudioInputBuffer;
    const AudioSampleBuffer*& currentCVInputBuffer     = audioAndCVBuffers->currentCVInputBuffer;
    AudioSampleBuffer&        currentAudioOutputBuffer = audioAndCVBuffers->currentAudioOutputBuffer;
    AudioSampleBuffer&        currentCVOutputBuffer    = audioAndCVBuffers->currentCVOutputBuffer;
    AudioSampleBuffer&        renderingAudioBuffers    = audioAndCVBuffers->renderingAudioBuffers;
    AudioSampleBuffer&        renderingCVBuffers       = audioAndCVBuffers->renderingCVBuffers;

    const int numSamples = audioBuffer.getNumSamples();

    if (! audioAndCVBuffers->currentAudioOutputBuffer.setSizeRT(numSamples))
        return;
    if (! audioAndCVBuffers->currentCVOutputBuffer.setSizeRT(numSamples))
        return;
    if (! audioAndCVBuffers->renderingAudioBuffers.setSizeRT(numSamples))
        return;
    if (! audioAndCVBuffers->renderingCVBuffers.setSizeRT(numSamples))
        return;

    currentAudioInputBuffer = &audioBuffer;
    currentCVInputBuffer = &cvInBuffer;
    currentMidiInputBuffer = &midiMessages;
    currentAudioOutputBuffer.clear();
    currentCVOutputBuffer.clear();
    currentMidiOutputBuffer.clear();

    for (int i = 0; i < renderingOps.size(); ++i)
    {
        GraphRenderingOps::AudioGraphRenderingOpBase* const op
            = (GraphRenderingOps::AudioGraphRenderingOpBase*) renderingOps.getUnchecked(i);

        op->perform (renderingAudioBuffers, renderingCVBuffers, midiBuffers, numSamples);
    }

    for (uint32_t i = 0; i < audioBuffer.getNumChannels(); ++i)
        audioBuffer.copyFrom (i, 0, currentAudioOutputBuffer, i, 0, numSamples);

    for (uint32_t i = 0; i < cvOutBuffer.getNumChannels(); ++i)
        cvOutBuffer.copyFrom (i, 0, currentCVOutputBuffer, i, 0, numSamples);

    midiMessages.clear();
    midiMessages.addEvents (currentMidiOutputBuffer, 0, audioBuffer.getNumSamples(), 0);
}

bool AudioProcessorGraph::acceptsMidi() const                       { return true; }
bool AudioProcessorGraph::producesMidi() const                      { return true; }

/*
void AudioProcessorGraph::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    processAudio (buffer, midiMessages);
}
*/

void AudioProcessorGraph::processBlockWithCV (AudioSampleBuffer& audioBuffer,
                                              const AudioSampleBuffer& cvInBuffer,
                                              AudioSampleBuffer& cvOutBuffer,
                                              MidiBuffer& midiMessages)
{
    processAudioAndCV (audioBuffer, cvInBuffer, cvOutBuffer, midiMessages);
}

void AudioProcessorGraph::reorderNowIfNeeded()
{
    if (needsReorder)
    {
        needsReorder = false;
        buildRenderingSequence();
    }
}

const CarlaRecursiveMutex& AudioProcessorGraph::getReorderMutex() const
{
    return reorderMutex;
}

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
        case cvOutputNode:      return "CV Output";
        case cvInputNode:       return "CV Input";
        case midiOutputNode:    return "Midi Output";
        case midiInputNode:     return "Midi Input";
        default:                break;
    }

    return String();
}

void AudioProcessorGraph::AudioGraphIOProcessor::prepareToPlay (double, int)
{
    CARLA_SAFE_ASSERT (graph != nullptr);
}

void AudioProcessorGraph::AudioGraphIOProcessor::releaseResources()
{
}

void AudioProcessorGraph::AudioGraphIOProcessor::processAudioAndCV (AudioSampleBuffer& audioBuffer,
                                                                    const AudioSampleBuffer& cvInBuffer,
                                                                    AudioSampleBuffer& cvOutBuffer,
                                                                    MidiBuffer& midiMessages)
{
    CARLA_SAFE_ASSERT_RETURN(graph != nullptr,);

    switch (type)
    {
        case audioOutputNode:
        {
            AudioSampleBuffer&  currentAudioOutputBuffer =
                graph->audioAndCVBuffers->currentAudioOutputBuffer;

            for (int i = jmin (currentAudioOutputBuffer.getNumChannels(),
                               audioBuffer.getNumChannels()); --i >= 0;)
            {
                currentAudioOutputBuffer.addFrom (i, 0, audioBuffer, i, 0, audioBuffer.getNumSamples());
            }

            break;
        }

        case audioInputNode:
        {
            AudioSampleBuffer*& currentAudioInputBuffer =
                graph->audioAndCVBuffers->currentAudioInputBuffer;

            for (int i = jmin (currentAudioInputBuffer->getNumChannels(),
                               audioBuffer.getNumChannels()); --i >= 0;)
            {
                audioBuffer.copyFrom (i, 0, *currentAudioInputBuffer, i, 0, audioBuffer.getNumSamples());
            }

            break;
        }

        case cvOutputNode:
        {
            AudioSampleBuffer&  currentCVOutputBuffer =
                graph->audioAndCVBuffers->currentCVOutputBuffer;

            for (int i = jmin (currentCVOutputBuffer.getNumChannels(),
                               cvInBuffer.getNumChannels()); --i >= 0;)
            {
                currentCVOutputBuffer.addFrom (i, 0, cvInBuffer, i, 0, cvInBuffer.getNumSamples());
            }

            break;
        }

        case cvInputNode:
        {
            const AudioSampleBuffer*& currentCVInputBuffer =
                graph->audioAndCVBuffers->currentCVInputBuffer;

            for (int i = jmin (currentCVInputBuffer->getNumChannels(),
                               cvOutBuffer.getNumChannels()); --i >= 0;)
            {
                cvOutBuffer.copyFrom (i, 0, *currentCVInputBuffer, i, 0, cvOutBuffer.getNumSamples());
            }

            break;
        }

        case midiOutputNode:
            graph->currentMidiOutputBuffer.addEvents (midiMessages, 0, audioBuffer.getNumSamples(), 0);
            break;

        case midiInputNode:
            midiMessages.addEvents (*graph->currentMidiInputBuffer, 0, audioBuffer.getNumSamples(), 0);
            break;

        default:
            break;
    }
}

void AudioProcessorGraph::AudioGraphIOProcessor::processBlockWithCV (AudioSampleBuffer& audioBuffer,
                                                                     const AudioSampleBuffer& cvInBuffer,
                                                                     AudioSampleBuffer& cvOutBuffer,
                                                                     MidiBuffer& midiMessages)
{
    processAudioAndCV (audioBuffer, cvInBuffer, cvOutBuffer, midiMessages);
}

bool AudioProcessorGraph::AudioGraphIOProcessor::acceptsMidi() const
{
    return type == midiOutputNode;
}

bool AudioProcessorGraph::AudioGraphIOProcessor::producesMidi() const
{
    return type == midiInputNode;
}

bool AudioProcessorGraph::AudioGraphIOProcessor::isInput() const noexcept
{
    return type == audioInputNode || type == cvInputNode || type == midiInputNode;
}

bool AudioProcessorGraph::AudioGraphIOProcessor::isOutput() const noexcept
{
    return type == audioOutputNode || type == cvOutputNode || type == midiOutputNode;
}

void AudioProcessorGraph::AudioGraphIOProcessor::setParentGraph (AudioProcessorGraph* const newGraph)
{
    graph = newGraph;

    if (graph != nullptr)
    {
        setPlayConfigDetails (type == audioOutputNode
                                   ? graph->getTotalNumOutputChannels(AudioProcessor::ChannelTypeAudio)
                                   : 0,
                              type == audioInputNode
                                   ? graph->getTotalNumInputChannels(AudioProcessor::ChannelTypeAudio)
                                   : 0,
                              type == cvOutputNode
                                   ? graph->getTotalNumOutputChannels(AudioProcessor::ChannelTypeCV)
                                   : 0,
                              type == cvInputNode
                                   ? graph->getTotalNumInputChannels(AudioProcessor::ChannelTypeCV)
                                   : 0,
                              type == midiOutputNode
                                   ? graph->getTotalNumOutputChannels(AudioProcessor::ChannelTypeMIDI)
                                   : 0,
                              type == midiInputNode
                                   ? graph->getTotalNumInputChannels(AudioProcessor::ChannelTypeMIDI)
                                   : 0,
                              getSampleRate(),
                              getBlockSize());
    }
}

}
