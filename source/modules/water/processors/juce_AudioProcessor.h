/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

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

#ifndef JUCE_AUDIOPROCESSOR_H_INCLUDED
#define JUCE_AUDIOPROCESSOR_H_INCLUDED

// #include "juce_AudioPlayHead.h"

//==============================================================================
/**
    Base class for audio processing filters or plugins.

    This is intended to act as a base class of audio filter that is general enough to
    be wrapped as a VST, AU, RTAS, etc, or used internally.

    It is also used by the plugin hosting code as the wrapper around an instance
    of a loaded plugin.

    Derive your filter class from this base class, and if you're building a plugin,
    you should implement a global function called createPluginFilter() which creates
    and returns a new instance of your subclass.
*/
class JUCE_API  AudioProcessor
{
protected:
    //==============================================================================
    /** Constructor.

        This constructor will create a main input and output bus which are diabled
        by default. If you need more fine grain control then use the other
        constructors.
    */
    AudioProcessor();

public:
    //==============================================================================
    /** Destructor. */
    virtual ~AudioProcessor();

    //==============================================================================
    /** Returns the name of this processor. */
    virtual const String getName() const = 0;

    //==============================================================================
    /** Called before playback starts, to let the filter prepare itself.

        The sample rate is the target sample rate, and will remain constant until
        playback stops.

        You can call getTotalNumInputChannels and getTotalNumOutputChannels
        or query the busLayout member variable to find out the number of
        channels your processBlock callback must process.

        The maximumExpectedSamplesPerBlock value is a strong hint about the maximum
        number of samples that will be provided in each block. You may want to use
        this value to resize internal buffers. You should program defensively in
        case a buggy host exceeds this value. The actual block sizes that the host
        uses may be different each time the callback happens: completely variable
        block sizes can be expected from some hosts.

       @see busLayout, getTotalNumInputChannels, getTotalNumOutputChannels
    */
    virtual void prepareToPlay (double sampleRate,
                                int maximumExpectedSamplesPerBlock) = 0;

    /** Called after playback has stopped, to let the filter free up any resources it
        no longer needs.
    */
    virtual void releaseResources() = 0;

    /** Renders the next block.

        When this method is called, the buffer contains a number of channels which is
        at least as great as the maximum number of input and output channels that
        this filter is using. It will be filled with the filter's input data and
        should be replaced with the filter's output.

        So for example if your filter has a total of 2 input channels and 4 output
        channels, then the buffer will contain 4 channels, the first two being filled
        with the input data. Your filter should read these, do its processing, and
        replace the contents of all 4 channels with its output.

        Or if your filter has a total of 5 inputs and 2 outputs, the buffer will have 5
        channels, all filled with data, and your filter should overwrite the first 2 of
        these with its output. But be VERY careful not to write anything to the last 3
        channels, as these might be mapped to memory that the host assumes is read-only!

        If your plug-in has more than one input or output buses then the buffer passed
        to the processBlock methods will contain a bundle of all channels of each bus.
        Use AudiobusLayout::getBusBuffer to obtain an audio buffer for a
        particular bus.

        Note that if you have more outputs than inputs, then only those channels that
        correspond to an input channel are guaranteed to contain sensible data - e.g.
        in the case of 2 inputs and 4 outputs, the first two channels contain the input,
        but the last two channels may contain garbage, so you should be careful not to
        let this pass through without being overwritten or cleared.

        Also note that the buffer may have more channels than are strictly necessary,
        but you should only read/write from the ones that your filter is supposed to
        be using.

        The number of samples in these buffers is NOT guaranteed to be the same for every
        callback, and may be more or less than the estimated value given to prepareToPlay().
        Your code must be able to cope with variable-sized blocks, or you're going to get
        clicks and crashes!

        Also note that some hosts will occasionally decide to pass a buffer containing
        zero samples, so make sure that your algorithm can deal with that!

        If the filter is receiving a midi input, then the midiMessages array will be filled
        with the midi messages for this block. Each message's timestamp will indicate the
        message's time, as a number of samples from the start of the block.

        Any messages left in the midi buffer when this method has finished are assumed to
        be the filter's midi output. This means that your filter should be careful to
        clear any incoming messages from the array if it doesn't want them to be passed-on.

        Be very careful about what you do in this callback - it's going to be called by
        the audio thread, so any kind of interaction with the UI is absolutely
        out of the question. If you change a parameter in here and need to tell your UI to
        update itself, the best way is probably to inherit from a ChangeBroadcaster, let
        the UI components register as listeners, and then call sendChangeMessage() inside the
        processBlock() method to send out an asynchronous message. You could also use
        the AsyncUpdater class in a similar way.

        @see AudiobusLayout::getBusBuffer
    */

    virtual void processBlock (AudioSampleBuffer& buffer,
                               MidiBuffer& midiMessages) = 0;

    /** Renders the next block when the processor is being bypassed.

        The default implementation of this method will pass-through any incoming audio, but
        you may override this method e.g. to add latency compensation to the data to match
        the processor's latency characteristics. This will avoid situations where bypassing
        will shift the signal forward in time, possibly creating pre-echo effects and odd timings.
        Another use for this method would be to cross-fade or morph between the wet (not bypassed)
        and dry (bypassed) signals.
    */
    virtual void processBlockBypassed (AudioSampleBuffer& buffer,
                                       MidiBuffer& midiMessages);

    //==============================================================================
    /** Returns the current AudioPlayHead object that should be used to find
        out the state and position of the playhead.

        You can ONLY call this from your processBlock() method! Calling it at other
        times will produce undefined behaviour, as the host may not have any context
        in which a time would make sense, and some hosts will almost certainly have
        multithreading issues if it's not called on the audio thread.

        The AudioPlayHead object that is returned can be used to get the details about
        the time of the start of the block currently being processed. But do not
        store this pointer or use it outside of the current audio callback, because
        the host may delete or re-use it.

        If the host can't or won't provide any time info, this will return nullptr.
    */
    AudioPlayHead* getPlayHead() const noexcept                 { return playHead; }

    //==============================================================================
    /** Returns the total number of input channels.

        This method will return the total number of input channels by accumulating
        the number of channels on each input bus. The number of channels of the
        buffer passed to your processBlock callback will be equivalent to either
        getTotalNumInputChannels or getTotalNumOutputChannels - which ever
        is greater.

        Note that getTotalNumInputChannels is equivalent to
        getMainBusNumInputChannels if your processor does not have any sidechains
        or aux buses.
     */
    int getTotalNumInputChannels()  const noexcept              { return cachedTotalIns; }

    /** Returns the total number of output channels.

        This method will return the total number of output channels by accumulating
        the number of channels on each output bus. The number of channels of the
        buffer passed to your processBlock callback will be equivalent to either
        getTotalNumInputChannels or getTotalNumOutputChannels - which ever
        is greater.

        Note that getTotalNumOutputChannels is equivalent to
        getMainBusNumOutputChannels if your processor does not have any sidechains
        or aux buses.
     */
    int getTotalNumOutputChannels() const noexcept              { return cachedTotalOuts; }

    //==============================================================================
    /** Returns the current sample rate.

        This can be called from your processBlock() method - it's not guaranteed
        to be valid at any other time, and may return 0 if it's unknown.
    */
    double getSampleRate() const noexcept                       { return currentSampleRate; }

    /** Returns the current typical block size that is being used.

        This can be called from your processBlock() method - it's not guaranteed
        to be valid at any other time.

        Remember it's not the ONLY block size that may be used when calling
        processBlock, it's just the normal one. The actual block sizes used may be
        larger or smaller than this, and will vary between successive calls.
    */
    int getBlockSize() const noexcept                           { return blockSize; }

    //==============================================================================

    /** This returns the number of samples delay that the filter imposes on the audio
        passing through it.

        The host will call this to find the latency - the filter itself should set this value
        by calling setLatencySamples() as soon as it can during its initialisation.
    */
    int getLatencySamples() const noexcept                      { return latencySamples; }

    /** The filter should call this to set the number of samples delay that it introduces.

        The filter should call this as soon as it can during initialisation, and can call it
        later if the value changes.
    */
    void setLatencySamples (int newLatency);

    /** Returns true if the processor wants midi messages. */
    virtual bool acceptsMidi() const = 0;

    /** Returns true if the processor produces midi messages. */
    virtual bool producesMidi() const = 0;

    /** Returns true if the processor supports MPE. */
    virtual bool supportsMPE() const                            { return false; }

    /** Returns true if this is a midi effect plug-in and does no audio processing. */
    virtual bool isMidiEffect() const                           { return false; }

    virtual const String getInputChannelName  (int) const { return String(); }
    virtual const String getOutputChannelName (int) const { return String(); }

    //==============================================================================
    /** This returns a critical section that will automatically be locked while the host
        is calling the processBlock() method.

        Use it from your UI or other threads to lock access to variables that are used
        by the process callback, but obviously be careful not to keep it locked for
        too long, because that could cause stuttering playback. If you need to do something
        that'll take a long time and need the processing to stop while it happens, use the
        suspendProcessing() method instead.

        @see suspendProcessing
    */
    const CarlaRecursiveMutex& getCallbackLock() const noexcept     { return callbackLock; }

    /** Enables and disables the processing callback.

        If you need to do something time-consuming on a thread and would like to make sure
        the audio processing callback doesn't happen until you've finished, use this
        to disable the callback and re-enable it again afterwards.

        E.g.
        @code
        void loadNewPatch()
        {
            suspendProcessing (true);

            ..do something that takes ages..

            suspendProcessing (false);
        }
        @endcode

        If the host tries to make an audio callback while processing is suspended, the
        filter will return an empty buffer, but won't block the audio thread like it would
        do if you use the getCallbackLock() critical section to synchronise access.

        Any code that calls processBlock() should call isSuspended() before doing so, and
        if the processor is suspended, it should avoid the call and emit silence or
        whatever is appropriate.

        @see getCallbackLock
    */
    void suspendProcessing (bool shouldBeSuspended);

    /** Returns true if processing is currently suspended.
        @see suspendProcessing
    */
    bool isSuspended() const noexcept                                   { return suspended; }

    /** A plugin can override this to be told when it should reset any playing voices.

        The default implementation does nothing, but a host may call this to tell the
        plugin that it should stop any tails or sounds that have been left running.
    */
    virtual void reset();

    //==============================================================================
    /** Returns true if the processor is being run in an offline mode for rendering.

        If the processor is being run live on realtime signals, this returns false.
        If the mode is unknown, this will assume it's realtime and return false.

        This value may be unreliable until the prepareToPlay() method has been called,
        and could change each time prepareToPlay() is called.

        @see setNonRealtime()
    */
    bool isNonRealtime() const noexcept                                 { return nonRealtime; }

    /** Called by the host to tell this processor whether it's being used in a non-realtime
        capacity for offline rendering or bouncing.
    */
    virtual void setNonRealtime (bool isNonRealtime) noexcept;

    //==============================================================================
    /** Tells the processor to use this playhead object.
        The processor will not take ownership of the object, so the caller must delete it when
        it is no longer being used.
    */
    virtual void setPlayHead (AudioPlayHead* newPlayHead);

    //==============================================================================
    /** This is called by the processor to specify its details before being played. Use this
        version of the function if you are not interested in any sidechain and/or aux buses
        and do not care about the layout of channels. Otherwise use setRateAndBufferSizeDetails.*/
    void setPlayConfigDetails (int numIns, int numOuts, double sampleRate, int blockSize);

    /** This is called by the processor to specify its details before being played. You
        should call this function after having informed the processor about the channel
        and bus layouts via setBusesLayout.

        @see setBusesLayout
    */
    void setRateAndBufferSizeDetails (double sampleRate, int blockSize) noexcept;

private:
    //==============================================================================
    /** @internal */
    AudioPlayHead* playHead;

    //==============================================================================
    double currentSampleRate;
    int blockSize, latencySamples;
    bool suspended, nonRealtime;
    CarlaRecursiveMutex callbackLock;

    String cachedInputSpeakerArrString;
    String cachedOutputSpeakerArrString;

    int cachedTotalIns, cachedTotalOuts;

    void processBypassed (AudioSampleBuffer&, MidiBuffer&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProcessor)
};


#endif   // JUCE_AUDIOPROCESSOR_H_INCLUDED
