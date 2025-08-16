/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2015 ROLI Ltd.
   Copyright (C) 2017-2025 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the GNU
   General Public License as published by the Free Software Foundation;
   either version 2 of the License, or any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

   For a full copy of the GNU General Public License see the doc/GPL.txt file.

  ==============================================================================
*/

#ifndef WATER_AUDIOPROCESSORGRAPH_H_INCLUDED
#define WATER_AUDIOPROCESSORGRAPH_H_INCLUDED

#include "AudioProcessor.h"
#include "../containers/OwnedArray.h"
#include "../containers/ReferenceCountedArray.h"
#include "../midi/MidiBuffer.h"

namespace water {

//==============================================================================
/**
    A type of AudioProcessor which plays back a graph of other AudioProcessors.

    Use one of these objects if you want to wire-up a set of AudioProcessors
    and play back the result.

    Processors can be added to the graph as "nodes" using addNode(), and once
    added, you can connect any of their input or output channels to other
    nodes using addConnection().

    To play back a graph through an audio device, you might want to use an
    AudioProcessorPlayer object.
*/
class AudioProcessorGraph   : public AudioProcessor
{
public:
    //==============================================================================
    /** Creates an empty graph. */
    AudioProcessorGraph();

    /** Destructor.
        Any processor objects that have been added to the graph will also be deleted.
    */
    ~AudioProcessorGraph();

    //==============================================================================
    /** Represents one of the nodes, or processors, in an AudioProcessorGraph.

        To create a node, call AudioProcessorGraph::addNode().
    */
    class Node   : public ReferenceCountedObject
    {
    public:
        //==============================================================================
        /** The ID number assigned to this node.
            This is assigned by the graph that owns it, and can't be changed.
        */
        const uint32 nodeId;

        /** The actual processor object that this node represents. */
        AudioProcessor* getProcessor() const noexcept { return processor.get(); }

        /** Custom properties for Carla usage. */
        struct Properties {
            bool isAudio;
            bool isCV;
            bool isMIDI;
            bool isOSC;
            bool isOutput;
            bool isPlugin;
            uint32_t pluginId;

            struct Position {
                int x1, x2, y1, y2;
                bool valid;

                Position() noexcept
                    : x1(0),
                      x2(0),
                      y1(0),
                      y2(0),
                      valid(false) {}
            } position;

            Properties() noexcept
                : isAudio(false),
                  isCV(false),
                  isMIDI(false),
                  isOSC(false),
                  isOutput(false),
                  isPlugin(false),
                  pluginId(0),
                  position() {}
        } properties;

        //==============================================================================
        /** A convenient typedef for referring to a pointer to a node object. */
        typedef ReferenceCountedObjectPtr<Node> Ptr;

    private:
        //==============================================================================
        friend class AudioProcessorGraph;

        const std::unique_ptr<AudioProcessor> processor;
        bool isPrepared;

        Node (uint32 nodeId, AudioProcessor*) noexcept;

        void setParentGraph (AudioProcessorGraph*) const;
        void prepare (double newSampleRate, int newBlockSize, AudioProcessorGraph*);
        void unprepare();

        CARLA_DECLARE_NON_COPYABLE (Node)
    };

    //==============================================================================
    /** Represents a connection between two channels of two nodes in an AudioProcessorGraph.

        To create a connection, use AudioProcessorGraph::addConnection().
    */
    struct Connection
    {
    public:
        //==============================================================================
        Connection (ChannelType channelType,
                    uint32 sourceNodeId, uint sourceChannelIndex,
                    uint32 destNodeId, uint destChannelIndex) noexcept;

        //==============================================================================
        /** Defines the connection type. */
        ChannelType channelType;

        /** The ID number of the node which is the input source for this connection.
            @see AudioProcessorGraph::getNodeForId
        */
        uint32 sourceNodeId;

        /** The index of the output channel of the source node from which this
            connection takes its data.

            If this value is the special number AudioProcessorGraph::midiChannelIndex, then
            it is referring to the source node's midi output. Otherwise, it is the zero-based
            index of an audio output channel in the source node.
        */
        uint sourceChannelIndex;

        /** The ID number of the node which is the destination for this connection.
            @see AudioProcessorGraph::getNodeForId
        */
        uint32 destNodeId;

        /** The index of the input channel of the destination node to which this
            connection delivers its data.

            If this value is the special number AudioProcessorGraph::midiChannelIndex, then
            it is referring to the destination node's midi input. Otherwise, it is the zero-based
            index of an audio input channel in the destination node.
        */
        uint destChannelIndex;
    };

    //==============================================================================
    /** Deletes all nodes and connections from this graph.
        Any processor objects in the graph will be deleted.
    */
    void clear();

    /** Returns the number of nodes in the graph. */
    int getNumNodes() const noexcept                                { return nodes.size(); }

    /** Returns a pointer to one of the nodes in the graph.
        This will return nullptr if the index is out of range.
        @see getNodeForId
    */
    Node* getNode (const int index) const noexcept                  { return nodes [index]; }

    /** Searches the graph for a node with the given ID number and returns it.
        If no such node was found, this returns nullptr.
        @see getNode
    */
    Node* getNodeForId (const uint32 nodeId) const;

    /** Adds a node to the graph.

        This creates a new node in the graph, for the specified processor. Once you have
        added a processor to the graph, the graph owns it and will delete it later when
        it is no longer needed.

        The optional nodeId parameter lets you specify an ID to use for the node, but
        if the value is already in use, this new node will overwrite the old one.

        If this succeeds, it returns a pointer to the newly-created node.
    */
    Node* addNode (AudioProcessor* newProcessor, uint32 nodeId = 0);

    /** Deletes a node within the graph which has the specified ID.

        This will also delete any connections that are attached to this node.
    */
    bool removeNode (uint32 nodeId);

    /** Deletes a node within the graph which has the specified ID.

        This will also delete any connections that are attached to this node.
     */
    bool removeNode (Node* node);

    //==============================================================================
    /** Returns the number of connections in the graph. */
    size_t getNumConnections() const                                    { return connections.size(); }

    /** Returns a pointer to one of the connections in the graph. */
    const Connection* getConnection (size_t index) const                { return connections [index]; }

    /** Searches for a connection between some specified channels.
        If no such connection is found, this returns nullptr.
    */
    const Connection* getConnectionBetween (ChannelType channelType,
                                            uint32 sourceNodeId,
                                            uint sourceChannelIndex,
                                            uint32 destNodeId,
                                            uint destChannelIndex) const;

    /** Returns true if there is a connection between any of the channels of
        two specified nodes.
    */
    bool isConnected (uint32 possibleSourceNodeId,
                      uint32 possibleDestNodeId) const;

    /** Returns true if it would be legal to connect the specified points. */
    bool canConnect (ChannelType channelType,
                     uint32 sourceNodeId, uint sourceChannelIndex,
                     uint32 destNodeId, uint destChannelIndex) const;

    /** Attempts to connect two specified channels of two nodes.

        If this isn't allowed (e.g. because you're trying to connect a midi channel
        to an audio one or other such nonsense), then it'll return false.
    */
    bool addConnection (ChannelType channelType,
                        uint32 sourceNodeId, uint sourceChannelIndex,
                        uint32 destNodeId, uint destChannelIndex);

    /** Deletes the connection with the specified index. */
    void removeConnection (int index);

    /** Deletes any connection between two specified points.
        Returns true if a connection was actually deleted.
    */
    bool removeConnection (ChannelType channelType,
                           uint32 sourceNodeId, uint sourceChannelIndex,
                           uint32 destNodeId, uint destChannelIndex);

    /** Removes all connections from the specified node. */
    bool disconnectNode (uint32 nodeId);

    /** Returns true if the given connection's channel numbers map on to valid
        channels at each end.
        Even if a connection is valid when created, its status could change if
        a node changes its channel config.
    */
    bool isConnectionLegal (const Connection* connection) const;

    /** Performs a sanity checks of all the connections.

        This might be useful if some of the processors are doing things like changing
        their channel counts, which could render some connections obsolete.
    */
    bool removeIllegalConnections();

    //==============================================================================
    /** A special number that represents the midi channel of a node.

        This is used as a channel index value if you want to refer to the midi input
        or output instead of an audio channel.
    */
    static const uint midiChannelIndex;


    //==============================================================================
    /** A special type of AudioProcessor that can live inside an AudioProcessorGraph
        in order to use the audio that comes into and out of the graph itself.

        If you create an AudioGraphIOProcessor in "input" mode, it will act as a
        node in the graph which delivers the audio that is coming into the parent
        graph. This allows you to stream the data to other nodes and process the
        incoming audio.

        Likewise, one of these in "output" mode can be sent data which it will add to
        the sum of data being sent to the graph's output.

        @see AudioProcessorGraph
    */
    class AudioGraphIOProcessor     : public AudioProcessor
    {
    public:
        /** Specifies the mode in which this processor will operate.
        */
        enum IODeviceType
        {
            audioInputNode,     /**< In this mode, the processor has output channels
                                     representing all the audio input channels that are
                                     coming into its parent audio graph. */
            audioOutputNode,    /**< In this mode, the processor has input channels
                                     representing all the audio output channels that are
                                     going out of its parent audio graph. */
            midiInputNode,      /**< In this mode, the processor has a midi output which
                                     delivers the same midi data that is arriving at its
                                     parent graph. */
            midiOutputNode,     /**< In this mode, the processor has a midi input and
                                     any data sent to it will be passed out of the parent
                                     graph. */
            cvInputNode, cvOutputNode,
        };

        //==============================================================================
        /** Returns the mode of this processor. */
        IODeviceType getType() const noexcept                       { return type; }

        /** Returns the parent graph to which this processor belongs, or nullptr if it
            hasn't yet been added to one. */
        AudioProcessorGraph* getParentGraph() const noexcept        { return graph; }

        /** True if this is an audio or midi input. */
        bool isInput() const noexcept;
        /** True if this is an audio or midi output. */
        bool isOutput() const noexcept;

        //==============================================================================
        AudioGraphIOProcessor (const IODeviceType type);
        ~AudioGraphIOProcessor();

        const String getName() const override;
        void prepareToPlay (double newSampleRate, int estimatedSamplesPerBlock) override;
        void releaseResources() override;
        void processBlockWithCV (AudioSampleBuffer& audioBuffer,
                                 const AudioSampleBuffer& cvInBuffer,
                                 AudioSampleBuffer& cvOutBuffer,
                                 MidiBuffer& midiMessages) override;

        bool acceptsMidi() const override;
        bool producesMidi() const override;

        /** @internal */
        void setParentGraph (AudioProcessorGraph*);

    private:
        const IODeviceType type;
        AudioProcessorGraph* graph;

        //==============================================================================
        //void processAudio (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
        void processAudioAndCV (AudioSampleBuffer& audioBuffer,
                                const AudioSampleBuffer& cvInBuffer,
                                AudioSampleBuffer& cvOutBuffer,
                                MidiBuffer& midiMessages);

        CARLA_DECLARE_NON_COPYABLE (AudioGraphIOProcessor)
    };

    //==============================================================================
    const String getName() const override;
    void prepareToPlay (double, int) override;
    void releaseResources() override;
    //void processBlock (AudioSampleBuffer&,  MidiBuffer&) override;
    void processBlockWithCV (AudioSampleBuffer& audioBuffer,
                             const AudioSampleBuffer& cvInBuffer,
                             AudioSampleBuffer& cvOutBuffer,
                             MidiBuffer& midiMessages) override;

    void reset() override;
    void setNonRealtime (bool) noexcept override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;

    void reorderNowIfNeeded();
    const CarlaRecursiveMutex& getReorderMutex() const;

private:
    //==============================================================================
    // void processAudio (AudioSampleBuffer& audioBuffer, MidiBuffer& midiMessages);
    void processAudioAndCV (AudioSampleBuffer& audioBuffer,
                            const AudioSampleBuffer& cvInBuffer,
                            AudioSampleBuffer& cvOutBuffer,
                            MidiBuffer& midiMessages);

    //==============================================================================
    ReferenceCountedArray<Node> nodes;
    OwnedArray<Connection> connections;
    uint32 lastNodeId;
    OwnedArray<MidiBuffer> midiBuffers;
    Array<void*> renderingOps;

    friend class AudioGraphIOProcessor;
    struct AudioProcessorGraphBufferHelpers;
    std::unique_ptr<AudioProcessorGraphBufferHelpers> audioAndCVBuffers;

    MidiBuffer* currentMidiInputBuffer;
    MidiBuffer currentMidiOutputBuffer;

    bool isPrepared, needsReorder;
    CarlaRecursiveMutex reorderMutex;

public:
    void clearRenderingSequence();
    void buildRenderingSequence();
    bool isAnInputTo (uint32 possibleInputId, uint32 possibleDestinationId, int recursionCheck) const;

    CARLA_DECLARE_NON_COPYABLE (AudioProcessorGraph)
};

}

#endif // WATER_AUDIOPROCESSORGRAPH_H_INCLUDED
