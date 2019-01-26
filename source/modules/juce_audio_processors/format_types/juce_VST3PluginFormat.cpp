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

#if JUCE_PLUGINHOST_VST3 && (JUCE_MAC || JUCE_WINDOWS)

#include <map>
#include "juce_VST3Headers.h"
#include "juce_VST3Common.h"

namespace juce
{

using namespace Steinberg;

//==============================================================================
struct VST3Classes
{

#ifndef JUCE_VST3_DEBUGGING
 #define JUCE_VST3_DEBUGGING 0
#endif

#if JUCE_VST3_DEBUGGING
 #define VST3_DBG(a) Logger::writeToLog (a);
#else
 #define VST3_DBG(a)
#endif

#if JUCE_DEBUG
static int warnOnFailure (int result) noexcept
{
    const char* message = "Unknown result!";

    switch (result)
    {
        case kResultOk:         return result;
        case kNotImplemented:   message = "kNotImplemented";  break;
        case kNoInterface:      message = "kNoInterface";     break;
        case kResultFalse:      message = "kResultFalse";     break;
        case kInvalidArgument:  message = "kInvalidArgument"; break;
        case kInternalError:    message = "kInternalError";   break;
        case kNotInitialized:   message = "kNotInitialized";  break;
        case kOutOfMemory:      message = "kOutOfMemory";     break;
        default:                break;
    }

    DBG (message);
    return result;
}
#else
 #define warnOnFailure(x) x
#endif

//==============================================================================
static int getHashForTUID (const TUID& tuid) noexcept
{
    int value = 0;

    for (int i = 0; i < numElementsInArray (tuid); ++i)
        value = (value * 31) + tuid[i];

    return value;
}

template <typename ObjectType>
static void fillDescriptionWith (PluginDescription& description, ObjectType& object)
{
    description.version  = toString (object.version).trim();
    description.category = toString (object.subCategories).trim();

    if (description.manufacturerName.trim().isEmpty())
        description.manufacturerName = toString (object.vendor).trim();
}

static void createPluginDescription (PluginDescription& description,
                                     const File& pluginFile, const String& company, const String& name,
                                     const PClassInfo& info, PClassInfo2* info2, PClassInfoW* infoW,
                                     int numInputs, int numOutputs)
{
    description.fileOrIdentifier    = pluginFile.getFullPathName();
    description.lastFileModTime     = pluginFile.getLastModificationTime();
    description.lastInfoUpdateTime  = Time::getCurrentTime();
    description.manufacturerName    = company;
    description.name                = name;
    description.descriptiveName     = name;
    description.pluginFormatName    = "VST3";
    description.numInputChannels    = numInputs;
    description.numOutputChannels   = numOutputs;
    description.uid                 = getHashForTUID (info.cid);

    if (infoW != nullptr)      fillDescriptionWith (description, *infoW);
    else if (info2 != nullptr) fillDescriptionWith (description, *info2);

    if (description.category.isEmpty())
        description.category = toString (info.category).trim();

    description.isInstrument = description.category.containsIgnoreCase ("Instrument"); // This seems to be the only way to find that out! ARGH!
}

static int getNumSingleDirectionBusesFor (Vst::IComponent* component,
                                           bool checkInputs,
                                           bool checkAudioChannels)
{
    jassert (component != nullptr);

    return (int) component->getBusCount (checkAudioChannels ? Vst::kAudio : Vst::kEvent,
                                         checkInputs ? Vst::kInput : Vst::kOutput);
}

/** Gives the total number of channels for a particular type of bus direction and media type */
static int getNumSingleDirectionChannelsFor (Vst::IComponent* component,
                                             bool checkInputs,
                                             bool checkAudioChannels)
{
    jassert (component != nullptr);

    const Vst::BusDirections direction  = checkInputs ? Vst::kInput : Vst::kOutput;
    const Vst::MediaTypes mediaType     = checkAudioChannels ? Vst::kAudio : Vst::kEvent;
    const Steinberg::int32 numBuses     = component->getBusCount (mediaType, direction);

    int numChannels = 0;

    for (Steinberg::int32 i = numBuses; --i >= 0;)
    {
        Vst::BusInfo busInfo;
        warnOnFailure (component->getBusInfo (mediaType, direction, i, busInfo));
        numChannels += ((busInfo.flags & Vst::BusInfo::kDefaultActive) != 0 ? (int) busInfo.channelCount : 0);
    }

    return numChannels;
}

static void setStateForAllBusesOfType (Vst::IComponent* component,
                                       bool state,
                                       bool activateInputs,
                                       bool activateAudioChannels)
{
    jassert (component != nullptr);

    const Vst::BusDirections direction  = activateInputs ? Vst::kInput : Vst::kOutput;
    const Vst::MediaTypes mediaType     = activateAudioChannels ? Vst::kAudio : Vst::kEvent;
    const Steinberg::int32 numBuses     = component->getBusCount (mediaType, direction);

    for (Steinberg::int32 i = numBuses; --i >= 0;)
        warnOnFailure (component->activateBus (mediaType, direction, i, state));
}

//==============================================================================
/** Assigns a complete AudioSampleBuffer's channels to an AudioBusBuffers' */
static void associateWholeBufferTo (Vst::AudioBusBuffers& vstBuffers, AudioSampleBuffer& buffer) noexcept
{
    vstBuffers.channelBuffers32 = buffer.getArrayOfWritePointers();
    vstBuffers.numChannels      = buffer.getNumChannels();
    vstBuffers.silenceFlags     = 0;
}

//==============================================================================
static void toProcessContext (Vst::ProcessContext& context, AudioPlayHead* playHead, double sampleRate)
{
    jassert (sampleRate > 0.0); //Must always be valid, as stated by the VST3 SDK

    using namespace Vst;

    zerostruct (context);
    context.sampleRate = sampleRate;
    auto& fr = context.frameRate;

    if (playHead != nullptr)
    {
        AudioPlayHead::CurrentPositionInfo position;
        playHead->getCurrentPosition (position);

        context.projectTimeSamples  = position.timeInSamples; //Must always be valid, as stated by the VST3 SDK
        context.projectTimeMusic    = position.timeInSeconds; //Does not always need to be valid...
        context.tempo               = position.bpm;
        context.timeSigNumerator    = position.timeSigNumerator;
        context.timeSigDenominator  = position.timeSigDenominator;
        context.barPositionMusic    = position.ppqPositionOfLastBarStart;
        context.cycleStartMusic     = position.ppqLoopStart;
        context.cycleEndMusic       = position.ppqLoopEnd;

        switch (position.frameRate)
        {
            case AudioPlayHead::fps23976:    fr.framesPerSecond = 24; fr.flags = FrameRate::kPullDownRate; break;
            case AudioPlayHead::fps24:       fr.framesPerSecond = 24; fr.flags = 0; break;
            case AudioPlayHead::fps25:       fr.framesPerSecond = 25; fr.flags = 0; break;
            case AudioPlayHead::fps2997:     fr.framesPerSecond = 30; fr.flags = FrameRate::kPullDownRate; break;
            case AudioPlayHead::fps2997drop: fr.framesPerSecond = 30; fr.flags = FrameRate::kPullDownRate | FrameRate::kDropRate; break;
            case AudioPlayHead::fps30:       fr.framesPerSecond = 30; fr.flags = 0; break;
            case AudioPlayHead::fps30drop:   fr.framesPerSecond = 30; fr.flags = FrameRate::kDropRate; break;
            case AudioPlayHead::fps60:       fr.framesPerSecond = 60; fr.flags = 0; break;
            case AudioPlayHead::fps60drop:   fr.framesPerSecond = 60; fr.flags = FrameRate::kDropRate; break;
            case AudioPlayHead::fpsUnknown:  break;
            default:                         jassertfalse; break; // New frame rate?
        }

        if (position.isPlaying)     context.state |= ProcessContext::kPlaying;
        if (position.isRecording)   context.state |= ProcessContext::kRecording;
        if (position.isLooping)     context.state |= ProcessContext::kCycleActive;
    }
    else
    {
        context.tempo               = 120.0;
        context.timeSigNumerator    = 4;
        context.timeSigDenominator  = 4;
        fr.framesPerSecond          = 30;
        fr.flags                    = 0;
    }

    if (context.projectTimeMusic >= 0.0)        context.state |= ProcessContext::kProjectTimeMusicValid;
    if (context.barPositionMusic >= 0.0)        context.state |= ProcessContext::kBarPositionValid;
    if (context.tempo > 0.0)                    context.state |= ProcessContext::kTempoValid;
    if (context.frameRate.framesPerSecond > 0)  context.state |= ProcessContext::kSmpteValid;

    if (context.cycleStartMusic >= 0.0
         && context.cycleEndMusic > 0.0
         && context.cycleEndMusic > context.cycleStartMusic)
    {
        context.state |= ProcessContext::kCycleValid;
    }

    if (context.timeSigNumerator > 0 && context.timeSigDenominator > 0)
        context.state |= ProcessContext::kTimeSigValid;
}

//==============================================================================
/** Get a list of speaker arrangements as per their speaker names

    (e.g.: 2 regular channels, aliased as 'kStringStereoS', is "L R")
*/
static StringArray getSpeakerArrangements()
{
    using namespace Vst::SpeakerArr;

    const Vst::CString arrangements[] =
    {
        kStringMonoS,       kStringStereoS,         kStringStereoRS,    kStringStereoCS,
        kStringStereoSS,    kStringStereoCLfeS,     kString30CineS,     kString30MusicS,
        kString31CineS,     kString31MusicS,        kString40CineS,     kString40MusicS,
        kString41CineS,     kString41MusicS,        kString50S,         kString51S,
        kString60CineS,     kString60MusicS,        kString61CineS,     kString61MusicS,
        kString70CineS,     kString70MusicS,        kString71CineS,     kString71MusicS,
        kString80CineS,     kString80MusicS,        kString81CineS,     kString81MusicS,
        kString80CubeS,             kStringBFormat1stOrderS,    kString71CineTopCenterS,
        kString71CineCenterHighS,   kString71CineFrontHighS,    kString71CineSideHighS,
        kString71CineFullRearS,     kString90S,                 kString91S,
        kString100S,        kString101S,            kString110S,        kString111S,
        kString130S,        kString131S,            kString102S,        kString122S,
        nullptr
    };

    return StringArray (arrangements);
}

/** Get a list of speaker arrangements as per their named configurations

    (e.g.: 2 regular channels, aliased as 'kStringStereoS', is "L R")
*/
static StringArray getNamedSpeakerArrangements()
{
    using namespace Vst::SpeakerArr;

    const Vst::CString arrangements[] =
    {
        kStringEmpty,       kStringMono,            kStringStereo,              kStringStereoR,
        kStringStereoC,     kStringStereoSide,      kStringStereoCLfe,          kString30Cine,
        kString30Music,     kString31Cine,          kString31Music,             kString40Cine,
        kString40Music,     kString41Cine,          kString41Music,             kString50,
        kString51,          kString60Cine,          kString60Music,             kString61Cine,
        kString61Music,     kString70Cine,          kString70Music,             kString71Cine,
        kString71Music,     kString71CineTopCenter, kString71CineCenterHigh,
        kString71CineFrontHigh,         kString71CineSideHigh,          kString71CineFullRear,
        kString80Cine,      kString80Music,         kString80Cube,              kString81Cine,
        kString81Music,     kString102,             kString122,                 kString90,
        kString91,          kString100,             kString101,                 kString110,
        kString111,         kString130,             kString131,
        nullptr
    };

    return StringArray (arrangements);
}

static Vst::SpeakerArrangement getSpeakerArrangementFrom (const String& string)
{
    return Vst::SpeakerArr::getSpeakerArrangementFromString (string.toUTF8());
}

//==============================================================================
static StringArray getPluginEffectCategories()
{
    using namespace Vst::PlugType;

    const Vst::CString categories[] =
    {
        kFxAnalyzer,            kFxDelay,       kFxDistortion,      kFxDynamics,
        kFxEQ,                  kFxFilter,      kFx,                kFxInstrument,
        kFxInstrumentExternal,  kFxSpatial,     kFxGenerator,       kFxMastering,
        kFxModulation,          kFxPitchShift,  kFxRestoration,     kFxReverb,
        kFxSurround,            kFxTools,       kSpatial,           kSpatialFx,
        nullptr
    };

    return StringArray (categories);
}

static StringArray getPluginInstrumentCategories()
{
    using namespace Vst::PlugType;

    const Vst::CString categories[] =
    {
        kInstrumentSynthSampler,    kInstrumentDrum,
        kInstrumentSampler,         kInstrumentSynth,
        kInstrumentExternal,        kFxInstrument,
        kFxInstrumentExternal,      kFxSpatial,
        kFxGenerator,
        nullptr
    };

    return StringArray (categories);
}

//==============================================================================
struct VST3PluginInstance;

struct VST3HostContext  : public Vst::IComponentHandler,  // From VST V3.0.0
                          public Vst::IComponentHandler2, // From VST V3.1.0 (a very well named class, of course!)
                          public Vst::IComponentHandler3, // From VST V3.5.0 (also very well named!)
                          public Vst::IContextMenuTarget,
                          public Vst::IHostApplication,
                          public Vst::IUnitHandler
{
    VST3HostContext()
    {
        appName = File::getSpecialLocation (File::currentApplicationFile).getFileNameWithoutExtension();
        attributeList = new AttributeList (this);
    }

    virtual ~VST3HostContext() {}

    JUCE_DECLARE_VST3_COM_REF_METHODS

    FUnknown* getFUnknown()     { return static_cast<Vst::IComponentHandler*> (this); }

    static bool hasFlag (Steinberg::int32 source, Steinberg::int32 flag) noexcept
    {
        return (source & flag) == flag;
    }

    //==============================================================================
    tresult PLUGIN_API beginEdit (Vst::ParamID paramID) override
    {
        if (plugin != nullptr)
        {
            auto index = getIndexOfParamID (paramID);

            if (index < 0)
                return kResultFalse;

            plugin->beginParameterChangeGesture (index);
        }

        return kResultTrue;
    }

    tresult PLUGIN_API performEdit (Vst::ParamID paramID, Vst::ParamValue valueNormalized) override
    {
        if (plugin != nullptr)
        {
            auto index = getIndexOfParamID (paramID);

            if (index < 0)
                return kResultFalse;

            plugin->sendParamChangeMessageToListeners (index, (float) valueNormalized);

            {
                Steinberg::int32 eventIndex;
                plugin->inputParameterChanges->addParameterData (paramID, eventIndex)->addPoint (0, valueNormalized, eventIndex);
            }

            // did the plug-in already update the parameter internally
            if (plugin->editController->getParamNormalized (paramID) != (float) valueNormalized)
                return plugin->editController->setParamNormalized (paramID, valueNormalized);
        }

        return kResultTrue;
    }

    tresult PLUGIN_API endEdit (Vst::ParamID paramID) override
    {
        if (plugin != nullptr)
        {
            auto index = getIndexOfParamID (paramID);

            if (index < 0)
                return kResultFalse;

            plugin->endParameterChangeGesture (index);
        }

        return kResultTrue;
    }

    tresult PLUGIN_API restartComponent (Steinberg::int32 flags) override
    {
        if (plugin != nullptr)
        {
            if (hasFlag (flags, Vst::kReloadComponent))
                plugin->reset();

            if (hasFlag (flags, Vst::kIoChanged))
            {
                auto sampleRate = plugin->getSampleRate();
                auto blockSize  = plugin->getBlockSize();

                plugin->prepareToPlay (sampleRate >= 8000 ? sampleRate : 44100.0,
                                       blockSize > 0 ? blockSize : 1024);
            }

            if (hasFlag (flags, Vst::kLatencyChanged))
                if (plugin->processor != nullptr)
                    plugin->setLatencySamples (jmax (0, (int) plugin->processor->getLatencySamples()));

            plugin->updateHostDisplay();
            return kResultTrue;
        }

        jassertfalse;
        return kResultFalse;
    }

    //==============================================================================
    tresult PLUGIN_API setDirty (TBool) override
    {
        return kResultFalse;
    }

    tresult PLUGIN_API requestOpenEditor (FIDString name) override
    {
        ignoreUnused (name);
        jassertfalse;
        return kResultFalse;
    }

    tresult PLUGIN_API startGroupEdit() override
    {
        jassertfalse;
        return kResultFalse;
    }

    tresult PLUGIN_API finishGroupEdit() override
    {
        jassertfalse;
        return kResultFalse;
    }

    void setPlugin (VST3PluginInstance* instance)
    {
        jassert (plugin == nullptr);
        plugin = instance;
    }

    //==============================================================================
    struct ContextMenu  : public Vst::IContextMenu
    {
        ContextMenu (VST3PluginInstance& pluginInstance)  : owner (pluginInstance) {}
        virtual ~ContextMenu() {}

        JUCE_DECLARE_VST3_COM_REF_METHODS
        JUCE_DECLARE_VST3_COM_QUERY_METHODS

        Steinberg::int32 PLUGIN_API getItemCount() override     { return (Steinberg::int32) items.size(); }

        tresult PLUGIN_API addItem (const Item& item, IContextMenuTarget* target) override
        {
            jassert (target != nullptr);

            ItemAndTarget newItem;
            newItem.item = item;
            newItem.target = target;

            items.add (newItem);
            return kResultOk;
        }

        tresult PLUGIN_API removeItem (const Item& toRemove, IContextMenuTarget* target) override
        {
            for (int i = items.size(); --i >= 0;)
            {
                auto& item = items.getReference(i);

                if (item.item.tag == toRemove.tag && item.target == target)
                    items.remove (i);
            }

            return kResultOk;
        }

        tresult PLUGIN_API getItem (Steinberg::int32 tag, Item& result, IContextMenuTarget** target) override
        {
            for (int i = 0; i < items.size(); ++i)
            {
                auto& item = items.getReference(i);

                if (item.item.tag == tag)
                {
                    result = item.item;

                    if (target != nullptr)
                        *target = item.target;

                    return kResultTrue;
                }
            }

            zerostruct (result);
            return kResultFalse;
        }

        tresult PLUGIN_API popup (Steinberg::UCoord x, Steinberg::UCoord y) override
        {
            Array<const Item*> subItemStack;
            OwnedArray<PopupMenu> menuStack;
            PopupMenu* topLevelMenu = menuStack.add (new PopupMenu());

            for (int i = 0; i < items.size(); ++i)
            {
                auto& item = items.getReference (i).item;
                auto* menuToUse = menuStack.getLast();

                if (hasFlag (item.flags, Item::kIsGroupStart & ~Item::kIsDisabled))
                {
                    subItemStack.add (&item);
                    menuStack.add (new PopupMenu());
                }
                else if (hasFlag (item.flags, Item::kIsGroupEnd))
                {
                    if (auto* subItem = subItemStack.getLast())
                    {
                        if (auto* m = menuStack [menuStack.size() - 2])
                            m->addSubMenu (toString (subItem->name), *menuToUse,
                                           ! hasFlag (subItem->flags, Item::kIsDisabled),
                                           nullptr,
                                           hasFlag (subItem->flags, Item::kIsChecked));

                        menuStack.removeLast (1);
                        subItemStack.removeLast (1);
                    }
                }
                else if (hasFlag (item.flags, Item::kIsSeparator))
                {
                    menuToUse->addSeparator();
                }
                else
                {
                    menuToUse->addItem (item.tag != 0 ? (int) item.tag : (int) zeroTagReplacement,
                                        toString (item.name),
                                        ! hasFlag (item.flags, Item::kIsDisabled),
                                        hasFlag (item.flags, Item::kIsChecked));
                }
            }

            PopupMenu::Options options;

            if (auto* ed = owner.getActiveEditor())
                options = options.withTargetScreenArea (ed->getScreenBounds().translated ((int) x, (int) y).withSize (1, 1));

           #if JUCE_MODAL_LOOPS_PERMITTED
            // Unfortunately, Steinberg's docs explicitly say this should be modal..
            handleResult (topLevelMenu->showMenu (options));
           #else
            topLevelMenu->showMenuAsync (options, ModalCallbackFunction::create (menuFinished, ComSmartPtr<ContextMenu> (this)));
           #endif

            return kResultOk;
        }

       #if ! JUCE_MODAL_LOOPS_PERMITTED
        static void menuFinished (int modalResult, ComSmartPtr<ContextMenu> menu)  { menu->handleResult (modalResult); }
       #endif

    private:
        enum { zeroTagReplacement = 0x7fffffff };

        Atomic<int> refCount;
        VST3PluginInstance& owner;

        struct ItemAndTarget
        {
            Item item;
            ComSmartPtr<IContextMenuTarget> target;
        };

        Array<ItemAndTarget> items;

        void handleResult (int result)
        {
            if (result == 0)
                return;

            if (result == zeroTagReplacement)
                result = 0;

            for (int i = 0; i < items.size(); ++i)
            {
                auto& item = items.getReference(i);

                if ((int) item.item.tag == result)
                {
                    if (item.target != nullptr)
                        item.target->executeMenuItem ((Steinberg::int32) result);

                    break;
                }
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContextMenu)
    };

    Vst::IContextMenu* PLUGIN_API createContextMenu (IPlugView*, const Vst::ParamID*) override
    {
        if (plugin != nullptr)
            return new ContextMenu (*plugin);

        return nullptr;
    }

    tresult PLUGIN_API executeMenuItem (Steinberg::int32) override
    {
        jassertfalse;
        return kResultFalse;
    }

    //==============================================================================
    tresult PLUGIN_API getName (Vst::String128 name) override
    {
        Steinberg::String str (appName.toUTF8());
        str.copyTo (name, 0, 127);
        return kResultOk;
    }

    tresult PLUGIN_API createInstance (TUID cid, TUID iid, void** obj) override
    {
        *obj = nullptr;

        if (! doUIDsMatch (cid, iid))
        {
            jassertfalse;
            return kInvalidArgument;
        }

        if (doUIDsMatch (cid, Vst::IMessage::iid) && doUIDsMatch (iid, Vst::IMessage::iid))
        {
            ComSmartPtr<Message> m (new Message (attributeList));
            messageQueue.add (m);
            m->addRef();
            *obj = m;
            return kResultOk;
        }
        else if (doUIDsMatch (cid, Vst::IAttributeList::iid) && doUIDsMatch (iid, Vst::IAttributeList::iid))
        {
            ComSmartPtr<AttributeList> l (new AttributeList (this));
            l->addRef();
            *obj = l;
            return kResultOk;
        }

        jassertfalse;
        return kNotImplemented;
    }

    //==============================================================================
    tresult PLUGIN_API notifyUnitSelection (Vst::UnitID) override
    {
        jassertfalse;
        return kResultFalse;
    }

    tresult PLUGIN_API notifyProgramListChange (Vst::ProgramListID, Steinberg::int32) override
    {
        if (plugin != nullptr)
            plugin->syncProgramNames();

        return kResultTrue;
    }

    //==============================================================================
    tresult PLUGIN_API queryInterface (const TUID iid, void** obj) override
    {
        if (doUIDsMatch (iid, Vst::IAttributeList::iid))
        {
            *obj = attributeList.get();
            return kResultOk;
        }

        TEST_FOR_AND_RETURN_IF_VALID (iid, Vst::IComponentHandler)
        TEST_FOR_AND_RETURN_IF_VALID (iid, Vst::IComponentHandler2)
        TEST_FOR_AND_RETURN_IF_VALID (iid, Vst::IComponentHandler3)
        TEST_FOR_AND_RETURN_IF_VALID (iid, Vst::IContextMenuTarget)
        TEST_FOR_AND_RETURN_IF_VALID (iid, Vst::IHostApplication)
        TEST_FOR_AND_RETURN_IF_VALID (iid, Vst::IUnitHandler)
        TEST_FOR_COMMON_BASE_AND_RETURN_IF_VALID (iid, FUnknown, Vst::IComponentHandler)

        *obj = nullptr;
        return kNotImplemented;
    }

private:
    //==============================================================================
    VST3PluginInstance* plugin = nullptr;
    Atomic<int> refCount;
    String appName;

    typedef std::map<Vst::ParamID, int> ParamMapType;
    ParamMapType paramToIndexMap;

    int getIndexOfParamID (Vst::ParamID paramID)
    {
        if (plugin == nullptr || plugin->editController == nullptr)
            return -1;

        auto result = getMappedParamID (paramID);

        if (result < 0)
        {
            auto numParams = plugin->editController->getParameterCount();

            for (int i = 0; i < numParams; ++i)
            {
                Vst::ParameterInfo paramInfo;
                plugin->editController->getParameterInfo (i, paramInfo);
                paramToIndexMap[paramInfo.id] = i;
            }

            result = getMappedParamID (paramID);
        }

        return result;
    }

    int getMappedParamID (Vst::ParamID paramID)
    {
        auto it = paramToIndexMap.find (paramID);
        return it != paramToIndexMap.end() ? it->second : -1;
    }

    //==============================================================================
    struct Message  : public Vst::IMessage
    {
        Message (Vst::IAttributeList* list)
           : attributeList (list)
        {
        }

        Message (Vst::IAttributeList* list, FIDString id)
           : attributeList (list), messageId (toString (id))
        {
        }

        Message (Vst::IAttributeList* list, FIDString id, const var& v)
           : value (v), attributeList (list), messageId (toString (id))
        {
        }

        virtual ~Message() {}

        JUCE_DECLARE_VST3_COM_REF_METHODS
        JUCE_DECLARE_VST3_COM_QUERY_METHODS

        FIDString PLUGIN_API getMessageID() override              { return messageId.toRawUTF8(); }
        void PLUGIN_API setMessageID (FIDString id) override      { messageId = toString (id); }
        Vst::IAttributeList* PLUGIN_API getAttributes() override  { return attributeList; }

        var value;

    private:
        ComSmartPtr<Vst::IAttributeList> attributeList;
        String messageId;
        Atomic<int> refCount;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Message)
    };

    Array<ComSmartPtr<Message>, CriticalSection> messageQueue;

    //==============================================================================
    struct AttributeList  : public Vst::IAttributeList
    {
        AttributeList (VST3HostContext* o)  : owner (o) {}
        virtual ~AttributeList() {}

        JUCE_DECLARE_VST3_COM_REF_METHODS
        JUCE_DECLARE_VST3_COM_QUERY_METHODS

        //==============================================================================
        tresult PLUGIN_API setInt (AttrID id, Steinberg::int64 value) override
        {
            addMessageToQueue (id, value);
            return kResultTrue;
        }

        tresult PLUGIN_API setFloat (AttrID id, double value) override
        {
            addMessageToQueue (id, value);
            return kResultTrue;
        }

        tresult PLUGIN_API setString (AttrID id, const Vst::TChar* string) override
        {
            addMessageToQueue (id, toString (string));
            return kResultTrue;
        }

        tresult PLUGIN_API setBinary (AttrID id, const void* data, Steinberg::uint32 size) override
        {
            jassert (size >= 0 && (data != nullptr || size == 0));
            addMessageToQueue (id, MemoryBlock (data, (size_t) size));
            return kResultTrue;
        }

        //==============================================================================
        tresult PLUGIN_API getInt (AttrID id, Steinberg::int64& result) override
        {
            jassert (id != nullptr);

            if (findMessageOnQueueWithID (id, result))
                return kResultTrue;

            jassertfalse;
            return kResultFalse;
        }

        tresult PLUGIN_API getFloat (AttrID id, double& result) override
        {
            jassert (id != nullptr);

            if (findMessageOnQueueWithID (id, result))
                return kResultTrue;

            jassertfalse;
            return kResultFalse;
        }

        tresult PLUGIN_API getString (AttrID id, Vst::TChar* result, Steinberg::uint32 length) override
        {
            jassert (id != nullptr);

            String stringToFetch;
            if (findMessageOnQueueWithID (id, stringToFetch))
            {
                Steinberg::String str (stringToFetch.toRawUTF8());
                str.copyTo (result, 0, (Steinberg::int32) jmin (length, (Steinberg::uint32) std::numeric_limits<Steinberg::int32>::max()));

                return kResultTrue;
            }

            jassertfalse;
            return kResultFalse;
        }

        tresult PLUGIN_API getBinary (AttrID id, const void*& data, Steinberg::uint32& size) override
        {
            jassert (id != nullptr);

            for (auto&& m : owner->messageQueue)
            {
                if (std::strcmp (m->getMessageID(), id) == 0)
                {
                    if (auto* binaryData = m->value.getBinaryData())
                    {
                        data = binaryData->getData();
                        size = (Steinberg::uint32) binaryData->getSize();
                        return kResultTrue;
                    }
                }
            }

            return kResultFalse;
        }

    private:
        VST3HostContext* owner;
        Atomic<int> refCount;

        //==============================================================================
        template <typename Type>
        void addMessageToQueue (AttrID id, const Type& value)
        {
            jassert (id != nullptr);

            for (auto&& m : owner->messageQueue)
            {
                if (std::strcmp (m->getMessageID(), id) == 0)
                {
                    m->value = value;
                    return;
                }
            }

            owner->messageQueue.add (ComSmartPtr<Message> (new Message (this, id, value)));
        }

        template <typename Type>
        bool findMessageOnQueueWithID (AttrID id, Type& value)
        {
            jassert (id != nullptr);

            for (auto&& m : owner->messageQueue)
            {
                if (std::strcmp (m->getMessageID(), id) == 0)
                {
                    value = m->value;
                    return true;
                }
            }

            return false;
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AttributeList)
    };

    ComSmartPtr<AttributeList> attributeList;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VST3HostContext)
};

//==============================================================================
struct DescriptionFactory
{
    DescriptionFactory (VST3HostContext* host, IPluginFactory* pluginFactory)
        : vst3HostContext (host), factory (pluginFactory)
    {
        jassert (pluginFactory != nullptr);
    }

    virtual ~DescriptionFactory() {}

    Result findDescriptionsAndPerform (const File& file)
    {
        StringArray foundNames;
        PFactoryInfo factoryInfo;
        factory->getFactoryInfo (&factoryInfo);
        auto companyName = toString (factoryInfo.vendor).trim();

        Result result (Result::ok());

        auto numClasses = factory->countClasses();

        for (Steinberg::int32 i = 0; i < numClasses; ++i)
        {
            PClassInfo info;
            factory->getClassInfo (i, &info);

            if (std::strcmp (info.category, kVstAudioEffectClass) != 0)
                continue;

            const String name (toString (info.name).trim());

            if (foundNames.contains (name, true))
                continue;

            ScopedPointer<PClassInfo2> info2;
            ScopedPointer<PClassInfoW> infoW;

            {
                ComSmartPtr<IPluginFactory2> pf2;
                ComSmartPtr<IPluginFactory3> pf3;

                if (pf2.loadFrom (factory))
                {
                    info2 = new PClassInfo2();
                    pf2->getClassInfo2 (i, info2);
                }

                if (pf3.loadFrom (factory))
                {
                    infoW = new PClassInfoW();
                    pf3->getClassInfoUnicode (i, infoW);
                }
            }

            foundNames.add (name);

            PluginDescription desc;

            {
                ComSmartPtr<Vst::IComponent> component;

                if (component.loadFrom (factory, info.cid))
                {
                    if (component->initialize (vst3HostContext->getFUnknown()) == kResultOk)
                    {
                        auto numInputs  = getNumSingleDirectionChannelsFor (component, true, true);
                        auto numOutputs = getNumSingleDirectionChannelsFor (component, false, true);

                        createPluginDescription (desc, file, companyName, name,
                                                 info, info2, infoW, numInputs, numOutputs);

                        component->terminate();
                    }
                    else
                    {
                        jassertfalse;
                    }
                }
                else
                {
                    jassertfalse;
                }
            }

            result = performOnDescription (desc);

            if (result.failed())
                break;
        }

        return result;
    }

    virtual Result performOnDescription (PluginDescription&) = 0;

private:
    ComSmartPtr<VST3HostContext> vst3HostContext;
    ComSmartPtr<IPluginFactory> factory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DescriptionFactory)
};

struct MatchingDescriptionFinder  : public DescriptionFactory
{
    MatchingDescriptionFinder (VST3HostContext* h, IPluginFactory* f, const PluginDescription& desc)
       : DescriptionFactory (h, f), description (desc)
    {
    }

    static const char* getSuccessString() noexcept  { return "Found Description"; }

    Result performOnDescription (PluginDescription& desc)
    {
        if (description.isDuplicateOf (desc))
            return Result::fail (getSuccessString());

        return Result::ok();
    }

private:
    const PluginDescription& description;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatchingDescriptionFinder)
};

struct DescriptionLister  : public DescriptionFactory
{
    DescriptionLister (VST3HostContext* host, IPluginFactory* pluginFactory)
        : DescriptionFactory (host, pluginFactory)
    {
    }

    Result performOnDescription (PluginDescription& desc)
    {
        list.add (new PluginDescription (desc));
        return Result::ok();
    }

    OwnedArray<PluginDescription> list;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DescriptionLister)
};

//==============================================================================
struct DLLHandle
{
    DLLHandle (const String& modulePath)
    {
        if (modulePath.trim().isNotEmpty())
            open (modulePath);
    }

    ~DLLHandle()
    {
        typedef bool (PLUGIN_API *ExitModuleFn) ();

       #if JUCE_WINDOWS
        releaseFactory();

        if (auto exitFn = (ExitModuleFn) getFunction ("ExitDll"))
            exitFn();

        library.close();

       #else
        if (bundleRef != nullptr)
        {
            releaseFactory();

            if (auto exitFn = (ExitModuleFn) getFunction ("bundleExit"))
                exitFn();

            CFRelease (bundleRef);
            bundleRef = nullptr;
        }
       #endif
    }

    void open (const PluginDescription& description)
    {
       #if JUCE_WINDOWS
        jassert (description.fileOrIdentifier.isNotEmpty());
        jassert (File (description.fileOrIdentifier).existsAsFile());
        library.open (description.fileOrIdentifier);
       #else
        open (description.fileOrIdentifier);
       #endif
    }

    /** @note The factory should begin with a refCount of 1,
              so don't increment the reference count
              (ie: don't use a ComSmartPtr in here)!
              Its lifetime will be handled by this DllHandle,
              when such will be destroyed.

        @see releaseFactory
    */
    IPluginFactory* JUCE_CALLTYPE getPluginFactory()
    {
        if (factory == nullptr)
            if (auto proc = (GetFactoryProc) getFunction ("GetPluginFactory"))
                factory = proc();

        // The plugin NEEDS to provide a factory to be able to be called a VST3!
        // Most likely you are trying to load a 32-bit VST3 from a 64-bit host
        // or vice versa.
        jassert (factory != nullptr);
        return factory;
    }

    void* getFunction (const char* functionName)
    {
       #if JUCE_WINDOWS
        return library.getFunction (functionName);
       #else
        if (bundleRef == nullptr)
            return nullptr;

        CFStringRef name = String (functionName).toCFString();
        void* fn = CFBundleGetFunctionPointerForName (bundleRef, name);
        CFRelease (name);
        return fn;
       #endif
    }

private:
    IPluginFactory* factory = nullptr;

    void releaseFactory()
    {
        if (factory != nullptr)
            factory->release();
    }

   #if JUCE_WINDOWS
    DynamicLibrary library;

    bool open (const String& filePath)
    {
        if (library.open (filePath))
        {
            typedef bool (PLUGIN_API *InitModuleProc) ();

            if (auto proc = (InitModuleProc) getFunction ("InitDll"))
            {
                if (proc())
                    return true;
            }
            else
            {
                return true;
            }

            library.close();
        }

        return false;
    }

   #else
    CFBundleRef bundleRef;

    bool open (const String& filePath)
    {
        const File file (filePath);
        const char* const utf8 = file.getFullPathName().toRawUTF8();

        if (CFURLRef url = CFURLCreateFromFileSystemRepresentation (0, (const UInt8*) utf8, (CFIndex) std::strlen (utf8), file.isDirectory()))
        {
            bundleRef = CFBundleCreate (kCFAllocatorDefault, url);
            CFRelease (url);

            if (bundleRef != nullptr)
            {
                CFErrorRef error = nullptr;

                if (CFBundleLoadExecutableAndReturnError (bundleRef, &error))
                {
                    typedef bool (*BundleEntryProc)(CFBundleRef);

                    if (auto proc = (BundleEntryProc) getFunction ("bundleEntry"))
                    {
                        if (proc (bundleRef))
                            return true;
                    }
                    else
                    {
                        return true;
                    }
                }

                if (error != nullptr)
                {
                    if (CFStringRef failureMessage = CFErrorCopyFailureReason (error))
                    {
                        DBG (String::fromCFString (failureMessage));
                        CFRelease (failureMessage);
                    }

                    CFRelease (error);
                }

                CFRelease (bundleRef);
                bundleRef = nullptr;
            }
        }

        return false;
    }
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DLLHandle)
};

//==============================================================================
struct VST3ModuleHandle  : public ReferenceCountedObject
{
    explicit VST3ModuleHandle (const File& pluginFile)  : file (pluginFile)
    {
        getActiveModules().add (this);
    }

    ~VST3ModuleHandle()
    {
        getActiveModules().removeFirstMatchingValue (this);
    }

    /**
        Since there is no apparent indication if a VST3 plugin is a shell or not,
        we're stuck iterating through a VST3's factory, creating a description
        for every housed plugin.
    */
    static bool getAllDescriptionsForFile (OwnedArray<PluginDescription>& results,
                                           const String& fileOrIdentifier)
    {
        DLLHandle tempModule (fileOrIdentifier);

        ComSmartPtr<IPluginFactory> pluginFactory (tempModule.getPluginFactory());

        if (pluginFactory != nullptr)
        {
            ComSmartPtr<VST3HostContext> host (new VST3HostContext());
            DescriptionLister lister (host, pluginFactory);
            auto result = lister.findDescriptionsAndPerform (File (fileOrIdentifier));

            results.addCopiesOf (lister.list);

            return result.wasOk();
        }

        jassertfalse;
        return false;
    }

    //==============================================================================
    typedef ReferenceCountedObjectPtr<VST3ModuleHandle> Ptr;

    static VST3ModuleHandle::Ptr findOrCreateModule (const File& file, const PluginDescription& description)
    {
        for (auto* module : getActiveModules())
            // VST3s are basically shells, you must therefore check their name along with their file:
            if (module->file == file && module->name == description.name)
                return module;

        VST3ModuleHandle::Ptr m (new VST3ModuleHandle (file));

        if (! m->open (file, description))
            m = nullptr;

        return m;
    }

    //==============================================================================
    IPluginFactory* getPluginFactory()      { return dllHandle->getPluginFactory(); }

    File file;
    String name;

private:
    ScopedPointer<DLLHandle> dllHandle;

    //==============================================================================
    static Array<VST3ModuleHandle*>& getActiveModules()
    {
        static Array<VST3ModuleHandle*> activeModules;
        return activeModules;
    }

    //==============================================================================
    bool open (const File& f, const PluginDescription& description)
    {
        dllHandle = new DLLHandle (f.getFullPathName());

        ComSmartPtr<IPluginFactory> pluginFactory (dllHandle->getPluginFactory());

        if (pluginFactory != nullptr)
        {
            ComSmartPtr<VST3HostContext> host (new VST3HostContext());
            MatchingDescriptionFinder finder (host, pluginFactory, description);

            auto result = finder.findDescriptionsAndPerform (f);

            if (result.getErrorMessage() == MatchingDescriptionFinder::getSuccessString() ||
                result.getErrorMessage().isEmpty())
            {
                name = description.name;
                return true;
            }
        }

        return false;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VST3ModuleHandle)
};

//==============================================================================
struct VST3PluginWindow : public AudioProcessorEditor,
                          public ComponentMovementWatcher,
                          public IPlugFrame
{
    VST3PluginWindow (AudioProcessor* owner, IPlugView* pluginView)
      : AudioProcessorEditor (owner),
        ComponentMovementWatcher (this),
        view (pluginView, false)
    {
        setSize (10, 10);
        setOpaque (true);
        setVisible (true);

        warnOnFailure (view->setFrame (this));
        resizeToFit();
    }

    ~VST3PluginWindow()
    {
        warnOnFailure (view->removed());
        warnOnFailure (view->setFrame (nullptr));

        processor.editorBeingDeleted (this);

       #if JUCE_MAC
        embeddedComponent.setView (nullptr);
       #endif

        view = nullptr;
    }

    JUCE_DECLARE_VST3_COM_REF_METHODS
    JUCE_DECLARE_VST3_COM_QUERY_METHODS

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
    }

    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& wheel) override
    {
        view->onWheel (wheel.deltaY);
    }

    void focusGained (FocusChangeType) override     { view->onFocus (true); }
    void focusLost (FocusChangeType) override       { view->onFocus (false); }

    /** It seems that most, if not all, plugins do their own keyboard hooks,
        but IPlugView does have a set of keyboard related methods...
    */
    bool keyStateChanged (bool /*isKeyDown*/) override  { return true; }
    bool keyPressed (const KeyPress& /*key*/) override  { return true; }

    //==============================================================================
    void componentMovedOrResized (bool, bool wasResized) override
    {
        if (recursiveResize)
            return;

        auto* topComp = getTopLevelComponent();

        if (topComp->getPeer() != nullptr)
        {
           #if JUCE_WINDOWS
            auto pos = topComp->getLocalPoint (this, Point<int>());
           #endif

            recursiveResize = true;

            ViewRect rect;

            if (wasResized && view->canResize() == kResultTrue)
            {
                rect.right  = (Steinberg::int32) getWidth();
                rect.bottom = (Steinberg::int32) getHeight();
                view->checkSizeConstraint (&rect);

                auto w = (int) rect.getWidth();
                auto h = (int) rect.getHeight();
                setSize (w, h);

               #if JUCE_WINDOWS
                SetWindowPos (pluginHandle, 0,
                              pos.x, pos.y, w, h,
                              isVisible() ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
               #elif JUCE_MAC
                embeddedComponent.setBounds (getLocalBounds());
               #endif

                view->onSize (&rect);
            }
            else
            {
                warnOnFailure (view->getSize (&rect));

               #if JUCE_WINDOWS
                SetWindowPos (pluginHandle, 0,
                              pos.x, pos.y, rect.getWidth(), rect.getHeight(),
                              isVisible() ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
               #elif JUCE_MAC
                embeddedComponent.setBounds (0, 0, (int) rect.getWidth(), (int) rect.getHeight());
               #endif
            }

            // Some plugins don't update their cursor correctly when mousing out the window
            Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();

            recursiveResize = false;
        }
    }

    void componentPeerChanged() override {}

    void componentVisibilityChanged() override
    {
        attachPluginWindow();
        componentMovedOrResized (true, true);
    }

    void resizeToFit()
    {
        ViewRect rect;
        warnOnFailure (view->getSize (&rect));
        resizeWithRect (*this, rect);
    }

    tresult PLUGIN_API resizeView (IPlugView* incomingView, ViewRect* newSize) override
    {
        if (incomingView != nullptr
             && newSize != nullptr
             && incomingView == view)
        {
            resizeWithRect (embeddedComponent, *newSize);
            setSize (embeddedComponent.getWidth(), embeddedComponent.getHeight());
            return kResultTrue;
        }

        jassertfalse;
        return kInvalidArgument;
    }

    void setScaleFactor (float newScale) override
    {
        Steinberg::IPlugViewContentScaleSupport* scaleInterface = nullptr;
        view->queryInterface (Steinberg::IPlugViewContentScaleSupport::iid, (void**) &scaleInterface);

        if (scaleInterface != nullptr)
        {
            scaleInterface->setContentScaleFactor ((Steinberg::IPlugViewContentScaleSupport::ScaleFactor) newScale);
            scaleInterface->release();
            resizeToFit();
        }
    }

private:
    //==============================================================================
    Atomic<int> refCount { 1 };
    ComSmartPtr<IPlugView> view;

   #if JUCE_WINDOWS
    struct ChildComponent  : public Component
    {
        ChildComponent() {}
        void paint (Graphics& g) override  { g.fillAll (Colours::cornflowerblue); }
        using Component::createNewPeer;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChildComponent)
    };

    ChildComponent embeddedComponent;
    ScopedPointer<ComponentPeer> peer;
    typedef HWND HandleFormat;
   #elif JUCE_MAC
    AutoResizingNSViewComponentWithParent embeddedComponent;
    typedef NSView* HandleFormat;
   #else
    Component embeddedComponent;
    typedef void* HandleFormat;
   #endif

    HandleFormat pluginHandle = {};
    bool recursiveResize = false;

    //==============================================================================
    static void resizeWithRect (Component& comp, const ViewRect& rect)
    {
        comp.setBounds ((int) rect.left, (int) rect.top,
                        jmax (10, std::abs ((int) rect.getWidth())),
                        jmax (10, std::abs ((int) rect.getHeight())));
    }

    void attachPluginWindow()
    {
        if (pluginHandle == nullptr)
        {
           #if JUCE_WINDOWS
            if (auto* topComp = getTopLevelComponent())
                peer = embeddedComponent.createNewPeer (0, topComp->getWindowHandle());
            else
                peer = nullptr;

            if (peer != nullptr)
                pluginHandle = (HandleFormat) peer->getNativeHandle();
           #elif JUCE_MAC
            embeddedComponent.setBounds (getLocalBounds());
            addAndMakeVisible (embeddedComponent);
            pluginHandle = (NSView*) embeddedComponent.getView();
            jassert (pluginHandle != nil);
           #endif

            if (pluginHandle != nullptr)
                warnOnFailure (view->attached (pluginHandle, defaultVST3WindowType));
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VST3PluginWindow)
};

#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4996) // warning about overriding deprecated methods
#endif

//==============================================================================
struct VST3ComponentHolder
{
    VST3ComponentHolder (const VST3ModuleHandle::Ptr& m)  : module (m)
    {
        host = new VST3HostContext();
    }

    ~VST3ComponentHolder()
    {
        terminate();

        component = nullptr;
        host = nullptr;
        factory = nullptr;
        module = nullptr;
    }

    // transfers ownership to the plugin instance!
    AudioPluginInstance* createPluginInstance();

    bool fetchController (ComSmartPtr<Vst::IEditController>& editController)
    {
        if (! isComponentInitialised && ! initialise())
            return false;

        // Get the IEditController:
        TUID controllerCID = { 0 };

        if (component->getControllerClassId (controllerCID) == kResultTrue && FUID (controllerCID).isValid())
            editController.loadFrom (factory, controllerCID);

        if (editController == nullptr)
        {
            // Try finding the IEditController the long way around:
            auto numClasses = factory->countClasses();

            for (Steinberg::int32 i = 0; i < numClasses; ++i)
            {
                PClassInfo classInfo;
                factory->getClassInfo (i, &classInfo);

                if (std::strcmp (classInfo.category, kVstComponentControllerClass) == 0)
                    editController.loadFrom (factory, classInfo.cid);
            }
        }

        if (editController == nullptr)
            editController.loadFrom (component);

        return (editController != nullptr);
    }

    //==============================================================================
    void fillInPluginDescription (PluginDescription& description) const
    {
        jassert (module != nullptr && isComponentInitialised);

        PFactoryInfo factoryInfo;
        factory->getFactoryInfo (&factoryInfo);

        auto classIdx = getClassIndex (module->name);

        if (classIdx >= 0)
        {
            PClassInfo info;
            bool success = (factory->getClassInfo (classIdx, &info) == kResultOk);
            ignoreUnused (success);
            jassert (success);

            ComSmartPtr<IPluginFactory2> pf2;
            ComSmartPtr<IPluginFactory3> pf3;

            ScopedPointer<PClassInfo2> info2;
            ScopedPointer<PClassInfoW> infoW;

            if (pf2.loadFrom (factory))
            {
                info2 = new PClassInfo2();
                pf2->getClassInfo2 (classIdx, info2);
            }
            else
            {
                info2 = nullptr;
            }

            if (pf3.loadFrom (factory))
            {
                pf3->setHostContext (host->getFUnknown());
                infoW = new PClassInfoW();
                pf3->getClassInfoUnicode (classIdx, infoW);
            }
            else
            {
                infoW = nullptr;
            }

            Vst::BusInfo bus;
            int totalNumInputChannels = 0, totalNumOutputChannels = 0;

            int n = component->getBusCount(Vst::kAudio, Vst::kInput);
            for (int i = 0; i < n; ++i)
                if (component->getBusInfo (Vst::kAudio, Vst::kInput, i, bus) == kResultOk)
                    totalNumInputChannels += ((bus.flags & Vst::BusInfo::kDefaultActive) != 0 ? bus.channelCount : 0);

            n = component->getBusCount(Vst::kAudio, Vst::kOutput);
            for (int i = 0; i < n; ++i)
                if (component->getBusInfo (Vst::kAudio, Vst::kOutput, i, bus) == kResultOk)
                    totalNumOutputChannels += ((bus.flags & Vst::BusInfo::kDefaultActive) != 0 ? bus.channelCount : 0);

            createPluginDescription (description, module->file,
                                     factoryInfo.vendor, module->name,
                                     info, info2, infoW,
                                     totalNumInputChannels,
                                     totalNumOutputChannels);

            return;
        }

        jassertfalse;
    }

    //==============================================================================
    bool initialise()
    {
        if (isComponentInitialised) return true;

       #if JUCE_WINDOWS
        // On Windows it's highly advisable to create your plugins using the message thread,
        // because many plugins need a chance to create HWNDs that will get their messages
        // delivered by the main message thread, and that's not possible from a background thread.
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
       #endif

        factory = ComSmartPtr<IPluginFactory> (module->getPluginFactory());

        int classIdx;
        if ((classIdx = getClassIndex (module->name)) < 0)
            return false;

        PClassInfo info;
        if (factory->getClassInfo (classIdx, &info) != kResultOk)
            return false;

        if (! component.loadFrom (factory, info.cid) || component == nullptr)
            return false;

        if (warnOnFailure (component->initialize (host->getFUnknown())) != kResultOk)
            return false;

        isComponentInitialised = true;

        return true;
    }

    void terminate()
    {
        if (isComponentInitialised) component->terminate();
        isComponentInitialised = false;
    }

    //==============================================================================
    int getClassIndex (const String& className) const
    {
        PClassInfo info;
        const Steinberg::int32 numClasses = factory->countClasses();

        for (Steinberg::int32 j = 0; j < numClasses; ++j)
            if (factory->getClassInfo (j, &info) == kResultOk
                 && std::strcmp (info.category, kVstAudioEffectClass) == 0
                 && toString (info.name).trim() == className)
                return j;

        return -1;
    }

    //==============================================================================
    VST3ModuleHandle::Ptr module;
    ComSmartPtr<IPluginFactory> factory;
    ComSmartPtr<VST3HostContext> host;
    ComSmartPtr<Vst::IComponent> component;

    bool isComponentInitialised = false;
};

//==============================================================================
struct VST3PluginInstance : public AudioPluginInstance
{
    VST3PluginInstance (VST3ComponentHolder* componentHolder)
      : AudioPluginInstance (getBusProperties (componentHolder->component)),
        holder (componentHolder),
        inputParameterChanges (new ParamValueQueueList()),
        outputParameterChanges (new ParamValueQueueList()),
        midiInputs (new MidiEventList()),
        midiOutputs (new MidiEventList())
    {
        holder->host->setPlugin (this);
    }

    ~VST3PluginInstance()
    {
        jassert (getActiveEditor() == nullptr); // You must delete any editors before deleting the plugin instance!

        releaseResources();

        if (editControllerConnection != nullptr && componentConnection != nullptr)
        {
            editControllerConnection->disconnect (componentConnection);
            componentConnection->disconnect (editControllerConnection);
        }

        editController->setComponentHandler (nullptr);

        if (isControllerInitialised)    editController->terminate();
        holder->terminate();

        componentConnection = nullptr;
        editControllerConnection = nullptr;
        unitData = nullptr;
        unitInfo = nullptr;
        programListData = nullptr;
        componentHandler2 = nullptr;
        componentHandler = nullptr;
        processor = nullptr;
        editController2 = nullptr;
        editController = nullptr;
    }

    bool initialise()
    {
       #if JUCE_WINDOWS
        // On Windows it's highly advisable to create your plugins using the message thread,
        // because many plugins need a chance to create HWNDs that will get their messages
        // delivered by the main message thread, and that's not possible from a background thread.
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
       #endif

        if (! holder->initialise())
            return false;

        if (! isControllerInitialised)
        {
            if (! holder->fetchController (editController))
                return false;
        }

        // (May return an error if the plugin combines the IComponent and IEditController implementations)
        editController->initialize (holder->host->getFUnknown());

        isControllerInitialised = true;
        editController->setComponentHandler (holder->host);
        grabInformationObjects();
        interconnectComponentAndController();
        synchroniseStates();
        syncProgramNames();
        setupIO();
        return true;
    }

    void* getPlatformSpecificData() override   { return holder->component; }
    void refreshParameterList() override {}

    //==============================================================================
    const String getName() const override
    {
        VST3ModuleHandle::Ptr& module = holder->module;
        return module != nullptr ? module->name : String();
    }

    void repopulateArrangements (Array<Vst::SpeakerArrangement>& inputArrangements, Array<Vst::SpeakerArrangement>& outputArrangements) const
    {
        inputArrangements.clearQuick();
        outputArrangements.clearQuick();

        auto numInputAudioBuses  = getBusCount (true);
        auto numOutputAudioBuses = getBusCount (false);

        for (int i = 0; i < numInputAudioBuses; ++i)
            inputArrangements.add (getArrangementForBus (processor, true, i));

        for (int i = 0; i < numOutputAudioBuses; ++i)
            outputArrangements.add (getArrangementForBus (processor, false, i));
    }

    void processorLayoutsToArrangements (Array<Vst::SpeakerArrangement>& inputArrangements, Array<Vst::SpeakerArrangement>& outputArrangements)
    {
        inputArrangements.clearQuick();
        outputArrangements.clearQuick();

        auto numInputBuses  = getBusCount (true);
        auto numOutputBuses = getBusCount (false);

        for (int i = 0; i < numInputBuses; ++i)
            inputArrangements.add (getVst3SpeakerArrangement (getBus (true, i)->getLastEnabledLayout()));

        for (int i = 0; i < numOutputBuses; ++i)
            outputArrangements.add (getVst3SpeakerArrangement (getBus (false, i)->getLastEnabledLayout()));
    }

    void prepareToPlay (double newSampleRate, int estimatedSamplesPerBlock) override
    {
        // Avoid redundantly calling things like setActive, which can be a heavy-duty call for some plugins:
        if (isActive
              && getSampleRate() == newSampleRate
              && getBlockSize() == estimatedSamplesPerBlock)
            return;

        using namespace Vst;

        ProcessSetup setup;
        setup.symbolicSampleSize    = isUsingDoublePrecision() ? kSample64 : kSample32;
        setup.maxSamplesPerBlock    = estimatedSamplesPerBlock;
        setup.sampleRate            = newSampleRate;
        setup.processMode           = isNonRealtime() ? kOffline : kRealtime;

        warnOnFailure (processor->setupProcessing (setup));

        holder->initialise();
        editController->setComponentHandler (holder->host);


        Array<Vst::SpeakerArrangement> inputArrangements, outputArrangements;
        processorLayoutsToArrangements (inputArrangements, outputArrangements);

        warnOnFailure (processor->setBusArrangements (inputArrangements.getRawDataPointer(), inputArrangements.size(),
                                                      outputArrangements.getRawDataPointer(), outputArrangements.size()));

        Array<Vst::SpeakerArrangement> actualInArr, actualOutArr;
        repopulateArrangements (actualInArr, actualOutArr);

        jassert (actualInArr == inputArrangements && actualOutArr == outputArrangements);

        // Needed for having the same sample rate in processBlock(); some plugins need this!
        setRateAndBufferSizeDetails (newSampleRate, estimatedSamplesPerBlock);

        auto numInputBuses  = getBusCount (true);
        auto numOutputBuses = getBusCount (false);

        for (int i = 0; i < numInputBuses; ++i)
            warnOnFailure (holder->component->activateBus (Vst::kAudio, Vst::kInput,  i, getBus (true,  i)->isEnabled() ? 1 : 0));

        for (int i = 0; i < numOutputBuses; ++i)
            warnOnFailure (holder->component->activateBus (Vst::kAudio, Vst::kOutput, i, getBus (false, i)->isEnabled() ? 1 : 0));

        setLatencySamples (jmax (0, (int) processor->getLatencySamples()));
        cachedBusLayouts = getBusesLayout();

        warnOnFailure (holder->component->setActive (true));
        warnOnFailure (processor->setProcessing (true));

        isActive = true;
    }

    void releaseResources() override
    {
        if (! isActive)
            return; // Avoids redundantly calling things like setActive

        isActive = false;

        setStateForAllMidiBuses (false);

        if (processor != nullptr)
            warnOnFailure (processor->setProcessing (false));

        if (holder->component != nullptr)
            warnOnFailure (holder->component->setActive (false));
    }

    bool supportsDoublePrecisionProcessing() const override
    {
        return (processor->canProcessSampleSize (Vst::kSample64) == kResultTrue);
    }

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        jassert (! isUsingDoublePrecision());

        if (isActive && processor != nullptr)
            processAudio (buffer, midiMessages, Vst::kSample32);
    }

    void processBlock (AudioBuffer<double>& buffer, MidiBuffer& midiMessages) override
    {
        jassert (isUsingDoublePrecision());

        if (isActive && processor != nullptr)
            processAudio (buffer, midiMessages, Vst::kSample64);
    }

    template <typename FloatType>
    void processAudio (AudioBuffer<FloatType>& buffer, MidiBuffer& midiMessages,
                       Vst::SymbolicSampleSizes sampleSize)
    {
        using namespace Vst;
        auto numSamples = buffer.getNumSamples();

        auto numInputAudioBuses  = getBusCount (true);
        auto numOutputAudioBuses = getBusCount (false);

        ProcessData data;
        data.processMode            = isNonRealtime() ? kOffline : kRealtime;
        data.symbolicSampleSize     = sampleSize;
        data.numInputs              = numInputAudioBuses;
        data.numOutputs             = numOutputAudioBuses;
        data.inputParameterChanges  = inputParameterChanges;
        data.outputParameterChanges = outputParameterChanges;
        data.numSamples             = (Steinberg::int32) numSamples;

        updateTimingInformation (data, getSampleRate());

        for (int i = getTotalNumInputChannels(); i < buffer.getNumChannels(); ++i)
            buffer.clear (i, 0, numSamples);

        associateTo (data, buffer);
        associateTo (data, midiMessages);

        processor->process (data);

        MidiEventList::toMidiBuffer (midiMessages, *midiOutputs);

        inputParameterChanges->clearAllQueues();
    }

    //==============================================================================
    bool canAddBus (bool) const override                                       { return false; }
    bool canRemoveBus (bool) const override                                    { return false; }

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        // if the processor is not active, we ask the underlying plug-in if the
        // layout is actually supported
        if (! isActive)
            return canApplyBusesLayout (layouts);

        // not much we can do to check the layout while the audio processor is running
        // Let's at least check if it is a VST3 compatible layout
        for (int dir = 0; dir < 2; ++dir)
        {
            bool isInput = (dir == 0);
            auto n = getBusCount (isInput);

            for (int i = 0; i < n; ++i)
                if (getChannelLayoutOfBus (isInput, i).isDiscreteLayout())
                    return false;
        }

        return true;
    }

    bool syncBusLayouts (const BusesLayout& layouts) const
    {
        for (int dir = 0; dir < 2; ++dir)
        {
            bool isInput = (dir == 0);
            auto n = getBusCount (isInput);
            const Vst::BusDirection vstDir = (isInput ? Vst::kInput : Vst::kOutput);

            for (int busIdx = 0; busIdx < n; ++busIdx)
            {
                const bool isEnabled = (! layouts.getChannelSet (isInput, busIdx).isDisabled());

                if (holder->component->activateBus (Vst::kAudio, vstDir, busIdx, (isEnabled ? 1 : 0)) != kResultOk)
                    return false;
            }
        }

        Array<Vst::SpeakerArrangement> inputArrangements, outputArrangements;

        for (int i = 0; i < layouts.inputBuses.size(); ++i)
        {
            const auto& requested = layouts.getChannelSet (true, i);
            inputArrangements.add (getVst3SpeakerArrangement (requested.isDisabled() ? getBus (true, i)->getLastEnabledLayout() : requested));
        }

        for (int i = 0; i < layouts.outputBuses.size(); ++i)
        {
            const auto& requested = layouts.getChannelSet (false, i);
            outputArrangements.add (getVst3SpeakerArrangement (requested.isDisabled() ? getBus (false, i)->getLastEnabledLayout() : requested));
        }

        if (processor->setBusArrangements (inputArrangements.getRawDataPointer(), inputArrangements.size(),
                                           outputArrangements.getRawDataPointer(), outputArrangements.size()) != kResultTrue)
            return false;

        // check if the layout matches the request
        Array<Vst::SpeakerArrangement> actualIn, actualOut;
        repopulateArrangements (actualIn, actualOut);

        return (actualIn == inputArrangements && actualOut == outputArrangements);
    }

    bool canApplyBusesLayout (const BusesLayout& layouts) const override
    {
        // someone tried to change the layout while the AudioProcessor is running
        // call releaseResources first!
        jassert (! isActive);

        bool result = syncBusLayouts (layouts);

        // didn't succeed? Make sure it's back in it's original state
        if (! result)
            syncBusLayouts (getBusesLayout());

        return result;
    }

    //==============================================================================
    void updateTrackProperties (const TrackProperties& properties) override
    {
        if (trackInfoListener != nullptr)
        {
            ComSmartPtr<Vst::IAttributeList> l (new TrackPropertiesAttributeList (properties));
            trackInfoListener->setChannelContextInfos (l);
        }
    }

    struct TrackPropertiesAttributeList    : public Vst::IAttributeList
    {
        TrackPropertiesAttributeList (const TrackProperties& properties) : props (properties) {}
        virtual ~TrackPropertiesAttributeList() {}

        JUCE_DECLARE_VST3_COM_REF_METHODS

        tresult PLUGIN_API queryInterface (const TUID queryIid, void** obj) override
        {
            TEST_FOR_AND_RETURN_IF_VALID (queryIid, Vst::IAttributeList)
            TEST_FOR_COMMON_BASE_AND_RETURN_IF_VALID (queryIid, FUnknown, Vst::IAttributeList)

            *obj = nullptr;
            return kNotImplemented;
        }

        tresult PLUGIN_API setInt    (AttrID, Steinberg::int64) override                 { return kOutOfMemory; }
        tresult PLUGIN_API setFloat  (AttrID, double) override                           { return kOutOfMemory; }
        tresult PLUGIN_API setString (AttrID, const Vst::TChar*) override                { return kOutOfMemory; }
        tresult PLUGIN_API setBinary (AttrID, const void*, Steinberg::uint32) override   { return kOutOfMemory; }
        tresult PLUGIN_API getFloat  (AttrID, double&) override                          { return kResultFalse; }
        tresult PLUGIN_API getBinary (AttrID, const void*&, Steinberg::uint32&) override { return kResultFalse; }

        tresult PLUGIN_API getString (AttrID id, Vst::TChar* string, Steinberg::uint32 size) override
        {
            if (! std::strcmp (id, Vst::ChannelContext::kChannelNameKey))
            {
                Steinberg::String str (props.name.toRawUTF8());
                str.copyTo (string, 0, (Steinberg::int32) jmin (size, (Steinberg::uint32) std::numeric_limits<Steinberg::int32>::max()));

                return kResultTrue;
            }

            return kResultFalse;
        }

        tresult PLUGIN_API getInt (AttrID id, Steinberg::int64& value) override
        {
            if      (! std::strcmp (Vst::ChannelContext::kChannelNameLengthKey, id)) value = props.name.length();
            else if (! std::strcmp (Vst::ChannelContext::kChannelColorKey,      id)) value = static_cast<Steinberg::int64> (props.colour.getARGB());
            else return kResultFalse;

            return kResultTrue;
        }

        Atomic<int> refCount;
        TrackProperties props;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackPropertiesAttributeList)
    };

    //==============================================================================
    String getChannelName (int channelIndex, bool forInput, bool forAudioChannel) const
    {
        auto numBuses = getNumSingleDirectionBusesFor (holder->component, forInput, forAudioChannel);
        int numCountedChannels = 0;

        for (int i = 0; i < numBuses; ++i)
        {
            auto busInfo = getBusInfo (forInput, forAudioChannel, i);

            numCountedChannels += busInfo.channelCount;

            if (channelIndex < numCountedChannels)
                return toString (busInfo.name);
        }

        return {};
    }

    const String getInputChannelName  (int channelIndex) const override   { return getChannelName (channelIndex, true, true); }
    const String getOutputChannelName (int channelIndex) const override   { return getChannelName (channelIndex, false, true); }

    bool isInputChannelStereoPair (int channelIndex) const override
    {
        int busIdx;
        return getOffsetInBusBufferForAbsoluteChannelIndex (true, channelIndex, busIdx) >= 0
                 && getBusInfo (true, true, busIdx).channelCount == 2;
    }

    bool isOutputChannelStereoPair (int channelIndex) const override
    {
        int busIdx;
        return getOffsetInBusBufferForAbsoluteChannelIndex (false, channelIndex, busIdx) >= 0
                 && getBusInfo (false, true, busIdx).channelCount == 2;
    }

    bool acceptsMidi() const override    { return getBusInfo (true,  false).channelCount > 0; }
    bool producesMidi() const override   { return getBusInfo (false, false).channelCount > 0; }

    //==============================================================================
    /** May return a negative value as a means of informing us that the plugin has "infinite tail," or 0 for "no tail." */
    double getTailLengthSeconds() const override
    {
        if (processor != nullptr)
        {
            auto sampleRate = getSampleRate();

            if (sampleRate > 0.0)
                return jlimit (0, 0x7fffffff, (int) processor->getTailSamples()) / sampleRate;
        }

        return 0.0;
    }

    //==============================================================================
    AudioProcessorEditor* createEditor() override
    {
        if (auto* view = tryCreatingView())
            return new VST3PluginWindow (this, view);

        return nullptr;
    }

    bool hasEditor() const override
    {
        // (if possible, avoid creating a second instance of the editor, because that crashes some plugins)
        if (getActiveEditor() != nullptr)
            return true;

        ComSmartPtr<IPlugView> view (tryCreatingView(), false);
        return view != nullptr;
    }

    //==============================================================================
    int getNumParameters() override
    {
        if (editController != nullptr)
            return (int) editController->getParameterCount();

        return 0;
    }

    const String getParameterName (int parameterIndex) override
    {
        return toString (getParameterInfoForIndex (parameterIndex).title);
    }

    const String getParameterText (int parameterIndex) override
    {
        if (editController != nullptr)
        {
            auto id = getParameterInfoForIndex (parameterIndex).id;

            Vst::String128 result;
            warnOnFailure (editController->getParamStringByValue (id, editController->getParamNormalized (id), result));

            return toString (result);
        }

        return {};
    }

    int getParameterNumSteps (int parameterIndex) override
    {
        if (editController != nullptr)
        {
            const auto numSteps = getParameterInfoForIndex (parameterIndex).stepCount;

            if (numSteps > 0)
                return numSteps;
        }

        return AudioProcessor::getDefaultNumParameterSteps();
    }

    bool isParameterDiscrete (int parameterIndex) const override
    {
        if (editController != nullptr)
        {
            const auto numSteps = getParameterInfoForIndex (parameterIndex).stepCount;
            return numSteps > 0;
        }

        return false;
    }

    float getParameter (int parameterIndex) override
    {
        if (editController != nullptr)
        {
            auto id = getParameterInfoForIndex (parameterIndex).id;
            return (float) editController->getParamNormalized (id);
        }

        return 0.0f;
    }

    void setParameter (int parameterIndex, float newValue) override
    {
        if (editController != nullptr)
        {
            auto paramID = getParameterInfoForIndex (parameterIndex).id;
            editController->setParamNormalized (paramID, (double) newValue);

            Steinberg::int32 index;
            inputParameterChanges->addParameterData (paramID, index)->addPoint (0, newValue, index);
        }
    }

    //==============================================================================
    int getNumPrograms() override                        { return programNames.size(); }
    const String getProgramName (int index) override     { return programNames[index]; }
    int getCurrentProgram() override                     { return jmax (0, (int) editController->getParamNormalized (programParameterID) * (programNames.size() - 1)); }
    void changeProgramName (int, const String&) override {}

    void setCurrentProgram (int program) override
    {
        if (programNames.size() > 0 && editController != nullptr)
        {
            auto value = static_cast<Vst::ParamValue> (program) / static_cast<Vst::ParamValue> (programNames.size());

            editController->setParamNormalized (programParameterID, value);
            Steinberg::int32 index;
            inputParameterChanges->addParameterData (programParameterID, index)->addPoint (0, value, index);
        }
    }

    //==============================================================================
    void reset() override
    {
        if (holder->component != nullptr && processor != nullptr)
        {
            processor->setProcessing (false);
            holder->component->setActive (false);

            holder->component->setActive (true);
            processor->setProcessing (true);
        }
    }

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override
    {
        XmlElement state ("VST3PluginState");

        appendStateFrom (state, holder->component, "IComponent");
        appendStateFrom (state, editController, "IEditController");

        AudioProcessor::copyXmlToBinary (state, destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        ScopedPointer<XmlElement> head (AudioProcessor::getXmlFromBinary (data, sizeInBytes));

        if (head != nullptr)
        {
            ComSmartPtr<Steinberg::MemoryStream> componentStream (createMemoryStreamForState (*head, "IComponent"));

            if (componentStream != nullptr && holder->component != nullptr)
                holder->component->setState (componentStream);

            if (editController != nullptr)
            {
                if (componentStream != nullptr)
                {
                    int64 result;
                    componentStream->seek (0, IBStream::kIBSeekSet, &result);
                    editController->setComponentState (componentStream);
                }

                ComSmartPtr<Steinberg::MemoryStream> controllerStream = createMemoryStreamForState (*head, "IEditController");

                if (controllerStream != nullptr)
                    editController->setState (controllerStream);
            }
        }
    }

    //==============================================================================
    void fillInPluginDescription (PluginDescription& description) const override
    {
        holder->fillInPluginDescription (description);
    }

    /** @note Not applicable to VST3 */
    void getCurrentProgramStateInformation (MemoryBlock& destData) override
    {
        destData.setSize (0, true);
    }

    /** @note Not applicable to VST3 */
    void setCurrentProgramStateInformation (const void* data, int sizeInBytes) override
    {
        ignoreUnused (data, sizeInBytes);
    }

    //==============================================================================
    // NB: this class and its subclasses must be public to avoid problems in
    // DLL builds under MSVC.
    struct ParamValueQueueList  : public Vst::IParameterChanges
    {
        ParamValueQueueList() {}
        virtual ~ParamValueQueueList() {}

        JUCE_DECLARE_VST3_COM_REF_METHODS
        JUCE_DECLARE_VST3_COM_QUERY_METHODS

        Steinberg::int32 PLUGIN_API getParameterCount() override                                { return numQueuesUsed; }
        Vst::IParamValueQueue* PLUGIN_API getParameterData (Steinberg::int32 index) override    { return isPositiveAndBelow (static_cast<int> (index), numQueuesUsed) ? queues[(int) index] : nullptr; }

        Vst::IParamValueQueue* PLUGIN_API addParameterData (const Vst::ParamID& id, Steinberg::int32& index) override
        {
            for (int i = numQueuesUsed; --i >= 0;)
            {
                if (queues.getUnchecked (i)->getParameterId() == id)
                {
                    index = (Steinberg::int32) i;
                    return queues.getUnchecked (i);
                }
            }

            index = numQueuesUsed++;
            ParamValueQueue* valueQueue = (index < queues.size() ? queues[index]
                                                                 : queues.add (new ParamValueQueue()));

            valueQueue->clear();
            valueQueue->setParamID (id);

            return valueQueue;
        }

        void clearAllQueues() noexcept
        {
            numQueuesUsed = 0;
        }

        struct ParamValueQueue  : public Vst::IParamValueQueue
        {
            ParamValueQueue()
            {
                points.ensureStorageAllocated (1024);
            }

            virtual ~ParamValueQueue() {}

            void setParamID (Vst::ParamID pID) noexcept    { paramID = pID; }

            JUCE_DECLARE_VST3_COM_REF_METHODS
            JUCE_DECLARE_VST3_COM_QUERY_METHODS

            Steinberg::Vst::ParamID PLUGIN_API getParameterId() override    { return paramID; }
            Steinberg::int32 PLUGIN_API getPointCount() override            { return (Steinberg::int32) points.size(); }

            Steinberg::tresult PLUGIN_API getPoint (Steinberg::int32 index,
                                                    Steinberg::int32& sampleOffset,
                                                    Steinberg::Vst::ParamValue& value) override
            {
                const ScopedLock sl (pointLock);

                if (isPositiveAndBelow ((int) index, points.size()))
                {
                    ParamPoint e (points.getUnchecked ((int) index));
                    sampleOffset = e.sampleOffset;
                    value = e.value;
                    return kResultTrue;
                }

                sampleOffset = -1;
                value = 0.0;
                return kResultFalse;
            }

            Steinberg::tresult PLUGIN_API addPoint (Steinberg::int32 sampleOffset,
                                                    Steinberg::Vst::ParamValue value,
                                                    Steinberg::int32& index) override
            {
                ParamPoint p = { sampleOffset, value };

                const ScopedLock sl (pointLock);
                index = (Steinberg::int32) points.size();
                points.add (p);
                return kResultTrue;
            }

            void clear() noexcept
            {
                const ScopedLock sl (pointLock);
                points.clearQuick();
            }

        private:
            struct ParamPoint
            {
                Steinberg::int32 sampleOffset;
                Steinberg::Vst::ParamValue value;
            };

            Atomic<int> refCount;
            Vst::ParamID paramID = static_cast<Vst::ParamID> (-1);
            Array<ParamPoint> points;
            CriticalSection pointLock;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamValueQueue)
        };

        Atomic<int> refCount;
        OwnedArray<ParamValueQueue> queues;
        int numQueuesUsed = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamValueQueueList)
    };

private:
    //==============================================================================
    ScopedPointer<VST3ComponentHolder> holder;

    friend VST3HostContext;

    // Information objects:
    String company;
    ScopedPointer<PClassInfo> info;
    ScopedPointer<PClassInfo2> info2;
    ScopedPointer<PClassInfoW> infoW;

    // Rudimentary interfaces:
    ComSmartPtr<Vst::IEditController> editController;
    ComSmartPtr<Vst::IEditController2> editController2;
    ComSmartPtr<Vst::IAudioProcessor> processor;
    ComSmartPtr<Vst::IComponentHandler> componentHandler;
    ComSmartPtr<Vst::IComponentHandler2> componentHandler2;
    ComSmartPtr<Vst::IUnitInfo> unitInfo;
    ComSmartPtr<Vst::IUnitData> unitData;
    ComSmartPtr<Vst::IProgramListData> programListData;
    ComSmartPtr<Vst::IConnectionPoint> componentConnection, editControllerConnection;
    ComSmartPtr<Vst::ChannelContext::IInfoListener> trackInfoListener;

    /** The number of IO buses MUST match that of the plugin,
        even if there aren't enough channels to process,
        as very poorly specified by the Steinberg SDK
    */
    VST3FloatAndDoubleBusMapComposite inputBusMap, outputBusMap;
    Array<Vst::AudioBusBuffers> inputBuses, outputBuses;
    AudioProcessor::BusesLayout cachedBusLayouts;

    StringArray programNames;
    Vst::ParamID programParameterID = (Vst::ParamID) -1;

    //==============================================================================
    template <typename Type>
    static void appendStateFrom (XmlElement& head, ComSmartPtr<Type>& object, const String& identifier)
    {
        if (object != nullptr)
        {
            Steinberg::MemoryStream stream;

            if (object->getState (&stream) == kResultTrue)
            {
                MemoryBlock info (stream.getData(), (size_t) stream.getSize());
                head.createNewChildElement (identifier)->addTextElement (info.toBase64Encoding());
            }
        }
    }

    static Steinberg::MemoryStream* createMemoryStreamForState (XmlElement& head, StringRef identifier)
    {
        Steinberg::MemoryStream* stream = nullptr;

        if (auto* state = head.getChildByName (identifier))
        {
            MemoryBlock mem;

            if (mem.fromBase64Encoding (state->getAllSubText()))
            {
                stream = new Steinberg::MemoryStream();
                stream->setSize ((TSize) mem.getSize());
                mem.copyTo (stream->getData(), 0, mem.getSize());
            }
        }

        return stream;
    }

    ComSmartPtr<ParamValueQueueList> inputParameterChanges, outputParameterChanges;
    ComSmartPtr<MidiEventList> midiInputs, midiOutputs;
    Vst::ProcessContext timingInfo; //< Only use this in processBlock()!
    bool isControllerInitialised = false, isActive = false;

    //==============================================================================
    /** Some plugins need to be "connected" to intercommunicate between their implemented classes */
    void interconnectComponentAndController()
    {
        componentConnection.loadFrom (holder->component);
        editControllerConnection.loadFrom (editController);

        if (componentConnection != nullptr && editControllerConnection != nullptr)
        {
            warnOnFailure (componentConnection->connect (editControllerConnection));
            warnOnFailure (editControllerConnection->connect (componentConnection));
        }
    }

    void synchroniseStates()
    {
        Steinberg::MemoryStream stream;

        if (holder->component->getState (&stream) == kResultTrue)
            if (stream.seek (0, Steinberg::IBStream::kIBSeekSet, nullptr) == kResultTrue)
                warnOnFailure (editController->setComponentState (&stream));
    }

    void grabInformationObjects()
    {
        processor.loadFrom (holder->component);
        unitInfo.loadFrom (holder->component);
        programListData.loadFrom (holder->component);
        unitData.loadFrom (holder->component);
        editController2.loadFrom (holder->component);
        componentHandler.loadFrom (holder->component);
        componentHandler2.loadFrom (holder->component);
        trackInfoListener.loadFrom (holder->component);

        if (processor == nullptr)           processor.loadFrom (editController);
        if (unitInfo == nullptr)            unitInfo.loadFrom (editController);
        if (programListData == nullptr)     programListData.loadFrom (editController);
        if (unitData == nullptr)            unitData.loadFrom (editController);
        if (editController2 == nullptr)     editController2.loadFrom (editController);
        if (componentHandler == nullptr)    componentHandler.loadFrom (editController);
        if (componentHandler2 == nullptr)   componentHandler2.loadFrom (editController);
        if (trackInfoListener == nullptr)   trackInfoListener.loadFrom (editController);
    }

    void setStateForAllMidiBuses (bool newState)
    {
        setStateForAllBusesOfType (holder->component, newState, true, false);   // Activate/deactivate MIDI inputs
        setStateForAllBusesOfType (holder->component, newState, false, false);  // Activate/deactivate MIDI outputs
    }

    void setupIO()
    {
        setStateForAllMidiBuses (true);

        Vst::ProcessSetup setup;
        setup.symbolicSampleSize   = Vst::kSample32;
        setup.maxSamplesPerBlock   = 1024;
        setup.sampleRate           = 44100.0;
        setup.processMode          = Vst::kRealtime;

        warnOnFailure (processor->setupProcessing (setup));

        cachedBusLayouts = getBusesLayout();
        setRateAndBufferSizeDetails (setup.sampleRate, (int) setup.maxSamplesPerBlock);
    }

    static AudioProcessor::BusesProperties getBusProperties (ComSmartPtr<Vst::IComponent>& component)
    {
        AudioProcessor::BusesProperties busProperties;
        ComSmartPtr<Vst::IAudioProcessor> processor;
        processor.loadFrom (component.get());

        for (int dirIdx = 0; dirIdx < 2; ++dirIdx)
        {
            const bool isInput = (dirIdx == 0);
            const Vst::BusDirection dir = (isInput ? Vst::kInput : Vst::kOutput);
            const int numBuses = component->getBusCount (Vst::kAudio, dir);

            for (int i = 0; i < numBuses; ++i)
            {
                Vst::BusInfo info;

                if (component->getBusInfo (Vst::kAudio, dir, (Steinberg::int32) i, info) != kResultOk)
                    continue;

                if (info.channelCount == 0)
                    continue;

                AudioChannelSet layout = AudioChannelSet::discreteChannels (info.channelCount);

                Vst::SpeakerArrangement arr;
                if (processor != nullptr && processor->getBusArrangement (dir, i, arr) == kResultOk)
                    layout = getChannelSetForSpeakerArrangement (arr);

                busProperties.addBus (isInput, toString (info.name), layout,
                                      (info.flags & Vst::BusInfo::kDefaultActive) != 0);
            }
        }

        return busProperties;
    }

    //==============================================================================
    Vst::BusInfo getBusInfo (bool forInput, bool forAudio, int index = 0) const
    {
        Vst::BusInfo busInfo;
        busInfo.mediaType = forAudio ? Vst::kAudio : Vst::kEvent;
        busInfo.direction = forInput ? Vst::kInput : Vst::kOutput;
        busInfo.channelCount = 0;

        holder->component->getBusInfo (busInfo.mediaType, busInfo.direction,
                                       (Steinberg::int32) index, busInfo);
        return busInfo;
    }

    //==============================================================================
    /** @note An IPlugView, when first created, should start with a ref-count of 1! */
    IPlugView* tryCreatingView() const
    {
        IPlugView* v = editController->createView (Vst::ViewType::kEditor);

        if (v == nullptr) v = editController->createView (nullptr);
        if (v == nullptr) editController->queryInterface (IPlugView::iid, (void**) &v);

        return v;
    }

    //==============================================================================
    template <typename FloatType>
    void associateTo (Vst::ProcessData& destination, AudioBuffer<FloatType>& buffer)
    {
        VST3BufferExchange<FloatType>::mapBufferToBuses (inputBuses, inputBusMap.get<FloatType>(), cachedBusLayouts.inputBuses, buffer);
        VST3BufferExchange<FloatType>::mapBufferToBuses (outputBuses, outputBusMap.get<FloatType>(), cachedBusLayouts.outputBuses, buffer);

        destination.inputs  = inputBuses.getRawDataPointer();
        destination.outputs = outputBuses.getRawDataPointer();
    }

    void associateTo (Vst::ProcessData& destination, MidiBuffer& midiBuffer)
    {
        midiInputs->clear();
        midiOutputs->clear();

        MidiEventList::toEventList (*midiInputs, midiBuffer);

        destination.inputEvents = midiInputs;
        destination.outputEvents = midiOutputs;
    }

    void updateTimingInformation (Vst::ProcessData& destination, double processSampleRate)
    {
        toProcessContext (timingInfo, getPlayHead(), processSampleRate);
        destination.processContext = &timingInfo;
    }

    Vst::ParameterInfo getParameterInfoForIndex (int index) const
    {
        Vst::ParameterInfo paramInfo = { 0 };

        if (processor != nullptr)
            editController->getParameterInfo (index, paramInfo);

        return paramInfo;
    }

    Vst::ProgramListInfo getProgramListInfo (int index) const
    {
        Vst::ProgramListInfo paramInfo = { 0 };

        if (unitInfo != nullptr)
            unitInfo->getProgramListInfo (index, paramInfo);

        return paramInfo;
    }

    void syncProgramNames()
    {
        programNames.clear();

        if (processor == nullptr || editController == nullptr)
            return;

        Vst::UnitID programUnitID;
        Vst::ParameterInfo paramInfo = { 0 };

        {
            int idx, num = editController->getParameterCount();
            for (idx = 0; idx < num; ++idx)
                if (editController->getParameterInfo (idx, paramInfo) == kResultOk
                     && (paramInfo.flags & Steinberg::Vst::ParameterInfo::kIsProgramChange) != 0)
                    break;

            if (idx >= num)
                return;

            programParameterID = paramInfo.id;
            programUnitID = paramInfo.unitId;
        }

        if (unitInfo != nullptr)
        {
            Vst::UnitInfo uInfo = { 0 };
            const int unitCount = unitInfo->getUnitCount();

            for (int idx = 0; idx < unitCount; ++idx)
            {
                if (unitInfo->getUnitInfo(idx, uInfo) == kResultOk
                      && uInfo.id == programUnitID)
                {
                    const int programListCount = unitInfo->getProgramListCount();

                    for (int j = 0; j < programListCount; ++j)
                    {
                        Vst::ProgramListInfo programListInfo = { 0 };

                        if (unitInfo->getProgramListInfo (j, programListInfo) == kResultOk
                              && programListInfo.id == uInfo.programListId)
                        {
                            Vst::String128 name;

                            for (int k = 0; k < programListInfo.programCount; ++k)
                                if (unitInfo->getProgramName (programListInfo.id, k, name) == kResultOk)
                                    programNames.add (toString (name));

                            return;
                        }
                    }

                    break;
                }
            }
        }

        if (editController != nullptr
               && paramInfo.stepCount > 0)
        {
            auto numPrograms = paramInfo.stepCount + 1;

            for (int i = 0; i < numPrograms; ++i)
            {
                auto valueNormalized = static_cast<Vst::ParamValue> (i) / static_cast<Vst::ParamValue> (paramInfo.stepCount);

                Vst::String128 programName;
                if (editController->getParamStringByValue (paramInfo.id, valueNormalized, programName) == kResultOk)
                    programNames.add (toString (programName));
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VST3PluginInstance)
};

#if JUCE_MSVC
 #pragma warning (pop)
#endif

};

//==============================================================================
AudioPluginInstance* VST3Classes::VST3ComponentHolder::createPluginInstance()
{
    if (! initialise())
        return nullptr;

    auto* plugin = new VST3PluginInstance (this);
    host->setPlugin (plugin);
    return plugin;
}


//==============================================================================
VST3PluginFormat::VST3PluginFormat() {}
VST3PluginFormat::~VST3PluginFormat() {}

void VST3PluginFormat::findAllTypesForFile (OwnedArray<PluginDescription>& results, const String& fileOrIdentifier)
{
    if (fileMightContainThisPluginType (fileOrIdentifier))
        VST3Classes::VST3ModuleHandle::getAllDescriptionsForFile (results, fileOrIdentifier);
}

void VST3PluginFormat::createPluginInstance (const PluginDescription& description, double, int, void* userData,
                                             void (*callback) (void*, AudioPluginInstance*, const String&))
{
    ScopedPointer<VST3Classes::VST3PluginInstance> result;

    if (fileMightContainThisPluginType (description.fileOrIdentifier))
    {
        File file (description.fileOrIdentifier);

        auto previousWorkingDirectory = File::getCurrentWorkingDirectory();
        file.getParentDirectory().setAsCurrentWorkingDirectory();

        if (const VST3Classes::VST3ModuleHandle::Ptr module = VST3Classes::VST3ModuleHandle::findOrCreateModule (file, description))
        {
            ScopedPointer<VST3Classes::VST3ComponentHolder> holder = new VST3Classes::VST3ComponentHolder (module);

            if (holder->initialise())
            {
                result = new VST3Classes::VST3PluginInstance (holder.release());

                if (! result->initialise())
                    result = nullptr;
            }
        }

        previousWorkingDirectory.setAsCurrentWorkingDirectory();
    }

    String errorMsg;

    if (result == nullptr)
        errorMsg = String (NEEDS_TRANS ("Unable to load XXX plug-in file")).replace ("XXX", "VST-3");

    callback (userData, result.release(), errorMsg);
}

bool VST3PluginFormat::requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept
{
    return false;
}

bool VST3PluginFormat::fileMightContainThisPluginType (const String& fileOrIdentifier)
{
    auto f = File::createFileWithoutCheckingPath (fileOrIdentifier);

    return f.hasFileExtension (".vst3")
          #if JUCE_MAC
           && f.exists();
          #else
           && f.existsAsFile();
          #endif
}

String VST3PluginFormat::getNameOfPluginFromIdentifier (const String& fileOrIdentifier)
{
    return fileOrIdentifier; //Impossible to tell because every VST3 is a type of shell...
}

bool VST3PluginFormat::pluginNeedsRescanning (const PluginDescription& description)
{
    return File (description.fileOrIdentifier).getLastModificationTime() != description.lastFileModTime;
}

bool VST3PluginFormat::doesPluginStillExist (const PluginDescription& description)
{
    return File (description.fileOrIdentifier).exists();
}

StringArray VST3PluginFormat::searchPathsForPlugins (const FileSearchPath& directoriesToSearch, const bool recursive, bool)
{
    StringArray results;

    for (int i = 0; i < directoriesToSearch.getNumPaths(); ++i)
        recursiveFileSearch (results, directoriesToSearch[i], recursive);

    return results;
}

void VST3PluginFormat::recursiveFileSearch (StringArray& results, const File& directory, const bool recursive)
{
    DirectoryIterator iter (directory, false, "*", File::findFilesAndDirectories);

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

FileSearchPath VST3PluginFormat::getDefaultLocationsToSearch()
{
   #if JUCE_WINDOWS
    auto programFiles = File::getSpecialLocation (File::globalApplicationsDirectory).getFullPathName();
    return FileSearchPath (programFiles + "\\Common Files\\VST3");
   #elif JUCE_MAC
    return FileSearchPath ("/Library/Audio/Plug-Ins/VST3;~/Library/Audio/Plug-Ins/VST3");
   #else
    return FileSearchPath();
   #endif
}

} // namespace juce

#endif // JUCE_PLUGINHOST_VST3
