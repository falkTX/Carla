/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#if JUCE_PLUGINHOST_VST

//==============================================================================
#undef PRAGMA_ALIGN_SUPPORTED

#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4996)
#elif ! JUCE_MINGW
 #define __cdecl
#endif

namespace Vst2
{
 #include "juce_VSTInterface.h"
}

using namespace Vst2;

#include "juce_VSTCommon.h"

#if JUCE_MSVC
 #pragma warning (pop)
 #pragma warning (disable: 4355) // ("this" used in initialiser list warning)
#endif

#include "juce_VSTMidiEventList.h"

#if JUCE_MINGW
 #ifndef WM_APPCOMMAND
  #define WM_APPCOMMAND 0x0319
 #endif
#elif ! JUCE_WINDOWS
 static void _fpreset() {}
 static void _clearfp() {}
#endif

#ifndef JUCE_VST_WRAPPER_LOAD_CUSTOM_MAIN
 #define JUCE_VST_WRAPPER_LOAD_CUSTOM_MAIN
#endif

#ifndef JUCE_VST_WRAPPER_INVOKE_MAIN
 #define JUCE_VST_WRAPPER_INVOKE_MAIN  effect = module->moduleMain (&audioMaster);
#endif

//==============================================================================
namespace juce
{

//==============================================================================
namespace
{
    const int fxbVersionNum = 1;

    struct fxProgram
    {
        int32 chunkMagic;    // 'CcnK'
        int32 byteSize;      // of this chunk, excl. magic + byteSize
        int32 fxMagic;       // 'FxCk'
        int32 version;
        int32 fxID;          // fx unique id
        int32 fxVersion;
        int32 numParams;
        char prgName[28];
        float params[1];        // variable no. of parameters
    };

    struct fxSet
    {
        int32 chunkMagic;    // 'CcnK'
        int32 byteSize;      // of this chunk, excl. magic + byteSize
        int32 fxMagic;       // 'FxBk'
        int32 version;
        int32 fxID;          // fx unique id
        int32 fxVersion;
        int32 numPrograms;
        char future[128];
        fxProgram programs[1];  // variable no. of programs
    };

    struct fxChunkSet
    {
        int32 chunkMagic;    // 'CcnK'
        int32 byteSize;      // of this chunk, excl. magic + byteSize
        int32 fxMagic;       // 'FxCh', 'FPCh', or 'FBCh'
        int32 version;
        int32 fxID;          // fx unique id
        int32 fxVersion;
        int32 numPrograms;
        char future[128];
        int32 chunkSize;
        char chunk[8];          // variable
    };

    struct fxProgramSet
    {
        int32 chunkMagic;    // 'CcnK'
        int32 byteSize;      // of this chunk, excl. magic + byteSize
        int32 fxMagic;       // 'FxCh', 'FPCh', or 'FBCh'
        int32 version;
        int32 fxID;          // fx unique id
        int32 fxVersion;
        int32 numPrograms;
        char name[28];
        int32 chunkSize;
        char chunk[8];          // variable
    };

    // Compares a magic value in either endianness.
    static bool compareMagic (int32 magic, const char* name) noexcept
    {
        return magic == (int32) ByteOrder::littleEndianInt (name)
            || magic == (int32) ByteOrder::bigEndianInt (name);
    }

    static int32 fxbName (const char* name) noexcept    { return (int32) ByteOrder::littleEndianInt (name); }
    static int32 fxbSwap (int32 x) noexcept             { return (int32) ByteOrder::swapIfLittleEndian ((uint32) x); }

    static float fxbSwapFloat (const float x) noexcept
    {
       #ifdef JUCE_LITTLE_ENDIAN
        union { uint32 asInt; float asFloat; } n;
        n.asFloat = x;
        n.asInt = ByteOrder::swap (n.asInt);
        return n.asFloat;
       #else
        return x;
       #endif
    }
}

//==============================================================================
namespace
{
    static double getVSTHostTimeNanoseconds() noexcept
    {
       #if JUCE_WINDOWS
        return timeGetTime() * 1000000.0;
       #elif JUCE_LINUX || JUCE_IOS || JUCE_ANDROID
        timeval micro;
        gettimeofday (&micro, 0);
        return micro.tv_usec * 1000.0;
       #elif JUCE_MAC
        UnsignedWide micro;
        Microseconds (&micro);
        return micro.lo * 1000.0;
       #endif
    }

    static int shellUIDToCreate = 0;
    static int insideVSTCallback = 0;

    struct IdleCallRecursionPreventer
    {
        IdleCallRecursionPreventer()  : isMessageThread (MessageManager::getInstance()->isThisTheMessageThread())
        {
            if (isMessageThread)
                ++insideVSTCallback;
        }

        ~IdleCallRecursionPreventer()
        {
            if (isMessageThread)
                --insideVSTCallback;
        }

        const bool isMessageThread;
        JUCE_DECLARE_NON_COPYABLE (IdleCallRecursionPreventer)
    };

   #if JUCE_MAC
    static bool makeFSRefFromPath (FSRef* destFSRef, const String& path)
    {
        return FSPathMakeRef (reinterpret_cast<const UInt8*> (path.toRawUTF8()), destFSRef, 0) == noErr;
    }
   #endif
}

//==============================================================================
typedef VstEffectInterface* (VSTINTERFACECALL *MainCall) (VstHostCallback);
static pointer_sized_int VSTINTERFACECALL audioMaster (VstEffectInterface*, int32, int32, pointer_sized_int, void*, float);

//==============================================================================
// Change this to disable logging of various VST activities
#ifndef VST_LOGGING
 #define VST_LOGGING 1
#endif

#if VST_LOGGING
 #define JUCE_VST_LOG(a) Logger::writeToLog(a);
#else
 #define JUCE_VST_LOG(a)
#endif

//==============================================================================
#if JUCE_LINUX

namespace
{
    typedef void (*EventProcPtr) (XEvent* ev);

    Window getChildWindow (Window windowToCheck)
    {
        Window rootWindow, parentWindow;
        Window* childWindows;
        unsigned int numChildren = 0;

        {
            ScopedXDisplay xDisplay;
            XQueryTree (xDisplay.display, windowToCheck, &rootWindow, &parentWindow, &childWindows, &numChildren);
        }

        if (numChildren > 0)
            return childWindows [0];

        return 0;
    }
}

#endif

//==============================================================================
struct ModuleHandle    : public ReferenceCountedObject
{
    File file;
    MainCall moduleMain, customMain = {};
    String pluginName;
    ScopedPointer<XmlElement> vstXml;

    typedef ReferenceCountedObjectPtr<ModuleHandle> Ptr;

    static Array<ModuleHandle*>& getActiveModules()
    {
        static Array<ModuleHandle*> activeModules;
        return activeModules;
    }

    //==============================================================================
    static Ptr findOrCreateModule (const File& file)
    {
        for (auto* module : getActiveModules())
            if (module->file == file)
                return module;

        const IdleCallRecursionPreventer icrp;
        shellUIDToCreate = 0;
        _fpreset();

        JUCE_VST_LOG ("Attempting to load VST: " + file.getFullPathName());

        Ptr m = new ModuleHandle (file, nullptr);

        if (m->open())
        {
            _fpreset();
            return m;
        }

        return {};
    }

    //==============================================================================
    ModuleHandle (const File& f, MainCall customMainCall)
        : file (f), moduleMain (customMainCall)
    {
        getActiveModules().add (this);

       #if JUCE_WINDOWS || JUCE_LINUX || JUCE_IOS || JUCE_ANDROID
        fullParentDirectoryPathName = f.getParentDirectory().getFullPathName();
       #elif JUCE_MAC
        FSRef ref;
        makeFSRefFromPath (&ref, f.getParentDirectory().getFullPathName());
        FSGetCatalogInfo (&ref, kFSCatInfoNone, 0, 0, &parentDirFSSpec, 0);
       #endif
    }

    ~ModuleHandle()
    {
        getActiveModules().removeFirstMatchingValue (this);
        close();
    }

    //==============================================================================
   #if ! JUCE_MAC
    String fullParentDirectoryPathName;
   #endif

  #if JUCE_WINDOWS || JUCE_LINUX || JUCE_ANDROID
    DynamicLibrary module;

    bool open()
    {
        if (moduleMain != nullptr)
            return true;

        pluginName = file.getFileNameWithoutExtension();

        module.open (file.getFullPathName());

        moduleMain = (MainCall) module.getFunction ("VSTPluginMain");

        if (moduleMain == nullptr)
            moduleMain = (MainCall) module.getFunction ("main");

        JUCE_VST_WRAPPER_LOAD_CUSTOM_MAIN

        if (moduleMain != nullptr)
        {
            vstXml = XmlDocument::parse (file.withFileExtension ("vstxml"));

           #if JUCE_WINDOWS
            if (vstXml == nullptr)
                vstXml = XmlDocument::parse (getDLLResource (file, "VSTXML", 1));
           #endif
        }

        return moduleMain != nullptr;
    }

    void close()
    {
        _fpreset(); // (doesn't do any harm)

        module.close();
    }

    void closeEffect (VstEffectInterface* eff)
    {
        eff->dispatchFunction (eff, plugInOpcodeClose, 0, 0, 0, 0);
    }

   #if JUCE_WINDOWS
    static String getDLLResource (const File& dllFile, const String& type, int resID)
    {
        DynamicLibrary dll (dllFile.getFullPathName());
        auto dllModule = (HMODULE) dll.getNativeHandle();

        if (dllModule != INVALID_HANDLE_VALUE)
        {
            if (auto res = FindResource (dllModule, MAKEINTRESOURCE (resID), type.toWideCharPointer()))
            {
                if (auto hGlob = LoadResource (dllModule, res))
                {
                    auto* data = static_cast<const char*> (LockResource (hGlob));
                    return String::fromUTF8 (data, SizeofResource (dllModule, res));
                }
            }
        }

        return {};
    }
   #endif
  #else
    Handle resHandle = {};
    CFBundleRef bundleRef = {};

   #if JUCE_MAC
    CFBundleRefNum resFileId = {};
    FSSpec parentDirFSSpec;
   #endif

    bool open()
    {
        if (moduleMain != nullptr)
            return true;

        bool ok = false;

        if (file.hasFileExtension (".vst"))
        {
            auto* utf8 = file.getFullPathName().toRawUTF8();

            if (CFURLRef url = CFURLCreateFromFileSystemRepresentation (0, (const UInt8*) utf8,
                                                                        (CFIndex) strlen (utf8), file.isDirectory()))
            {
                bundleRef = CFBundleCreate (kCFAllocatorDefault, url);
                CFRelease (url);

                if (bundleRef != 0)
                {
                    if (CFBundleLoadExecutable (bundleRef))
                    {
                        moduleMain = (MainCall) CFBundleGetFunctionPointerForName (bundleRef, CFSTR("main_macho"));

                        if (moduleMain == nullptr)
                            moduleMain = (MainCall) CFBundleGetFunctionPointerForName (bundleRef, CFSTR("VSTPluginMain"));

                        JUCE_VST_WRAPPER_LOAD_CUSTOM_MAIN

                        if (moduleMain != nullptr)
                        {
                            if (CFTypeRef name = CFBundleGetValueForInfoDictionaryKey (bundleRef, CFSTR("CFBundleName")))
                            {
                                if (CFGetTypeID (name) == CFStringGetTypeID())
                                {
                                    char buffer[1024];

                                    if (CFStringGetCString ((CFStringRef) name, buffer, sizeof (buffer), CFStringGetSystemEncoding()))
                                        pluginName = buffer;
                                }
                            }

                            if (pluginName.isEmpty())
                                pluginName = file.getFileNameWithoutExtension();

                           #if JUCE_MAC
                            resFileId = CFBundleOpenBundleResourceMap (bundleRef);
                           #endif

                            ok = true;

                            Array<File> vstXmlFiles;
                            file
                               #if JUCE_MAC
                                .getChildFile ("Contents")
                                .getChildFile ("Resources")
                               #endif
                                .findChildFiles (vstXmlFiles, File::findFiles, false, "*.vstxml");

                            if (vstXmlFiles.size() > 0)
                                vstXml = XmlDocument::parse (vstXmlFiles.getReference(0));
                        }
                    }

                    if (! ok)
                    {
                        CFBundleUnloadExecutable (bundleRef);
                        CFRelease (bundleRef);
                        bundleRef = 0;
                    }
                }
            }
        }

        return ok;
    }

    void close()
    {
        if (bundleRef != 0)
        {
           #if JUCE_MAC
            CFBundleCloseBundleResourceMap (bundleRef, resFileId);
           #endif

            if (CFGetRetainCount (bundleRef) == 1)
                CFBundleUnloadExecutable (bundleRef);

            if (CFGetRetainCount (bundleRef) > 0)
                CFRelease (bundleRef);
        }
    }

    void closeEffect (VstEffectInterface* eff)
    {
        eff->dispatchFunction (eff, plugInOpcodeClose, 0, 0, 0, 0);
    }

  #endif

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleHandle)
};

static const int defaultVSTSampleRateValue = 44100;
static const int defaultVSTBlockSizeValue = 512;

#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4996) // warning about overriding deprecated methods
#endif

//==============================================================================
//==============================================================================
struct VSTPluginInstance     : public AudioPluginInstance,
                               private Timer,
                               private AsyncUpdater
{
    VSTPluginInstance (const ModuleHandle::Ptr& mh, const BusesProperties& ioConfig, VstEffectInterface* effect,
                       double sampleRateToUse, int blockSizeToUse)
        : AudioPluginInstance (ioConfig),
          vstEffect (effect),
          vstModule (mh),
          name (mh->pluginName)
    {
        setRateAndBufferSizeDetails (sampleRateToUse, blockSizeToUse);
    }

    ~VSTPluginInstance()
    {
        if (vstEffect != nullptr && vstEffect->interfaceIdentifier == juceVstInterfaceIdentifier)
        {
            struct VSTDeleter : public CallbackMessage
            {
                VSTDeleter (VSTPluginInstance& inInstance, WaitableEvent& inEvent)
                    : vstInstance (inInstance), completionSignal (inEvent)
                {}

                void messageCallback() override
                {
                    vstInstance.cleanup();
                    completionSignal.signal();
                }

                VSTPluginInstance& vstInstance;
                WaitableEvent& completionSignal;
            };

            if (MessageManager::getInstance()->isThisTheMessageThread())
            {
                cleanup();
            }
            else
            {
                WaitableEvent completionEvent;
                (new VSTDeleter (*this, completionEvent))->post();
                completionEvent.wait();
            }
        }
    }

    void cleanup()
    {
        if (vstEffect != nullptr && vstEffect->interfaceIdentifier == juceVstInterfaceIdentifier)
        {
           #if JUCE_MAC
            if (vstModule->resFileId != 0)
                UseResFile (vstModule->resFileId);
           #endif

            // Must delete any editors before deleting the plugin instance!
            jassert (getActiveEditor() == 0);

            _fpreset(); // some dodgy plugs fuck around with this

            vstModule->closeEffect (vstEffect);
        }

        vstModule = nullptr;
        vstEffect = nullptr;
    }

    static VSTPluginInstance* create (const ModuleHandle::Ptr& newModule,
                                      double initialSampleRate,
                                      int initialBlockSize)
    {
        if (auto* newEffect = constructEffect (newModule))
        {
            newEffect->hostSpace2 = 0;

            newEffect->dispatchFunction (newEffect, plugInOpcodeIdentify, 0, 0, 0, 0);

            auto blockSize = jmax (32, initialBlockSize);

            newEffect->dispatchFunction (newEffect, plugInOpcodeSetSampleRate, 0, 0, 0, static_cast<float> (initialSampleRate));
            newEffect->dispatchFunction (newEffect, plugInOpcodeSetBlockSize,  0, blockSize, 0, 0);

            newEffect->dispatchFunction (newEffect, plugInOpcodeOpen, 0, 0, 0, 0);
            BusesProperties ioConfig = queryBusIO (newEffect);

            return new VSTPluginInstance (newModule, ioConfig, newEffect, initialSampleRate, blockSize);
        }

        return nullptr;
    }

    //==============================================================================
    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = name;

        {
            char buffer[512] = { 0 };
            dispatch (plugInOpcodeGetPlugInName, 0, 0, buffer, 0);

            desc.descriptiveName = String::createStringFromData (buffer, (int) sizeof (buffer)).trim();

            if (desc.descriptiveName.isEmpty())
                desc.descriptiveName = name;
        }

        desc.fileOrIdentifier = vstModule->file.getFullPathName();
        desc.uid = getUID();
        desc.lastFileModTime = vstModule->file.getLastModificationTime();
        desc.lastInfoUpdateTime = Time::getCurrentTime();
        desc.pluginFormatName = "VST";
        desc.category = getCategory();

        {
            char buffer[512] = { 0 };
            dispatch (plugInOpcodeGetManufacturerName, 0, 0, buffer, 0);
            desc.manufacturerName = String::createStringFromData (buffer, (int) sizeof (buffer)).trim();
        }

        desc.version = getVersion();
        desc.numInputChannels = getTotalNumInputChannels();
        desc.numOutputChannels = getTotalNumOutputChannels();
        desc.isInstrument = (vstEffect != nullptr && (vstEffect->flags & vstEffectFlagIsSynth) != 0);
    }

    bool initialiseEffect (double initialSampleRate, int initialBlockSize)
    {
        if (vstEffect != nullptr)
        {
            vstEffect->hostSpace2 = (pointer_sized_int) (pointer_sized_int) this;
            initialise (initialSampleRate, initialBlockSize);
            return true;
        }

        return false;
    }

    void initialise (double initialSampleRate, int initialBlockSize)
    {
        if (initialised || vstEffect == nullptr)
            return;

       #if JUCE_WINDOWS
        // On Windows it's highly advisable to create your plugins using the message thread,
        // because many plugins need a chance to create HWNDs that will get their
        // messages delivered by the main message thread, and that's not possible from
        // a background thread.
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
       #endif

        JUCE_VST_LOG ("Initialising VST: " + vstModule->pluginName + " (" + getVersion() + ")");
        initialised = true;

        setRateAndBufferSizeDetails (initialSampleRate, initialBlockSize);

        dispatch (plugInOpcodeIdentify, 0, 0, 0, 0);

        if (getSampleRate() > 0)
            dispatch (plugInOpcodeSetSampleRate, 0, 0, 0, (float) getSampleRate());

        if (getBlockSize() > 0)
            dispatch (plugInOpcodeSetBlockSize, 0, jmax (32, getBlockSize()), 0, 0);

        dispatch (plugInOpcodeOpen, 0, 0, 0, 0);

        setRateAndBufferSizeDetails (getSampleRate(), getBlockSize());

        if (getNumPrograms() > 1)
            setCurrentProgram (0);
        else
            dispatch (plugInOpcodeSetCurrentProgram, 0, 0, 0, 0);

        for (int i = vstEffect->numInputChannels;  --i >= 0;)  dispatch (plugInOpcodeConnectInput,  i, 1, 0, 0);
        for (int i = vstEffect->numOutputChannels; --i >= 0;)  dispatch (plugInOpcodeConnectOutput, i, 1, 0, 0);

        if (getVstCategory() != kPlugCategShell) // (workaround for Waves 5 plugins which crash during this call)
            updateStoredProgramNames();

        wantsMidiMessages = pluginCanDo ("receiveVstMidiEvent") > 0;

       #if JUCE_MAC && JUCE_SUPPORT_CARBON
        usesCocoaNSView = ((unsigned int) pluginCanDo ("hasCockosViewAsConfig") & 0xffff0000ul) == 0xbeef0000ul;
       #endif

        setLatencySamples (vstEffect->latency);
    }

    void* getPlatformSpecificData() override    { return vstEffect; }

    const String getName() const override
    {
        if (vstEffect != nullptr)
        {
            char buffer[512] = { 0 };

            if (dispatch (plugInOpcodeGetManufacturerProductName, 0, 0, buffer, 0) != 0)
            {
                String productName = String::createStringFromData (buffer, (int) sizeof (buffer));

                if (productName.isNotEmpty())
                    return productName;
            }
        }

        return name;
    }

    int getUID() const
    {
        int uid = vstEffect != nullptr ? vstEffect->plugInIdentifier : 0;

        if (uid == 0)
            uid = vstModule->file.hashCode();

        return uid;
    }

    double getTailLengthSeconds() const override
    {
        if (vstEffect == nullptr)
            return 0.0;

        auto sampleRate = getSampleRate();

        if (sampleRate <= 0)
            return 0.0;

        return dispatch (plugInOpcodeGetTailSize, 0, 0, 0, 0) / sampleRate;
    }

    bool acceptsMidi() const override    { return wantsMidiMessages; }
    bool producesMidi() const override   { return pluginCanDo ("sendVstMidiEvent") > 0; }
    bool supportsMPE() const override    { return pluginCanDo ("MPE") > 0; }

    VstPlugInCategory getVstCategory() const noexcept     { return (VstPlugInCategory) dispatch (plugInOpcodeGetPlugInCategory, 0, 0, 0, 0); }

    int pluginCanDo (const char* text) const     { return (int) dispatch (plugInOpcodeCanPlugInDo, 0, 0, (void*) text,  0); }

    //==============================================================================
    void prepareToPlay (double rate, int samplesPerBlockExpected) override
    {
        setRateAndBufferSizeDetails (rate, samplesPerBlockExpected);

        SpeakerMappings::VstSpeakerConfigurationHolder inArr  (getChannelLayoutOfBus (true,  0));
        SpeakerMappings::VstSpeakerConfigurationHolder outArr (getChannelLayoutOfBus (false, 0));

        dispatch (plugInOpcodeSetSpeakerConfiguration, 0, (pointer_sized_int) &inArr.get(), (void*) &outArr.get(), 0.0f);

        vstHostTime.tempoBPM = 120.0;
        vstHostTime.timeSignatureNumerator = 4;
        vstHostTime.timeSignatureDenominator = 4;
        vstHostTime.sampleRate = rate;
        vstHostTime.samplePosition = 0;
        vstHostTime.flags = vstTimingInfoFlagNanosecondsValid
                              | vstTimingInfoFlagAutomationWriteModeActive
                              | vstTimingInfoFlagAutomationReadModeActive;

        initialise (rate, samplesPerBlockExpected);

        if (initialised)
        {
            wantsMidiMessages = wantsMidiMessages || (pluginCanDo ("receiveVstMidiEvent") > 0);

            if (wantsMidiMessages)
                midiEventsToSend.ensureSize (256);
            else
                midiEventsToSend.freeEvents();

            incomingMidi.clear();

            dispatch (plugInOpcodeSetSampleRate, 0, 0, 0, (float) rate);
            dispatch (plugInOpcodeSetBlockSize, 0, jmax (16, samplesPerBlockExpected), 0, 0);

            if (supportsDoublePrecisionProcessing())
            {
                int32 vstPrecision = isUsingDoublePrecision() ? vstProcessingSampleTypeDouble
                                                              : vstProcessingSampleTypeFloat;

                dispatch (plugInOpcodeSetSampleFloatType, 0, (pointer_sized_int) vstPrecision, 0, 0);
            }

            auto maxChannels = jmax (1, jmax (vstEffect->numInputChannels, vstEffect->numOutputChannels));

            tmpBufferFloat .setSize (maxChannels, samplesPerBlockExpected);
            tmpBufferDouble.setSize (maxChannels, samplesPerBlockExpected);

            channelBufferFloat .calloc (static_cast<size_t> (maxChannels));
            channelBufferDouble.calloc (static_cast<size_t> (maxChannels));

            outOfPlaceBuffer.setSize (jmax (1, vstEffect->numOutputChannels), samplesPerBlockExpected);

            if (! isPowerOn)
                setPower (true);

            // dodgy hack to force some plugins to initialise the sample rate..
            if ((! hasEditor()) && getNumParameters() > 0)
            {
                auto old = getParameter (0);
                setParameter (0, (old < 0.5f) ? 1.0f : 0.0f);
                setParameter (0, old);
            }

            dispatch (plugInOpcodeStartProcess, 0, 0, 0, 0);

            setLatencySamples (vstEffect->latency);
        }
    }

    void releaseResources() override
    {
        if (initialised)
        {
            dispatch (plugInOpcodeStopProcess, 0, 0, 0, 0);
            setPower (false);
        }

        channelBufferFloat.free();
        tmpBufferFloat.setSize (0, 0);

        channelBufferDouble.free();
        tmpBufferDouble.setSize (0, 0);

        outOfPlaceBuffer.setSize (1, 1);
        incomingMidi.clear();

        midiEventsToSend.freeEvents();
    }

    void reset() override
    {
        if (isPowerOn)
        {
            setPower (false);
            setPower (true);
        }
    }

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        jassert (! isUsingDoublePrecision());
        processAudio (buffer, midiMessages, tmpBufferFloat, channelBufferFloat);
    }

    void processBlock (AudioBuffer<double>& buffer, MidiBuffer& midiMessages) override
    {
        jassert (isUsingDoublePrecision());
        processAudio (buffer, midiMessages, tmpBufferDouble, channelBufferDouble);
    }

    bool supportsDoublePrecisionProcessing() const override
    {
        return ((vstEffect->flags & vstEffectFlagInplaceAudio) != 0
             && (vstEffect->flags & vstEffectFlagInplaceDoubleAudio) != 0);
    }

    //==============================================================================
    bool canAddBus (bool) const override                                       { return false; }
    bool canRemoveBus (bool) const override                                    { return false; }

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        auto numInputBuses  = getBusCount (true);
        auto numOutputBuses = getBusCount (false);

        // it's not possible to change layout if there are sidechains/aux buses
        if (numInputBuses > 1 || numOutputBuses > 1)
            return (layouts == getBusesLayout());

        return (layouts.getNumChannels (true,  0) <= vstEffect->numInputChannels
             && layouts.getNumChannels (false, 0) <= vstEffect->numOutputChannels);
    }

    //==============================================================================
   #if JUCE_IOS || JUCE_ANDROID
    bool hasEditor() const override                  { return false; }
   #else
    bool hasEditor() const override                  { return vstEffect != nullptr && (vstEffect->flags & vstEffectFlagHasEditor) != 0; }
   #endif

    AudioProcessorEditor* createEditor() override;

    //==============================================================================
    const String getInputChannelName (int index) const override
    {
        if (isValidChannel (index, true))
        {
            VstPinInfo pinProps;
            if (dispatch (plugInOpcodeGetInputPinProperties, index, 0, &pinProps, 0.0f) != 0)
                return String (pinProps.text, sizeof (pinProps.text));
        }

        return {};
    }

    bool isInputChannelStereoPair (int index) const override
    {
        if (! isValidChannel (index, true))
            return false;

        VstPinInfo pinProps;
        if (dispatch (plugInOpcodeGetInputPinProperties, index, 0, &pinProps, 0.0f) != 0)
            return (pinProps.flags & vstPinInfoFlagIsStereo) != 0;

        return true;
    }

    const String getOutputChannelName (int index) const override
    {
        if (isValidChannel (index, false))
        {
            VstPinInfo pinProps;
            if (dispatch (plugInOpcodeGetOutputPinProperties, index, 0, &pinProps, 0.0f) != 0)
                return String (pinProps.text, sizeof (pinProps.text));
        }

        return {};
    }

    bool isOutputChannelStereoPair (int index) const override
    {
        if (! isValidChannel (index, false))
            return false;

        VstPinInfo pinProps;
        if (dispatch (plugInOpcodeGetOutputPinProperties, index, 0, &pinProps, 0.0f) != 0)
            return (pinProps.flags & vstPinInfoFlagIsStereo) != 0;

        return true;
    }

    bool isValidChannel (int index, bool isInput) const noexcept
    {
        return isPositiveAndBelow (index, isInput ? getTotalNumInputChannels()
                                                  : getTotalNumOutputChannels());
    }

    //==============================================================================
    int getNumParameters() override      { return vstEffect != nullptr ? vstEffect->numParameters : 0; }

    float getParameter (int index) override
    {
        if (vstEffect != nullptr && isPositiveAndBelow (index, vstEffect->numParameters))
        {
            const ScopedLock sl (lock);
            return vstEffect->getParameterValueFunction (vstEffect, index);
        }

        return 0.0f;
    }

    void setParameter (int index, float newValue) override
    {
        if (vstEffect != nullptr && isPositiveAndBelow (index, vstEffect->numParameters))
        {
            const ScopedLock sl (lock);

            if (vstEffect->getParameterValueFunction (vstEffect, index) != newValue)
                vstEffect->setParameterValueFunction (vstEffect, index, newValue);
        }
    }

    const String getParameterName (int index) override       { return getTextForOpcode (index, plugInOpcodeGetParameterName); }
    const String getParameterText (int index) override       { return getTextForOpcode (index, plugInOpcodeGetParameterText); }
    String getParameterLabel (int index) const override      { return getTextForOpcode (index, plugInOpcodeGetParameterLabel); }

    bool isParameterAutomatable (int index) const override
    {
        if (vstEffect != nullptr)
        {
            jassert (index >= 0 && index < vstEffect->numParameters);
            return dispatch (plugInOpcodeIsParameterAutomatable, index, 0, 0, 0) != 0;
        }

        return false;
    }

    //==============================================================================
    int getNumPrograms() override          { return vstEffect != nullptr ? jmax (0, vstEffect->numPrograms) : 0; }

    // NB: some plugs return negative numbers from this function.
    int getCurrentProgram() override       { return (int) dispatch (plugInOpcodeGetCurrentProgram, 0, 0, 0, 0); }

    void setCurrentProgram (int newIndex) override
    {
        if (getNumPrograms() > 0 && newIndex != getCurrentProgram())
            dispatch (plugInOpcodeSetCurrentProgram, 0, jlimit (0, getNumPrograms() - 1, newIndex), 0, 0);
    }

    const String getProgramName (int index) override
    {
        if (index >= 0)
        {
            if (index == getCurrentProgram())
                return getCurrentProgramName();

            if (vstEffect != nullptr)
            {
                char nm[264] = { 0 };

                if (dispatch (plugInOpcodeGetProgramName, jlimit (0, getNumPrograms(), index), -1, nm, 0) != 0)
                    return String::fromUTF8 (nm).trim();
            }
        }

        return programNames [index];
    }

    void changeProgramName (int index, const String& newName) override
    {
        if (index >= 0 && index == getCurrentProgram())
        {
            if (getNumPrograms() > 0 && newName != getCurrentProgramName())
                dispatch (plugInOpcodeSetCurrentProgramName, 0, 0, (void*) newName.substring (0, 24).toRawUTF8(), 0.0f);
        }
        else
        {
            jassertfalse; // xxx not implemented!
        }
    }

    //==============================================================================
    void getStateInformation (MemoryBlock& mb) override                  { saveToFXBFile (mb, true); }
    void getCurrentProgramStateInformation (MemoryBlock& mb) override    { saveToFXBFile (mb, false); }

    void setStateInformation (const void* data, int size) override               { loadFromFXBFile (data, (size_t) size); }
    void setCurrentProgramStateInformation (const void* data, int size) override { loadFromFXBFile (data, (size_t) size); }

    //==============================================================================
    void timerCallback() override
    {
        if (dispatch (plugInOpcodeIdle, 0, 0, 0, 0) == 0)
            stopTimer();
    }

    void handleAsyncUpdate() override
    {
        // indicates that something about the plugin has changed..
        updateHostDisplay();
    }

    pointer_sized_int handleCallback (int32 opcode, int32 index, pointer_sized_int value, void* ptr, float opt)
    {
        switch (opcode)
        {
            case hostOpcodeParameterChanged:            sendParamChangeMessageToListeners (index, opt); break;
            case hostOpcodePreAudioProcessingEvents:    handleMidiFromPlugin ((const VstEventBlock*) ptr); break;
            case hostOpcodeGetTimingInfo:               return getVSTTime();
            case hostOpcodeIdle:                        handleIdle(); break;
            case hostOpcodeWindowSize:                  setWindowSize (index, (int) value); return 1;
            case hostOpcodeUpdateView:                  triggerAsyncUpdate(); break;
            case hostOpcodeIOModified:                  setLatencySamples (vstEffect->latency); break;
            case hostOpcodeNeedsIdle:                   startTimer (50); break;

            case hostOpcodeGetSampleRate:               return (pointer_sized_int) (getSampleRate() > 0 ? getSampleRate() : defaultVSTSampleRateValue);
            case hostOpcodeGetBlockSize:                return (pointer_sized_int) (getBlockSize() > 0  ? getBlockSize()  : defaultVSTBlockSizeValue);
            case hostOpcodePlugInWantsMidi:             wantsMidiMessages = true; break;
            case hostOpcodeGetDirectory:                return getVstDirectory();

            case hostOpcodeTempoAt:                     return (pointer_sized_int) (extraFunctions != nullptr ? extraFunctions->getTempoAt ((int64) value) : 0);
            case hostOpcodeGetAutomationState:          return (pointer_sized_int) (extraFunctions != nullptr ? extraFunctions->getAutomationState() : 0);

            case hostOpcodeParameterChangeGestureBegin: beginParameterChangeGesture (index); break;
            case hostOpcodeParameterChangeGestureEnd:   endParameterChangeGesture (index); break;

            case hostOpcodePinConnected:                    return isValidChannel (index, value == 0) ? 0 : 1; // (yes, 0 = true)
            case hostOpcodeGetCurrentAudioProcessingLevel:  return isNonRealtime() ? 4 : 0;

            // none of these are handled (yet)...
            case hostOpcodeSetTime:
            case hostOpcodeGetParameterInterval:
            case hostOpcodeGetInputLatency:
            case hostOpcodeGetOutputLatency:
            case hostOpcodeGetPreviousPlugIn:
            case hostOpcodeGetNextPlugIn:
            case hostOpcodeWillReplace:
            case hostOpcodeOfflineStart:
            case hostOpcodeOfflineReadSource:
            case hostOpcodeOfflineWrite:
            case hostOpcodeOfflineGetCurrentPass:
            case hostOpcodeOfflineGetCurrentMetaPass:
            case hostOpcodeGetOutputSpeakerConfiguration:
            case hostOpcodeManufacturerSpecific:
            case hostOpcodeSetIcon:
            case hostOpcodeGetLanguage:
            case hostOpcodeOpenEditorWindow:
            case hostOpcodeCloseEditorWindow:
                break;

            default:
                return handleGeneralCallback (opcode, index, value, ptr, opt);
        }

        return 0;
    }

    // handles non plugin-specific callbacks..
    static pointer_sized_int handleGeneralCallback (int32 opcode, int32 /*index*/, pointer_sized_int /*value*/, void* ptr, float /*opt*/)
    {
        switch (opcode)
        {
            case hostOpcodeCanHostDo:                         return handleCanDo ((const char*) ptr);
            case hostOpcodeVstVersion:                        return 2400;
            case hostOpcodeCurrentId:                         return shellUIDToCreate;
            case hostOpcodeGetNumberOfAutomatableParameters:  return 0;
            case hostOpcodeGetAutomationState:                return 1;
            case hostOpcodeGetManufacturerVersion:            return 0x0101;

            case hostOpcodeGetManufacturerName:
            case hostOpcodeGetProductName:                    return getHostName ((char*) ptr);

            case hostOpcodeGetSampleRate:                     return (pointer_sized_int) defaultVSTSampleRateValue;
            case hostOpcodeGetBlockSize:                      return (pointer_sized_int) defaultVSTBlockSizeValue;
            case hostOpcodeSetOutputSampleRate:               return 0;

            default:
                DBG ("*** Unhandled VST Callback: " + String ((int) opcode));
                break;
        }

        return 0;
    }

    //==============================================================================
    pointer_sized_int dispatch (int opcode, int index, pointer_sized_int value, void* const ptr, float opt) const
    {
        pointer_sized_int result = 0;

        if (vstEffect != nullptr)
        {
            const ScopedLock sl (lock);
            const IdleCallRecursionPreventer icrp;

            try
            {
               #if JUCE_MAC
                auto oldResFile = CurResFile();

                if (vstModule->resFileId != 0)
                    UseResFile (vstModule->resFileId);
               #endif

                result = vstEffect->dispatchFunction (vstEffect, opcode, index, value, ptr, opt);

               #if JUCE_MAC
                auto newResFile = CurResFile();

                if (newResFile != oldResFile)  // avoid confusing the parent app's resource file with the plug-in's
                {
                    vstModule->resFileId = newResFile;
                    UseResFile (oldResFile);
                }
               #endif
            }
            catch (...)
            {}
        }

        return result;
    }

    bool loadFromFXBFile (const void* const data, const size_t dataSize)
    {
        if (dataSize < 28)
            return false;

        auto set = (const fxSet*) data;

        if ((! compareMagic (set->chunkMagic, "CcnK")) || fxbSwap (set->version) > fxbVersionNum)
            return false;

        if (compareMagic (set->fxMagic, "FxBk"))
        {
            // bank of programs
            if (fxbSwap (set->numPrograms) >= 0)
            {
                auto oldProg = getCurrentProgram();
                auto numParams = fxbSwap (((const fxProgram*) (set->programs))->numParams);
                auto progLen = (int) sizeof (fxProgram) + (numParams - 1) * (int) sizeof (float);

                for (int i = 0; i < fxbSwap (set->numPrograms); ++i)
                {
                    if (i != oldProg)
                    {
                        auto prog = (const fxProgram*) (((const char*) (set->programs)) + i * progLen);

                        if (((const char*) prog) - ((const char*) set) >= (ssize_t) dataSize)
                            return false;

                        if (fxbSwap (set->numPrograms) > 0)
                            setCurrentProgram (i);

                        if (! restoreProgramSettings (prog))
                            return false;
                    }
                }

                if (fxbSwap (set->numPrograms) > 0)
                    setCurrentProgram (oldProg);

                auto prog = (const fxProgram*) (((const char*) (set->programs)) + oldProg * progLen);

                if (((const char*) prog) - ((const char*) set) >= (ssize_t) dataSize)
                    return false;

                if (! restoreProgramSettings (prog))
                    return false;
            }
        }
        else if (compareMagic (set->fxMagic, "FxCk"))
        {
            // single program
            auto prog = (const fxProgram*) data;

            if (! compareMagic (prog->chunkMagic, "CcnK"))
                return false;

            changeProgramName (getCurrentProgram(), prog->prgName);

            for (int i = 0; i < fxbSwap (prog->numParams); ++i)
                setParameter (i, fxbSwapFloat (prog->params[i]));
        }
        else if (compareMagic (set->fxMagic, "FBCh"))
        {
            // non-preset chunk
            auto cset = (const fxChunkSet*) data;

            if ((size_t) fxbSwap (cset->chunkSize) + sizeof (fxChunkSet) - 8 > (size_t) dataSize)
                return false;

            setChunkData (cset->chunk, fxbSwap (cset->chunkSize), false);
        }
        else if (compareMagic (set->fxMagic, "FPCh"))
        {
            // preset chunk
            auto cset = (const fxProgramSet*) data;

            if ((size_t) fxbSwap (cset->chunkSize) + sizeof (fxProgramSet) - 8 > (size_t) dataSize)
                return false;

            setChunkData (cset->chunk, fxbSwap (cset->chunkSize), true);

            changeProgramName (getCurrentProgram(), cset->name);
        }
        else
        {
            return false;
        }

        return true;
    }

    bool saveToFXBFile (MemoryBlock& dest, bool isFXB, int maxSizeMB = 128)
    {
        auto numPrograms = getNumPrograms();
        auto numParams = getNumParameters();

        if (usesChunks())
        {
            MemoryBlock chunk;
            getChunkData (chunk, ! isFXB, maxSizeMB);

            if (isFXB)
            {
                auto totalLen = sizeof (fxChunkSet) + chunk.getSize() - 8;
                dest.setSize (totalLen, true);

                auto set = (fxChunkSet*) dest.getData();
                set->chunkMagic = fxbName ("CcnK");
                set->byteSize = 0;
                set->fxMagic = fxbName ("FBCh");
                set->version = fxbSwap (fxbVersionNum);
                set->fxID = fxbSwap (getUID());
                set->fxVersion = fxbSwap (getVersionNumber());
                set->numPrograms = fxbSwap (numPrograms);
                set->chunkSize = fxbSwap ((int32) chunk.getSize());

                chunk.copyTo (set->chunk, 0, chunk.getSize());
            }
            else
            {
                auto totalLen = sizeof (fxProgramSet) + chunk.getSize() - 8;
                dest.setSize (totalLen, true);

                auto set = (fxProgramSet*) dest.getData();
                set->chunkMagic = fxbName ("CcnK");
                set->byteSize = 0;
                set->fxMagic = fxbName ("FPCh");
                set->version = fxbSwap (fxbVersionNum);
                set->fxID = fxbSwap (getUID());
                set->fxVersion = fxbSwap (getVersionNumber());
                set->numPrograms = fxbSwap (numPrograms);
                set->chunkSize = fxbSwap ((int32) chunk.getSize());

                getCurrentProgramName().copyToUTF8 (set->name, sizeof (set->name) - 1);
                chunk.copyTo (set->chunk, 0, chunk.getSize());
            }
        }
        else
        {
            if (isFXB)
            {
                auto progLen = (int) sizeof (fxProgram) + (numParams - 1) * (int) sizeof (float);
                auto len = (sizeof (fxSet) - sizeof (fxProgram)) + (size_t) (progLen * jmax (1, numPrograms));
                dest.setSize (len, true);

                auto set = (fxSet*) dest.getData();
                set->chunkMagic = fxbName ("CcnK");
                set->byteSize = 0;
                set->fxMagic = fxbName ("FxBk");
                set->version = fxbSwap (fxbVersionNum);
                set->fxID = fxbSwap (getUID());
                set->fxVersion = fxbSwap (getVersionNumber());
                set->numPrograms = fxbSwap (numPrograms);

                MemoryBlock oldSettings;
                createTempParameterStore (oldSettings);

                auto oldProgram = getCurrentProgram();

                if (oldProgram >= 0)
                    setParamsInProgramBlock ((fxProgram*) (((char*) (set->programs)) + oldProgram * progLen));

                for (int i = 0; i < numPrograms; ++i)
                {
                    if (i != oldProgram)
                    {
                        setCurrentProgram (i);
                        setParamsInProgramBlock ((fxProgram*) (((char*) (set->programs)) + i * progLen));
                    }
                }

                if (oldProgram >= 0)
                    setCurrentProgram (oldProgram);

                restoreFromTempParameterStore (oldSettings);
            }
            else
            {
                dest.setSize (sizeof (fxProgram) + (size_t) ((numParams - 1) * (int) sizeof (float)), true);
                setParamsInProgramBlock ((fxProgram*) dest.getData());
            }
        }

        return true;
    }

    bool usesChunks() const noexcept        { return vstEffect != nullptr && (vstEffect->flags & vstEffectFlagDataInChunks) != 0; }

    bool getChunkData (MemoryBlock& mb, bool isPreset, int maxSizeMB) const
    {
        if (usesChunks())
        {
            void* data = nullptr;
            auto bytes = (size_t) dispatch (plugInOpcodeGetData, isPreset ? 1 : 0, 0, &data, 0.0f);

            if (data != nullptr && bytes <= (size_t) maxSizeMB * 1024 * 1024)
            {
                mb.setSize (bytes);
                mb.copyFrom (data, 0, bytes);

                return true;
            }
        }

        return false;
    }

    bool setChunkData (const void* data, const int size, bool isPreset)
    {
        if (size > 0 && usesChunks())
        {
            dispatch (plugInOpcodeSetData, isPreset ? 1 : 0, size, (void*) data, 0.0f);

            if (! isPreset)
                updateStoredProgramNames();

            return true;
        }

        return false;
    }

    VstEffectInterface* vstEffect;
    ModuleHandle::Ptr vstModule;

    ScopedPointer<VSTPluginFormat::ExtraFunctions> extraFunctions;
    bool usesCocoaNSView = false;

private:
    String name;
    CriticalSection lock;
    bool wantsMidiMessages = false, initialised = false, isPowerOn = false;
    mutable StringArray programNames;
    AudioBuffer<float> outOfPlaceBuffer;

    CriticalSection midiInLock;
    MidiBuffer incomingMidi;
    VSTMidiEventList midiEventsToSend;
    VstTimingInformation vstHostTime;

    AudioBuffer<float> tmpBufferFloat;
    HeapBlock<float*> channelBufferFloat;

    AudioBuffer<double> tmpBufferDouble;
    HeapBlock<double*> channelBufferDouble;

    static pointer_sized_int handleCanDo (const char* name)
    {
        static const char* canDos[] = { "supplyIdle",
                                        "sendVstEvents",
                                        "sendVstMidiEvent",
                                        "sendVstTimeInfo",
                                        "receiveVstEvents",
                                        "receiveVstMidiEvent",
                                        "supportShell",
                                        "sizeWindow",
                                        "shellCategory" };

        for (int i = 0; i < numElementsInArray (canDos); ++i)
            if (strcmp (canDos[i], name) == 0)
                return 1;

        return 0;
    }

    static pointer_sized_int getHostName (char* name)
    {
        String hostName ("Carla Plugin Host");

        if (auto* app = JUCEApplicationBase::getInstance())
            hostName = app->getApplicationName();

        hostName.copyToUTF8 (name, (size_t) jmin (vstMaxManufacturerStringLength, vstMaxPlugInNameStringLength) - 1);
        return 1;
    }

    pointer_sized_int getVSTTime() noexcept
    {
       #if JUCE_MSVC
        #pragma warning (push)
        #pragma warning (disable: 4311)
       #endif

        return (pointer_sized_int) &vstHostTime;

       #if JUCE_MSVC
        #pragma warning (pop)
       #endif
    }

    void handleIdle()
    {
        if (insideVSTCallback == 0 && MessageManager::getInstance()->isThisTheMessageThread())
        {
            const IdleCallRecursionPreventer icrp;

           #if JUCE_MAC
            if (getActiveEditor() != nullptr)
                dispatch (plugInOpcodeEditorIdle, 0, 0, 0, 0);
           #endif

            Timer::callPendingTimersSynchronously();
            handleUpdateNowIfNeeded();

            for (int i = ComponentPeer::getNumPeers(); --i >= 0;)
                if (auto* p = ComponentPeer::getPeer(i))
                    p->performAnyPendingRepaintsNow();
        }
    }

    void setWindowSize (int width, int height)
    {
        if (auto* ed = getActiveEditor())
        {
           #if JUCE_LINUX
            const MessageManagerLock mmLock;
           #endif
            ed->setSize (width, height);
        }
    }

    //==============================================================================
    static VstEffectInterface* constructEffect (const ModuleHandle::Ptr& module)
    {
        VstEffectInterface* effect = nullptr;
        try
        {
            const IdleCallRecursionPreventer icrp;
            _fpreset();

            JUCE_VST_LOG ("Creating VST instance: " + module->pluginName);

           #if JUCE_MAC
            if (module->resFileId != 0)
                UseResFile (module->resFileId);
           #endif

            {
                JUCE_VST_WRAPPER_INVOKE_MAIN
            }

            if (effect != nullptr && effect->interfaceIdentifier == juceVstInterfaceIdentifier)
            {
                jassert (effect->hostSpace2 == 0);
                jassert (effect->effectPointer != 0);

                _fpreset(); // some dodgy plugs mess around with this
            }
            else
            {
                effect = nullptr;
            }
        }
        catch (...)
        {}

        return effect;
    }

    static BusesProperties queryBusIO (VstEffectInterface* effect)
    {
        BusesProperties returnValue;

        if (effect->numInputChannels == 0 && effect->numOutputChannels == 0)
            return returnValue;

        // Workaround for old broken JUCE plug-ins which would return an invalid
        // speaker arrangement if the host didn't ask for a specific arrangement
        // beforehand.
        // Check if the plug-in reports any default layouts. If it doesn't, then
        // try setting a default layout compatible with the number of pins this
        // plug-in is reporting.
        if (! pluginHasDefaultChannelLayouts (effect))
        {
            SpeakerMappings::VstSpeakerConfigurationHolder canonicalIn  (AudioChannelSet::canonicalChannelSet (effect->numInputChannels));
            SpeakerMappings::VstSpeakerConfigurationHolder canonicalOut (AudioChannelSet::canonicalChannelSet (effect->numOutputChannels));

            effect->dispatchFunction (effect, plugInOpcodeSetSpeakerConfiguration, 0,
                                      (pointer_sized_int) &canonicalIn.get(), (void*) &canonicalOut.get(), 0.0f);
        }

        HeapBlock<VstSpeakerConfiguration> inArrBlock (1, true), outArrBlock (1, true);

        auto* inArr  = inArrBlock.get();
        auto* outArr = outArrBlock.get();

        if (! getSpeakerArrangementWrapper (effect, inArr, outArr))
            inArr = outArr = nullptr;

        for (int dir = 0; dir < 2; ++dir)
        {
            const bool isInput = (dir == 0);
            const int opcode = (isInput ? plugInOpcodeGetInputPinProperties : plugInOpcodeGetOutputPinProperties);
            const int maxChannels = (isInput ? effect->numInputChannels : effect->numOutputChannels);
            const VstSpeakerConfiguration* arr = (isInput ? inArr : outArr);
            bool busAdded = false;

            VstPinInfo pinProps;
            AudioChannelSet layout;

            for (int ch = 0; ch < maxChannels; ch += layout.size())
            {
                if (effect->dispatchFunction (effect, opcode, ch, 0, &pinProps, 0.0f) == 0)
                    break;

                if ((pinProps.flags & vstPinInfoFlagValid) != 0)
                {
                    layout = SpeakerMappings::vstArrangementTypeToChannelSet (pinProps.configurationType, 0);

                    if (layout.isDisabled())
                        break;
                }
                else if (arr == nullptr)
                {
                    layout = ((pinProps.flags & vstPinInfoFlagIsStereo) != 0 ? AudioChannelSet::stereo() : AudioChannelSet::mono());
                }
                else
                    break;

                busAdded = true;
                returnValue.addBus (isInput, pinProps.text, layout, true);
            }

            // no buses?
            if (! busAdded && maxChannels > 0)
            {
                String busName = (isInput ? "Input" : "Output");

                if (effect->dispatchFunction (effect, opcode, 0, 0, &pinProps, 0.0f) != 0)
                    busName = pinProps.text;

                if (arr != nullptr)
                    layout = SpeakerMappings::vstArrangementTypeToChannelSet (*arr);
                else
                    layout = AudioChannelSet::canonicalChannelSet (maxChannels);

                returnValue.addBus (isInput, busName, layout, true);
            }
        }

        return returnValue;
    }

    static bool pluginHasDefaultChannelLayouts (VstEffectInterface* effect)
    {
        HeapBlock<VstSpeakerConfiguration> inArrBlock (1, true), outArrBlock (1, true);

        auto* inArr  = inArrBlock.get();
        auto* outArr = outArrBlock.get();

        if (getSpeakerArrangementWrapper (effect, inArr, outArr))
            return true;

        for (int dir = 0; dir < 2; ++dir)
        {
            const bool isInput = (dir == 0);
            const int opcode = (isInput ? plugInOpcodeGetInputPinProperties : plugInOpcodeGetOutputPinProperties);
            const int maxChannels = (isInput ? effect->numInputChannels : effect->numOutputChannels);

            int channels = 1;

            for (int ch = 0; ch < maxChannels; ch += channels)
            {
                VstPinInfo pinProps;

                if (effect->dispatchFunction (effect, opcode, ch, 0, &pinProps, 0.0f) == 0)
                    return false;

                if ((pinProps.flags & vstPinInfoFlagValid) != 0)
                    return true;

                channels = (pinProps.flags & vstPinInfoFlagIsStereo) != 0 ? 2 : 1;
            }
        }

        return false;
    }

    static bool getSpeakerArrangementWrapper (VstEffectInterface* effect,
                                              VstSpeakerConfiguration* inArr,
                                              VstSpeakerConfiguration* outArr)
    {
        // Workaround: unfortunately old JUCE VST-2 plug-ins had a bug and would crash if
        // you try to get the speaker arrangement when there are no input channels present.
        // Hopefully, one day (when there are no more old JUCE plug-ins around), we can
        // commment out the next two lines.
        if (effect->numInputChannels == 0)
            return false;

        return (effect->dispatchFunction (effect, plugInOpcodeGetSpeakerArrangement, 0,
                                          reinterpret_cast<pointer_sized_int> (&inArr), &outArr, 0.0f) != 0);
    }

    //==============================================================================
    template <typename FloatType>
    void processAudio (AudioBuffer<FloatType>& buffer, MidiBuffer& midiMessages,
                       AudioBuffer<FloatType>& tmpBuffer,
                       HeapBlock<FloatType*>& channelBuffer)
    {
        auto numSamples  = buffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();

        if (initialised)
        {
            if (auto* currentPlayHead = getPlayHead())
            {
                AudioPlayHead::CurrentPositionInfo position;

                if (currentPlayHead->getCurrentPosition (position))
                {

                    vstHostTime.samplePosition           = (double) position.timeInSamples;
                    vstHostTime.tempoBPM                 = position.bpm;
                    vstHostTime.timeSignatureNumerator   = position.timeSigNumerator;
                    vstHostTime.timeSignatureDenominator = position.timeSigDenominator;
                    vstHostTime.musicalPosition          = position.ppqPosition;
                    vstHostTime.lastBarPosition          = position.ppqPositionOfLastBarStart;
                    vstHostTime.flags |= vstTimingInfoFlagTempoValid
                                           | vstTimingInfoFlagTimeSignatureValid
                                           | vstTimingInfoFlagMusicalPositionValid
                                           | vstTimingInfoFlagLastBarPositionValid;

                    int32 newTransportFlags = 0;
                    if (position.isPlaying)     newTransportFlags |= vstTimingInfoFlagCurrentlyPlaying;
                    if (position.isRecording)   newTransportFlags |= vstTimingInfoFlagCurrentlyRecording;

                    if (newTransportFlags != (vstHostTime.flags & (vstTimingInfoFlagCurrentlyPlaying
                                                                   | vstTimingInfoFlagCurrentlyRecording)))
                        vstHostTime.flags = (vstHostTime.flags & ~(vstTimingInfoFlagCurrentlyPlaying | vstTimingInfoFlagCurrentlyRecording)) | newTransportFlags | vstTimingInfoFlagTransportChanged;
                    else
                        vstHostTime.flags &= ~vstTimingInfoFlagTransportChanged;

                    switch (position.frameRate)
                    {
                        case AudioPlayHead::fps24:       setHostTimeFrameRate (vstSmpteRateFps24, 24.0, position.timeInSeconds); break;
                        case AudioPlayHead::fps25:       setHostTimeFrameRate (vstSmpteRateFps25, 25.0, position.timeInSeconds); break;
                        case AudioPlayHead::fps30:       setHostTimeFrameRate (vstSmpteRateFps30, 30.0, position.timeInSeconds); break;
                        case AudioPlayHead::fps60:       setHostTimeFrameRate (vstSmpteRateFps60, 60.0, position.timeInSeconds); break;

                        case AudioPlayHead::fps23976:    setHostTimeFrameRateDrop (vstSmpteRateFps239,      24.0, position.timeInSeconds); break;
                        case AudioPlayHead::fps2997:     setHostTimeFrameRateDrop (vstSmpteRateFps2997,     30.0, position.timeInSeconds); break;
                        case AudioPlayHead::fps2997drop: setHostTimeFrameRateDrop (vstSmpteRateFps2997drop, 30.0, position.timeInSeconds); break;
                        case AudioPlayHead::fps30drop:   setHostTimeFrameRateDrop (vstSmpteRateFps30drop,   30.0, position.timeInSeconds); break;
                        case AudioPlayHead::fps60drop:   setHostTimeFrameRateDrop (vstSmpteRateFps599,      60.0, position.timeInSeconds); break;
                        default: break;
                    }

                    if (position.isLooping)
                    {
                        vstHostTime.loopStartPosition = position.ppqLoopStart;
                        vstHostTime.loopEndPosition   = position.ppqLoopEnd;
                        vstHostTime.flags |= (vstTimingInfoFlagLoopPositionValid | vstTimingInfoFlagLoopActive);
                    }
                    else
                    {
                        vstHostTime.flags &= ~(vstTimingInfoFlagLoopPositionValid | vstTimingInfoFlagLoopActive);
                    }
                }
            }

            vstHostTime.systemTimeNanoseconds = getVSTHostTimeNanoseconds();

            if (wantsMidiMessages)
            {
                midiEventsToSend.clear();
                midiEventsToSend.ensureSize (1);

                MidiBuffer::Iterator iter (midiMessages);
                const uint8* midiData;
                int numBytesOfMidiData, samplePosition;

                while (iter.getNextEvent (midiData, numBytesOfMidiData, samplePosition))
                    midiEventsToSend.addEvent (midiData, numBytesOfMidiData,
                                               jlimit (0, numSamples - 1, samplePosition));

                vstEffect->dispatchFunction (vstEffect, plugInOpcodePreAudioProcessingEvents, 0, 0, midiEventsToSend.events, 0);
            }

            _clearfp();

            // always ensure that the buffer is at least as large as the maximum number of channels
            auto maxChannels = jmax (vstEffect->numInputChannels, vstEffect->numOutputChannels);
            auto channels = channelBuffer.get();

            if (numChannels < maxChannels)
            {
                if (numSamples > tmpBuffer.getNumSamples())
                    tmpBuffer.setSize (tmpBuffer.getNumChannels(), numSamples);

                tmpBuffer.clear();
            }

            for (int ch = 0; ch < maxChannels; ++ch)
                channels[ch] = (ch < numChannels ? buffer.getWritePointer (ch) : tmpBuffer.getWritePointer (ch));

            {
                AudioBuffer<FloatType> processBuffer (channels, maxChannels, numSamples);

                invokeProcessFunction (processBuffer, numSamples);
            }
        }
        else
        {
            // Not initialised, so just bypass..
            for (int i = getTotalNumOutputChannels(); --i >= 0;)
                buffer.clear (i, 0, buffer.getNumSamples());
        }

        {
            // copy any incoming midi..
            const ScopedLock sl (midiInLock);

            midiMessages.swapWith (incomingMidi);
            incomingMidi.clear();
        }
    }

    //==============================================================================
    inline void invokeProcessFunction (AudioBuffer<float>& buffer, int32 sampleFrames)
    {
        if ((vstEffect->flags & vstEffectFlagInplaceAudio) != 0)
        {
            vstEffect->processAudioInplaceFunction (vstEffect, buffer.getArrayOfWritePointers(),
                                                    buffer.getArrayOfWritePointers(), sampleFrames);
        }
        else
        {
            outOfPlaceBuffer.setSize (vstEffect->numOutputChannels, sampleFrames);
            outOfPlaceBuffer.clear();

            vstEffect->processAudioFunction (vstEffect, buffer.getArrayOfWritePointers(),
                                             outOfPlaceBuffer.getArrayOfWritePointers(), sampleFrames);

            for (int i = vstEffect->numOutputChannels; --i >= 0;)
                buffer.copyFrom (i, 0, outOfPlaceBuffer.getReadPointer (i), sampleFrames);
        }
    }

    inline void invokeProcessFunction (AudioBuffer<double>& buffer, int32 sampleFrames)
    {
        vstEffect->processDoubleAudioInplaceFunction (vstEffect, buffer.getArrayOfWritePointers(),
                                                      buffer.getArrayOfWritePointers(), sampleFrames);
    }

    //==============================================================================
    void setHostTimeFrameRate (long frameRateIndex, double frameRate, double currentTime) noexcept
    {
        vstHostTime.flags |= vstTimingInfoFlagSmpteValid;
        vstHostTime.smpteRate   = (int32) frameRateIndex;
        vstHostTime.smpteOffset = (int32) (currentTime * 80.0 * frameRate + 0.5);
    }

    void setHostTimeFrameRateDrop (long frameRateIndex, double frameRate, double currentTime) noexcept
    {
        setHostTimeFrameRate (frameRateIndex, frameRate * 1000.0 / 1001.0, currentTime);
    }

    bool restoreProgramSettings (const fxProgram* const prog)
    {
        if (compareMagic (prog->chunkMagic, "CcnK")
             && compareMagic (prog->fxMagic, "FxCk"))
        {
            changeProgramName (getCurrentProgram(), prog->prgName);

            for (int i = 0; i < fxbSwap (prog->numParams); ++i)
                setParameter (i, fxbSwapFloat (prog->params[i]));

            return true;
        }

        return false;
    }

    String getTextForOpcode (const int index, const VstHostToPlugInOpcodes opcode) const
    {
        if (vstEffect == nullptr)
            return {};

        jassert (index >= 0 && index < vstEffect->numParameters);
        char nm[256] = { 0 };
        dispatch (opcode, index, 0, nm, 0);
        return String::createStringFromData (nm, (int) sizeof (nm)).trim();
    }

    String getCurrentProgramName()
    {
        String progName;

        if (vstEffect != nullptr)
        {
            {
                char nm[256] = { 0 };
                dispatch (plugInOpcodeGetCurrentProgramName, 0, 0, nm, 0);
                progName = String::createStringFromData (nm, (int) sizeof (nm)).trim();
            }

            const int index = getCurrentProgram();

            if (index >= 0 && programNames[index].isEmpty())
            {
                while (programNames.size() < index)
                    programNames.add (String());

                programNames.set (index, progName);
            }
        }

        return progName;
    }

    void setParamsInProgramBlock (fxProgram* prog)
    {
        auto numParams = getNumParameters();

        prog->chunkMagic = fxbName ("CcnK");
        prog->byteSize = 0;
        prog->fxMagic = fxbName ("FxCk");
        prog->version = fxbSwap (fxbVersionNum);
        prog->fxID = fxbSwap (getUID());
        prog->fxVersion = fxbSwap (getVersionNumber());
        prog->numParams = fxbSwap (numParams);

        getCurrentProgramName().copyToUTF8 (prog->prgName, sizeof (prog->prgName) - 1);

        for (int i = 0; i < numParams; ++i)
            prog->params[i] = fxbSwapFloat (getParameter (i));
    }

    void updateStoredProgramNames()
    {
        if (vstEffect != nullptr && getNumPrograms() > 0)
        {
            char nm[256] = { 0 };

            // only do this if the plugin can't use indexed names..
            if (dispatch (plugInOpcodeGetProgramName, 0, -1, nm, 0) == 0)
            {
                auto oldProgram = getCurrentProgram();
                MemoryBlock oldSettings;
                createTempParameterStore (oldSettings);

                for (int i = 0; i < getNumPrograms(); ++i)
                {
                    setCurrentProgram (i);
                    getCurrentProgramName();  // (this updates the list)
                }

                setCurrentProgram (oldProgram);
                restoreFromTempParameterStore (oldSettings);
            }
        }
    }

    void handleMidiFromPlugin (const VstEventBlock* events)
    {
        if (events != nullptr)
        {
            const ScopedLock sl (midiInLock);
            VSTMidiEventList::addEventsToMidiBuffer (events, incomingMidi);
        }
    }

    //==============================================================================
    void createTempParameterStore (MemoryBlock& dest)
    {
        dest.setSize (64 + 4 * (size_t) getNumParameters());
        dest.fillWith (0);

        getCurrentProgramName().copyToUTF8 ((char*) dest.getData(), 63);

        auto p = (float*) (((char*) dest.getData()) + 64);

        for (int i = 0; i < getNumParameters(); ++i)
            p[i] = getParameter(i);
    }

    void restoreFromTempParameterStore (const MemoryBlock& m)
    {
        changeProgramName (getCurrentProgram(), (const char*) m.getData());

        auto p = (float*) (((char*) m.getData()) + 64);

        for (int i = 0; i < getNumParameters(); ++i)
            setParameter (i, p[i]);
    }

    pointer_sized_int getVstDirectory() const
    {
       #if JUCE_MAC
        return (pointer_sized_int) (void*) &vstModule->parentDirFSSpec;
       #else
        return (pointer_sized_int) (pointer_sized_uint) vstModule->fullParentDirectoryPathName.toRawUTF8();
       #endif
    }

    //==============================================================================
    int getVersionNumber() const noexcept   { return vstEffect != nullptr ? vstEffect->plugInVersion : 0; }

    String getVersion() const
    {
        auto v = (unsigned int) dispatch (plugInOpcodeGetManufacturerVersion, 0, 0, 0, 0);

        String s;

        if (v == 0 || (int) v == -1)
            v = (unsigned int) getVersionNumber();

        if (v != 0)
        {
            int versionBits[32];
            int n = 0;

            for (auto vv = v; vv != 0; vv /= 10)
                versionBits [n++] = vv % 10;

            if (n > 4) // if the number ends up silly, it's probably encoded as hex instead of decimal..
            {
                n = 0;

                for (auto vv = v; vv != 0; vv >>= 8)
                    versionBits [n++] = vv & 255;
            }

            while (n > 1 && versionBits [n - 1] == 0)
                --n;

            s << 'V';

            while (n > 0)
            {
                s << versionBits [--n];

                if (n > 0)
                    s << '.';
            }
        }

        return s;
    }

    const char* getCategory() const
    {
        switch (getVstCategory())
        {
            case kPlugCategEffect:       return "Effect";
            case kPlugCategSynth:        return "Synth";
            case kPlugCategAnalysis:     return "Analysis";
            case kPlugCategMastering:    return "Mastering";
            case kPlugCategSpacializer:  return "Spacial";
            case kPlugCategRoomFx:       return "Reverb";
            case kPlugSurroundFx:        return "Surround";
            case kPlugCategRestoration:  return "Restoration";
            case kPlugCategGenerator:    return "Tone generation";
            default:                     break;
        }

        return nullptr;
    }

    void setPower (const bool on)
    {
        dispatch (plugInOpcodeResumeSuspend, 0, on ? 1 : 0, 0, 0);
        isPowerOn = on;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTPluginInstance)
};

//==============================================================================
#if ! (JUCE_IOS || JUCE_ANDROID)
struct VSTPluginWindow;
static Array<VSTPluginWindow*> activeVSTWindows;

//==============================================================================
struct VSTPluginWindow   : public AudioProcessorEditor,
                          #if ! JUCE_MAC
                           private ComponentMovementWatcher,
                          #endif
                           private Timer
{
public:
    VSTPluginWindow (VSTPluginInstance& plug)
        : AudioProcessorEditor (&plug),
         #if ! JUCE_MAC
          ComponentMovementWatcher (this),
         #endif
          plugin (plug)
    {
       #if JUCE_LINUX
        pluginWindow = None;
        display = XWindowSystem::getInstance()->displayRef();

       #elif JUCE_MAC
        ignoreUnused (recursiveResize, pluginRefusesToResize, alreadyInside);

        #if JUCE_SUPPORT_CARBON
        if (! plug.usesCocoaNSView)
            addAndMakeVisible (carbonWrapper = new CarbonWrapperComponent (*this));
        else
        #endif
            addAndMakeVisible (cocoaWrapper = new AutoResizingNSViewComponentWithParent());
       #endif

        activeVSTWindows.add (this);

        setSize (1, 1);
        setOpaque (true);
        setVisible (true);
    }

    ~VSTPluginWindow()
    {
        closePluginWindow();

       #if JUCE_MAC
        #if JUCE_SUPPORT_CARBON
        carbonWrapper = nullptr;
        #endif
        cocoaWrapper = nullptr;
       #elif JUCE_LINUX
        display = XWindowSystem::getInstance()->displayUnref();
       #endif

        activeVSTWindows.removeFirstMatchingValue (this);
        plugin.editorBeingDeleted (this);
    }

    //==============================================================================
   #if ! JUCE_MAC
    void componentMovedOrResized (bool /*wasMoved*/, bool /*wasResized*/) override
    {
        if (recursiveResize)
            return;

        auto* topComp = getTopLevelComponent();

        if (topComp->getPeer() != nullptr)
        {
            auto pos = topComp->getLocalPoint (this, Point<int>());

            recursiveResize = true;

           #if JUCE_WINDOWS
            if (pluginHWND != 0)
                MoveWindow (pluginHWND, pos.getX(), pos.getY(), getWidth(), getHeight(), TRUE);
           #elif JUCE_LINUX
            if (pluginWindow != 0)
            {
                XMoveResizeWindow (display, pluginWindow,
                                   pos.getX(), pos.getY(),
                                   (unsigned int) getWidth(),
                                   (unsigned int) getHeight());

                XMapRaised (display, pluginWindow);
                XFlush (display);
            }
           #endif

            recursiveResize = false;
        }
    }

    void componentVisibilityChanged() override
    {
        if (isShowing())
            openPluginWindow();
        else if (! shouldAvoidDeletingWindow())
            closePluginWindow();

        componentMovedOrResized (true, true);
    }

    void componentPeerChanged() override
    {
        closePluginWindow();
        openPluginWindow();

       #if JUCE_LINUX
        componentMovedOrResized (true, true);
       #endif
    }
   #endif

   #if JUCE_MAC
    void visibilityChanged() override
    {
        if (cocoaWrapper != nullptr)
        {
            if (isVisible())
                openPluginWindow ((NSView*) cocoaWrapper->getView());
            else
                closePluginWindow();
        }
    }

    void childBoundsChanged (Component*) override
    {
        if (cocoaWrapper != nullptr)
        {
            auto w = cocoaWrapper->getWidth();
            auto h = cocoaWrapper->getHeight();

            if (w != getWidth() || h != getHeight())
                setSize (w, h);
        }
    }
   #endif

    //==============================================================================
    bool keyStateChanged (bool) override                 { return pluginWantsKeys; }
    bool keyPressed (const juce::KeyPress&) override     { return pluginWantsKeys; }

    //==============================================================================
   #if JUCE_MAC
    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
    }
   #else
    void paint (Graphics& g) override
    {
        if (isOpen)
        {
           #if JUCE_LINUX
            if (pluginWindow != 0)
            {
                auto clip = g.getClipBounds();

                XClearArea (display, pluginWindow,
                            clip.getX(), clip.getY(),
                            static_cast<unsigned int> (clip.getWidth()),
                            static_cast<unsigned int> (clip.getHeight()),
                            True);
            }
           #endif
        }
        else
        {
            g.fillAll (Colours::black);
        }
    }
   #endif

    //==============================================================================
    void timerCallback() override
    {
        if (isShowing())
        {
           #if JUCE_WINDOWS
            if (--sizeCheckCount <= 0)
            {
                sizeCheckCount = 10;
                checkPluginWindowSize();
            }
           #endif

            static bool reentrantGuard = false;

            if (! reentrantGuard)
            {
                reentrantGuard = true;
                plugin.dispatch (plugInOpcodeEditorIdle, 0, 0, 0, 0);
                reentrantGuard = false;
            }

           #if JUCE_LINUX
            if (pluginWindow == 0)
            {
                updatePluginWindowHandle();

                if (pluginWindow != 0)
                    componentMovedOrResized (true, true);
            }
           #endif
        }
    }

    //==============================================================================
    void mouseDown (const MouseEvent& e) override
    {
        ignoreUnused (e);

       #if JUCE_WINDOWS || JUCE_LINUX
        toFront (true);
       #endif
    }

    void broughtToFront() override
    {
        activeVSTWindows.removeFirstMatchingValue (this);
        activeVSTWindows.add (this);

       #if JUCE_MAC
        dispatch (plugInOpcodeeffEditorTop, 0, 0, 0, 0);
       #endif
    }

    void setScaleFactor (float newScale) override
    {
        scaleFactor = newScale;
        dispatch (plugInOpcodeManufacturerSpecific, presonusVendorID,
                  presonusSetContentScaleFactor, nullptr, newScale);
    }

    void sendScaleFactorIfNotUnity()
    {
        if (scaleFactor != 1.0f)
            setScaleFactor (scaleFactor);
    }

    //==============================================================================
private:
    VSTPluginInstance& plugin;
    float scaleFactor = 1.0f;
    bool isOpen = false, recursiveResize = false;
    bool pluginWantsKeys = false, pluginRefusesToResize = false, alreadyInside = false;

   #if JUCE_WINDOWS
    HWND pluginHWND = {};
    void* originalWndProc = {};
    int sizeCheckCount = 0;
   #elif JUCE_LINUX
    ::Display* display;
    Window pluginWindow;
   #endif

    // This is a workaround for old Mackie plugins that crash if their
    // window is deleted more than once.
    bool shouldAvoidDeletingWindow() const
    {
        return plugin.getPluginDescription()
                .manufacturerName.containsIgnoreCase ("Loud Technologies");
    }

    // This is an old workaround for some plugins that need a repaint when their
    // windows are first created, but it breaks some Izotope plugins..
    bool shouldRepaintCarbonWindowWhenCreated()
    {
        return ! plugin.getName().containsIgnoreCase ("izotope");
    }

    //==============================================================================
#if JUCE_MAC
    void openPluginWindow (void* parentWindow)
    {
        if (isOpen || parentWindow == 0)
            return;

        isOpen = true;

        VstEditorBounds* rect = nullptr;
        dispatch (plugInOpcodeGetEditorBounds, 0, 0, &rect, 0);
        dispatch (plugInOpcodeOpenEditor, 0, 0, parentWindow, 0);
        sendScaleFactorIfNotUnity();

        // do this before and after like in the steinberg example
        dispatch (plugInOpcodeGetEditorBounds, 0, 0, &rect, 0);
        dispatch (plugInOpcodeGetCurrentProgram, 0, 0, 0, 0); // also in steinberg code

        // Install keyboard hooks
        pluginWantsKeys = (dispatch (plugInOpcodeKeyboardFocusRequired, 0, 0, 0, 0) == 0);

        // double-check it's not too tiny
        int w = 250, h = 150;

        if (rect != nullptr)
        {
            w = rect->rightmost - rect->leftmost;
            h = rect->lower - rect->upper;

            if (w == 0 || h == 0)
            {
                w = 250;
                h = 150;
            }
        }

        w = jmax (w, 32);
        h = jmax (h, 32);

        setSize (w, h);

        startTimer (18 + juce::Random::getSystemRandom().nextInt (5));
        repaint();
    }

#else
    void openPluginWindow()
    {
        if (isOpen || getWindowHandle() == 0)
            return;

        JUCE_VST_LOG ("Opening VST UI: " + plugin.getName());
        isOpen = true;

        VstEditorBounds* rect = nullptr;
        dispatch (plugInOpcodeGetEditorBounds, 0, 0, &rect, 0);
        dispatch (plugInOpcodeOpenEditor, 0, 0, getWindowHandle(), 0);
        sendScaleFactorIfNotUnity();

        // do this before and after like in the steinberg example
        dispatch (plugInOpcodeGetEditorBounds, 0, 0, &rect, 0);
        dispatch (plugInOpcodeGetCurrentProgram, 0, 0, 0, 0); // also in steinberg code

        // Install keyboard hooks
        pluginWantsKeys = (dispatch (plugInOpcodeKeyboardFocusRequired, 0, 0, 0, 0) == 0);

       #if JUCE_WINDOWS
        originalWndProc = 0;
        pluginHWND = GetWindow ((HWND) getWindowHandle(), GW_CHILD);

        if (pluginHWND == 0)
        {
            isOpen = false;
            setSize (300, 150);
            return;
        }

        #pragma warning (push)
        #pragma warning (disable: 4244)

        if (! pluginWantsKeys)
        {
            originalWndProc = (void*) GetWindowLongPtr (pluginHWND, GWLP_WNDPROC);
            SetWindowLongPtr (pluginHWND, GWLP_WNDPROC, (LONG_PTR) vstHookWndProc);
        }

        #pragma warning (pop)

        RECT r;
        GetWindowRect (pluginHWND, &r);
        int w = r.right - r.left;
        int h = r.bottom - r.top;

        if (rect != nullptr)
        {
            const int rw = rect->rightmost - rect->leftmost;
            const int rh = rect->lower - rect->upper;

            if ((rw > 50 && rh > 50 && rw < 2000 && rh < 2000 && rw != w && rh != h)
                || ((w == 0 && rw > 0) || (h == 0 && rh > 0)))
            {
                // very dodgy logic to decide which size is right.
                if (abs (rw - w) > 350 || abs (rh - h) > 350)
                {
                    SetWindowPos (pluginHWND, 0,
                                  0, 0, rw, rh,
                                  SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

                    GetWindowRect (pluginHWND, &r);

                    w = r.right - r.left;
                    h = r.bottom - r.top;

                    pluginRefusesToResize = (w != rw) || (h != rh);

                    w = rw;
                    h = rh;
                }
            }
        }

       #elif JUCE_LINUX
        updatePluginWindowHandle();

        int w = 250, h = 150;

        if (rect != nullptr)
        {
            w = rect->rightmost - rect->leftmost;
            h = rect->lower - rect->upper;

            if (w == 0 || h == 0)
            {
                w = 250;
                h = 150;
            }
        }

        if (pluginWindow != 0)
            XMapRaised (display, pluginWindow);
       #endif

        // double-check it's not too tiny
        w = jmax (w, 32);
        h = jmax (h, 32);

        setSize (w, h);

       #if JUCE_WINDOWS
        checkPluginWindowSize();
       #endif

        startTimer (18 + juce::Random::getSystemRandom().nextInt (5));
        repaint();
    }
#endif

    //==============================================================================
    void closePluginWindow()
    {
        if (isOpen)
        {
            // You shouldn't end up hitting this assertion unless the host is trying to do GUI
            // cleanup on a non-GUI thread.. If it does that, bad things could happen in here..
            jassert (MessageManager::getInstance()->currentThreadHasLockedMessageManager());

            JUCE_VST_LOG ("Closing VST UI: " + plugin.getName());
            isOpen = false;
            dispatch (plugInOpcodeCloseEditor, 0, 0, 0, 0);
            stopTimer();

           #if JUCE_WINDOWS
            #pragma warning (push)
            #pragma warning (disable: 4244)
            if (originalWndProc != 0 && pluginHWND != 0 && IsWindow (pluginHWND))
                SetWindowLongPtr (pluginHWND, GWLP_WNDPROC, (LONG_PTR) originalWndProc);
            #pragma warning (pop)

            originalWndProc = 0;
            pluginHWND = 0;
           #elif JUCE_LINUX
            pluginWindow = 0;
           #endif
        }
    }

    //==============================================================================
    pointer_sized_int dispatch (const int opcode, const int index, const int value, void* const ptr, float opt)
    {
        return plugin.dispatch (opcode, index, value, ptr, opt);
    }

    //==============================================================================
   #if JUCE_WINDOWS
    void checkPluginWindowSize()
    {
        RECT r;
        GetWindowRect (pluginHWND, &r);
        auto w = r.right - r.left;
        auto h = r.bottom - r.top;

        if (isShowing() && w > 0 && h > 0
             && (w != getWidth() || h != getHeight())
             && ! pluginRefusesToResize)
        {
            setSize (w, h);
            sizeCheckCount = 0;
        }
    }

    // hooks to get keyboard events from VST windows..
    static LRESULT CALLBACK vstHookWndProc (HWND hW, UINT message, WPARAM wParam, LPARAM lParam)
    {
        for (int i = activeVSTWindows.size(); --i >= 0;)
        {
            Component::SafePointer<VSTPluginWindow> w (activeVSTWindows[i]);

            if (w != nullptr && w->pluginHWND == hW)
            {
                if (message == WM_CHAR
                    || message == WM_KEYDOWN
                    || message == WM_SYSKEYDOWN
                    || message == WM_KEYUP
                    || message == WM_SYSKEYUP
                    || message == WM_APPCOMMAND)
                {
                    SendMessage ((HWND) w->getTopLevelComponent()->getWindowHandle(),
                                 message, wParam, lParam);
                }

                if (w != nullptr) // (may have been deleted in SendMessage callback)
                    return CallWindowProc ((WNDPROC) w->originalWndProc,
                                           (HWND) w->pluginHWND,
                                           message, wParam, lParam);
            }
        }

        return DefWindowProc (hW, message, wParam, lParam);
    }
   #endif

   #if JUCE_LINUX
    void updatePluginWindowHandle()
    {
        pluginWindow = getChildWindow ((Window) getWindowHandle());
    }
   #endif

    //==============================================================================
#if JUCE_MAC
   #if JUCE_SUPPORT_CARBON
    struct CarbonWrapperComponent   : public CarbonViewWrapperComponent
    {
        CarbonWrapperComponent (VSTPluginWindow& w)  : owner (w)
        {
            keepPluginWindowWhenHidden = w.shouldAvoidDeletingWindow();
            setRepaintsChildHIViewWhenCreated (w.shouldRepaintCarbonWindowWhenCreated());
        }

        ~CarbonWrapperComponent()
        {
            deleteWindow();
        }

        HIViewRef attachView (WindowRef windowRef, HIViewRef /*rootView*/) override
        {
            owner.openPluginWindow (windowRef);
            return {};
        }

        void removeView (HIViewRef) override
        {
            if (owner.isOpen)
            {
                owner.isOpen = false;
                owner.dispatch (plugInOpcodeCloseEditor, 0, 0, 0, 0);
                owner.dispatch (plugInOpcodeSleepEditor, 0, 0, 0, 0);
            }
        }

        bool getEmbeddedViewSize (int& w, int& h) override
        {
            VstEditorBounds* rect = nullptr;
            owner.dispatch (plugInOpcodeGetEditorBounds, 0, 0, &rect, 0);
            w = rect->rightmost - rect->leftmost;
            h = rect->lower - rect->upper;
            return true;
        }

        void handleMouseDown (int x, int y) override
        {
            if (! alreadyInside)
            {
                alreadyInside = true;
                getTopLevelComponent()->toFront (true);
                owner.dispatch (plugInOpcodeGetMouse, x, y, 0, 0);
                alreadyInside = false;
            }
            else
            {
                PostEvent (::mouseDown, 0);
            }
        }

        void handlePaint() override
        {
            if (auto* peer = getPeer())
            {
                auto pos = peer->globalToLocal (getScreenPosition());
                VstEditorBounds r;
                r.leftmost  = (int16) pos.getX();
                r.upper     = (int16) pos.getY();
                r.rightmost = (int16) (r.leftmost + getWidth());
                r.lower     = (int16) (r.upper + getHeight());

                owner.dispatch (plugInOpcodeDrawEditor, 0, 0, &r, 0);
            }
        }

    private:
        VSTPluginWindow& owner;
        bool alreadyInside = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CarbonWrapperComponent)
    };

    friend struct CarbonWrapperComponent;
    ScopedPointer<CarbonWrapperComponent> carbonWrapper;
   #endif

    ScopedPointer<AutoResizingNSViewComponentWithParent> cocoaWrapper;

    void resized() override
    {
       #if JUCE_SUPPORT_CARBON
        if (carbonWrapper != nullptr)
            carbonWrapper->setSize (getWidth(), getHeight());
       #endif

        if (cocoaWrapper != nullptr)
            cocoaWrapper->setSize (getWidth(), getHeight());
    }
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTPluginWindow)
};
#endif
#if JUCE_MSVC
 #pragma warning (pop)
#endif

//==============================================================================
AudioProcessorEditor* VSTPluginInstance::createEditor()
{
   #if JUCE_IOS || JUCE_ANDROID
    return nullptr;
   #else
    return hasEditor() ? new VSTPluginWindow (*this)
                       : nullptr;
   #endif
}

//==============================================================================
// entry point for all callbacks from the plugin
static pointer_sized_int VSTINTERFACECALL audioMaster (VstEffectInterface* effect, int32 opcode, int32 index, pointer_sized_int value, void* ptr, float opt)
{
    if (effect != nullptr)
        if (auto* instance = (VSTPluginInstance*) (effect->hostSpace2))
            return instance->handleCallback (opcode, index, value, ptr, opt);

    return VSTPluginInstance::handleGeneralCallback (opcode, index, value, ptr, opt);
}

//==============================================================================
VSTPluginFormat::VSTPluginFormat() {}
VSTPluginFormat::~VSTPluginFormat() {}

static VSTPluginInstance* createAndUpdateDesc (VSTPluginFormat& format, PluginDescription& desc)
{
    if (auto* p = format.createInstanceFromDescription (desc, 44100.0, 512))
    {
        if (auto* instance = dynamic_cast<VSTPluginInstance*> (p))
        {
           #if JUCE_MAC
            if (instance->vstModule->resFileId != 0)
                UseResFile (instance->vstModule->resFileId);
           #endif

            instance->fillInPluginDescription (desc);
            return instance;
        }

        jassertfalse;
    }

    return nullptr;
}

void VSTPluginFormat::findAllTypesForFile (OwnedArray<PluginDescription>& results,
                                           const String& fileOrIdentifier)
{
    if (! fileMightContainThisPluginType (fileOrIdentifier))
        return;

    PluginDescription desc;
    desc.fileOrIdentifier = fileOrIdentifier;
    desc.uid = 0;

    ScopedPointer<VSTPluginInstance> instance (createAndUpdateDesc (*this, desc));

    if (instance == nullptr)
        return;

    if (instance->getVstCategory() != kPlugCategShell)
    {
        // Normal plugin...
        results.add (new PluginDescription (desc));

        instance->dispatch (plugInOpcodeOpen, 0, 0, 0, 0);
    }
    else
    {
        // It's a shell plugin, so iterate all the subtypes...
        for (;;)
        {
            char shellEffectName [256] = { 0 };
            auto uid = (int) instance->dispatch (plugInOpcodeNextPlugInUniqueID, 0, 0, shellEffectName, 0);

            if (uid == 0)
                break;

            desc.uid = uid;
            desc.name = shellEffectName;

            aboutToScanVSTShellPlugin (desc);

            ScopedPointer<VSTPluginInstance> shellInstance (createAndUpdateDesc (*this, desc));

            if (shellInstance != nullptr)
            {
                jassert (desc.uid == uid);
                desc.hasSharedContainer = true;
                desc.name = shellEffectName;

                if (! arrayContainsPlugin (results, desc))
                    results.add (new PluginDescription (desc));
            }
        }
    }
}

void VSTPluginFormat::createPluginInstance (const PluginDescription& desc,
                                            double sampleRate,
                                            int blockSize,
                                            void* userData,
                                            void (*callback) (void*, AudioPluginInstance*, const String&))
{
    ScopedPointer<VSTPluginInstance> result;

    if (fileMightContainThisPluginType (desc.fileOrIdentifier))
    {
        File file (desc.fileOrIdentifier);

        auto previousWorkingDirectory = File::getCurrentWorkingDirectory();
        file.getParentDirectory().setAsCurrentWorkingDirectory();

        if (auto module = ModuleHandle::findOrCreateModule (file))
        {
            shellUIDToCreate = desc.uid;

            result = VSTPluginInstance::create (module, sampleRate, blockSize);

            if (result != nullptr && ! result->initialiseEffect (sampleRate, blockSize))
                result = nullptr;
        }

        previousWorkingDirectory.setAsCurrentWorkingDirectory();
    }

    String errorMsg;

    if (result == nullptr)
        errorMsg = String (NEEDS_TRANS ("Unable to load XXX plug-in file")).replace ("XXX", "VST-2");

    callback (userData, result.release(), errorMsg);
}

bool VSTPluginFormat::requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept
{
    return false;
}

bool VSTPluginFormat::fileMightContainThisPluginType (const String& fileOrIdentifier)
{
    auto f = File::createFileWithoutCheckingPath (fileOrIdentifier);

  #if JUCE_MAC || JUCE_IOS
    return f.isDirectory() && f.hasFileExtension (".vst");
  #elif JUCE_WINDOWS
    return f.existsAsFile() && f.hasFileExtension (".dll");
  #elif JUCE_LINUX || JUCE_ANDROID
    return f.existsAsFile() && f.hasFileExtension (".so");
  #endif
}

String VSTPluginFormat::getNameOfPluginFromIdentifier (const String& fileOrIdentifier)
{
    return fileOrIdentifier;
}

bool VSTPluginFormat::pluginNeedsRescanning (const PluginDescription& desc)
{
    return File (desc.fileOrIdentifier).getLastModificationTime() != desc.lastFileModTime;
}

bool VSTPluginFormat::doesPluginStillExist (const PluginDescription& desc)
{
    return File (desc.fileOrIdentifier).exists();
}

StringArray VSTPluginFormat::searchPathsForPlugins (const FileSearchPath& directoriesToSearch, const bool recursive, bool)
{
    StringArray results;

    for (int j = 0; j < directoriesToSearch.getNumPaths(); ++j)
        recursiveFileSearch (results, directoriesToSearch [j], recursive);

    return results;
}

void VSTPluginFormat::recursiveFileSearch (StringArray& results, const File& dir, const bool recursive)
{
    // avoid allowing the dir iterator to be recursive, because we want to avoid letting it delve inside
    // .component or .vst directories.
    DirectoryIterator iter (dir, false, "*", File::findFilesAndDirectories);

    while (iter.next())
    {
        auto f = iter.getFile();
        bool isPlugin = false;

        if (fileMightContainThisPluginType (f.getFullPathName()))
        {
            isPlugin = true;
            results.add (f.getFullPathName());
        }

        if (recursive && (! isPlugin) && f.isDirectory())
            recursiveFileSearch (results, f, true);
    }
}

FileSearchPath VSTPluginFormat::getDefaultLocationsToSearch()
{
   #if JUCE_MAC
    return FileSearchPath ("~/Library/Audio/Plug-Ins/VST;/Library/Audio/Plug-Ins/VST");
   #elif JUCE_LINUX || JUCE_ANDROID
    return FileSearchPath (SystemStats::getEnvironmentVariable ("VST_PATH",
                                                                "/usr/lib/vst;/usr/local/lib/vst;~/.vst")
                             .replace (":", ";"));
   #elif JUCE_WINDOWS
    auto programFiles = File::getSpecialLocation (File::globalApplicationsDirectory).getFullPathName();

    FileSearchPath paths;
    paths.add (WindowsRegistry::getValue ("HKEY_LOCAL_MACHINE\\Software\\VST\\VSTPluginsPath"));
    paths.addIfNotAlreadyThere (programFiles + "\\Steinberg\\VstPlugins");
    paths.removeNonExistentPaths();
    paths.addIfNotAlreadyThere (programFiles + "\\VstPlugins");
    paths.removeRedundantPaths();
    return paths;
   #elif JUCE_IOS
    // on iOS you can only load plug-ins inside the hosts bundle folder
    CFURLRef relativePluginDir = CFBundleCopyBuiltInPlugInsURL (CFBundleGetMainBundle());
    CFURLRef pluginDir = CFURLCopyAbsoluteURL (relativePluginDir);
    CFRelease (relativePluginDir);

    CFStringRef path = CFURLCopyFileSystemPath (pluginDir, kCFURLPOSIXPathStyle);
    CFRelease (pluginDir);

    FileSearchPath retval (String (CFStringGetCStringPtr (path, kCFStringEncodingUTF8)));
    CFRelease (path);

    return retval;
   #endif
}

const XmlElement* VSTPluginFormat::getVSTXML (AudioPluginInstance* plugin)
{
    if (auto* vst = dynamic_cast<VSTPluginInstance*> (plugin))
        if (vst->vstModule != nullptr)
            return vst->vstModule->vstXml.get();

    return nullptr;
}

bool VSTPluginFormat::loadFromFXBFile (AudioPluginInstance* plugin, const void* data, size_t dataSize)
{
    if (auto* vst = dynamic_cast<VSTPluginInstance*> (plugin))
        return vst->loadFromFXBFile (data, dataSize);

    return false;
}

bool VSTPluginFormat::saveToFXBFile (AudioPluginInstance* plugin, MemoryBlock& dest, bool asFXB)
{
    if (auto* vst = dynamic_cast<VSTPluginInstance*> (plugin))
        return vst->saveToFXBFile (dest, asFXB);

    return false;
}

bool VSTPluginFormat::getChunkData (AudioPluginInstance* plugin, MemoryBlock& result, bool isPreset)
{
    if (auto* vst = dynamic_cast<VSTPluginInstance*> (plugin))
        return vst->getChunkData (result, isPreset, 128);

    return false;
}

bool VSTPluginFormat::setChunkData (AudioPluginInstance* plugin, const void* data, int size, bool isPreset)
{
    if (auto* vst = dynamic_cast<VSTPluginInstance*> (plugin))
        return vst->setChunkData (data, size, isPreset);

    return false;
}

AudioPluginInstance* VSTPluginFormat::createCustomVSTFromMainCall (void* entryPointFunction,
                                                                   double initialSampleRate, int initialBufferSize)
{
    ModuleHandle::Ptr module = new ModuleHandle (File(), (MainCall) entryPointFunction);

    if (module->open())
        if (ScopedPointer<VSTPluginInstance> result = VSTPluginInstance::create (module, initialSampleRate, initialBufferSize))
            if (result->initialiseEffect (initialSampleRate, initialBufferSize))
                return result.release();

    return nullptr;
}

void VSTPluginFormat::setExtraFunctions (AudioPluginInstance* plugin, ExtraFunctions* functions)
{
    ScopedPointer<ExtraFunctions> f (functions);

    if (auto* vst = dynamic_cast<VSTPluginInstance*> (plugin))
        vst->extraFunctions = f;
}

AudioPluginInstance* VSTPluginFormat::getPluginInstanceFromVstEffectInterface (void* aEffect)
{
    if (auto* vstAEffect = reinterpret_cast<VstEffectInterface*> (aEffect))
        if (auto* instanceVST = reinterpret_cast<VSTPluginInstance*> (vstAEffect->hostSpace2))
            return dynamic_cast<AudioPluginInstance*> (instanceVST);

    return nullptr;
}

pointer_sized_int JUCE_CALLTYPE VSTPluginFormat::dispatcher (AudioPluginInstance* plugin, int32 opcode, int32 index, pointer_sized_int value, void* ptr, float opt)
{
    if (auto* vst = dynamic_cast<VSTPluginInstance*> (plugin))
        return vst->dispatch (opcode, index, value, ptr, opt);

    return {};
}

void VSTPluginFormat::aboutToScanVSTShellPlugin (const PluginDescription&) {}

} // namespace juce

#endif
