// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#define CARLA_NATIVE_PLUGIN_LV2
#include "carla-base.cpp"

#include "CarlaLv2Utils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaPipeUtils.hpp"

#include "extra/String.hpp"
#include "water/files/File.h"

static const char* const kPathForCarlaFiles = "carlafiles";

template<>
void Lv2PluginBaseClass<NativeTimeInfo>::clearTimeData() noexcept
{
    fLastPositionData.clear();
    carla_zeroStruct(fTimeInfo);
}

struct PreviewData {
    char type;
    uint32_t size;
    const void* buffer;
    bool shouldSend;

    PreviewData()
        : type(0),
          size(0),
          buffer(nullptr),
          shouldSend(false) {}
};

// --------------------------------------------------------------------------------------------------------------------
// Carla Internal Plugin API exposed as LV2 plugin

class NativePlugin : public Lv2PluginBaseClass<NativeTimeInfo>
{
public:
    static const uint32_t kMaxMidiEvents = 512;

    NativePlugin(const NativePluginDescriptor* const desc,
                 const double sampleRate,
                 const char* const bundlePath,
                 const LV2_Feature* const* const features)
        : Lv2PluginBaseClass<NativeTimeInfo>(sampleRate, features),
          fHandle(nullptr),
          fHost(),
          fDescriptor(desc),
#ifdef CARLA_PROPER_CPP11_SUPPORT
          fProgramDesc({0, 0, nullptr}),
#endif
          kIgnoreParameters(std::strncmp(desc->label, "carla", 5) == 0),
          fAtomForge(),
          fMidiEventCount(0),
          fLastProjectPath(),
          fLoadedFile(),
          fPreviewData(),
          fNeedsNotifyFileChanged(false),
          fPluginNeedsIdle(0),
          fWorkerUISignal(0)
    {
        carla_zeroStruct(fHost);
#ifndef CARLA_PROPER_CPP11_SUPPORT
        carla_zeroStruct(fProgramDesc);
#endif

        if (! loadedInProperHost())
            return;

        using water::File;
        using water::String;

        String resourceDir(water::File(bundlePath).getChildFile("resources").getFullPathName());

        fHost.handle      = this;
        fHost.resourceDir = carla_strdup(resourceDir.toRawUTF8());
        fHost.uiName      = nullptr;
        fHost.uiParentId  = 0;

        fHost.get_buffer_size        = host_get_buffer_size;
        fHost.get_sample_rate        = host_get_sample_rate;
        fHost.is_offline             = host_is_offline;
        fHost.get_time_info          = host_get_time_info;
        fHost.write_midi_event       = host_write_midi_event;
        fHost.ui_parameter_changed   = host_ui_parameter_changed;
        fHost.ui_custom_data_changed = host_ui_custom_data_changed;
        fHost.ui_closed              = host_ui_closed;
        fHost.ui_open_file           = host_ui_open_file;
        fHost.ui_save_file           = host_ui_save_file;
        fHost.dispatcher             = host_dispatcher;

        carla_zeroStruct(fAtomForge);
        lv2_atom_forge_init(&fAtomForge, fUridMap);
    }

    ~NativePlugin()
    {
        CARLA_SAFE_ASSERT(fHandle == nullptr);

        if (fHost.resourceDir != nullptr)
        {
            delete[] fHost.resourceDir;
            fHost.resourceDir = nullptr;
        }

        if (fHost.uiName != nullptr)
        {
            delete[] fHost.uiName;
            fHost.uiName = nullptr;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    bool init()
    {
        if (fHost.resourceDir == nullptr)
            return false;

        if (fDescriptor->instantiate == nullptr || fDescriptor->process == nullptr)
        {
            carla_stderr("Plugin is missing something...");
            return false;
        }

        carla_zeroStructs(fMidiEvents, kMaxMidiEvents);

        fHandle = fDescriptor->instantiate(&fHost);
        CARLA_SAFE_ASSERT_RETURN(fHandle != nullptr, false);

        fPorts.hasUI        = fDescriptor->hints & NATIVE_PLUGIN_HAS_UI;
        fPorts.usesTime     = fDescriptor->hints & NATIVE_PLUGIN_USES_TIME;
        fPorts.numAudioIns  = fDescriptor->audioIns;
        fPorts.numAudioOuts = fDescriptor->audioOuts;
        fPorts.numCVIns     = fDescriptor->cvIns;
        fPorts.numCVOuts    = fDescriptor->cvOuts;
        fPorts.numMidiIns   = fDescriptor->midiIns;
        fPorts.numMidiOuts  = fDescriptor->midiOuts;

        if (fDescriptor->get_parameter_count != nullptr &&
            fDescriptor->get_parameter_info  != nullptr &&
            fDescriptor->get_parameter_value != nullptr &&
            fDescriptor->set_parameter_value != nullptr &&
            ! kIgnoreParameters)
        {
            fPorts.numParams = fDescriptor->get_parameter_count(fHandle);
        }

        fPorts.init();

        if (fPorts.numParams > 0)
        {
            for (uint32_t i=0; i < fPorts.numParams; ++i)
            {
                fPorts.paramsLast[i] = fDescriptor->get_parameter_value(fHandle, i);
                fPorts.paramsOut [i] = fDescriptor->get_parameter_info(fHandle, i)->hints & NATIVE_PARAMETER_IS_OUTPUT;
            }
        }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // LV2 functions

    void lv2_activate()
    {
        CARLA_SAFE_ASSERT_RETURN(! fIsActive,);

        resetTimeInfo();

        if (fDescriptor->activate != nullptr)
            fDescriptor->activate(fHandle);

        fIsActive = true;
    }

    void lv2_deactivate()
    {
        CARLA_SAFE_ASSERT_RETURN(fIsActive,);

        fIsActive = false;

        if (fDescriptor->deactivate != nullptr)
            fDescriptor->deactivate(fHandle);
    }

    void lv2_cleanup()
    {
        if (fIsActive)
        {
            carla_stderr("Warning: Host forgot to call deactivate!");
            fIsActive = false;

            if (fDescriptor->deactivate != nullptr)
                fDescriptor->deactivate(fHandle);
        }

        if (fDescriptor->cleanup != nullptr)
            fDescriptor->cleanup(fHandle);

        fHandle = nullptr;
    }

    // ----------------------------------------------------------------------------------------------------------------

    void lv2_run(const uint32_t frames)
    {
        if (! lv2_pre_run(frames))
        {
            updateParameterOutputs();
            return;
        }

        if (fPorts.numMidiIns > 0 || fPorts.hasUI)
        {
            uint32_t numEventPortsIn;

            if (fPorts.numMidiIns > 0)
            {
                numEventPortsIn = fPorts.numMidiIns;
                fMidiEventCount = 0;
                carla_zeroStructs(fMidiEvents, kMaxMidiEvents);
            }
            else
            {
                numEventPortsIn = 1;
            }

            for (uint32_t i=0; i < numEventPortsIn; ++i)
            {
                const LV2_Atom_Sequence* const eventPortIn(fPorts.eventsIn[i]);
                CARLA_SAFE_ASSERT_CONTINUE(eventPortIn != nullptr);

                LV2_ATOM_SEQUENCE_FOREACH(eventPortIn, event)
                {
                    if (event == nullptr)
                        continue;

                    if (event->body.type == fURIs.carlaUiEvents && fWorkerUISignal != -1)
                    {
                        CARLA_SAFE_ASSERT_CONTINUE((fDescriptor->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE) == 0);

                        if (fWorker != nullptr)
                        {
                            // worker is supported by the host, we can continue
                            fWorkerUISignal = 1;
                            const char* const msg((const char*)(event + 1));
                            const size_t msgSize = std::strlen(msg);
                            fWorker->schedule_work(fWorker->handle, static_cast<uint32_t>(msgSize + 1U), msg);
                        }
                        else
                        {
                            // worker is not supported, cancel
                            fWorkerUISignal = -1;
                        }
                        continue;
                    }

                    if (event->body.type == fURIs.atomObject)
                    {
                        const LV2_Atom_Object* const obj = (const LV2_Atom_Object*)(&event->body);

                        if (obj->body.otype == fURIs.patchSet)
                        {
                            // Get property URI.
                            const LV2_Atom* property = nullptr;
                            lv2_atom_object_get(obj, fURIs.patchProperty, &property, 0);
                            CARLA_SAFE_ASSERT_CONTINUE(property != nullptr);
                            CARLA_SAFE_ASSERT_CONTINUE(property->type == fURIs.atomURID);

                            const LV2_URID urid = ((const LV2_Atom_URID*)property)->body;

                            /*  */ if (std::strcmp(fDescriptor->label, "audiofile") == 0) {
                                CARLA_SAFE_ASSERT_CONTINUE(urid == fURIs.carlaFileAudio);
                            } else if (std::strcmp(fDescriptor->label, "midifile") == 0) {
                                CARLA_SAFE_ASSERT_CONTINUE(urid == fURIs.carlaFileMIDI);
                            } else {
                                CARLA_SAFE_ASSERT_CONTINUE(urid == fURIs.carlaFile);
                            }

                            // Get value.
                            const LV2_Atom* fileobj = nullptr;
                            lv2_atom_object_get(obj, fURIs.patchValue, &fileobj, 0);
                            CARLA_SAFE_ASSERT_CONTINUE(fileobj != nullptr);
                            CARLA_SAFE_ASSERT_CONTINUE(fileobj->type == fURIs.atomPath);

                            const char* const filepath((const char*)(fileobj + 1));

                            fWorker->schedule_work(fWorker->handle,
                                                   static_cast<uint32_t>(std::strlen(filepath) + 1U),
                                                   filepath);
                        }
                        else if (obj->body.otype == fURIs.patchGet)
                        {
                            if (fDescriptor->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE)
                                fNeedsNotifyFileChanged = true;
                            if (fPreviewData.buffer != nullptr)
                                fPreviewData.shouldSend = true;
                        }
                        continue;
                    }

                    if (event->body.type != fURIs.midiEvent)
                        continue;

                    // anything past this point assumes plugin with MIDI input
                    CARLA_SAFE_ASSERT_CONTINUE(fPorts.numMidiIns > 0);

                    if (event->body.size > 4)
                        continue;
                    if (event->time.frames >= frames)
                        break;

                    const uint8_t* const data((const uint8_t*)(event + 1));

                    NativeMidiEvent& nativeEvent(fMidiEvents[fMidiEventCount++]);

                    nativeEvent.port = (uint8_t)i;
                    nativeEvent.size = (uint8_t)event->body.size;
                    nativeEvent.time = (uint32_t)event->time.frames;

                    uint32_t j=0;
                    for (uint32_t size=event->body.size; j<size; ++j)
                        nativeEvent.data[j] = data[j];
                    for (; j<4; ++j)
                        nativeEvent.data[j] = 0;

                    if (fMidiEventCount >= kMaxMidiEvents)
                        break;
                }
            }

            if (fNeedsNotifyFileChanged || fPreviewData.shouldSend)
            {
                uint8_t atomBuf[4096];
                LV2_Atom* atom = (LV2_Atom*)atomBuf;
                LV2_Atom_Sequence* const seq = fPorts.eventsOut[0];
                Ports::EventsOutData& mData(fPorts.eventsOutData[0]);

                if (fNeedsNotifyFileChanged)
                {
                    fNeedsNotifyFileChanged = false;

                    LV2_Atom_Forge atomForge = fAtomForge;
                    lv2_atom_forge_set_buffer(&atomForge, atomBuf, sizeof(atomBuf));

                    LV2_Atom_Forge_Frame forgeFrame;
                    lv2_atom_forge_object(&atomForge, &forgeFrame, 0, fURIs.patchSet);

                    lv2_atom_forge_key(&atomForge, fURIs.patchProperty);

                    /*  */ if (std::strcmp(fDescriptor->label, "audiofile") == 0) {
                        lv2_atom_forge_urid(&atomForge, fURIs.carlaFileAudio);
                    } else if (std::strcmp(fDescriptor->label, "midifile") == 0) {
                        lv2_atom_forge_urid(&atomForge, fURIs.carlaFileMIDI);
                    } else {
                        lv2_atom_forge_urid(&atomForge, fURIs.carlaFile);
                    }

                    lv2_atom_forge_key(&atomForge, fURIs.patchValue);
                    lv2_atom_forge_path(&atomForge,
                                        fLoadedFile.buffer(),
                                        static_cast<uint32_t>(fLoadedFile.length()+1));

                    lv2_atom_forge_pop(&atomForge, &forgeFrame);

                    if (sizeof(LV2_Atom_Event) + atom->size <= mData.capacity - mData.offset)
                    {
                        LV2_Atom_Event* const aev = (LV2_Atom_Event*)(LV2_ATOM_CONTENTS(LV2_Atom_Sequence, seq) + mData.offset);

                        aev->time.frames = 0;
                        aev->body.size   = atom->size;
                        aev->body.type   = atom->type;
                        std::memcpy(LV2_ATOM_BODY(&aev->body), atom + 1, atom->size);

                        const uint32_t size = lv2_atom_pad_size(static_cast<uint32_t>(sizeof(LV2_Atom_Event) + atom->size));
                        mData.offset       += size;
                        seq->atom.size     += size;
                    }
                }

                if (fPreviewData.shouldSend)
                {
                    const char ptype = fPreviewData.type;
                    const uint32_t psize = fPreviewData.size;
                    const void* const pbuffer = fPreviewData.buffer;
                    fPreviewData.shouldSend = false;

                    LV2_Atom_Forge atomForge = fAtomForge;
                    lv2_atom_forge_set_buffer(&atomForge, atomBuf, sizeof(atomBuf));

                    LV2_Atom_Forge_Frame forgeFrame;
                    lv2_atom_forge_object(&atomForge, &forgeFrame, 0, fURIs.patchSet);

                    lv2_atom_forge_key(&atomForge, fURIs.patchProperty);
                    lv2_atom_forge_urid(&atomForge, fURIs.carlaPreview);

                    lv2_atom_forge_key(&atomForge, fURIs.patchValue);

                    switch (ptype)
                    {
                    case 'b':
                        lv2_atom_forge_vector(&atomForge, sizeof(int32_t), fURIs.atomBool, psize, pbuffer);
                        break;
                    case 'i':
                        lv2_atom_forge_vector(&atomForge, sizeof(int32_t), fURIs.atomInt, psize, pbuffer);
                        break;
                    case 'f':
                        lv2_atom_forge_vector(&atomForge, sizeof(float), fURIs.atomFloat, psize, pbuffer);
                        break;
                    default:
                        carla_stderr2("Preview data buffer has wrong type '%c' (and size %u)", ptype, psize);
                        break;
                    }

                    lv2_atom_forge_pop(&atomForge, &forgeFrame);

                    if (sizeof(LV2_Atom_Event) + atom->size <= mData.capacity - mData.offset)
                    {
                        LV2_Atom_Event* const aev = (LV2_Atom_Event*)(LV2_ATOM_CONTENTS(LV2_Atom_Sequence, seq) + mData.offset);

                        aev->time.frames = 0;
                        aev->body.size   = atom->size;
                        aev->body.type   = atom->type;
                        std::memcpy(LV2_ATOM_BODY(&aev->body), atom + 1, atom->size);

                        const uint32_t size = lv2_atom_pad_size(static_cast<uint32_t>(sizeof(LV2_Atom_Event) + atom->size));
                        mData.offset       += size;
                        seq->atom.size     += size;
                    }
                }
            }
        }

        fDescriptor->process(fHandle, fPorts.audioCVIns, fPorts.audioCVOuts, frames, fMidiEvents, fMidiEventCount);

        if (fPluginNeedsIdle == 1)
        {
            fPluginNeedsIdle = 2;
            const char* const msg = "_idle_";
            const size_t msgSize = std::strlen(msg);
            fWorker->schedule_work(fWorker->handle, static_cast<uint32_t>(msgSize + 1U), msg);
        }

        if (fWorkerUISignal == -1 && fPorts.hasUI)
        {
            const char* const msg = "quit";
            const size_t msgSize  = 5;

            LV2_Atom_Sequence* const seq(fPorts.eventsOut[0]);
            Ports::EventsOutData& mData(fPorts.eventsOutData[0]);

            if (sizeof(LV2_Atom_Event) + msgSize <= mData.capacity - mData.offset)
            {
                LV2_Atom_Event* const aev = (LV2_Atom_Event*)(LV2_ATOM_CONTENTS(LV2_Atom_Sequence, seq) + mData.offset);

                aev->time.frames = 0;
                aev->body.size   = msgSize;
                aev->body.type   = fURIs.carlaUiEvents;
                std::memcpy(LV2_ATOM_BODY(&aev->body), msg, msgSize);

                const uint32_t size = lv2_atom_pad_size(static_cast<uint32_t>(sizeof(LV2_Atom_Event) + msgSize));
                mData.offset       += size;
                seq->atom.size     += size;

                fWorkerUISignal = 0;
            }
        }

        lv2_post_run(frames);
        updateParameterOutputs();
    }

    // ----------------------------------------------------------------------------------------------------------------

    const LV2_Program_Descriptor* lv2_get_program(const uint32_t index)
    {
        if (fDescriptor->category == NATIVE_PLUGIN_CATEGORY_SYNTH)
            return nullptr;
        if (fDescriptor->get_midi_program_count == nullptr)
            return nullptr;
        if (fDescriptor->get_midi_program_info == nullptr)
            return nullptr;
        if (index >= fDescriptor->get_midi_program_count(fHandle))
            return nullptr;

        const NativeMidiProgram* const midiProg(fDescriptor->get_midi_program_info(fHandle, index));

        if (midiProg == nullptr)
            return nullptr;

        fProgramDesc.bank    = midiProg->bank;
        fProgramDesc.program = midiProg->program;
        fProgramDesc.name    = midiProg->name;

        return &fProgramDesc;
    }

    void lv2_select_program(uint32_t bank, uint32_t program)
    {
        if (fDescriptor->category == NATIVE_PLUGIN_CATEGORY_SYNTH)
            return;
        if (fDescriptor->set_midi_program == nullptr)
            return;

        fDescriptor->set_midi_program(fHandle, 0, bank, program);

        for (uint32_t i=0; i < fPorts.numParams; ++i)
        {
            fPorts.paramsLast[i] = fDescriptor->get_parameter_value(fHandle, i);

            if (fPorts.paramsPtr[i] != nullptr)
                *fPorts.paramsPtr[i] = fPorts.paramsLast[i];
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    void cleanupLastProjectPath()
    {
        fLastProjectPath.clear();
    }

    void saveLastProjectPathIfPossible(const LV2_Feature* const* const features)
    {
        if (features == nullptr)
            return cleanupLastProjectPath();

        const LV2_State_Free_Path* freePath = nullptr;
        const LV2_State_Make_Path* makePath = nullptr;

        for (int i=0; features[i] != nullptr; ++i)
        {
            /**/ if (freePath == nullptr && std::strcmp(features[i]->URI, LV2_STATE__freePath) == 0)
                freePath = (const LV2_State_Free_Path*)features[i]->data;
            else if (makePath == nullptr && std::strcmp(features[i]->URI, LV2_STATE__makePath) == 0)
                makePath = (const LV2_State_Make_Path*)features[i]->data;
        }

        if (makePath == nullptr || makePath->path == nullptr)
            return cleanupLastProjectPath();

        if (freePath == nullptr)
            freePath = fFreePath;

        char* const newpath = makePath->path(makePath->handle, kPathForCarlaFiles);

        if (newpath == nullptr)
            return cleanupLastProjectPath();

        fLastProjectPath = String(water::File(newpath).getParentDirectory().getFullPathName().toRawUTF8());

        if (freePath != nullptr && freePath->free_path != nullptr)
            freePath->free_path(freePath->handle, newpath);
#ifndef CARLA_OS_WIN
        // this is not safe to call under windows
        else
            std::free(newpath);
#endif
    }

    LV2_State_Status lv2_save(const LV2_State_Store_Function store, const LV2_State_Handle handle,
                              const uint32_t /*flags*/, const LV2_Feature* const* const features)
    {
        saveLastProjectPathIfPossible(features);

        if (fDescriptor->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE)
        {
            if (fLoadedFile.isEmpty())
                return LV2_STATE_SUCCESS;

            const LV2_State_Free_Path* freePath = nullptr;
            const LV2_State_Map_Path* mapPath = nullptr;

            if (features != nullptr)
            {
                for (int i=0; features[i] != nullptr; ++i)
                {
                    /**/ if (freePath == nullptr && std::strcmp(features[i]->URI, LV2_STATE__freePath) == 0)
                        freePath = (const LV2_State_Free_Path*)features[i]->data;
                    else if (mapPath == nullptr && std::strcmp(features[i]->URI, LV2_STATE__mapPath) == 0)
                        mapPath = (const LV2_State_Map_Path*)features[i]->data;
                }
            }

            if (mapPath == nullptr || mapPath->abstract_path == nullptr)
                return LV2_STATE_ERR_NO_FEATURE;

            char* path = mapPath->abstract_path(mapPath->handle, fLoadedFile.buffer());

            store(handle,
                  fUridMap->map(fUridMap->handle, "http://kxstudio.sf.net/ns/carla/file"),
                  path,
                  std::strlen(path)+1,
                  fURIs.atomPath,
                  LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE);

            if (freePath != nullptr && freePath->free_path != nullptr)
                freePath->free_path(freePath->handle, path);
#ifndef CARLA_OS_WIN
            // this is not safe to call under windows
            else
                std::free(path);
#endif

            return LV2_STATE_SUCCESS;
        }

        if ((fDescriptor->hints & NATIVE_PLUGIN_USES_STATE) == 0 || fDescriptor->get_state == nullptr)
            return LV2_STATE_ERR_UNKNOWN;

        if (char* const state = fDescriptor->get_state(fHandle))
        {
            store(handle,
                  fUridMap->map(fUridMap->handle, "http://kxstudio.sf.net/ns/carla/chunk"),
                  state,
                  std::strlen(state)+1,
                  fURIs.atomString,
                  LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE);
            std::free(state);
            return LV2_STATE_SUCCESS;
        }

        return LV2_STATE_ERR_UNKNOWN;
    }

    LV2_State_Status lv2_restore(const LV2_State_Retrieve_Function retrieve,
                                 const LV2_State_Handle handle,
                                 uint32_t flags,
                                 const LV2_Feature* const* const features)
    {
        saveLastProjectPathIfPossible(features);

        size_t   size = 0;
        uint32_t type = 0;

        if (fDescriptor->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE)
        {
            size = type = 0;
            const void* const data = retrieve(handle,
                                              fUridMap->map(fUridMap->handle, "http://kxstudio.sf.net/ns/carla/file"),
                                              &size, &type, &flags);
            if (size <= 1 || type == 0)
                return LV2_STATE_ERR_NO_PROPERTY;

            CARLA_SAFE_ASSERT_RETURN(type == fURIs.atomPath, LV2_STATE_ERR_UNKNOWN);

            const LV2_State_Free_Path* freePath = nullptr;
            const LV2_State_Map_Path* mapPath = nullptr;

            if (features != nullptr)
            {
                for (int i=0; features[i] != nullptr; ++i)
                {
                    /**/ if (freePath == nullptr && std::strcmp(features[i]->URI, LV2_STATE__freePath) == 0)
                        freePath = (const LV2_State_Free_Path*)features[i]->data;
                    else if (mapPath == nullptr && std::strcmp(features[i]->URI, LV2_STATE__mapPath) == 0)
                        mapPath = (const LV2_State_Map_Path*)features[i]->data;
                }
            }

            if (mapPath == nullptr || mapPath->absolute_path == nullptr)
                return LV2_STATE_ERR_NO_FEATURE;

            const char* const filename = (const char*)data;

            char* const absolute_filename = mapPath->absolute_path(mapPath->handle, filename);
            fLoadedFile = absolute_filename;

            if (freePath != nullptr && freePath->free_path != nullptr)
                freePath->free_path(freePath->handle, absolute_filename);
#ifndef CARLA_OS_WIN
            // this is not safe to call under windows
            else
                std::free(absolute_filename);
#endif

            fNeedsNotifyFileChanged = true;
            fDescriptor->set_custom_data(fHandle, "file", fLoadedFile);
            return LV2_STATE_SUCCESS;
        }

        if ((fDescriptor->hints & NATIVE_PLUGIN_USES_STATE) == 0 || fDescriptor->set_state == nullptr)
            return LV2_STATE_ERR_UNKNOWN;

        size = type = 0;
        const void* const data = retrieve(handle,
                                          fUridMap->map(fUridMap->handle, "http://kxstudio.sf.net/ns/carla/chunk"),
                                          &size, &type, &flags);

        if (size == 0)
            return LV2_STATE_ERR_UNKNOWN;
        if (type == 0)
            return LV2_STATE_ERR_UNKNOWN;
        if (data == nullptr)
            return LV2_STATE_ERR_UNKNOWN;
        if (type != fURIs.atomString)
            return LV2_STATE_ERR_BAD_TYPE;

        fDescriptor->set_state(fHandle, (const char*)data);

        return LV2_STATE_SUCCESS;
    }

    // ----------------------------------------------------------------------------------------------------------------

    LV2_Worker_Status lv2_work(LV2_Worker_Respond_Function, LV2_Worker_Respond_Handle, uint32_t, const void* data)
    {
        const char* const msg = (const char*)data;

        if (std::strcmp(msg, "_idle_") == 0)
        {
            if (fDescriptor->hints & NATIVE_PLUGIN_REQUESTS_IDLE)
            {
                fPluginNeedsIdle = 0;
                fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_IDLE, 0, 0, nullptr, 0.0f);
                return LV2_WORKER_SUCCESS;
            }
            return LV2_WORKER_ERR_UNKNOWN;
        }

        if (fDescriptor->hints & NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE)
        {
            fLoadedFile = msg;
            fDescriptor->set_custom_data(fHandle, "file", msg);
            return LV2_WORKER_SUCCESS;
        }

        /**/ if (std::strncmp(msg, "control ", 8) == 0)
        {
            if (fDescriptor->ui_set_parameter_value == nullptr)
                return LV2_WORKER_SUCCESS;

            if (const char* const msgSplit = std::strstr(msg+8, " "))
            {
                const char* const msgIndex = msg+8;
                CARLA_SAFE_ASSERT_RETURN(msgSplit - msgIndex < 8, LV2_WORKER_ERR_UNKNOWN);
                CARLA_SAFE_ASSERT_RETURN(msgSplit[0] != '\0', LV2_WORKER_ERR_UNKNOWN);

                char strBufIndex[8];
                carla_zeroChars(strBufIndex, 8);
                std::strncpy(strBufIndex, msgIndex, static_cast<size_t>(msgSplit - msgIndex));

                const int index = std::atoi(msgIndex) - static_cast<int>(fPorts.indexOffset);
                CARLA_SAFE_ASSERT_RETURN(index >= 0, LV2_WORKER_ERR_UNKNOWN);

                float value;

                {
                    const ScopedSafeLocale ssl;
                    value = static_cast<float>(std::atof(msgSplit+1));
                }

                fDescriptor->ui_set_parameter_value(fHandle, static_cast<uint32_t>(index), value);
            }
        }
        else if (std::strcmp(msg, "show") == 0)
        {
            handleUiShow();
        }
        else if (std::strcmp(msg, "hide") == 0)
        {
            handleUiHide();
        }
        else if (std::strcmp(msg, "idle") == 0)
        {
            handleUiRun();
        }
        else if (std::strcmp(msg, "quit") == 0)
        {
            handleUiClosed();
        }
        else
        {
            carla_stdout("lv2_work unknown msg '%s'", msg);
            return LV2_WORKER_ERR_UNKNOWN;
        }

        return LV2_WORKER_SUCCESS;
    }

    LV2_Worker_Status lv2_work_resp(uint32_t /*size*/, const void* /*body*/)
    {
        return LV2_WORKER_SUCCESS;
    }

    const LV2_Inline_Display_Image_Surface* lv2_idisp_render(const uint32_t width, const uint32_t height)
    {
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->hints & NATIVE_PLUGIN_HAS_INLINE_DISPLAY, nullptr);
        CARLA_SAFE_ASSERT_RETURN(fDescriptor->render_inline_display, nullptr);
        CARLA_SAFE_ASSERT_RETURN(width > 0, nullptr);
        CARLA_SAFE_ASSERT_RETURN(height > 0, nullptr);

        const NativeInlineDisplayImageSurface* nsur = fDescriptor->render_inline_display(fHandle, width, height);
        CARLA_SAFE_ASSERT_RETURN(nsur != nullptr, nullptr);

        return (const LV2_Inline_Display_Image_Surface*)(nsur);
    }

    // ----------------------------------------------------------------------------------------------------------------

    void lv2ui_instantiate(LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
                           LV2UI_Widget* widget, const LV2_Feature* const* features)
    {
        fUI.writeFunction = writeFunction;
        fUI.controller = controller;

        if (fHost.uiName != nullptr)
        {
            delete[] fHost.uiName;
            fHost.uiName = nullptr;
        }

        // ---------------------------------------------------------------
        // see if the host supports external-ui

        for (int i=0; features[i] != nullptr; ++i)
        {
            if (std::strcmp(features[i]->URI, LV2_EXTERNAL_UI__Host) == 0 ||
                std::strcmp(features[i]->URI, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0)
            {
                fUI.host = (const LV2_External_UI_Host*)features[i]->data;
            }
            if (std::strcmp(features[i]->URI, LV2_UI__touch) == 0)
            {
                fUI.touch = (const LV2UI_Touch*)features[i]->data;
            }
        }

        if (fUI.host != nullptr)
        {
            fHost.uiName = carla_strdup(fUI.host->plugin_human_id);
            *widget = (LV2_External_UI_Widget_Compat*)this;
            return;
        }

        // ---------------------------------------------------------------
        // no external-ui support, use showInterface

        for (int i=0; features[i] != nullptr; ++i)
        {
            if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) != 0)
                continue;

            const LV2_Options_Option* const options((const LV2_Options_Option*)features[i]->data);
            CARLA_SAFE_ASSERT_BREAK(options != nullptr);

            for (int j=0; options[j].key != 0; ++j)
            {
                if (options[j].key != fUridMap->map(fUridMap->handle, LV2_UI__windowTitle))
                    continue;

                const char* const title((const char*)options[j].value);
                CARLA_SAFE_ASSERT_BREAK(title != nullptr && title[0] != '\0');

                fHost.uiName = carla_strdup(title);
                break;
            }
            break;
        }

        if (fHost.uiName == nullptr)
            fHost.uiName = carla_strdup(fDescriptor->name);

        *widget = nullptr;
        return;
    }

    void lv2ui_port_event(uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer) const
    {
        if (format != 0 || bufferSize != sizeof(float) || buffer == nullptr)
            return;
        if (portIndex < fPorts.indexOffset || ! fUI.isVisible)
            return;
        if (fDescriptor->ui_set_parameter_value == nullptr)
            return;

        const float value(*(const float*)buffer);
        fDescriptor->ui_set_parameter_value(fHandle, portIndex-fPorts.indexOffset, value);
    }

    // ----------------------------------------------------------------------------------------------------------------

    void lv2ui_select_program(uint32_t bank, uint32_t program) const
    {
        if (fDescriptor->category == NATIVE_PLUGIN_CATEGORY_SYNTH)
            return;
        if (fDescriptor->ui_set_midi_program == nullptr)
            return;

        fDescriptor->ui_set_midi_program(fHandle, 0, bank, program);
    }

    // ----------------------------------------------------------------------------------------------------------------

protected:
    void handleUiRun() const override
    {
        if (fDescriptor->ui_idle != nullptr)
            fDescriptor->ui_idle(fHandle);
    }

    void handleUiShow() override
    {
        if (fDescriptor->ui_show != nullptr)
            fDescriptor->ui_show(fHandle, true);

        fUI.isVisible = true;
    }

    void handleUiHide() override
    {
        if (fDescriptor->ui_show != nullptr)
            fDescriptor->ui_show(fHandle, false);

        fUI.isVisible = false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    void handleParameterValueChanged(const uint32_t index, const float value) override
    {
        fDescriptor->set_parameter_value(fHandle, index, value);
    }

    void handleBufferSizeChanged(const uint32_t bufferSize) override
    {
        if (fDescriptor->dispatcher == nullptr)
            return;

        fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_BUFFER_SIZE_CHANGED, 0, bufferSize, nullptr, 0.0f);
    }

    void handleSampleRateChanged(const double sampleRate) override
    {
        if (fDescriptor->dispatcher == nullptr)
            return;

        fDescriptor->dispatcher(fHandle, NATIVE_PLUGIN_OPCODE_SAMPLE_RATE_CHANGED, 0, 0, nullptr, (float)sampleRate);
    }

    // ----------------------------------------------------------------------------------------------------------------

    bool handleWriteMidiEvent(const NativeMidiEvent* const event)
    {
        CARLA_SAFE_ASSERT_RETURN(fPorts.numMidiOuts > 0, false);
        CARLA_SAFE_ASSERT_RETURN(event != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(event->size > 0, false);

        const uint8_t port(event->port);
        CARLA_SAFE_ASSERT_RETURN(port < fPorts.numMidiOuts, false);

        LV2_Atom_Sequence* const seq(fPorts.eventsOut[port]);
        CARLA_SAFE_ASSERT_RETURN(seq != nullptr, false);

        Ports::EventsOutData& mData(fPorts.eventsOutData[port]);

        if (sizeof(LV2_Atom_Event) + event->size > mData.capacity - mData.offset)
            return false;

        LV2_Atom_Event* const aev = (LV2_Atom_Event*)(LV2_ATOM_CONTENTS(LV2_Atom_Sequence, seq) + mData.offset);

        aev->time.frames = event->time;
        aev->body.size   = event->size;
        aev->body.type   = fURIs.midiEvent;
        std::memcpy(LV2_ATOM_BODY(&aev->body), event->data, event->size);

        const uint32_t size = lv2_atom_pad_size(static_cast<uint32_t>(sizeof(LV2_Atom_Event) + event->size));
        mData.offset       += size;
        seq->atom.size     += size;

        return true;
    }

    void handleUiParameterChanged(const uint32_t index, const float value) const
    {
        if (kIgnoreParameters || fWorkerUISignal)
            return;

        if (fUI.writeFunction != nullptr && fUI.controller != nullptr)
            fUI.writeFunction(fUI.controller, index+fPorts.indexOffset, sizeof(float), 0, &value);
    }

    void handleUiParameterTouch(const uint32_t index, const bool touch) const
    {
        if (kIgnoreParameters)
            return;

        if (fUI.touch != nullptr && fUI.touch->touch != nullptr)
            fUI.touch->touch(fUI.touch->handle, index+fPorts.indexOffset, touch);
    }

    void handleUiResize(const uint32_t, const uint32_t) const
    {
        // nothing here
    }

    void handlePreviewBufferData(const char type, const uint32_t size, const void* const buffer) noexcept
    {
        fPreviewData.type = type;
        fPreviewData.size = size;
        fPreviewData.buffer = buffer;
        fPreviewData.shouldSend = true;
    }

    void handleUiCustomDataChanged(const char* const key, const char* const value) const
    {
        carla_stdout("TODO: handleUiCustomDataChanged %s %s", key, value);
        //storeCustomData(key, value);

        if (fUI.writeFunction == nullptr || fUI.controller == nullptr)
            return;
    }

    void handleUiClosed()
    {
        fUI.isVisible = false;

        if (fWorkerUISignal)
            fWorkerUISignal = -1;

        if (fUI.host != nullptr && fUI.host->ui_closed != nullptr && fUI.controller != nullptr)
            fUI.host->ui_closed(fUI.controller);

        fUI.host = nullptr;
        fUI.touch = nullptr;
        fUI.writeFunction = nullptr;
        fUI.controller = nullptr;
    }

    const char* handleUiOpenFile(const bool /*isDir*/, const char* const /*title*/, const char* const /*filter*/) const
    {
        // TODO
        return nullptr;
    }

    const char* handleUiSaveFile(const bool /*isDir*/, const char* const /*title*/, const char* const /*filter*/) const
    {
        // TODO
        return nullptr;
    }

    intptr_t handleDispatcher(const NativeHostDispatcherOpcode opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
#ifdef DEBUG
        if (opcode != NATIVE_HOST_OPCODE_GET_FILE_PATH)
        {
            carla_debug("NativePlugin::handleDispatcher(%i, %i, " P_INTPTR ", %p, %f)",
                        opcode, index, value, ptr, static_cast<double>(opt));
        }
#endif

        intptr_t ret = 0;

        switch (opcode)
        {
        case NATIVE_HOST_OPCODE_NULL:
        case NATIVE_HOST_OPCODE_UPDATE_PARAMETER:
        case NATIVE_HOST_OPCODE_UPDATE_MIDI_PROGRAM:
        case NATIVE_HOST_OPCODE_RELOAD_PARAMETERS:
        case NATIVE_HOST_OPCODE_RELOAD_MIDI_PROGRAMS:
        case NATIVE_HOST_OPCODE_RELOAD_ALL:
        case NATIVE_HOST_OPCODE_HOST_IDLE:
        case NATIVE_HOST_OPCODE_INTERNAL_PLUGIN:
            // nothing
            break;

        case NATIVE_HOST_OPCODE_REQUEST_IDLE:
            CARLA_SAFE_ASSERT_RETURN(fDescriptor->hints & NATIVE_PLUGIN_REQUESTS_IDLE, 0);
            if (fWorker != nullptr && fPluginNeedsIdle == 0)
            {
                fPluginNeedsIdle = 1;
                return 1;
            }
            return 0;

        case NATIVE_HOST_OPCODE_QUEUE_INLINE_DISPLAY:
            if (fInlineDisplay != nullptr && fInlineDisplay->queue_draw != nullptr)
            {
                fInlineDisplay->queue_draw(fInlineDisplay->handle);
                return 1;
            }
            return 0;

        case NATIVE_HOST_OPCODE_GET_FILE_PATH:
            CARLA_SAFE_ASSERT_RETURN(ptr != nullptr, 0);
            if (std::strcmp((char*)ptr, "carla") == 0 && fLastProjectPath != nullptr)
                return static_cast<intptr_t>((uintptr_t)fLastProjectPath.buffer());
            break;

        case NATIVE_HOST_OPCODE_UI_UNAVAILABLE:
            handleUiClosed();
            break;

        case NATIVE_HOST_OPCODE_UI_TOUCH_PARAMETER:
            CARLA_SAFE_ASSERT_RETURN(index >= 0, 0);
            handleUiParameterTouch(static_cast<uint32_t>(index), value != 0);
            break;

        case NATIVE_HOST_OPCODE_UI_RESIZE:
            CARLA_SAFE_ASSERT_RETURN(index > 0, 0);
            CARLA_SAFE_ASSERT_RETURN(value > 0, 0);
            handleUiResize(static_cast<uint32_t>(index), static_cast<uint32_t>(value));
            break;

        case NATIVE_HOST_OPCODE_PREVIEW_BUFFER_DATA:
            CARLA_SAFE_ASSERT_RETURN(index != 0, 0);
            CARLA_SAFE_ASSERT_RETURN(index >= 'a', 0);
            CARLA_SAFE_ASSERT_RETURN(index <= 'z', 0);
            CARLA_SAFE_ASSERT_RETURN(value > 0, 0);
            CARLA_SAFE_ASSERT_RETURN(ptr != nullptr, 0);
            handlePreviewBufferData(static_cast<char>(index), static_cast<uint32_t>(value), (const void*)ptr);
            break;
        }

        return ret;

        // unused for now
        (void)opt;
    }

    void updateParameterOutputs()
    {
        float value;

        for (uint32_t i=0; i < fPorts.numParams; ++i)
        {
            if (! fPorts.paramsOut[i])
                continue;

            fPorts.paramsLast[i] = value = fDescriptor->get_parameter_value(fHandle, i);

            if (fPorts.paramsPtr[i] != nullptr)
                *fPorts.paramsPtr[i] = value;
        }
    }

    // -------------------------------------------------------------------

private:
    // Native data
    NativePluginHandle   fHandle;
    NativeHostDescriptor fHost;
    const NativePluginDescriptor* const fDescriptor;
    LV2_Program_Descriptor              fProgramDesc;

    // carla as plugin does not implement lv2 parameter API yet, needed for feedback
    const bool kIgnoreParameters;

    LV2_Atom_Forge  fAtomForge;
    uint32_t        fMidiEventCount;
    NativeMidiEvent fMidiEvents[kMaxMidiEvents];

    String fLastProjectPath;
    String fLoadedFile;
    PreviewData fPreviewData;
    volatile bool fNeedsNotifyFileChanged;
    volatile int fPluginNeedsIdle;

    int fWorkerUISignal;
    // -1 needs close, 0 idle, 1 stuff is writing??

    // -------------------------------------------------------------------

    #define handlePtr ((NativePlugin*)handle)

    static uint32_t host_get_buffer_size(NativeHostHandle handle)
    {
        return handlePtr->fBufferSize;
    }

    static double host_get_sample_rate(NativeHostHandle handle)
    {
        return handlePtr->fSampleRate;
    }

    static bool host_is_offline(NativeHostHandle handle)
    {
        return handlePtr->fIsOffline;
    }

    static const NativeTimeInfo* host_get_time_info(NativeHostHandle handle)
    {
        return &(handlePtr->fTimeInfo);
    }

    static bool host_write_midi_event(NativeHostHandle handle, const NativeMidiEvent* event)
    {
        return handlePtr->handleWriteMidiEvent(event);
    }

    static void host_ui_parameter_changed(NativeHostHandle handle, uint32_t index, float value)
    {
        handlePtr->handleUiParameterChanged(index, value);
    }

    static void host_ui_parameter_touch(NativeHostHandle handle, uint32_t index, bool touch)
    {
        handlePtr->handleUiParameterTouch(index, touch);
    }

    static void host_ui_custom_data_changed(NativeHostHandle handle, const char* key, const char* value)
    {
        handlePtr->handleUiCustomDataChanged(key, value);
    }

    static void host_ui_closed(NativeHostHandle handle)
    {
        handlePtr->handleUiClosed();
    }

    static const char* host_ui_open_file(NativeHostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiOpenFile(isDir, title, filter);
    }

    static const char* host_ui_save_file(NativeHostHandle handle, bool isDir, const char* title, const char* filter)
    {
        return handlePtr->handleUiSaveFile(isDir, title, filter);
    }

    static intptr_t host_dispatcher(NativeHostHandle handle, NativeHostDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        return handlePtr->handleDispatcher(opcode, index, value, ptr, opt);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativePlugin)
};

// -----------------------------------------------------------------------
// LV2 plugin descriptor functions

static LV2_Handle lv2_instantiate(const LV2_Descriptor* lv2Descriptor, double sampleRate, const char* bundlePath, const LV2_Feature* const* features)
{
    carla_debug("lv2_instantiate(%p, %g, %s, %p)", lv2Descriptor, sampleRate, bundlePath, features);

    const NativePluginDescriptor* pluginDesc  = nullptr;
    const char*                   pluginLabel = nullptr;

    if (std::strncmp(lv2Descriptor->URI, "http://kxstudio.sf.net/carla/plugins/", 37) == 0)
        pluginLabel = lv2Descriptor->URI+37;

    if (pluginLabel == nullptr)
    {
        carla_stderr("Failed to find carla native plugin with URI \"%s\"", lv2Descriptor->URI);
        return nullptr;
    }

    carla_debug("lv2_instantiate() - looking up label \"%s\"", pluginLabel);

    PluginListManager& plm(PluginListManager::getInstance());

    for (LinkedList<const NativePluginDescriptor*>::Itenerator it = plm.descs.begin2(); it.valid(); it.next())
    {
        const NativePluginDescriptor* const& tmpDesc(it.getValue(nullptr));
        CARLA_SAFE_ASSERT_CONTINUE(tmpDesc != nullptr);

        if (std::strcmp(tmpDesc->label, pluginLabel) == 0)
        {
            pluginDesc = tmpDesc;
            break;
        }
    }

    if (pluginDesc == nullptr)
    {
        carla_stderr("Failed to find carla native plugin with label \"%s\"", pluginLabel);
        return nullptr;
    }

    NativePlugin* const plugin(new NativePlugin(pluginDesc, sampleRate, bundlePath, features));

    if (! plugin->init())
    {
        carla_stderr("Failed to init plugin");
        delete plugin;
        return nullptr;
    }

    return (LV2_Handle)plugin;
}

#define instancePtr ((NativePlugin*)instance)

static void lv2_connect_port(LV2_Handle instance, uint32_t port, void* dataLocation)
{
    instancePtr->lv2_connect_port(port, dataLocation);
}

static void lv2_activate(LV2_Handle instance)
{
    carla_debug("lv2_activate(%p)", instance);
    instancePtr->lv2_activate();
}

static void lv2_run(LV2_Handle instance, uint32_t sampleCount)
{
    instancePtr->lv2_run(sampleCount);
}

static void lv2_deactivate(LV2_Handle instance)
{
    carla_debug("lv2_deactivate(%p)", instance);
    instancePtr->lv2_deactivate();
}

static void lv2_cleanup(LV2_Handle instance)
{
    carla_debug("lv2_cleanup(%p)", instance);
    instancePtr->lv2_cleanup();
    delete instancePtr;
}

static uint32_t lv2_get_options(LV2_Handle instance, LV2_Options_Option* options)
{
    carla_debug("lv2_get_options(%p, %p)", instance, options);
    return instancePtr->lv2_get_options(options);
}

static uint32_t lv2_set_options(LV2_Handle instance, const LV2_Options_Option* options)
{
    carla_debug("lv2_set_options(%p, %p)", instance, options);
    return instancePtr->lv2_set_options(options);
}

static const LV2_Program_Descriptor* lv2_get_program(LV2_Handle instance, uint32_t index)
{
    carla_debug("lv2_get_program(%p, %i)", instance, index);
    return instancePtr->lv2_get_program(index);
}

static void lv2_select_program(LV2_Handle instance, uint32_t bank, uint32_t program)
{
    carla_debug("lv2_select_program(%p, %i, %i)", instance, bank, program);
    return instancePtr->lv2_select_program(bank, program);
}

static LV2_State_Status lv2_save(LV2_Handle instance, LV2_State_Store_Function store, LV2_State_Handle handle, uint32_t flags, const LV2_Feature* const* features)
{
    carla_debug("lv2_save(%p, %p, %p, %i, %p)", instance, store, handle, flags, features);
    return instancePtr->lv2_save(store, handle, flags, features);
}

static LV2_State_Status lv2_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle, uint32_t flags, const LV2_Feature* const* features)
{
    carla_debug("lv2_restore(%p, %p, %p, %i, %p)", instance, retrieve, handle, flags, features);
    return instancePtr->lv2_restore(retrieve, handle, flags, features);
}

static LV2_Worker_Status lv2_work(LV2_Handle instance, LV2_Worker_Respond_Function respond, LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
{
    carla_debug("work(%p, %p, %p, %u, %p)", instance, respond, handle, size, data);
    return instancePtr->lv2_work(respond, handle, size, data);
}

static LV2_Worker_Status lv2_work_resp(LV2_Handle instance, uint32_t size, const void* body)
{
    carla_debug("work_resp(%p, %u, %p)", instance, size, body);
    return instancePtr->lv2_work_resp(size, body);
}

static const LV2_Inline_Display_Image_Surface* lv2_idisp_render(LV2_Handle instance, uint32_t w, uint32_t h)
{
    // carla_debug("lv2_idisp_render(%p, %u, %u)", instance, w, h);
    return instancePtr->lv2_idisp_render(w, h);
}

static const void* lv2_extension_data(const char* uri)
{
    carla_debug("lv2_extension_data(\"%s\")", uri);

    static const LV2_Options_Interface  options     = { lv2_get_options, lv2_set_options };
    static const LV2_Programs_Interface programs    = { lv2_get_program, lv2_select_program };
    static const LV2_State_Interface    state       = { lv2_save, lv2_restore };
    static const LV2_Worker_Interface   worker      = { lv2_work, lv2_work_resp, nullptr };
    static const LV2_Inline_Display_Interface idisp = { lv2_idisp_render };

    if (std::strcmp(uri, LV2_OPTIONS__interface) == 0)
        return &options;
    if (std::strcmp(uri, LV2_PROGRAMS__Interface) == 0)
        return &programs;
    if (std::strcmp(uri, LV2_STATE__interface) == 0)
        return &state;
    if (std::strcmp(uri, LV2_WORKER__interface) == 0)
        return &worker;
    if (std::strcmp(uri, LV2_INLINEDISPLAY__interface) == 0)
        return &idisp;

    return nullptr;
}

#undef instancePtr

#ifdef HAVE_PYQT
// -----------------------------------------------------------------------
// LV2 UI descriptor functions

static LV2UI_Handle lv2ui_instantiate(const LV2UI_Descriptor*, const char*, const char*,
                                            LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
                                            LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    carla_debug("lv2ui_instantiate(..., %p, %p, %p)", writeFunction, controller, widget, features);

    NativePlugin* plugin = nullptr;

    for (int i=0; features[i] != nullptr; ++i)
    {
        if (std::strcmp(features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0)
        {
            plugin = (NativePlugin*)features[i]->data;
            break;
        }
    }

    if (plugin == nullptr)
    {
        carla_stderr("Host doesn't support instance-access, cannot show UI");
        return nullptr;
    }

    plugin->lv2ui_instantiate(writeFunction, controller, widget, features);

    return (LV2UI_Handle)plugin;
}

#define uiPtr ((NativePlugin*)ui)

static void lv2ui_port_event(LV2UI_Handle ui, uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
{
    carla_debug("lv2ui_port_event(%p, %i, %i, %i, %p)", ui, portIndex, bufferSize, format, buffer);
    uiPtr->lv2ui_port_event(portIndex, bufferSize, format, buffer);
}

static void lv2ui_cleanup(LV2UI_Handle ui)
{
    carla_debug("lv2ui_cleanup(%p)", ui);
    uiPtr->lv2ui_cleanup();
}

static void lv2ui_select_program(LV2UI_Handle ui, uint32_t bank, uint32_t program)
{
    carla_debug("lv2ui_select_program(%p, %i, %i)", ui, bank, program);
    uiPtr->lv2ui_select_program(bank, program);
}

static int lv2ui_idle(LV2UI_Handle ui)
{
    return uiPtr->lv2ui_idle();
}

static int lv2ui_show(LV2UI_Handle ui)
{
    carla_debug("lv2ui_show(%p)", ui);
    return uiPtr->lv2ui_show();
}

static int lv2ui_hide(LV2UI_Handle ui)
{
    carla_debug("lv2ui_hide(%p)", ui);
    return uiPtr->lv2ui_hide();
}

static const void* lv2ui_extension_data(const char* uri)
{
    carla_stdout("lv2ui_extension_data(\"%s\")", uri);

    static const LV2UI_Idle_Interface uiidle = { lv2ui_idle };
    static const LV2UI_Show_Interface uishow = { lv2ui_show, lv2ui_hide };
    static const LV2_Programs_UI_Interface uiprograms = { lv2ui_select_program };

    if (std::strcmp(uri, LV2_UI__idleInterface) == 0)
        return &uiidle;
    if (std::strcmp(uri, LV2_UI__showInterface) == 0)
        return &uishow;
    if (std::strcmp(uri, LV2_PROGRAMS__UIInterface) == 0)
        return &uiprograms;

    return nullptr;
}
#endif

#undef uiPtr

// -----------------------------------------------------------------------
// Startup code

CARLA_PLUGIN_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    carla_debug("lv2_descriptor(%i)", index);

    PluginListManager& plm(PluginListManager::getInstance());

    if (index >= plm.descs.count())
    {
        carla_debug("lv2_descriptor(%i) - out of bounds", index);
        return nullptr;
    }
    if (index < plm.lv2Descs.count())
    {
        carla_debug("lv2_descriptor(%i) - found previously allocated", index);
        return plm.lv2Descs.getAt(index, nullptr);
    }

    const NativePluginDescriptor* const pluginDesc(plm.descs.getAt(index, nullptr));
    CARLA_SAFE_ASSERT_RETURN(pluginDesc != nullptr, nullptr);

    String tmpURI;
    tmpURI  = "http://kxstudio.sf.net/carla/plugins/";
    tmpURI += pluginDesc->label;

    carla_debug("lv2_descriptor(%i) - not found, allocating new with uri \"%s\"", index, (const char*)tmpURI);

    const LV2_Descriptor lv2DescTmp = {
    /* URI            */ carla_strdup(tmpURI),
    /* instantiate    */ lv2_instantiate,
    /* connect_port   */ lv2_connect_port,
    /* activate       */ lv2_activate,
    /* run            */ lv2_run,
    /* deactivate     */ lv2_deactivate,
    /* cleanup        */ lv2_cleanup,
    /* extension_data */ lv2_extension_data
    };

    LV2_Descriptor* lv2Desc;

    try {
        lv2Desc = new LV2_Descriptor;
    } CARLA_SAFE_EXCEPTION_RETURN("new LV2_Descriptor", nullptr);

    std::memcpy(lv2Desc, &lv2DescTmp, sizeof(LV2_Descriptor));

    plm.lv2Descs.append(lv2Desc);
    return lv2Desc;
}

#ifdef HAVE_PYQT
CARLA_PLUGIN_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    carla_debug("lv2ui_descriptor(%i)", index);

    static const LV2UI_Descriptor lv2UiExtDesc = {
    /* URI            */ "http://kxstudio.sf.net/carla/ui-ext",
    /* instantiate    */ lv2ui_instantiate,
    /* cleanup        */ lv2ui_cleanup,
    /* port_event     */ lv2ui_port_event,
    /* extension_data */ lv2ui_extension_data
    };

    return (index == 0) ? &lv2UiExtDesc : nullptr;
}
#endif

// -----------------------------------------------------------------------
