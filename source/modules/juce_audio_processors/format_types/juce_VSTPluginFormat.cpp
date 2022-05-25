/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

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


#if ! JUCE_MINGW && ! JUCE_MSVC
 #define __cdecl
#endif

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wzero-as-null-pointer-constant")
JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4996)

#define VST_FORCE_DEPRECATED 0

namespace Vst2
{
#include "juce_VSTInterface.h"
}

#include "juce_VSTCommon.h"

JUCE_END_IGNORE_WARNINGS_GCC_LIKE
JUCE_END_IGNORE_WARNINGS_MSVC

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")
JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4355)

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

#ifndef JUCE_VST_FALLBACK_HOST_NAME
 #define JUCE_VST_FALLBACK_HOST_NAME "Carla Plugin Host"
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
       #elif JUCE_LINUX || JUCE_BSD || JUCE_IOS || JUCE_ANDROID
        timeval micro;
        gettimeofday (&micro, nullptr);
        return (double) micro.tv_usec * 1000.0;
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
        return FSPathMakeRef (reinterpret_cast<const UInt8*> (path.toRawUTF8()), destFSRef, nullptr) == noErr;
    }
   #endif
}

//==============================================================================
typedef Vst2::VstEffectInterface* (VSTINTERFACECALL *MainCall) (Vst2::VstHostCallback);
static pointer_sized_int VSTINTERFACECALL audioMaster (Vst2::VstEffectInterface*, int32, int32, pointer_sized_int, void*, float);

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
#if JUCE_LINUX || JUCE_BSD

namespace
{
    using EventProcPtr = void (*)(XEvent*);

    Window getChildWindow (Window windowToCheck)
    {
        Window rootWindow, parentWindow;
        Window* childWindows;
        unsigned int numChildren = 0;

        X11Symbols::getInstance()->xQueryTree (XWindowSystem::getInstance()->getDisplay(),
                                               windowToCheck, &rootWindow, &parentWindow, &childWindows, &numChildren);

        if (numChildren > 0)
            return childWindows [0];

        return 0;
    }
}

#endif

//==============================================================================
class VSTXMLInfo
{
public:
    static VSTXMLInfo* createFor (const juce::XmlElement& xml)
    {
        if (xml.hasTagName ("VSTParametersStructure"))
            return new VSTXMLInfo (xml);

        if (const auto* x = xml.getChildByName ("VSTParametersStructure"))
            return new VSTXMLInfo (*x);

        return nullptr;
    }

    struct Group;

    struct Base
    {
        Base() noexcept {}
        virtual ~Base() {}

        Group* parent = nullptr;
    };

    struct Param  : public Base
    {
        int paramID;
        juce::String expr, name, label;
        juce::StringArray shortNames;
        juce::String type;
        int numberOfStates;
        float defaultValue;
    };

    struct Group  : public Base
    {
        juce::String name;
        juce::OwnedArray<Base> paramTree;
    };

    struct Range
    {
        Range() noexcept {}
        Range (const juce::String& s)       { set (s); }

        void set (const juce::String& s)
        {
            inclusiveLow  = s.startsWithChar ('[');
            inclusiveHigh = s.endsWithChar   (']');

            auto str = s.removeCharacters ("[]");

            low  = str.upToFirstOccurrenceOf (",", false, false).getFloatValue();
            high = str.fromLastOccurrenceOf  (",", false, false).getFloatValue();
        }

        bool contains (float f) const noexcept
        {
            return (inclusiveLow  ? (f >= low)  : (f > low))
                && (inclusiveHigh ? (f <= high) : (f < high));
        }

        float low = 0;
        float high = 0;

        bool inclusiveLow = false;
        bool inclusiveHigh = false;
    };

    struct Entry
    {
        juce::String name;
        Range range;
    };

    struct ValueType
    {
        juce::String name, label;
        juce::OwnedArray<Entry> entries;
    };

    struct Template
    {
        juce::String name;
        juce::OwnedArray<Param> params;
    };

    const Param* getParamForID (const int paramID, const Group* const grp) const
    {
        for (auto item : (grp != nullptr ? grp->paramTree : paramTree))
        {
            if (auto param = dynamic_cast<const Param*> (item))
                if (param->paramID == paramID)
                    return param;

            if (auto group = dynamic_cast<const Group*> (item))
                if (auto res = getParamForID (paramID, group))
                    return res;
        }

        return nullptr;
    }

    const ValueType* getValueType (const juce::String& name) const
    {
        for (auto v : valueTypes)
            if (v->name == name)
                return v;

        return nullptr;
    }

    juce::OwnedArray<Base> paramTree;
    juce::OwnedArray<ValueType> valueTypes;
    juce::OwnedArray<Template> templates;

    ValueType switchValueType;

private:
    VSTXMLInfo (const juce::XmlElement& xml)
    {
        switchValueType.entries.add (new Entry({ TRANS("Off"), Range ("[0, 0.5[") }));
        switchValueType.entries.add (new Entry({ TRANS("On"),  Range ("[0.5, 1]") }));

        for (auto* item : xml.getChildIterator())
        {
            if (item->hasTagName ("Param"))           parseParam (*item, nullptr, nullptr);
            else if (item->hasTagName ("ValueType"))  parseValueType (*item);
            else if (item->hasTagName ("Template"))   parseTemplate (*item);
            else if (item->hasTagName ("Group"))      parseGroup (*item, nullptr);
        }
    }

    void parseParam (const juce::XmlElement& item, Group* group, Template* temp)
    {
        auto param = new Param();

        if (temp != nullptr)
            param->expr = item.getStringAttribute ("id");
        else
            param->paramID = item.getIntAttribute ("id");

        param->name           = item.getStringAttribute ("name");
        param->label          = item.getStringAttribute ("label");
        param->type           = item.getStringAttribute ("type");
        param->numberOfStates = item.getIntAttribute ("numberOfStates");
        param->defaultValue   = (float) item.getDoubleAttribute ("defaultValue");

        param->shortNames.addTokens (item.getStringAttribute ("shortName"), ",", juce::StringRef());
        param->shortNames.trim();
        param->shortNames.removeEmptyStrings();

        if (group != nullptr)
        {
            group->paramTree.add (param);
            param->parent = group;
        }
        else if (temp != nullptr)
        {
            temp->params.add (param);
        }
        else
        {
            paramTree.add (param);
        }
    }

    void parseValueType (const juce::XmlElement& item)
    {
        auto vt = new ValueType();
        valueTypes.add (vt);

        vt->name  = item.getStringAttribute ("name");
        vt->label = item.getStringAttribute ("label");

        int curEntry = 0;
        const int numEntries = item.getNumChildElements();

        for (auto* entryXml : item.getChildWithTagNameIterator ("Entry"))
        {
            auto entry = new Entry();
            entry->name = entryXml->getStringAttribute ("name");

            if (entryXml->hasAttribute ("value"))
            {
                entry->range.set(entryXml->getStringAttribute ("value"));
            }
            else
            {
                entry->range.low  = (float) curEntry / (float) numEntries;
                entry->range.high = (float) (curEntry + 1) / (float) numEntries;

                entry->range.inclusiveLow  = true;
                entry->range.inclusiveHigh = (curEntry == numEntries - 1);
            }

            vt->entries.add (entry);
            ++curEntry;
        }
    }

    void parseTemplate (const juce::XmlElement& item)
    {
        auto temp = new Template();
        templates.add (temp);
        temp->name = item.getStringAttribute ("name");

        for (auto* param : item.getChildIterator())
            parseParam (*param, nullptr, temp);
    }

    void parseGroup (const juce::XmlElement& item, Group* parentGroup)
    {
        auto group = new Group();

        if (parentGroup)
        {
            parentGroup->paramTree.add (group);
            group->parent = parentGroup;
        }
        else
        {
            paramTree.add (group);
        }

        group->name = item.getStringAttribute ("name");

        if (item.hasAttribute ("template"))
        {
            juce::StringArray variables;
            variables.addTokens (item.getStringAttribute ("values"), ";", juce::StringRef());
            variables.trim();

            for (auto temp : templates)
            {
                if (temp->name == item.getStringAttribute ("template"))
                {
                    for (int i = 0; i < temp->params.size(); ++i)
                    {
                        auto param = new Param();
                        group->paramTree.add (param);

                        param->parent         = group;
                        param->paramID        = evaluate (temp->params[i]->expr, variables);
                        param->defaultValue   = temp->params[i]->defaultValue;
                        param->label          = temp->params[i]->label;
                        param->name           = temp->params[i]->name;
                        param->numberOfStates = temp->params[i]->numberOfStates;
                        param->shortNames     = temp->params[i]->shortNames;
                        param->type           = temp->params[i]->type;
                    }
                }
            }
        }
        else
        {
            for (auto* subItem : item.getChildIterator())
            {
                if (subItem->hasTagName ("Param"))       parseParam (*subItem, group, nullptr);
                else if (subItem->hasTagName ("Group"))  parseGroup (*subItem, group);
            }
        }
    }

    int evaluate (juce::String expr, const juce::StringArray& variables) const
    {
        juce::StringArray names;
        juce::Array<int> vals;

        for (auto& v : variables)
        {
            if (v.contains ("="))
            {
                names.add (v.upToFirstOccurrenceOf ("=", false, false));
                vals.add  (v.fromFirstOccurrenceOf ("=", false, false).getIntValue());
            }
        }

        for (int i = 0; i < names.size(); ++i)
        {
            for (;;)
            {
                const int idx = expr.indexOfWholeWord (names[i]);
                if (idx < 0)
                    break;

                expr = expr.replaceSection (idx, names[i].length(), juce::String (vals[i]));
            }
        }

        expr = expr.retainCharacters ("01234567890-+")
                   .replace ("+", " + ")
                   .replace ("-", " - ");

        juce::StringArray tokens;
        tokens.addTokens (expr, " ", juce::StringRef());

        bool add = true;
        int val = 0;

        for (const auto& s : tokens)
        {
            if (s == "+")
            {
                add = true;
            }
            else if (s == "-")
            {
                add = false;
            }
            else
            {
                if (add)
                    val += s.getIntValue();
                else
                    val -= s.getIntValue();
            }
        }

        return val;
    }
};

//==============================================================================
struct ModuleHandle    : public ReferenceCountedObject
{
    File file;
    MainCall moduleMain, customMain = {};
    String pluginName;
    std::unique_ptr<XmlElement> vstXml;

    using Ptr = ReferenceCountedObjectPtr<ModuleHandle>;

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

       #if JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD || JUCE_IOS || JUCE_ANDROID
        fullParentDirectoryPathName = f.getParentDirectory().getFullPathName();
       #elif JUCE_MAC
        FSRef ref;
        makeFSRefFromPath (&ref, f.getParentDirectory().getFullPathName());
        FSGetCatalogInfo (&ref, kFSCatInfoNone, nullptr, nullptr, &parentDirFSSpec, nullptr);
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

  #if JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD || JUCE_ANDROID
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
            vstXml = parseXML (file.withFileExtension ("vstxml"));

           #if JUCE_WINDOWS
            if (vstXml == nullptr)
                vstXml = parseXML (getDLLResource (file, "VSTXML", 1));
           #endif
        }

        return moduleMain != nullptr;
    }

    void close()
    {
        _fpreset(); // (doesn't do any harm)

        module.close();
    }

    void closeEffect (Vst2::VstEffectInterface* eff)
    {
        eff->dispatchFunction (eff, Vst2::plugInOpcodeClose, 0, 0, nullptr, 0);
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
    CFUniquePtr<CFBundleRef> bundleRef;

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

            if (auto url = CFUniquePtr<CFURLRef> (CFURLCreateFromFileSystemRepresentation (nullptr, (const UInt8*) utf8,
                                                                                           (CFIndex) strlen (utf8), file.isDirectory())))
            {
                bundleRef.reset (CFBundleCreate (kCFAllocatorDefault, url.get()));

                if (bundleRef != nullptr)
                {
                    if (CFBundleLoadExecutable (bundleRef.get()))
                    {
                        moduleMain = (MainCall) CFBundleGetFunctionPointerForName (bundleRef.get(), CFSTR("main_macho"));

                        if (moduleMain == nullptr)
                            moduleMain = (MainCall) CFBundleGetFunctionPointerForName (bundleRef.get(), CFSTR("VSTPluginMain"));

                        JUCE_VST_WRAPPER_LOAD_CUSTOM_MAIN

                        if (moduleMain != nullptr)
                        {
                            if (CFTypeRef name = CFBundleGetValueForInfoDictionaryKey (bundleRef.get(), CFSTR("CFBundleName")))
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
                            resFileId = CFBundleOpenBundleResourceMap (bundleRef.get());
                           #endif

                            ok = true;

                            auto vstXmlFiles = file
                                                   #if JUCE_MAC
                                                    .getChildFile ("Contents")
                                                    .getChildFile ("Resources")
                                                   #endif
                                                    .findChildFiles (File::findFiles, false, "*.vstxml");

                            if (! vstXmlFiles.isEmpty())
                                vstXml = parseXML (vstXmlFiles.getReference(0));
                        }
                    }

                    if (! ok)
                    {
                        CFBundleUnloadExecutable (bundleRef.get());
                        bundleRef = nullptr;
                    }
                }
            }
        }

        return ok;
    }

    void close()
    {
        if (bundleRef != nullptr)
        {
           #if JUCE_MAC
            CFBundleCloseBundleResourceMap (bundleRef.get(), resFileId);
           #endif

            if (CFGetRetainCount (bundleRef.get()) == 1)
                CFBundleUnloadExecutable (bundleRef.get());

            if (CFGetRetainCount (bundleRef.get()) > 0)
                bundleRef = nullptr;
        }
    }

    void closeEffect (Vst2::VstEffectInterface* eff)
    {
        eff->dispatchFunction (eff, Vst2::plugInOpcodeClose, 0, 0, nullptr, 0);
    }

  #endif

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleHandle)
};

static const int defaultVSTSampleRateValue = 44100;
static const int defaultVSTBlockSizeValue = 512;

JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4996)

//==============================================================================
struct VSTPluginInstance final   : public AudioPluginInstance,
                                   private Timer,
                                   private AsyncUpdater
{
    struct VSTParameter final   : public Parameter
    {
        VSTParameter (VSTPluginInstance& parent,
                      const String& paramName,
                      const Array<String>& shortParamNames,
                      float paramDefaultValue,
                      const String& paramLabel,
                      bool paramIsAutomatable,
                      bool paramIsDiscrete,
                      int numParamSteps,
                      bool isBoolSwitch,
                      const StringArray& paramValueStrings,
                      const VSTXMLInfo::ValueType* paramValueType)
            : pluginInstance (parent),
              name (paramName),
              shortNames (shortParamNames),
              defaultValue (paramDefaultValue),
              label (paramLabel),
              automatable (paramIsAutomatable),
              discrete (paramIsDiscrete),
              numSteps (numParamSteps),
              isSwitch (isBoolSwitch),
              vstValueStrings (paramValueStrings),
              valueType (paramValueType)
        {
        }

        float getValue() const override
        {
            if (auto* effect = pluginInstance.vstEffect)
            {
                const ScopedLock sl (pluginInstance.lock);

                return effect->getParameterValueFunction (effect, getParameterIndex());
            }

            return 0.0f;
        }

        void setValue (float newValue) override
        {
            if (auto* effect = pluginInstance.vstEffect)
            {
                const ScopedLock sl (pluginInstance.lock);

                if (effect->getParameterValueFunction (effect, getParameterIndex()) != newValue)
                    effect->setParameterValueFunction (effect, getParameterIndex(), newValue);
            }
        }

        String getText (float value, int maximumStringLength) const override
        {
            if (valueType != nullptr)
            {
                for (auto& v : valueType->entries)
                    if (v->range.contains (value))
                        return v->name;
            }

            return Parameter::getText (value, maximumStringLength);
        }

        float getValueForText (const String& text) const override
        {
            if (valueType != nullptr)
            {
                for (auto& v : valueType->entries)
                    if (v->name == text)
                        return (v->range.high + v->range.low) / 2.0f;
            }

            return Parameter::getValueForText (text);
        }

        String getCurrentValueAsText() const override
        {
            if (valueType != nullptr || ! vstValueStrings.isEmpty())
                return getText (getValue(), 1024);

            return pluginInstance.getTextForOpcode (getParameterIndex(), Vst2::plugInOpcodeGetParameterText);
        }

        float getDefaultValue() const override
        {
            return defaultValue;
        }

        String getName (int maximumStringLength) const override
        {
            if (name.isEmpty())
                return pluginInstance.getTextForOpcode (getParameterIndex(),
                                                        Vst2::plugInOpcodeGetParameterName);

            if (name.length() <= maximumStringLength)
                return name;

            if (! shortNames.isEmpty())
            {
                for (auto& n : shortNames)
                    if (n.length() <= maximumStringLength)
                        return n;

                return shortNames.getLast();
            }

            return name;
        }

        String getLabel() const override
        {
            return label.isEmpty() ? pluginInstance.getTextForOpcode (getParameterIndex(),
                                                                      Vst2::plugInOpcodeGetParameterLabel)
                                   : label;
        }

        bool isAutomatable() const override
        {
            return automatable;
        }

        bool isDiscrete() const override
        {
            return discrete;
        }

        bool isBoolean() const override
        {
            return isSwitch;
        }

        int getNumSteps() const override
        {
            return numSteps;
        }

        StringArray getAllValueStrings() const override
        {
            return vstValueStrings;
        }

        String getParameterID() const override
        {
            return String (getParameterIndex());
        }

        VSTPluginInstance& pluginInstance;

        const String name;
        const Array<String> shortNames;
        const float defaultValue;
        const String label;
        const bool automatable, discrete;
        const int numSteps;
        const bool isSwitch;
        const StringArray vstValueStrings;
        const VSTXMLInfo::ValueType* const valueType;
    };

    VSTPluginInstance (const ModuleHandle::Ptr& mh, const BusesProperties& ioConfig, Vst2::VstEffectInterface* effect,
                       double sampleRateToUse, int blockSizeToUse)
        : AudioPluginInstance (ioConfig),
          vstEffect (effect),
          vstModule (mh),
          name (mh->pluginName),
          bypassParam (new VST2BypassParameter (*this))
    {
        jassert (vstEffect != nullptr);

        if (auto* xml = vstModule->vstXml.get())
            xmlInfo.reset (VSTXMLInfo::createFor (*xml));

        refreshParameterList();

        vstSupportsBypass = (pluginCanDo ("bypass") > 0);
        setRateAndBufferSizeDetails (sampleRateToUse, blockSizeToUse);
    }

    void refreshParameterList() override
    {
        AudioProcessorParameterGroup newParameterTree;

        for (int i = 0; i < vstEffect->numParameters; ++i)
        {
            String paramName;
            Array<String> shortParamNames;
            float defaultValue = 0;
            String label;
            bool isAutomatable = dispatch (Vst2::plugInOpcodeIsParameterAutomatable, i, 0, nullptr, 0) != 0;
            bool isDiscrete = false;
            int numSteps = AudioProcessor::getDefaultNumParameterSteps();
            bool isBoolSwitch = false;
            StringArray parameterValueStrings;
            const VSTXMLInfo::ValueType* valueType = nullptr;

            if (xmlInfo != nullptr)
            {
                if (auto* param = xmlInfo->getParamForID (i, nullptr))
                {
                    paramName = param->name;

                    for (auto& n : param->shortNames)
                        shortParamNames.add (n);

                    struct LengthComparator
                    {
                        static int compareElements (const juce::String& first, const juce::String& second) noexcept
                        {
                            return first.length() - second.length();
                        }
                    };

                    LengthComparator comp;
                    shortParamNames.sort (comp);

                    defaultValue = param->defaultValue;
                    label = param->label;

                    if (param->type == "switch")
                    {
                        isBoolSwitch = true;
                        numSteps = 2;
                        valueType = &xmlInfo->switchValueType;
                    }
                    else
                    {
                        valueType = xmlInfo->getValueType (param->type);
                    }

                    if (param->numberOfStates >= 2)
                    {
                        numSteps = param->numberOfStates;

                        if (valueType != nullptr)
                        {
                            for (auto* entry : valueType->entries)
                                parameterValueStrings.add (entry->name);

                            parameterValueStrings.removeEmptyStrings();
                        }
                    }

                    isDiscrete = (numSteps != AudioProcessor::getDefaultNumParameterSteps());
                }
            }

            newParameterTree.addChild (std::make_unique<VSTParameter> (*this, paramName, shortParamNames, defaultValue,
                                                                       label, isAutomatable, isDiscrete, numSteps,
                                                                       isBoolSwitch, parameterValueStrings, valueType));
        }

        setHostedParameterTree (std::move (newParameterTree));
    }

    ~VSTPluginInstance() override
    {
        if (vstEffect != nullptr && vstEffect->interfaceIdentifier == Vst2::juceVstInterfaceIdentifier)
            callOnMessageThread ([this] { cleanup(); });
    }

    void cleanup()
    {
        if (vstEffect != nullptr && vstEffect->interfaceIdentifier == Vst2::juceVstInterfaceIdentifier)
        {
           #if JUCE_MAC
            if (vstModule->resFileId != 0)
                UseResFile (vstModule->resFileId);
           #endif

            // Must delete any editors before deleting the plugin instance!
            jassert (getActiveEditor() == nullptr);

            _fpreset(); // some dodgy plug-ins mess around with this

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

            newEffect->dispatchFunction (newEffect, Vst2::plugInOpcodeIdentify, 0, 0, nullptr, 0);

            auto blockSize = jmax (32, initialBlockSize);

            newEffect->dispatchFunction (newEffect, Vst2::plugInOpcodeSetSampleRate, 0, 0, nullptr, static_cast<float> (initialSampleRate));
            newEffect->dispatchFunction (newEffect, Vst2::plugInOpcodeSetBlockSize,  0, blockSize, nullptr, 0);

            newEffect->dispatchFunction (newEffect, Vst2::plugInOpcodeOpen, 0, 0, nullptr, 0);
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
            dispatch (Vst2::plugInOpcodeGetPlugInName, 0, 0, buffer, 0);

            desc.descriptiveName = String::createStringFromData (buffer, (int) sizeof (buffer)).trim();

            if (desc.descriptiveName.isEmpty())
                desc.descriptiveName = name;
        }

        desc.fileOrIdentifier = vstModule->file.getFullPathName();
        desc.uniqueId = desc.deprecatedUid = getUID();
        desc.lastFileModTime = vstModule->file.getLastModificationTime();
        desc.lastInfoUpdateTime = Time::getCurrentTime();
        desc.pluginFormatName = "VST";
        desc.category = getCategory();

        {
            char buffer[512] = { 0 };
            dispatch (Vst2::plugInOpcodeGetManufacturerName, 0, 0, buffer, 0);
            desc.manufacturerName = String::createStringFromData (buffer, (int) sizeof (buffer)).trim();
        }

        desc.version = getVersion();
        desc.numInputChannels = getTotalNumInputChannels();
        desc.numOutputChannels = getTotalNumOutputChannels();
        desc.isInstrument = isSynthPlugin();
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
        JUCE_ASSERT_MESSAGE_THREAD
       #endif

        JUCE_VST_LOG ("Initialising VST: " + vstModule->pluginName + " (" + getVersion() + ")");
        initialised = true;

        setRateAndBufferSizeDetails (initialSampleRate, initialBlockSize);

        dispatch (Vst2::plugInOpcodeIdentify, 0, 0, nullptr, 0);

        if (getSampleRate() > 0)
            dispatch (Vst2::plugInOpcodeSetSampleRate, 0, 0, nullptr, (float) getSampleRate());

        if (getBlockSize() > 0)
            dispatch (Vst2::plugInOpcodeSetBlockSize, 0, jmax (32, getBlockSize()), nullptr, 0);

        dispatch (Vst2::plugInOpcodeOpen, 0, 0, nullptr, 0);

        setRateAndBufferSizeDetails (getSampleRate(), getBlockSize());

        if (getNumPrograms() > 1)
            setCurrentProgram (0);
        else
            dispatch (Vst2::plugInOpcodeSetCurrentProgram, 0, 0, nullptr, 0);

        for (int i = vstEffect->numInputChannels;  --i >= 0;)  dispatch (Vst2::plugInOpcodeConnectInput,  i, 1, nullptr, 0);
        for (int i = vstEffect->numOutputChannels; --i >= 0;)  dispatch (Vst2::plugInOpcodeConnectOutput, i, 1, nullptr, 0);

        if (getVstCategory() != Vst2::kPlugCategShell) // (workaround for Waves 5 plugins which crash during this call)
            updateStoredProgramNames();

        wantsMidiMessages = pluginCanDo ("receiveVstMidiEvent") > 0 || isSynthPlugin();

        setLatencySamples (vstEffect->latency);
    }

    void getExtensions (ExtensionsVisitor& visitor) const override
    {
        struct Extensions : public ExtensionsVisitor::VSTClient
        {
            explicit Extensions (const VSTPluginInstance* instanceIn) : instance (instanceIn) {}

            AEffect* getAEffectPtr() const noexcept override   { return reinterpret_cast<AEffect*> (instance->vstEffect); }

            const VSTPluginInstance* instance = nullptr;
        };

        visitor.visitVSTClient (Extensions { this });
    }

    void* getPlatformSpecificData() override    { return vstEffect; }

    const String getName() const override
    {
        if (vstEffect != nullptr)
        {
            char buffer[512] = { 0 };

            if (dispatch (Vst2::plugInOpcodeGetManufacturerProductName, 0, 0, buffer, 0) != 0)
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

        if ((vstEffect->flags & 512) != 0)
            return 0.0;

        auto tailSize = dispatch (Vst2::plugInOpcodeGetTailSize, 0, 0, nullptr, 0);
        auto sampleRate = getSampleRate();

        // remain backward compatible with old JUCE plug-ins: anything larger
        // than INT32_MAX is an invalid tail time but old JUCE 64-bit plug-ins
        // would return INT64_MAX for infinite tail time. So treat anything
        // equal or greater than INT32_MAX as infinite tail time.
        if (tailSize >= std::numeric_limits<int32>::max())
            return std::numeric_limits<double>::infinity();

        if (tailSize >= 0 && sampleRate > 0)
            return static_cast<double> (tailSize) / sampleRate;

        return 0.0;
    }

    bool acceptsMidi() const override    { return wantsMidiMessages; }
    bool producesMidi() const override   { return pluginCanDo ("sendVstMidiEvent") > 0; }
    bool supportsMPE() const override    { return pluginCanDo ("MPE") > 0; }

    Vst2::VstPlugInCategory getVstCategory() const noexcept     { return (Vst2::VstPlugInCategory) dispatch (Vst2::plugInOpcodeGetPlugInCategory, 0, 0, nullptr, 0); }

    bool isSynthPlugin() const  { return (vstEffect != nullptr && (vstEffect->flags & Vst2::vstEffectFlagIsSynth) != 0); }

    int pluginCanDo (const char* text) const  { return (int) dispatch (Vst2::plugInOpcodeCanPlugInDo, 0, 0, (void*) text,  0); }

    //==============================================================================
    void prepareToPlay (double rate, int samplesPerBlockExpected) override
    {
        auto numInputBuses  = getBusCount (true);
        auto numOutputBuses = getBusCount (false);

        setRateAndBufferSizeDetails (rate, samplesPerBlockExpected);

        if (numInputBuses <= 1 && numOutputBuses <= 1)
        {
            SpeakerMappings::VstSpeakerConfigurationHolder inArr  (getChannelLayoutOfBus (true,  0));
            SpeakerMappings::VstSpeakerConfigurationHolder outArr (getChannelLayoutOfBus (false, 0));

            dispatch (Vst2::plugInOpcodeSetSpeakerConfiguration, 0, (pointer_sized_int) &inArr.get(), (void*) &outArr.get(), 0.0f);
        }

        vstHostTime.tempoBPM = 120.0;
        vstHostTime.timeSignatureNumerator = 4;
        vstHostTime.timeSignatureDenominator = 4;
        vstHostTime.sampleRate = rate;
        vstHostTime.samplePosition = 0;
        vstHostTime.flags = Vst2::vstTimingInfoFlagNanosecondsValid
                              | Vst2::vstTimingInfoFlagAutomationWriteModeActive
                              | Vst2::vstTimingInfoFlagAutomationReadModeActive;

        initialise (rate, samplesPerBlockExpected);

        if (initialised)
        {
            wantsMidiMessages = wantsMidiMessages || (pluginCanDo ("receiveVstMidiEvent") > 0) || isSynthPlugin();

            if (wantsMidiMessages)
                midiEventsToSend.ensureSize (256);
            else
                midiEventsToSend.freeEvents();

            incomingMidi.clear();

            dispatch (Vst2::plugInOpcodeSetSampleRate, 0, 0, nullptr, (float) rate);
            dispatch (Vst2::plugInOpcodeSetBlockSize, 0, jmax (16, samplesPerBlockExpected), nullptr, 0);

            if (supportsDoublePrecisionProcessing())
            {
                int32 vstPrecision = isUsingDoublePrecision() ? Vst2::vstProcessingSampleTypeDouble
                                                              : Vst2::vstProcessingSampleTypeFloat;

                dispatch (Vst2::plugInOpcodeSetSampleFloatType, 0, (pointer_sized_int) vstPrecision, nullptr, 0);
            }

            auto maxChannels = jmax (1, jmax (vstEffect->numInputChannels, vstEffect->numOutputChannels));

            tmpBufferFloat .setSize (maxChannels, samplesPerBlockExpected);
            tmpBufferDouble.setSize (maxChannels, samplesPerBlockExpected);

            channelBufferFloat .calloc (static_cast<size_t> (maxChannels));
            channelBufferDouble.calloc (static_cast<size_t> (maxChannels));

            outOfPlaceBuffer.setSize (jmax (1, vstEffect->numOutputChannels), samplesPerBlockExpected);

            if (! isPowerOn)
                setPower (true);

            // dodgy hack to force some plugins to initialise the sample rate.
            if (! hasEditor())
            {
                if (auto* firstParam = getParameters()[0])
                {
                    auto old = firstParam->getValue();
                    firstParam->setValue ((old < 0.5f) ? 1.0f : 0.0f);
                    firstParam->setValue (old);
                }
            }

            dispatch (Vst2::plugInOpcodeStartProcess, 0, 0, nullptr, 0);

            setLatencySamples (vstEffect->latency);
        }
    }

    void releaseResources() override
    {
        if (initialised)
        {
            dispatch (Vst2::plugInOpcodeStopProcess, 0, 0, nullptr, 0);
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

    //==============================================================================
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        jassert (! isUsingDoublePrecision());
        processAudio (buffer, midiMessages, tmpBufferFloat, channelBufferFloat, false);
    }

    void processBlock (AudioBuffer<double>& buffer, MidiBuffer& midiMessages) override
    {
        jassert (isUsingDoublePrecision());
        processAudio (buffer, midiMessages, tmpBufferDouble, channelBufferDouble, false);
    }

    void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        jassert (! isUsingDoublePrecision());
        processAudio (buffer, midiMessages, tmpBufferFloat, channelBufferFloat, true);
    }

    void processBlockBypassed (AudioBuffer<double>& buffer, MidiBuffer& midiMessages) override
    {
        jassert (isUsingDoublePrecision());
        processAudio (buffer, midiMessages, tmpBufferDouble, channelBufferDouble, true);
    }

    //==============================================================================
    bool supportsDoublePrecisionProcessing() const override
    {
        return ((vstEffect->flags & Vst2::vstEffectFlagInplaceAudio) != 0
             && (vstEffect->flags & Vst2::vstEffectFlagInplaceDoubleAudio) != 0);
    }

    AudioProcessorParameter* getBypassParameter() const override               { return vstSupportsBypass ? bypassParam.get() : nullptr; }

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
    bool hasEditor() const override                  { return vstEffect != nullptr && (vstEffect->flags & Vst2::vstEffectFlagHasEditor) != 0; }
   #endif

    AudioProcessorEditor* createEditor() override;

    //==============================================================================
    const String getInputChannelName (int index) const override
    {
        if (isValidChannel (index, true))
        {
            Vst2::VstPinInfo pinProps;
            if (dispatch (Vst2::plugInOpcodeGetInputPinProperties, index, 0, &pinProps, 0.0f) != 0)
                return String (pinProps.text, sizeof (pinProps.text));
        }

        return {};
    }

    bool isInputChannelStereoPair (int index) const override
    {
        if (! isValidChannel (index, true))
            return false;

        Vst2::VstPinInfo pinProps;
        if (dispatch (Vst2::plugInOpcodeGetInputPinProperties, index, 0, &pinProps, 0.0f) != 0)
            return (pinProps.flags & Vst2::vstPinInfoFlagIsStereo) != 0;

        return true;
    }

    const String getOutputChannelName (int index) const override
    {
        if (isValidChannel (index, false))
        {
            Vst2::VstPinInfo pinProps;
            if (dispatch (Vst2::plugInOpcodeGetOutputPinProperties, index, 0, &pinProps, 0.0f) != 0)
                return String (pinProps.text, sizeof (pinProps.text));
        }

        return {};
    }

    bool isOutputChannelStereoPair (int index) const override
    {
        if (! isValidChannel (index, false))
            return false;

        Vst2::VstPinInfo pinProps;
        if (dispatch (Vst2::plugInOpcodeGetOutputPinProperties, index, 0, &pinProps, 0.0f) != 0)
            return (pinProps.flags & Vst2::vstPinInfoFlagIsStereo) != 0;

        return true;
    }

    bool isValidChannel (int index, bool isInput) const noexcept
    {
        return isPositiveAndBelow (index, isInput ? getTotalNumInputChannels()
                                                  : getTotalNumOutputChannels());
    }

    //==============================================================================
    int getNumPrograms() override          { return vstEffect != nullptr ? jmax (0, vstEffect->numPrograms) : 0; }

    // NB: some plugs return negative numbers from this function.
    int getCurrentProgram() override       { return (int) dispatch (Vst2::plugInOpcodeGetCurrentProgram, 0, 0, nullptr, 0); }

    void setCurrentProgram (int newIndex) override
    {
        if (getNumPrograms() > 0 && newIndex != getCurrentProgram())
            dispatch (Vst2::plugInOpcodeSetCurrentProgram, 0, jlimit (0, getNumPrograms() - 1, newIndex), nullptr, 0);
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

                if (dispatch (Vst2::plugInOpcodeGetProgramName, jlimit (0, getNumPrograms() - 1, index), -1, nm, 0) != 0)
                    return String::fromUTF8 (nm).trim();
            }
        }

        return {};
    }

    void changeProgramName (int index, const String& newName) override
    {
        if (index >= 0 && index == getCurrentProgram())
        {
            if (getNumPrograms() > 0 && newName != getCurrentProgramName())
                dispatch (Vst2::plugInOpcodeSetCurrentProgramName, 0, 0, (void*) newName.substring (0, 24).toRawUTF8(), 0.0f);
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
        if (dispatch (Vst2::plugInOpcodeIdle, 0, 0, nullptr, 0) == 0)
            stopTimer();
    }

    void handleAsyncUpdate() override
    {
        updateHostDisplay (AudioProcessorListener::ChangeDetails().withProgramChanged (true)
                                                                  .withParameterInfoChanged (true));
    }

    pointer_sized_int handleCallback (int32 opcode, int32 index, pointer_sized_int value, void* ptr, float opt)
    {
        switch (opcode)
        {
            case Vst2::hostOpcodeParameterChanged:
                if (auto* param = getParameters()[index])
                    param->sendValueChangedMessageToListeners (opt);
                else
                    jassertfalse; // Invalid parameter index!

                break;

            case Vst2::hostOpcodePreAudioProcessingEvents:    handleMidiFromPlugin ((const Vst2::VstEventBlock*) ptr); break;
            case Vst2::hostOpcodeGetTimingInfo:               return getVSTTime();
            case Vst2::hostOpcodeIdle:                        handleIdle(); break;
            case Vst2::hostOpcodeWindowSize:                  setWindowSize (index, (int) value); return 1;
            case Vst2::hostOpcodeUpdateView:                  triggerAsyncUpdate(); break;
            case Vst2::hostOpcodeIOModified:                  setLatencySamples (vstEffect->latency); break;
            case Vst2::hostOpcodeNeedsIdle:                   startTimer (50); break;

            case Vst2::hostOpcodeGetSampleRate:               return (pointer_sized_int) (getSampleRate() > 0 ? getSampleRate() : defaultVSTSampleRateValue);
            case Vst2::hostOpcodeGetBlockSize:                return (pointer_sized_int) (getBlockSize() > 0  ? getBlockSize()  : defaultVSTBlockSizeValue);
            case Vst2::hostOpcodePlugInWantsMidi:             wantsMidiMessages = true; break;
            case Vst2::hostOpcodeGetDirectory:                return getVstDirectory();

            case Vst2::hostOpcodeTempoAt:                     return (pointer_sized_int) (extraFunctions != nullptr ? extraFunctions->getTempoAt ((int64) value) : 0);
            case Vst2::hostOpcodeGetAutomationState:          return (pointer_sized_int) (extraFunctions != nullptr ? extraFunctions->getAutomationState() : 0);

            case Vst2::hostOpcodeParameterChangeGestureBegin:
                if (auto* param = getParameters()[index])
                    param->beginChangeGesture();
                else
                    jassertfalse; // Invalid parameter index!

                break;

            case Vst2::hostOpcodeParameterChangeGestureEnd:
                if (auto* param = getParameters()[index])
                    param->endChangeGesture();
                else
                    jassertfalse; // Invalid parameter index!

                break;

            case Vst2::hostOpcodePinConnected:                   return isValidChannel (index, value == 0) ? 0 : 1; // (yes, 0 = true)
            case Vst2::hostOpcodeGetCurrentAudioProcessingLevel: return isNonRealtime() ? 4 : 0;

            // none of these are handled (yet)...
            case Vst2::hostOpcodeSetTime:
            case Vst2::hostOpcodeGetParameterInterval:
            case Vst2::hostOpcodeGetInputLatency:
            case Vst2::hostOpcodeGetOutputLatency:
            case Vst2::hostOpcodeGetPreviousPlugIn:
            case Vst2::hostOpcodeGetNextPlugIn:
            case Vst2::hostOpcodeWillReplace:
            case Vst2::hostOpcodeOfflineStart:
            case Vst2::hostOpcodeOfflineReadSource:
            case Vst2::hostOpcodeOfflineWrite:
            case Vst2::hostOpcodeOfflineGetCurrentPass:
            case Vst2::hostOpcodeOfflineGetCurrentMetaPass:
            case Vst2::hostOpcodeGetOutputSpeakerConfiguration:
            case Vst2::hostOpcodeManufacturerSpecific:
            case Vst2::hostOpcodeSetIcon:
            case Vst2::hostOpcodeGetLanguage:
            case Vst2::hostOpcodeOpenEditorWindow:
            case Vst2::hostOpcodeCloseEditorWindow:
                break;

            default:
                return handleGeneralCallback (opcode, index, value, ptr, opt);
        }

        return 0;
    }

    // handles non plugin-specific callbacks.
    static pointer_sized_int handleGeneralCallback (int32 opcode, int32 /*index*/, pointer_sized_int /*value*/, void* ptr, float /*opt*/)
    {
        switch (opcode)
        {
            case Vst2::hostOpcodeCanHostDo:                         return handleCanDo ((const char*) ptr);
            case Vst2::hostOpcodeVstVersion:                        return 2400;
            case Vst2::hostOpcodeCurrentId:                         return shellUIDToCreate;
            case Vst2::hostOpcodeGetNumberOfAutomatableParameters:  return 0;
            case Vst2::hostOpcodeGetAutomationState:                return 1;
            case Vst2::hostOpcodeGetManufacturerVersion:            return 0x0101;

            case Vst2::hostOpcodeGetManufacturerName:
            case Vst2::hostOpcodeGetProductName:                    return getHostName ((char*) ptr);

            case Vst2::hostOpcodeGetSampleRate:                     return (pointer_sized_int) defaultVSTSampleRateValue;
            case Vst2::hostOpcodeGetBlockSize:                      return (pointer_sized_int) defaultVSTBlockSizeValue;
            case Vst2::hostOpcodeSetOutputSampleRate:               return 0;

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
                        auto prog = addBytesToPointer (set->programs, i * progLen);

                        if (getAddressDifference (prog, set) >= (int) dataSize)
                            return false;

                        if (fxbSwap (set->numPrograms) > 0)
                            setCurrentProgram (i);

                        if (! restoreProgramSettings (prog))
                            return false;
                    }
                }

                if (fxbSwap (set->numPrograms) > 0)
                    setCurrentProgram (oldProg);

                auto prog = addBytesToPointer (set->programs, oldProg * progLen);

                if (getAddressDifference (prog, set) >= (int) dataSize)
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
                if (auto* param = getParameters()[i])
                    param->setValue (fxbSwapFloat (prog->params[i]));
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
        auto numParams = getParameters().size();

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
                auto len = (size_t) (progLen * jmax (1, numPrograms)) + (sizeof (fxSet) - sizeof (fxProgram));
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
                    setParamsInProgramBlock (addBytesToPointer (set->programs, oldProgram * progLen));

                for (int i = 0; i < numPrograms; ++i)
                {
                    if (i != oldProgram)
                    {
                        setCurrentProgram (i);
                        setParamsInProgramBlock (addBytesToPointer (set->programs, i * progLen));
                    }
                }

                if (oldProgram >= 0)
                    setCurrentProgram (oldProgram);

                restoreFromTempParameterStore (oldSettings);
            }
            else
            {
                dest.setSize ((size_t) ((numParams - 1) * (int) sizeof (float)) + sizeof (fxProgram), true);
                setParamsInProgramBlock ((fxProgram*) dest.getData());
            }
        }

        return true;
    }

    bool usesChunks() const noexcept        { return vstEffect != nullptr && (vstEffect->flags & Vst2::vstEffectFlagDataInChunks) != 0; }

    bool getChunkData (MemoryBlock& mb, bool isPreset, int maxSizeMB) const
    {
        if (usesChunks())
        {
            void* data = nullptr;
            auto bytes = (size_t) dispatch (Vst2::plugInOpcodeGetData, isPreset ? 1 : 0, 0, &data, 0.0f);

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
            dispatch (Vst2::plugInOpcodeSetData, isPreset ? 1 : 0, size, (void*) data, 0.0f);

            if (! isPreset)
                updateStoredProgramNames();

            return true;
        }

        return false;
    }

    bool updateSizeFromEditor (int w, int h);

    Vst2::VstEffectInterface* vstEffect;
    ModuleHandle::Ptr vstModule;

    std::unique_ptr<VSTPluginFormat::ExtraFunctions> extraFunctions;

private:
    //==============================================================================
    struct VST2BypassParameter final : public Parameter
    {
        VST2BypassParameter (VSTPluginInstance& effectToUse)
            : parent (effectToUse),
              vstOnStrings (TRANS("on"), TRANS("yes"), TRANS("true")),
              vstOffStrings (TRANS("off"), TRANS("no"), TRANS("false")),
              values (TRANS("Off"), TRANS("On"))
        {
        }

        void setValue (float newValue) override
        {
            currentValue = (newValue != 0.0f);

            if (parent.vstSupportsBypass)
                parent.dispatch (Vst2::plugInOpcodeSetBypass, 0, currentValue ? 1 : 0, nullptr, 0.0f);
        }

        float getValueForText (const String& text) const override
        {
            String lowercaseText (text.toLowerCase());

            for (auto& testText : vstOnStrings)
                if (lowercaseText == testText)
                    return 1.0f;

            for (auto& testText : vstOffStrings)
                if (lowercaseText == testText)
                    return 0.0f;

            return text.getIntValue() != 0 ? 1.0f : 0.0f;
        }

        float getValue() const override                                     { return currentValue; }
        float getDefaultValue() const override                              { return 0.0f; }
        String getName (int /*maximumStringLength*/) const override         { return "Bypass"; }
        String getText (float value, int) const override                    { return (value != 0.0f ? TRANS("On") : TRANS("Off")); }
        bool isAutomatable() const override                                 { return true; }
        bool isDiscrete() const override                                    { return true; }
        bool isBoolean() const override                                     { return true; }
        int getNumSteps() const override                                    { return 2; }
        StringArray getAllValueStrings() const override                     { return values; }
        String getLabel() const override                                    { return {}; }
        String getParameterID() const override                              { return {}; }

        VSTPluginInstance& parent;
        bool currentValue = false;
        StringArray vstOnStrings, vstOffStrings, values;
    };

    //==============================================================================
    String name;
    CriticalSection lock;
    std::atomic<bool> wantsMidiMessages { false };
    bool initialised = false;
    std::atomic<bool> isPowerOn { false };
    bool lastProcessBlockCallWasBypass = false, vstSupportsBypass = false;
    mutable StringArray programNames;
    AudioBuffer<float> outOfPlaceBuffer;

    CriticalSection midiInLock;
    MidiBuffer incomingMidi;
    VSTMidiEventList midiEventsToSend;
    Vst2::VstTimingInformation vstHostTime;

    AudioBuffer<float> tmpBufferFloat;
    HeapBlock<float*> channelBufferFloat;

    AudioBuffer<double> tmpBufferDouble;
    HeapBlock<double*> channelBufferDouble;
    std::unique_ptr<VST2BypassParameter> bypassParam;

    std::unique_ptr<VSTXMLInfo> xmlInfo;

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
        String hostName (JUCE_VST_FALLBACK_HOST_NAME);

        if (auto* app = JUCEApplicationBase::getInstance())
            hostName = app->getApplicationName();

        hostName.copyToUTF8 (name, (size_t) jmin (Vst2::vstMaxManufacturerStringLength, Vst2::vstMaxPlugInNameStringLength) - 1);
        return 1;
    }

    pointer_sized_int getVSTTime() noexcept
    {
        JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4311)

        return (pointer_sized_int) &vstHostTime;

        JUCE_END_IGNORE_WARNINGS_MSVC
    }

    void handleIdle()
    {
        if (insideVSTCallback == 0 && MessageManager::getInstance()->isThisTheMessageThread())
        {
            const IdleCallRecursionPreventer icrp;

           #if JUCE_MAC
            if (getActiveEditor() != nullptr)
                dispatch (Vst2::plugInOpcodeEditorIdle, 0, 0, nullptr, 0);
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
       #if JUCE_LINUX || JUCE_BSD
        const MessageManagerLock mmLock;
       #endif

        updateSizeFromEditor (width, height);
    }

    //==============================================================================
    static Vst2::VstEffectInterface* constructEffect (const ModuleHandle::Ptr& module)
    {
        Vst2::VstEffectInterface* effect = nullptr;
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

            if (effect != nullptr && effect->interfaceIdentifier == Vst2::juceVstInterfaceIdentifier)
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

    static BusesProperties queryBusIO (Vst2::VstEffectInterface* effect)
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

            effect->dispatchFunction (effect, Vst2::plugInOpcodeSetSpeakerConfiguration, 0,
                                      (pointer_sized_int) &canonicalIn.get(), (void*) &canonicalOut.get(), 0.0f);
        }

        const auto arrangement = getSpeakerArrangementWrapper (effect);

        for (int dir = 0; dir < 2; ++dir)
        {
            const bool isInput = (dir == 0);
            const int opcode = (isInput ? Vst2::plugInOpcodeGetInputPinProperties : Vst2::plugInOpcodeGetOutputPinProperties);
            const int maxChannels = (isInput ? effect->numInputChannels : effect->numOutputChannels);
            const auto* arr = (isInput ? arrangement.in : arrangement.out);
            bool busAdded = false;

            Vst2::VstPinInfo pinProps;
            AudioChannelSet layout;

            for (int ch = 0; ch < maxChannels; ch += layout.size())
            {
                if (effect->dispatchFunction (effect, opcode, ch, 0, &pinProps, 0.0f) == 0)
                    break;

                if ((pinProps.flags & Vst2::vstPinInfoFlagValid) != 0)
                {
                    layout = SpeakerMappings::vstArrangementTypeToChannelSet (pinProps.configurationType, 0);

                    if (layout.isDisabled())
                        break;
                }
                else if (arr == nullptr)
                {
                    layout = ((pinProps.flags & Vst2::vstPinInfoFlagIsStereo) != 0 ? AudioChannelSet::stereo() : AudioChannelSet::mono());
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

    static bool pluginHasDefaultChannelLayouts (Vst2::VstEffectInterface* effect)
    {
        if (getSpeakerArrangementWrapper (effect).isValid())
            return true;

        for (int dir = 0; dir < 2; ++dir)
        {
            const bool isInput = (dir == 0);
            const int opcode = (isInput ? Vst2::plugInOpcodeGetInputPinProperties : Vst2::plugInOpcodeGetOutputPinProperties);
            const int maxChannels = (isInput ? effect->numInputChannels : effect->numOutputChannels);

            int channels = 1;

            for (int ch = 0; ch < maxChannels; ch += channels)
            {
                Vst2::VstPinInfo pinProps;

                if (effect->dispatchFunction (effect, opcode, ch, 0, &pinProps, 0.0f) == 0)
                    return false;

                if ((pinProps.flags & Vst2::vstPinInfoFlagValid) != 0)
                    return true;

                channels = (pinProps.flags & Vst2::vstPinInfoFlagIsStereo) != 0 ? 2 : 1;
            }
        }

        return false;
    }

    struct SpeakerArrangements
    {
        const Vst2::VstSpeakerConfiguration* in;
        const Vst2::VstSpeakerConfiguration* out;

        bool isValid() const noexcept { return in != nullptr && out != nullptr; }
    };

    static SpeakerArrangements getSpeakerArrangementWrapper (Vst2::VstEffectInterface* effect)
    {
        // Workaround: unfortunately old JUCE VST-2 plug-ins had a bug and would crash if
        // you try to get the speaker arrangement when there are no input channels present.
        // Hopefully, one day (when there are no more old JUCE plug-ins around), we can
        // comment out the next two lines.
        if (effect->numInputChannels == 0)
            return { nullptr, nullptr };

        SpeakerArrangements result { nullptr, nullptr };
        const auto dispatchResult = effect->dispatchFunction (effect,
                                                              Vst2::plugInOpcodeGetSpeakerArrangement,
                                                              0,
                                                              reinterpret_cast<pointer_sized_int> (&result.in),
                                                              &result.out,
                                                              0.0f);

        if (dispatchResult != 0)
            return result;

        return { nullptr, nullptr };
    }

    //==============================================================================
    template <typename FloatType>
    void processAudio (AudioBuffer<FloatType>& buffer, MidiBuffer& midiMessages,
                       AudioBuffer<FloatType>& tmpBuffer,
                       HeapBlock<FloatType*>& channelBuffer,
                       bool processBlockBypassedCalled)
    {
        if (vstSupportsBypass)
        {
            updateBypass (processBlockBypassedCalled);
        }
        else if (processBlockBypassedCalled)
        {
            // if this vst does not support bypass then we will have to do this ourselves
            AudioProcessor::processBlockBypassed (buffer, midiMessages);
            return;
        }

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
                    vstHostTime.flags |= Vst2::vstTimingInfoFlagTempoValid
                                       | Vst2::vstTimingInfoFlagTimeSignatureValid
                                       | Vst2::vstTimingInfoFlagMusicalPositionValid
                                       | Vst2::vstTimingInfoFlagLastBarPositionValid;

                    if (const auto* hostTimeNs = getHostTimeNs())
                    {
                        vstHostTime.systemTimeNanoseconds = (double) *hostTimeNs;
                        vstHostTime.flags |= Vst2::vstTimingInfoFlagNanosecondsValid;
                    }
                    else
                    {
                        vstHostTime.flags &= ~Vst2::vstTimingInfoFlagNanosecondsValid;
                    }

                    int32 newTransportFlags = 0;
                    if (position.isPlaying)     newTransportFlags |= Vst2::vstTimingInfoFlagCurrentlyPlaying;
                    if (position.isRecording)   newTransportFlags |= Vst2::vstTimingInfoFlagCurrentlyRecording;

                    if (newTransportFlags != (vstHostTime.flags & (Vst2::vstTimingInfoFlagCurrentlyPlaying
                                                                   | Vst2::vstTimingInfoFlagCurrentlyRecording)))
                        vstHostTime.flags = (vstHostTime.flags & ~(Vst2::vstTimingInfoFlagCurrentlyPlaying | Vst2::vstTimingInfoFlagCurrentlyRecording)) | newTransportFlags | Vst2::vstTimingInfoFlagTransportChanged;
                    else
                        vstHostTime.flags &= ~Vst2::vstTimingInfoFlagTransportChanged;

                    const auto optionalFrameRate = [&fr = position.frameRate]() -> Optional<int32>
                    {
                        switch (fr.getBaseRate())
                        {
                            case 24:        return fr.isPullDown() ? Vst2::vstSmpteRateFps239 : Vst2::vstSmpteRateFps24;
                            case 25:        return fr.isPullDown() ? Vst2::vstSmpteRateFps249 : Vst2::vstSmpteRateFps25;
                            case 30:        return fr.isPullDown() ? (fr.isDrop() ? Vst2::vstSmpteRateFps2997drop : Vst2::vstSmpteRateFps2997)
                                                                   : (fr.isDrop() ? Vst2::vstSmpteRateFps30drop   : Vst2::vstSmpteRateFps30);
                            case 60:        return fr.isPullDown() ? Vst2::vstSmpteRateFps599 : Vst2::vstSmpteRateFps60;
                        }

                        return {};
                    }();

                    vstHostTime.flags |= optionalFrameRate ? Vst2::vstTimingInfoFlagSmpteValid : 0;
                    vstHostTime.smpteRate = optionalFrameRate.orFallback (0);
                    vstHostTime.smpteOffset = (int32) (position.timeInSeconds * 80.0 * position.frameRate.getEffectiveRate() + 0.5);

                    if (position.isLooping)
                    {
                        vstHostTime.loopStartPosition = position.ppqLoopStart;
                        vstHostTime.loopEndPosition   = position.ppqLoopEnd;
                        vstHostTime.flags |= (Vst2::vstTimingInfoFlagLoopPositionValid | Vst2::vstTimingInfoFlagLoopActive);
                    }
                    else
                    {
                        vstHostTime.flags &= ~(Vst2::vstTimingInfoFlagLoopPositionValid | Vst2::vstTimingInfoFlagLoopActive);
                    }
                }
            }

            vstHostTime.systemTimeNanoseconds = getVSTHostTimeNanoseconds();

            if (wantsMidiMessages)
            {
                midiEventsToSend.clear();
                midiEventsToSend.ensureSize (1);

                for (const auto metadata : midiMessages)
                    midiEventsToSend.addEvent (metadata.data, metadata.numBytes,
                                               jlimit (0, numSamples - 1, metadata.samplePosition));

                vstEffect->dispatchFunction (vstEffect, Vst2::plugInOpcodePreAudioProcessingEvents, 0, 0, midiEventsToSend.events, 0);
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
            // Not initialised, so just bypass.
            for (int i = getTotalNumOutputChannels(); --i >= 0;)
                buffer.clear (i, 0, buffer.getNumSamples());
        }

        {
            // copy any incoming midi.
            const ScopedLock sl (midiInLock);

            midiMessages.swapWith (incomingMidi);
            incomingMidi.clear();
        }
    }

    //==============================================================================
    inline void invokeProcessFunction (AudioBuffer<float>& buffer, int32 sampleFrames)
    {
        if ((vstEffect->flags & Vst2::vstEffectFlagInplaceAudio) != 0)
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
    bool restoreProgramSettings (const fxProgram* const prog)
    {
        if (compareMagic (prog->chunkMagic, "CcnK")
             && compareMagic (prog->fxMagic, "FxCk"))
        {
            changeProgramName (getCurrentProgram(), prog->prgName);

            for (int i = 0; i < fxbSwap (prog->numParams); ++i)
                if (auto* param = getParameters()[i])
                    param->setValue (fxbSwapFloat (prog->params[i]));

            return true;
        }

        return false;
    }

    String getTextForOpcode (const int index, const int opcode) const
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
                dispatch (Vst2::plugInOpcodeGetCurrentProgramName, 0, 0, nm, 0);
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
        auto numParams = getParameters().size();

        prog->chunkMagic = fxbName ("CcnK");
        prog->byteSize = 0;
        prog->fxMagic = fxbName ("FxCk");
        prog->version = fxbSwap (fxbVersionNum);
        prog->fxID = fxbSwap (getUID());
        prog->fxVersion = fxbSwap (getVersionNumber());
        prog->numParams = fxbSwap (numParams);

        getCurrentProgramName().copyToUTF8 (prog->prgName, sizeof (prog->prgName) - 1);

        for (int i = 0; i < numParams; ++i)
            if (auto* param = getParameters()[i])
                prog->params[i] = fxbSwapFloat (param->getValue());
    }

    void updateStoredProgramNames()
    {
        if (vstEffect != nullptr && getNumPrograms() > 0)
        {
            char nm[256] = { 0 };

            // only do this if the plugin can't use indexed names.
            if (dispatch (Vst2::plugInOpcodeGetProgramName, 0, -1, nm, 0) == 0)
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

    void handleMidiFromPlugin (const Vst2::VstEventBlock* events)
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
        auto numParameters = getParameters().size();
        dest.setSize (64 + 4 * (size_t) numParameters);
        dest.fillWith (0);

        getCurrentProgramName().copyToUTF8 ((char*) dest.getData(), 63);

        auto p = unalignedPointerCast<float*> (((char*) dest.getData()) + 64);

        for (int i = 0; i < numParameters; ++i)
            if (auto* param = getParameters()[i])
                p[i] = param->getValue();
    }

    void restoreFromTempParameterStore (const MemoryBlock& m)
    {
        changeProgramName (getCurrentProgram(), (const char*) m.getData());

        auto p = unalignedPointerCast<float*> (((char*) m.getData()) + 64);
        auto numParameters = getParameters().size();

        for (int i = 0; i < numParameters; ++i)
            if (auto* param = getParameters()[i])
                param->setValue (p[i]);
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
        auto v = (unsigned int) dispatch (Vst2::plugInOpcodeGetManufacturerVersion, 0, 0, nullptr, 0);

        String s;

        if (v == 0 || (int) v == -1)
            v = (unsigned int) getVersionNumber();

        if (v != 0)
        {
            // See yfede's post for the rational on this encoding
            // https://forum.juce.com/t/issues-with-version-integer-reported-by-vst2/23867/6

            unsigned int major = 0, minor = 0, bugfix = 0, build = 0;

            if (v < 10)            // Encoding A
            {
                major = v;
            }
            else if (v < 10000)    // Encoding B
            {
                major  = (v / 1000);
                minor  = (v % 1000) / 100;
                bugfix = (v % 100)  / 10;
                build  = (v % 10);
            }
            else if (v < 0x10000)  // Encoding C
            {
                major  = (v / 10000);
                minor  = (v % 10000) / 1000;
                bugfix = (v % 1000)  / 100;
                build  = (v % 100)   / 10;
            }
            else if (v < 0x650000) // Encoding D
            {
                major  = (v >> 16) & 0xff;
                minor  = (v >> 8)  & 0xff;
                bugfix = (v >> 0)  & 0xff;
            }
            else                  // Encoding E
            {
                major  = (v / 10000000);
                minor  = (v % 10000000) / 100000;
                bugfix = (v % 100000)   / 1000;
                build  = (v % 1000);
            }

            s << (int) major << '.' << (int) minor << '.' << (int) bugfix << '.' << (int) build;
        }

        return s;
    }

    const char* getCategory() const
    {
        switch (getVstCategory())
        {
            case Vst2::kPlugCategEffect:          return "Effect";
            case Vst2::kPlugCategSynth:           return "Synth";
            case Vst2::kPlugCategAnalysis:        return "Analysis";
            case Vst2::kPlugCategMastering:       return "Mastering";
            case Vst2::kPlugCategSpacializer:     return "Spacial";
            case Vst2::kPlugCategRoomFx:          return "Reverb";
            case Vst2::kPlugSurroundFx:           return "Surround";
            case Vst2::kPlugCategRestoration:     return "Restoration";
            case Vst2::kPlugCategGenerator:       return "Tone generation";
            case Vst2::kPlugCategOfflineProcess:  return "Offline Process";
            case Vst2::kPlugCategShell:           return "Shell";
            case Vst2::kPlugCategUnknown:         return "Unknown";
            default:                              break;
        }

        return nullptr;
    }

    void setPower (const bool on)
    {
        dispatch (Vst2::plugInOpcodeResumeSuspend, 0, on ? 1 : 0, nullptr, 0);
        isPowerOn = on;
    }

    //==============================================================================
    void updateBypass (bool processBlockBypassedCalled)
    {
        if (processBlockBypassedCalled)
        {
            if (bypassParam->getValue() == 0.0f || ! lastProcessBlockCallWasBypass)
                bypassParam->setValue (1.0f);
        }
        else
        {
            if (lastProcessBlockCallWasBypass)
                bypassParam->setValue (0.0f);
        }

        lastProcessBlockCallWasBypass = processBlockBypassedCalled;
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
       #if JUCE_LINUX || JUCE_BSD
        pluginWindow = None;
        ignoreUnused (pluginRefusesToResize, alreadyInside);
       #elif JUCE_MAC
        ignoreUnused (recursiveResize, pluginRefusesToResize, alreadyInside);

        cocoaWrapper.reset (new NSViewComponentWithParent (plugin));
        addAndMakeVisible (cocoaWrapper.get());
       #endif

        activeVSTWindows.add (this);

        Vst2::VstEditorBounds* rect = nullptr;
        dispatch (Vst2::plugInOpcodeGetEditorBounds, 0, 0, &rect, 0);

        if (rect != nullptr)
            updateSizeFromEditor (rect->rightmost - rect->leftmost, rect->lower - rect->upper);
        else
            updateSizeFromEditor (1, 1);

        setOpaque (true);
        setVisible (true);

       #if JUCE_WINDOWS
        addAndMakeVisible (embeddedComponent);
       #endif
    }

    ~VSTPluginWindow() override
    {
        closePluginWindow();

       #if JUCE_MAC
        cocoaWrapper.reset();
       #endif

        activeVSTWindows.removeFirstMatchingValue (this);
        plugin.editorBeingDeleted (this);
    }

    //==============================================================================
    /*  Convert from the hosted VST's coordinate system to the component's coordinate system. */
    Rectangle<int> vstToComponentRect (Component& editor, const Rectangle<int>& vr) const
    {
        return editor.getLocalArea (nullptr, vr / (nativeScaleFactor * getDesktopScaleFactor()));
    }

    Rectangle<int> componentToVstRect (Component& editor, const Rectangle<int>& vr) const
    {
        if (auto* tl = editor.getTopLevelComponent())
            return tl->getLocalArea (&editor, vr) * nativeScaleFactor * tl->getDesktopScaleFactor();

        return {};
    }

    bool updateSizeFromEditor (int w, int h)
    {
        const auto correctedBounds = vstToComponentRect (*this, { w, h });
        setSize (correctedBounds.getWidth(), correctedBounds.getHeight());

      #if JUCE_MAC
        if (cocoaWrapper != nullptr)
            cocoaWrapper->setSize (correctedBounds.getWidth(), correctedBounds.getHeight());
      #endif

        return true;
    }

   #if JUCE_MAC
    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
    }

    void visibilityChanged() override
    {
        if (isShowing())
            openPluginWindow ((NSView*) cocoaWrapper->getView());
        else
            closePluginWindow();
    }

    void childBoundsChanged (Component*) override
    {
        auto w = cocoaWrapper->getWidth();
        auto h = cocoaWrapper->getHeight();

        if (w != getWidth() || h != getHeight())
            setSize (w, h);
    }

    void parentHierarchyChanged() override { visibilityChanged(); }
   #else
    float getEffectiveScale() const
    {
        return nativeScaleFactor * userScaleFactor;
    }

    void paint (Graphics& g) override
    {
       #if JUCE_LINUX || JUCE_BSD
        if (isOpen)
        {
            if (pluginWindow != 0)
            {
                auto clip = componentToVstRect (*this, g.getClipBounds().toNearestInt());

                X11Symbols::getInstance()->xClearArea (display, pluginWindow, clip.getX(), clip.getY(),
                                                       static_cast<unsigned int> (clip.getWidth()),
                                                       static_cast<unsigned int> (clip.getHeight()), True);
            }
        }
        else
       #endif
        {
            g.fillAll (Colours::black);
        }
    }

    void componentMovedOrResized (bool /*wasMoved*/, bool /*wasResized*/) override
    {
        if (recursiveResize)
            return;

        if (getPeer() != nullptr)
        {
            const ScopedValueSetter<bool> recursiveResizeSetter (recursiveResize, true);

           #if JUCE_WINDOWS
            embeddedComponent.setBounds (getLocalBounds());
           #elif JUCE_LINUX || JUCE_BSD
            const auto pos = componentToVstRect (*this, getLocalBounds());

            if (pluginWindow != 0)
            {
                auto* symbols = X11Symbols::getInstance();
                symbols->xMoveResizeWindow (display,
                                            pluginWindow,
                                            pos.getX(),
                                            pos.getY(),
                                            (unsigned int) pos.getWidth(),
                                            (unsigned int) pos.getHeight());
                symbols->xMapRaised (display, pluginWindow);
                symbols->xFlush (display);
            }
           #endif
        }
    }

    using ComponentMovementWatcher::componentMovedOrResized;

    void componentVisibilityChanged() override
    {
        if (isShowing())
            openPluginWindow();
        else if (! shouldAvoidDeletingWindow())
            closePluginWindow();

        setContentScaleFactor();

       #if JUCE_LINUX || JUCE_BSD
        MessageManager::callAsync ([safeThis = SafePointer<VSTPluginWindow> { this }]
        {
            if (safeThis != nullptr)
                safeThis->componentMovedOrResized (true, true);
        });
       #else
        componentMovedOrResized (true, true);
       #endif
    }

    using ComponentMovementWatcher::componentVisibilityChanged;

    void componentPeerChanged() override
    {
        closePluginWindow();

        if (getPeer() != nullptr)
        {
            openPluginWindow();
            componentMovedOrResized (true, true);
        }
    }

    void setContentScaleFactor()
    {
        if (pluginRespondsToDPIChanges)
            dispatch (Vst2::plugInOpcodeManufacturerSpecific,
                      (int) ByteOrder::bigEndianInt ("PreS"),
                      (int) ByteOrder::bigEndianInt ("AeCs"),
                      nullptr, getEffectiveScale());
    }
   #endif

    void setScaleFactor (float scale) override
    {
        userScaleFactor = scale;

       #if ! JUCE_MAC
        setContentScaleFactor();
       #endif

       #if JUCE_WINDOWS
        resizeToFit();
       #endif
    }

    //==============================================================================
    bool keyStateChanged (bool) override                 { return pluginWantsKeys; }
    bool keyPressed (const juce::KeyPress&) override     { return pluginWantsKeys; }

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

               #if JUCE_WINDOWS
                // Some plugins may draw/resize inside their idle callback, so ensure that
                // DPI awareness is set correctly inside this call.
                ScopedThreadDPIAwarenessSetter scope (getPluginHWND());
               #endif
                plugin.dispatch (Vst2::plugInOpcodeEditorIdle, 0, 0, nullptr, 0);

                reentrantGuard = false;
            }

           #if JUCE_LINUX || JUCE_BSD
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

       #if JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD
        toFront (true);
       #endif
    }

    void broughtToFront() override
    {
        activeVSTWindows.removeFirstMatchingValue (this);
        activeVSTWindows.add (this);

       #if JUCE_MAC
        dispatch (Vst2::plugInOpcodeeffEditorTop, 0, 0, nullptr, 0);
       #endif
    }

private:
    //==============================================================================
    // This is a workaround for old Mackie plugins that crash if their
    // window is deleted more than once.
    bool shouldAvoidDeletingWindow() const
    {
        return plugin.getPluginDescription()
                .manufacturerName.containsIgnoreCase ("Loud Technologies");
    }

    // This is an old workaround for some plugins that need a repaint when their
    // windows are first created, but it breaks some Izotope plugins.
    bool shouldRepaintCarbonWindowWhenCreated()
    {
        return ! plugin.getName().containsIgnoreCase ("izotope");
    }

    //==============================================================================
   #if JUCE_MAC
    void openPluginWindow (void* parentWindow)
    {
        if (isOpen || parentWindow == nullptr)
            return;

        isOpen = true;

        Vst2::VstEditorBounds* rect = nullptr;
        dispatch (Vst2::plugInOpcodeGetEditorBounds, 0, 0, &rect, 0);
        dispatch (Vst2::plugInOpcodeOpenEditor, 0, 0, parentWindow, 0);

        // do this before and after like in the steinberg example
        dispatch (Vst2::plugInOpcodeGetEditorBounds, 0, 0, &rect, 0);
        dispatch (Vst2::plugInOpcodeGetCurrentProgram, 0, 0, nullptr, 0); // also in steinberg code

        // Install keyboard hooks
        pluginWantsKeys = (dispatch (Vst2::plugInOpcodeKeyboardFocusRequired, 0, 0, nullptr, 0) == 0);

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

        updateSizeFromEditor (w, h);

        startTimer (18 + juce::Random::getSystemRandom().nextInt (5));
        repaint();
    }
   #else
    void openPluginWindow()
    {
        if (isOpen)
            return;

        JUCE_VST_LOG ("Opening VST UI: " + plugin.getName());
        isOpen = true;

        pluginRespondsToDPIChanges = plugin.pluginCanDo ("supportsViewDpiScaling") > 0;

        setContentScaleFactor();

        Vst2::VstEditorBounds* rect = nullptr;

        dispatch (Vst2::plugInOpcodeGetEditorBounds, 0, 0, &rect, 0);

       #if JUCE_WINDOWS
        auto* handle = embeddedComponent.getHWND();
       #else
        auto* handle = getWindowHandle();
       #endif
        dispatch (Vst2::plugInOpcodeOpenEditor, 0, 0, handle, 0);
        dispatch (Vst2::plugInOpcodeGetEditorBounds, 0, 0, &rect, 0);  // do this before and after like in the steinberg example
        dispatch (Vst2::plugInOpcodeGetCurrentProgram, 0, 0, nullptr, 0); // also in steinberg code

        pluginWantsKeys = (dispatch (Vst2::plugInOpcodeKeyboardFocusRequired, 0, 0, nullptr, 0) == 0);

       #if JUCE_WINDOWS
        originalWndProc = 0;
        auto* pluginHWND = getPluginHWND();

        if (pluginHWND == 0)
        {
            isOpen = false;
            setSize (300, 150);
            return;
        }

        JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4244)

        if (! pluginWantsKeys)
        {
            originalWndProc = (void*) GetWindowLongPtr (pluginHWND, GWLP_WNDPROC);
            SetWindowLongPtr (pluginHWND, GWLP_WNDPROC, (LONG_PTR) vstHookWndProc);
        }

        JUCE_END_IGNORE_WARNINGS_MSVC

        RECT r;

        {
            ScopedThreadDPIAwarenessSetter threadDpiAwarenessSetter { pluginHWND };
            GetWindowRect (pluginHWND, &r);
        }

        auto w = (int) (r.right - r.left);
        auto h = (int) (r.bottom - r.top);

        if (rect != nullptr)
        {
            auto rw = rect->rightmost - rect->leftmost;
            auto rh = rect->lower - rect->upper;

            if ((rw > 50 && rh > 50 && rw < 2000 && rh < 2000 && (! isWithin (w, rw, 2) || ! isWithin (h, rh, 2)))
                || ((w == 0 && rw > 0) || (h == 0 && rh > 0)))
            {
                // very dodgy logic to decide which size is right.
                if (std::abs (rw - w) > 350 || std::abs (rh - h) > 350)
                {
                    ScopedThreadDPIAwarenessSetter threadDpiAwarenessSetter { pluginHWND };

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
       #elif JUCE_LINUX || JUCE_BSD
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
            X11Symbols::getInstance()->xMapRaised (display, pluginWindow);
       #endif

        // double-check it's not too tiny
        w = jmax (w, 32);
        h = jmax (h, 32);

        updateSizeFromEditor (w, h);

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
            // cleanup on a non-GUI thread. If it does that, bad things could happen in here.
            JUCE_ASSERT_MESSAGE_MANAGER_IS_LOCKED

            JUCE_VST_LOG ("Closing VST UI: " + plugin.getName());
            isOpen = false;
            dispatch (Vst2::plugInOpcodeCloseEditor, 0, 0, nullptr, 0);
            stopTimer();

           #if JUCE_WINDOWS
            JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4244)
            auto* pluginHWND = getPluginHWND();

            if (originalWndProc != 0 && pluginHWND != 0 && IsWindow (pluginHWND))
                SetWindowLongPtr (pluginHWND, GWLP_WNDPROC, (LONG_PTR) originalWndProc);
            JUCE_END_IGNORE_WARNINGS_MSVC

            originalWndProc = 0;
           #elif JUCE_LINUX || JUCE_BSD
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
    bool isWindowSizeCorrectForPlugin (int w, int h)
    {
        if (pluginRefusesToResize)
            return true;

        const auto converted = vstToComponentRect (*this, { w, h });
        return (isWithin (converted.getWidth(), getWidth(), 5) && isWithin (converted.getHeight(), getHeight(), 5));
    }

    void resizeToFit()
    {
        Vst2::VstEditorBounds* rect = nullptr;
        dispatch (Vst2::plugInOpcodeGetEditorBounds, 0, 0, &rect, 0);

        auto w = rect->rightmost - rect->leftmost;
        auto h = rect->lower - rect->upper;

        if (! isWindowSizeCorrectForPlugin (w, h))
        {
            updateSizeFromEditor (w, h);
            sizeCheckCount = 0;
        }

        embeddedComponent.updateHWNDBounds();
    }

    void checkPluginWindowSize()
    {
        if (! pluginRespondsToDPIChanges)
            resizeToFit();
    }

    // hooks to get keyboard events from VST windows.
    static LRESULT CALLBACK vstHookWndProc (HWND hW, UINT message, WPARAM wParam, LPARAM lParam)
    {
        for (int i = activeVSTWindows.size(); --i >= 0;)
        {
            Component::SafePointer<VSTPluginWindow> w (activeVSTWindows[i]);

            auto* pluginHWND = w->getPluginHWND();

            if (w != nullptr && pluginHWND == hW)
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
                                           (HWND) pluginHWND,
                                           message, wParam, lParam);
            }
        }

        return DefWindowProc (hW, message, wParam, lParam);
    }
   #endif

   #if JUCE_LINUX || JUCE_BSD
    void updatePluginWindowHandle()
    {
        pluginWindow = getChildWindow ((Window) getWindowHandle());
    }
   #endif

    //==============================================================================
    #if JUCE_MAC
      std::unique_ptr<NSViewComponentWithParent> cocoaWrapper;

      void resized() override
      {
      }
    #endif

    //==============================================================================
    VSTPluginInstance& plugin;
    float userScaleFactor = 1.0f;
    bool isOpen = false, recursiveResize = false;
    bool pluginWantsKeys = false, pluginRefusesToResize = false, alreadyInside = false;

   #if ! JUCE_MAC
    bool pluginRespondsToDPIChanges = false;

    float nativeScaleFactor = 1.0f;

    struct ScaleNotifierCallback
    {
        VSTPluginWindow& window;

        void operator() (float platformScale) const
        {
            MessageManager::callAsync ([ref = Component::SafePointer<VSTPluginWindow> (&window), platformScale]
            {
                if (auto* r = ref.getComponent())
                {
                    r->nativeScaleFactor = platformScale;
                    r->setContentScaleFactor();

                   #if JUCE_WINDOWS
                    r->resizeToFit();
                   #endif
                    r->componentMovedOrResized (true, true);
                }
            });
        }
    };

    NativeScaleFactorNotifier scaleNotifier { this, ScaleNotifierCallback { *this } };

    #if JUCE_WINDOWS
     struct ViewComponent : public HWNDComponent
     {
         ViewComponent()
         {
             setOpaque (true);
             inner.addToDesktop (0);

             if (auto* peer = inner.getPeer())
                 setHWND (peer->getNativeHandle());
         }

         void paint (Graphics& g) override { g.fillAll (Colours::black); }

     private:
         struct Inner : public Component
         {
             Inner() { setOpaque (true); }
             void paint (Graphics& g) override { g.fillAll (Colours::black); }
         };

         Inner inner;
     };

     HWND getPluginHWND() const
     {
         return GetWindow ((HWND) embeddedComponent.getHWND(), GW_CHILD);
     }

     ViewComponent embeddedComponent;
     void* originalWndProc = {};
     int sizeCheckCount = 0;
    #elif JUCE_LINUX || JUCE_BSD
     ::Display* display = XWindowSystem::getInstance()->getDisplay();
     Window pluginWindow = 0;
    #endif
   #else
     static constexpr auto nativeScaleFactor = 1.0f;
   #endif

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTPluginWindow)
};
#endif

JUCE_END_IGNORE_WARNINGS_MSVC

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

bool VSTPluginInstance::updateSizeFromEditor (int w, int h)
{
    if (auto* editor = dynamic_cast<VSTPluginWindow*> (getActiveEditor()))
        return editor->updateSizeFromEditor (w, h);

    return false;
}

//==============================================================================
// entry point for all callbacks from the plugin
static pointer_sized_int VSTINTERFACECALL audioMaster (Vst2::VstEffectInterface* effect, int32 opcode, int32 index, pointer_sized_int value, void* ptr, float opt)
{
    if (effect != nullptr)
        if (auto* instance = (VSTPluginInstance*) (effect->hostSpace2))
            return instance->handleCallback (opcode, index, value, ptr, opt);

    return VSTPluginInstance::handleGeneralCallback (opcode, index, value, ptr, opt);
}

//==============================================================================
VSTPluginFormat::VSTPluginFormat() {}
VSTPluginFormat::~VSTPluginFormat() {}

static std::unique_ptr<VSTPluginInstance> createAndUpdateDesc (VSTPluginFormat& format, PluginDescription& desc)
{
    if (auto p = format.createInstanceFromDescription (desc, 44100.0, 512))
    {
        if (auto instance = dynamic_cast<VSTPluginInstance*> (p.release()))
        {
           #if JUCE_MAC
            if (instance->vstModule->resFileId != 0)
                UseResFile (instance->vstModule->resFileId);
           #endif

            instance->fillInPluginDescription (desc);
            return std::unique_ptr<VSTPluginInstance> (instance);
        }

        jassertfalse;
    }

    return {};
}

void VSTPluginFormat::findAllTypesForFile (OwnedArray<PluginDescription>& results,
                                           const String& fileOrIdentifier)
{
    if (! fileMightContainThisPluginType (fileOrIdentifier))
        return;

    PluginDescription desc;
    desc.fileOrIdentifier = fileOrIdentifier;
    desc.uniqueId = desc.deprecatedUid = 0;

    auto instance = createAndUpdateDesc (*this, desc);

    if (instance == nullptr)
        return;

    if (instance->getVstCategory() != Vst2::kPlugCategShell)
    {
        // Normal plugin...
        results.add (new PluginDescription (desc));

        instance->dispatch (Vst2::plugInOpcodeOpen, 0, 0, nullptr, 0);
    }
    else
    {
        // It's a shell plugin, so iterate all the subtypes...
        for (;;)
        {
            char shellEffectName [256] = { 0 };
            auto uid = (int) instance->dispatch (Vst2::plugInOpcodeNextPlugInUniqueID, 0, 0, shellEffectName, 0);

            if (uid == 0)
                break;

            desc.uniqueId = desc.deprecatedUid = uid;
            desc.name = shellEffectName;

            aboutToScanVSTShellPlugin (desc);

            std::unique_ptr<VSTPluginInstance> shellInstance (createAndUpdateDesc (*this, desc));

            if (shellInstance != nullptr)
            {
                jassert (desc.deprecatedUid == uid);
                jassert (desc.uniqueId == uid);
                desc.hasSharedContainer = true;
                desc.name = shellEffectName;

                if (! arrayContainsPlugin (results, desc))
                    results.add (new PluginDescription (desc));
            }
        }
    }
}

void VSTPluginFormat::createPluginInstance (const PluginDescription& desc,
                                            double sampleRate, int blockSize,
                                            PluginCreationCallback callback)
{
    std::unique_ptr<VSTPluginInstance> result;

    if (fileMightContainThisPluginType (desc.fileOrIdentifier))
    {
        File file (desc.fileOrIdentifier);

        auto previousWorkingDirectory = File::getCurrentWorkingDirectory();
        file.getParentDirectory().setAsCurrentWorkingDirectory();

        if (auto module = ModuleHandle::findOrCreateModule (file))
        {
            shellUIDToCreate = desc.uniqueId != 0 ? desc.uniqueId : desc.deprecatedUid;

            result.reset (VSTPluginInstance::create (module, sampleRate, blockSize));

            if (result != nullptr && ! result->initialiseEffect (sampleRate, blockSize))
                result.reset();
        }

        previousWorkingDirectory.setAsCurrentWorkingDirectory();
    }

    String errorMsg;

    if (result == nullptr)
        errorMsg = TRANS ("Unable to load XXX plug-in file").replace ("XXX", "VST-2");

    callback (std::move (result), errorMsg);
}

bool VSTPluginFormat::requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const
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
  #elif JUCE_LINUX || JUCE_BSD || JUCE_ANDROID
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
    for (const auto& iter : RangedDirectoryIterator (dir, false, "*", File::findFilesAndDirectories))
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
   #elif JUCE_LINUX || JUCE_BSD || JUCE_ANDROID
    return FileSearchPath (SystemStats::getEnvironmentVariable ("VST_PATH",
                                                                "/usr/lib/vst;/usr/local/lib/vst;~/.vst")
                             .replace (":", ";"));
   #elif JUCE_WINDOWS
    auto programFiles = File::getSpecialLocation (File::globalApplicationsDirectory).getFullPathName();

    FileSearchPath paths;
    paths.add (WindowsRegistry::getValue ("HKEY_LOCAL_MACHINE\\Software\\VST\\VSTPluginsPath"));
    paths.addIfNotAlreadyThere (programFiles + "\\Steinberg\\VstPlugins");
    paths.addIfNotAlreadyThere (programFiles + "\\VstPlugins");
    paths.removeRedundantPaths();
    return paths;
   #elif JUCE_IOS
    // on iOS you can only load plug-ins inside the hosts bundle folder
    CFUniquePtr<CFURLRef> relativePluginDir (CFBundleCopyBuiltInPlugInsURL (CFBundleGetMainBundle()));
    CFUniquePtr<CFURLRef> pluginDir (CFURLCopyAbsoluteURL (relativePluginDir.get()));

    CFUniquePtr<CFStringRef> path (CFURLCopyFileSystemPath (pluginDir.get(), kCFURLPOSIXPathStyle));
    FileSearchPath retval (String (CFStringGetCStringPtr (path.get(), kCFStringEncodingUTF8)));
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
    {
        std::unique_ptr<VSTPluginInstance> result (VSTPluginInstance::create (module, initialSampleRate, initialBufferSize));

        if (result != nullptr)
            if (result->initialiseEffect (initialSampleRate, initialBufferSize))
                return result.release();
    }

    return nullptr;
}

void VSTPluginFormat::setExtraFunctions (AudioPluginInstance* plugin, ExtraFunctions* functions)
{
    std::unique_ptr<ExtraFunctions> f (functions);

    if (auto* vst = dynamic_cast<VSTPluginInstance*> (plugin))
        std::swap (vst->extraFunctions, f);
}

AudioPluginInstance* VSTPluginFormat::getPluginInstanceFromVstEffectInterface (void* aEffect)
{
    if (auto* vstAEffect = reinterpret_cast<Vst2::VstEffectInterface*> (aEffect))
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

JUCE_END_IGNORE_WARNINGS_GCC_LIKE
JUCE_END_IGNORE_WARNINGS_MSVC

#endif
