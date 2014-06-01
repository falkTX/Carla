/*
 * Carla Jack Plugin
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaEngine.hpp"
#include "CarlaHost.h"

#include "CarlaBackendUtils.hpp"
#include "CarlaThread.hpp"

#include "../CarlaBridgeClient.hpp"

#include "../backend/plugin/CarlaPluginInternal.hpp"
#include "CarlaMathUtils.hpp"

#include "jackbridge/JackBridge.hpp"

// -------------------------------------------------------------------------------------------------------------------

struct _jack_client {
    bool isActive;

    JackShutdownCallback shutdown_cb;
    void*                shutdown_ptr;

    JackProcessCallback process_cb;
    void*               process_ptr;

    _jack_client()
    {
        clear();
    }

    void clear()
    {
        isActive     = false;
        shutdown_cb  = nullptr;
        shutdown_ptr = nullptr;
        process_cb   = nullptr;
        process_ptr  = nullptr;
    }
};

static jack_client_t gJackClient;

// -------------------------------------------------------------------------------------------------------------------

struct _jack_port {
    bool  used;
    char  name[128+1];
    void* buffer;

    _jack_port()
        : used(false),
          buffer(nullptr) {}
};

// system ports
static jack_port_t gPortSystemIn1;  // 0
static jack_port_t gPortSystemIn2;  // 1
static jack_port_t gPortSystemOut1; // 2
static jack_port_t gPortSystemOut2; // 3

// client ports
static jack_port_t gPortAudioIn1;  // 4
static jack_port_t gPortAudioIn2;  // 5
static jack_port_t gPortAudioOut1; // 6
static jack_port_t gPortAudioOut2; // 7
static jack_port_t gPortMidiIn;    // 8
static jack_port_t gPortMidiOut;   // 9

// -------------------------------------------------------------------------------------------------------------------

CARLA_BRIDGE_START_NAMESPACE

class JackBridgeClient : public CarlaBridgeClient,
                                CarlaThread
{
public:
    JackBridgeClient()
        : CarlaBridgeClient(nullptr),
          fEngine(nullptr)
    {
        carla_debug("JackBridgeClient::JackBridgeClient()");

        carla_set_engine_callback(callback, this);

        const char* const shmIds(std::getenv("ENGINE_BRIDGE_SHM_IDS"));
        CARLA_SAFE_ASSERT_RETURN(shmIds != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(std::strlen(shmIds) == 6*2,);

        char bridgeBaseAudioName[6+1];
        char bridgeBaseControlName[6+1];

        std::strncpy(bridgeBaseAudioName,   shmIds,   6);
        std::strncpy(bridgeBaseControlName, shmIds+6, 6);
        bridgeBaseAudioName[6]   = '\0';
        bridgeBaseControlName[6] = '\0';

        const char* const clientName(std::getenv("ENGINE_BRIDGE_CLIENT_NAME"));
        CARLA_SAFE_ASSERT_RETURN(clientName != nullptr,);

        const char* const oscUrl(std::getenv("ENGINE_BRIDGE_OSC_URL"));
        CARLA_SAFE_ASSERT_RETURN(oscUrl != nullptr,);

        if (! carla_engine_init_bridge(bridgeBaseAudioName, bridgeBaseControlName, clientName))
            return;

        fEngine = carla_get_engine();
        CARLA_SAFE_ASSERT_RETURN(fEngine != nullptr,);

        CarlaBridgeClient::oscInit(oscUrl);
        fEngine->setOscBridgeData(&fOscData);

        carla_add_plugin(CarlaBackend::BINARY_NATIVE, CarlaBackend::PLUGIN_JACK, "bridge", clientName, clientName, nullptr);

        sendOscUpdate();
        sendOscBridgeUpdate();
    }

    ~JackBridgeClient() override
    {
        carla_debug("JackBridgeClient::~JackBridgeClient()");
        oscClose();
        carla_engine_close();
    }

    bool isOk() const noexcept
    {
        return (fEngine != nullptr);
    }

protected:
    void handleCallback(const CarlaBackend::EngineCallbackOpcode /*action*/, const uint /*pluginId*/, const int /*value1*/, const int /*value2*/, const float /*value3*/, const char* const /*valueStr*/)
    {
        CARLA_BACKEND_USE_NAMESPACE;
    }

    void run()
    {
        for (; carla_is_engine_running();)
        {
            carla_engine_idle();
            CarlaBridgeClient::oscIdle();
            carla_msleep(30);
        }
    }

private:
    const CarlaBackend::CarlaEngine* fEngine;

    static void callback(void* ptr, CarlaBackend::EngineCallbackOpcode action, uint pluginId, int value1, int value2, float value3, const char* valueStr)
    {
        carla_debug("CarlaPluginClient::callback(%p, %i:%s, %i, %i, %i, %f, \"%s\")", ptr, action, CarlaBackend::EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);
        CARLA_SAFE_ASSERT_RETURN(ptr != nullptr,);

        return ((JackBridgeClient*)ptr)->handleCallback(action, pluginId, value1, value2, value3, valueStr);
    }
};

// -------------------------------------------------------------------------

int CarlaBridgeOsc::handleMsgShow()
{
    carla_debug("CarlaBridgeOsc::handleMsgShow()");

    return 0;
}

int CarlaBridgeOsc::handleMsgHide()
{
    carla_debug("CarlaBridgeOsc::handleMsgHide()");

    return 0;
}

int CarlaBridgeOsc::handleMsgQuit()
{
    carla_debug("CarlaBridgeOsc::handleMsgQuit()");

    if (gJackClient.shutdown_cb != nullptr)
        gJackClient.shutdown_cb(gJackClient.shutdown_ptr);

    carla_engine_close();

    std::exit(0);

    return 0;
}

CARLA_BRIDGE_END_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT jack_client_t* jack_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...);
CARLA_EXPORT int jack_client_close(jack_client_t* client);

CARLA_EXPORT int   jack_client_name_size();
CARLA_EXPORT char* jack_get_client_name(jack_client_t* client);

CARLA_EXPORT int jack_activate(jack_client_t* client);
CARLA_EXPORT int jack_deactivate(jack_client_t* client);
CARLA_EXPORT int jack_is_realtime(jack_client_t* client);

CARLA_EXPORT jack_nframes_t jack_get_sample_rate(jack_client_t* client);
CARLA_EXPORT jack_nframes_t jack_get_buffer_size(jack_client_t* client);

CARLA_EXPORT jack_port_t* jack_port_register(jack_client_t* client, const char* port_name, const char* port_type, ulong flags, ulong buffer_size);
CARLA_EXPORT void* jack_port_get_buffer(jack_port_t* port, jack_nframes_t frames);

CARLA_EXPORT const char* jack_port_name(const jack_port_t* port);

CARLA_EXPORT const char** jack_get_ports(jack_client_t*, const char* port_name_pattern, const char* type_name_pattern, ulong flags);
CARLA_EXPORT jack_port_t* jack_port_by_name(jack_client_t* client, const char* port_name);
CARLA_EXPORT jack_port_t* jack_port_by_id(jack_client_t* client, jack_port_id_t port_id);

CARLA_EXPORT int jack_connect(jack_client_t* client, const char* source_port, const char* destination_port);
CARLA_EXPORT int jack_disconnect(jack_client_t* client, const char* source_port, const char* destination_port);

CARLA_EXPORT void jack_on_shutdown(jack_client_t* client, JackShutdownCallback function, void* arg);
CARLA_EXPORT int  jack_set_process_callback(jack_client_t* client, JackProcessCallback process_callback, void* arg);

CARLA_EXPORT void jack_set_info_function(void (*func)(const char*));
CARLA_EXPORT void jack_set_error_function(void (*func)(const char*));
CARLA_EXPORT void jack_free(void* ptr);

// -------------------------------------------------------------------------------------------------------------------

typedef void (*jack_error_callback)(const char* msg);
typedef void (*jack_info_callback)(const char* msg);

jack_error_callback sErrorCallback = nullptr;
jack_info_callback  sInfoCallback  = nullptr;

// -------------------------------------------------------------------------------------------------------------------

jack_client_t* jack_client_open(const char* client_name, jack_options_t /*options*/, jack_status_t* status, ...)
{
    carla_stderr2("JACKBRIDGE CLIENT OPEN HERE");

    if (carla_is_engine_running())
    {
        if (status != nullptr)
            *status = JackServerStarted;
        return nullptr;
    }

    static CarlaBridge::JackBridgeClient bridge;

    if (! bridge.isOk())
    {
        if (status != nullptr)
            *status = JackServerFailed;
        return nullptr;
    }

    if (! gPortSystemIn1.used)
    {
        gPortSystemIn1.used = true;
        gPortSystemIn2.used = true;
        gPortSystemOut1.used = true;
        gPortSystemOut2.used = true;
        std::strcpy(gPortSystemIn1.name, "system:capture_1");
        std::strcpy(gPortSystemIn2.name, "system:capture_2");
        std::strcpy(gPortSystemOut1.name, "system:playback_1");
        std::strcpy(gPortSystemOut2.name, "system:playback_2");
    }

    return &gJackClient;
}

int jack_client_close(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    carla_stderr2("JACKBRIDGE CLIENT CLOSE HERE");

    if (! carla_is_engine_running())
        return -1;

    carla_engine_close();
    gJackClient.clear();
    gPortAudioIn1.used = 0;
    gPortAudioIn2.used = 0;
    gPortAudioOut1.used = 0;
    gPortAudioOut2.used = 0;
    gPortMidiIn.used = 0;
    gPortMidiOut.used = 0;
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

int jack_client_name_size()
{
    return 32+1; // same as JACK1
}

char* jack_get_client_name(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, nullptr);

    if (const CarlaEngine* const engine = carla_get_engine())
        return const_cast<char*>(engine->getName());

    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

int jack_activate(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    gJackClient.isActive = true;
    return 0;
}

int jack_deactivate(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    gJackClient.isActive = false;
    return 0;
}

int jack_is_realtime(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, 0);
    return 1;
}

// -------------------------------------------------------------------------------------------------------------------

jack_nframes_t jack_get_sample_rate(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, 0);
    return static_cast<uint32_t>(carla_get_sample_rate());
}

jack_nframes_t jack_get_buffer_size(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, 0);
    return carla_get_buffer_size();
}

// -------------------------------------------------------------------------------------------------------------------

jack_port_t* jack_port_register(jack_client_t* client, const char* port_name, const char* port_type, ulong flags, ulong)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, nullptr);

    if (std::strcmp(port_type, JACK_DEFAULT_AUDIO_TYPE) == 0)
    {
        if (flags & JackPortIsInput)
        {
            if (gPortAudioIn1.used && gPortAudioIn2.used)
                return nullptr;

            if (! gPortAudioIn1.used)
            {
                std::strncpy(gPortAudioIn1.name, port_name, 128);
                return &gPortAudioIn1;
            }
            else
            {
                std::strncpy(gPortAudioIn2.name, port_name, 128);
                return &gPortAudioIn2;
            }
        }
        else
        {
            if (gPortAudioOut1.used && gPortAudioOut2.used)
                return nullptr;

            if (! gPortAudioOut1.used)
            {
                std::strncpy(gPortAudioOut1.name, port_name, 128);
                return &gPortAudioOut1;
            }
            else
            {
                std::strncpy(gPortAudioOut2.name, port_name, 128);
                return &gPortAudioOut2;
            }
        }
    }

    if (std::strcmp(port_type, JACK_DEFAULT_MIDI_TYPE) == 0)
    {
        if (flags & JackPortIsInput)
        {
            if (gPortMidiIn.used)
                return nullptr;
            std::strncpy(gPortMidiIn.name, port_name, 128);
            return &gPortMidiIn;
        }
        else
        {
            if (gPortMidiOut.used)
                return nullptr;
            std::strncpy(gPortMidiOut.name, port_name, 128);
            return &gPortMidiOut;
        }
    }

    return nullptr;
}

void* jack_port_get_buffer(jack_port_t* port, jack_nframes_t frames)
{
    return port->buffer;
}

// -------------------------------------------------------------------------------------------------------------------

const char* jack_port_name(const jack_port_t* port)
{
    return port->name;
}

// -------------------------------------------------------------------------------------------------------------------

const char** jack_get_ports(jack_client_t* client, const char* port_name_pattern, const char* type_name_pattern, ulong flags)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, nullptr);

    if (port_name_pattern != nullptr)
    {
        if (std::strstr("system:playback_", port_name_pattern) == nullptr)
            return nullptr;
        if (std::strstr("system:capture_", port_name_pattern) == nullptr)
            return nullptr;
    }
    if (type_name_pattern != nullptr)
    {
        if (std::strstr(JACK_DEFAULT_AUDIO_TYPE, type_name_pattern) == nullptr)
            return nullptr;
    }

    uint numPorts = 0;

    if (flags == 0)
    {
        numPorts = 4;
    }
    else
    {
        if (flags & JackPortIsInput)
            numPorts += 2;
        if (flags & JackPortIsOutput)
            numPorts += 2;
    }

    if (numPorts == 0)
        return nullptr;

    const char** const ports = new const char*[numPorts+1];
    uint i = 0;

    if (flags == 0 || (flags & JackPortIsInput) != 0)
    {
        ports[i++] = gPortSystemIn1.name;
        ports[i++] = gPortSystemIn1.name;
    }
    if (flags == 0 || (flags & JackPortIsOutput) != 0)
    {
        ports[i++] = gPortSystemOut1.name;
        ports[i++] = gPortSystemOut1.name;
    }
    ports[i++] = nullptr;

    return ports;
}

jack_port_t* jack_port_by_name(jack_client_t* client, const char* port_name)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, nullptr);
    CARLA_SAFE_ASSERT_RETURN(gPortSystemIn1.used, nullptr);

    if (std::strcmp(port_name, gPortSystemIn1.name) == 0)
        return &gPortSystemIn1;
    if (std::strcmp(port_name, gPortSystemIn2.name) == 0)
        return &gPortSystemIn2;
    if (std::strcmp(port_name, gPortSystemOut1.name) == 0)
        return &gPortSystemOut1;
    if (std::strcmp(port_name, gPortSystemOut2.name) == 0)
        return &gPortSystemOut2;

    if (gPortAudioIn1.used && std::strcmp(port_name, gPortAudioIn1.name) == 0)
        return &gPortAudioIn1;
    if (gPortAudioIn2.used && std::strcmp(port_name, gPortAudioIn2.name) == 0)
        return &gPortAudioIn2;
    if (gPortAudioOut1.used && std::strcmp(port_name, gPortAudioOut1.name) == 0)
        return &gPortAudioOut1;
    if (gPortAudioOut2.used && std::strcmp(port_name, gPortAudioOut2.name) == 0)
        return &gPortAudioOut2;
    if (gPortMidiIn.used && std::strcmp(port_name, gPortMidiIn.name) == 0)
        return &gPortMidiIn;
    if (gPortMidiOut.used && std::strcmp(port_name, gPortMidiOut.name) == 0)
        return &gPortMidiOut;

    return nullptr;
}

jack_port_t* jack_port_by_id(jack_client_t* client, jack_port_id_t port_id)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, nullptr);
    CARLA_SAFE_ASSERT_RETURN(gPortSystemIn1.used, nullptr);

    switch (port_id)
    {
    case 0:
        return &gPortSystemIn1;
    case 1:
        return &gPortSystemIn2;
    case 2:
        return &gPortSystemOut1;
    case 3:
        return &gPortSystemOut2;
    case 4:
        if (gPortAudioIn1.used)
            return &gPortAudioIn1;
        break;
    case 5:
        if (gPortAudioIn2.used)
            return &gPortAudioIn2;
        break;
    case 6:
        if (gPortAudioOut1.used)
            return &gPortAudioOut1;
        break;
    case 7:
        if (gPortAudioOut2.used)
            return &gPortAudioOut2;
        break;
    case 8:
        if (gPortMidiIn.used)
            return &gPortMidiIn;
        break;
    case 9:
        if (gPortMidiOut.used)
            return &gPortMidiOut;
        break;
    }

    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

int jack_connect(jack_client_t* client, const char*, const char*)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    return 0;
}

int jack_disconnect(jack_client_t* client, const char*, const char*)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

void jack_on_shutdown(jack_client_t* client, JackShutdownCallback function, void* arg)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient,);

    gJackClient.shutdown_cb = function;
    gJackClient.shutdown_ptr = arg;
}

int jack_set_process_callback(jack_client_t* client, JackProcessCallback process_callback, void* arg)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    gJackClient.process_cb = process_callback;
    gJackClient.process_ptr = arg;
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

void jack_set_error_function(void (*func)(const char*))
{
    sErrorCallback = func;
}

void jack_set_info_function(void (*func)(const char*))
{
    sInfoCallback = func;
}

void jack_free(void* ptr)
{
    delete[] (char**)ptr;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT void carla_register_all_plugins();
void carla_register_all_plugins() {}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

class JackPlugin : public CarlaPlugin
{
public:
    JackPlugin(CarlaEngine* const engine, const uint id) noexcept
        : CarlaPlugin(engine, id)
    {
        carla_debug("JackPlugin::JackPlugin(%p, %i)", engine, id);
    }

    ~JackPlugin() noexcept override
    {
        carla_debug("JackPlugin::~JackPlugin()");

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate();

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        clearBuffers();
    }

    void clearBuffers() noexcept override
    {
        pData->audioIn.count = 0;
        pData->audioOut.count = 0;

        CarlaPlugin::initBuffers();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_JACK;
    }

    PluginCategory getCategory() const noexcept override
    {
        return PLUGIN_CATEGORY_NONE;
    }

    void getLabel(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, "zita-rev1", STR_MAX);
    }

    void getRealName(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, "zita-rev1", STR_MAX);
    }

    // -------------------------------------------------------------------
    // Information (count)

    // nothing

    // -------------------------------------------------------------------
    // Information (current data)

    // nothing

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        uint options = 0x0;

        //options |= PLUGIN_OPTION_FIXED_BUFFERS;
        options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        //options |= PLUGIN_OPTION_USE_CHUNKS;

        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
        }

        return options;
    }

    // -------------------------------------------------------------------
    // Set data (state)

    // nothing

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    // -------------------------------------------------------------------
    // Set ui stuff

    // -------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        carla_stderr2("JackPlugin::reload() - start");

        //const EngineProcessMode processMode(pData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        pData->audioIn.count = 2;
        pData->audioOut.count = 2;

        bufferSizeChanged(pData->engine->getBufferSize());
        //reloadPrograms(true);

        if (pData->active)
            activate();

        carla_stderr2("JackPlugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
    }

    void deactivate() noexcept override
    {
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames) override
    {
        carla_stderr2("JackPlugin::process");
        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! (gJackClient.isActive && pData->active))
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FLOAT_CLEAR(outBuffer[i], static_cast<int>(frames));
            carla_stderr2("JackPlugin::process disabled");
            return;
        }
        carla_stderr2("JackPlugin::working");

        // --------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            pData->needsReset = false;
        }

        processSingle(inBuffer, outBuffer, frames);
    }

    bool processSingle(float** const inBuffer, float** const outBuffer, const uint32_t frames)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(inBuffer != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(outBuffer != nullptr, false);
        }

        // --------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                FLOAT_CLEAR(outBuffer[i], frames);
            return false;
        }

        // --------------------------------------------------------------------------------------------------------
        // Set audio in buffers

        if (pData->audioIn.count == 2)
        {
            gPortAudioIn1.buffer = inBuffer[0];
            gPortAudioIn2.buffer = inBuffer[1];
        }
        else if (pData->audioIn.count == 1)
        {
            gPortAudioIn1.buffer = inBuffer[0];
        }

        if (pData->audioOut.count == 2)
        {
            gPortAudioOut1.buffer = outBuffer[0];
            gPortAudioOut2.buffer = outBuffer[1];
        }
        else if (pData->audioOut.count == 1)
        {
            gPortAudioOut1.buffer = outBuffer[0];
        }

        // --------------------------------------------------------------------------------------------------------
        // Run plugin

        //if (gJackClient.process_cb != nullptr)
        //    gJackClient.process_cb(frames, gJackClient.process_ptr);

        // --------------------------------------------------------------------------------------------------------
        // Set audio out buffers

        //for (uint32_t i=0; i < pData->audioOut.count; ++i)
        //    FloatVectorOperations::copy(outBuffer[i], fAudioBuffer.getSampleData(static_cast<int>(i)), static_cast<int>(frames));

        // --------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("VstPlugin::bufferSizeChanged(%i)", newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("VstPlugin::sampleRateChanged(%g)", newSampleRate);
    }

    // -------------------------------------------------------------------
    // Plugin buffers

    // nothing

    // -------------------------------------------------------------------
    // Post-poned UI Stuff

    // nothing

    // -------------------------------------------------------------------

protected:
    // TODO

    // -------------------------------------------------------------------

public:
    bool init(const char* const filename, const char* const name, const char* const label)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ---------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr || filename[0] == '\0')
        {
            pData->engine->setLastError("null filename");
            return false;
        }

        if (label == nullptr || label[0] == '\0')
        {
            pData->engine->setLastError("null label");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (name != nullptr && name[0] != '\0')
            pData->name = pData->engine->getUniquePluginName(name);
        else
            pData->name = pData->engine->getUniquePluginName("test");

        pData->filename = carla_strdup(filename);

        // ---------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(this);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // load plugin settings

        {
            // set default options
            pData->options = 0x0;

            //pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;
            pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
            //pData->options |= PLUGIN_OPTION_USE_CHUNKS;

            //if (fInstance->acceptsMidi())
            {
                pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
                pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
                pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
                pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            }

            // set identifier string
            CarlaString identifier("JACK/");
            //identifier += juceId.toRawUTF8();
            pData->identifier = identifier.dup();

            // load settings
            pData->options = pData->loadSettings(pData->options, getOptionsAvailable());
        }

        return true;
    }

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JackPlugin)
};

CarlaPlugin* CarlaPlugin::newJACK(const Initializer& init)
{
    carla_debug("CarlaPlugin::newJACK({%p, \"%s\", \"%s\", \"%s\"})", init.engine, init.filename, init.name, init.label);

    JackPlugin* const plugin(new JackPlugin(init.engine, init.id));

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

//     if (init.engine->getProccessMode() == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && ! plugin->canRunInRack())
//     {
//         init.engine->setLastError("Carla's rack mode can only work with Stereo bridged apps, sorry!");
//         delete plugin;
//         return nullptr;
//     }

    return plugin;
}

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
