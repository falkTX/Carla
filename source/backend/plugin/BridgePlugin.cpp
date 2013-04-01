/*
 * Carla Bridge Plugin
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaPluginInternal.hpp"

#ifndef BUILD_BRIDGE

#include "CarlaBridgeUtils.hpp"
#include "CarlaShmUtils.hpp"

#include <cerrno>
#include <ctime>

#ifdef CARLA_OS_WIN
# include <sys/time.h>
#endif

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>

#define CARLA_BRIDGE_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare)                                       \
    /* check argument count */                                                                                               \
    if (argc != argcToCompare)                                                                                               \
    {                                                                                                                        \
        carla_stderr("BridgePlugin::%s() - argument count mismatch: %i != %i", __FUNCTION__, argc, argcToCompare);           \
        return 1;                                                                                                            \
    }                                                                                                                        \
    if (argc > 0)                                                                                                            \
    {                                                                                                                        \
        /* check for nullness */                                                                                             \
        if (! (types && typesToCompare))                                                                                     \
        {                                                                                                                    \
            carla_stderr("BridgePlugin::%s() - argument types are null", __FUNCTION__);                                      \
            return 1;                                                                                                        \
        }                                                                                                                    \
        /* check argument types */                                                                                           \
        if (std::strcmp(types, typesToCompare) != 0)                                                                         \
        {                                                                                                                    \
            carla_stderr("BridgePlugin::%s() - argument types mismatch: '%s' != '%s'", __FUNCTION__, types, typesToCompare); \
            return 1;                                                                                                        \
        }                                                                                                                    \
    }

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Engine Helpers

extern void registerEnginePlugin(CarlaEngine* const engine, const unsigned int id, CarlaPlugin* const plugin);

// -------------------------------------------------------------------------------------------------------------------

shm_t shm_mkstemp(char* const fileBase)
{
    static const char charSet[] = "abcdefghijklmnopqrstuvwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "0123456789";

    const int size = (fileBase != nullptr) ? std::strlen(fileBase) : 0;

    shm_t shm;
    carla_shm_init(shm);

    if (size < 6)
    {
        errno = EINVAL;
        return shm;
    }

    if (std::strcmp(fileBase + size - 6, "XXXXXX") != 0)
    {
        errno = EINVAL;
        return shm;
    }

    while (true)
    {
        for (int c = size - 6; c < size; c++)
        {
            // Note the -1 to avoid the trailing '\0' in charSet.
            fileBase[c] = charSet[std::rand() % (sizeof(charSet) - 1)];
        }

        shm_t shm = carla_shm_create(fileBase);

        if (carla_is_shm_valid(shm) || errno != EEXIST)
            return shm;
    }
}

// -------------------------------------------------------------------------------------------------------------------

struct BridgeParamInfo {
    float value;
    CarlaString name;
    CarlaString unit;

    BridgeParamInfo()
        : value(0.0f) {}
};

class BridgePlugin : public CarlaPlugin
{
public:
    BridgePlugin(CarlaEngine* const engine, const unsigned int id, const BinaryType btype, const PluginType ptype)
        : CarlaPlugin(engine, id),
          fBinaryType(btype),
          fPluginType(ptype),
          fInitiated(false),
          fInitError(false),
          fSaved(false),
          fParams(nullptr)
    {
        carla_debug("BridgePlugin::BridgePlugin(%p, %i, %s, %s)", engine, id, BinaryType2Str(btype), PluginType2Str(ptype));

        kData->osc.thread.setMode(CarlaPluginThread::PLUGIN_THREAD_BRIDGE);
    }

    ~BridgePlugin()
    {
        carla_debug("BridgePlugin::~BridgePlugin()");

        kData->singleMutex.lock();
        kData->masterMutex.lock();

        _cleanup();

        if (kData->osc.data.target != nullptr)
        {
            osc_send_hide(&kData->osc.data);
            osc_send_quit(&kData->osc.data);
        }

        kData->osc.data.free();

        // Wait a bit first, then force kill
        if (kData->osc.thread.isRunning() && ! kData->osc.thread.wait(1000)) // kData->engine->getOptions().oscUiTimeout
        {
            carla_stderr("Failed to properly stop Plugin Bridge thread");
            kData->osc.thread.terminate();
        }

        //info.chunk.clear();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType type() const
    {
        return fPluginType;
    }

    PluginCategory category()
    {
        return fInfo.category;
    }

    long uniqueId() const
    {
        return fInfo.uniqueId;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t audioInCount()
    {
        return fInfo.aIns;
    }

    uint32_t audioOutCount()
    {
        return fInfo.aOuts;
    }

    uint32_t midiInCount()
    {
        return fInfo.mIns;
    }

    uint32_t midiOutCount()
    {
        return fInfo.mOuts;
    }

#if 0
    // -------------------------------------------------------------------
    // Information (current data)

    int32_t chunkData(void** const dataPtr)
    {
        CARLA_ASSERT(dataPtr);

        if (! info.chunk.isEmpty())
        {
            *dataPtr = info.chunk.data();
            return info.chunk.size();
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double getParameterValue(const uint32_t parameterId)
    {
        CARLA_ASSERT(parameterId < param.count);

        return params[parameterId].value;
    }
#endif

    void getLabel(char* const strBuf)
    {
        std::strncpy(strBuf, (const char*)fInfo.label, STR_MAX);
    }

    void getMaker(char* const strBuf)
    {
        std::strncpy(strBuf, (const char*)fInfo.maker, STR_MAX);
    }

    void getCopyright(char* const strBuf)
    {
        std::strncpy(strBuf, (const char*)fInfo.copyright, STR_MAX);
    }

    void getRealName(char* const strBuf)
    {
        std::strncpy(strBuf, (const char*)fInfo.name, STR_MAX);
    }

#if 0
    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(parameterId < param.count);

        strncpy(strBuf, params[parameterId].name.toUtf8().constData(), STR_MAX);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf)
    {
        CARLA_ASSERT(parameterId < param.count);

        strncpy(strBuf, params[parameterId].unit.toUtf8().constData(), STR_MAX);
    }
#endif

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    int setOscPluginBridgeInfo(const PluginBridgeInfoType type, const int argc, const lo_arg* const* const argv, const char* const types)
    {
        carla_stdout("setOscPluginBridgeInfo(%i, %i, %p, \"%s\")", type, argc, argv, types);

        switch (type)
        {
        case kPluginBridgeAudioCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iii");

            const int32_t aIns   = argv[0]->i;
            const int32_t aOuts  = argv[1]->i;
            const int32_t aTotal = argv[2]->i;

            CARLA_ASSERT(aIns >= 0);
            CARLA_ASSERT(aOuts >= 0);
            CARLA_ASSERT(aIns + aOuts == aTotal);

            fInfo.aIns  = aIns;
            fInfo.aOuts = aOuts;

            break;

            // unused
            (void)aTotal;
        }

        case kPluginBridgeMidiCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iii");

            const int32_t mIns   = argv[0]->i;
            const int32_t mOuts  = argv[1]->i;
            const int32_t mTotal = argv[2]->i;

            CARLA_ASSERT(mIns >= 0);
            CARLA_ASSERT(mOuts >= 0);
            CARLA_ASSERT(mIns + mOuts == mTotal);

            fInfo.mIns  = mIns;
            fInfo.mOuts = mOuts;

            break;

            // unused
            (void)mTotal;
        }

#if 0
        case PluginBridgeParameterCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iii");

            const int32_t pIns   = argv[0]->i;
            const int32_t pOuts  = argv[1]->i;
            const int32_t pTotal = argv[2]->i;

            CARLA_ASSERT(pIns >= 0);
            CARLA_ASSERT(pOuts >= 0);
            CARLA_ASSERT(pIns + pOuts <= pTotal);

            // delete old data
            if (param.count > 0)
            {
                delete[] param.data;
                delete[] param.ranges;
                delete[] params;
            }

            // create new if needed
            const int32_t maxParams = x_engine->getOptions().maxParameters;
            param.count = (pTotal < maxParams) ? pTotal : 0;

            if (param.count > 0)
            {
                param.data   = new ParameterData[param.count];
                param.ranges = new ParameterRanges[param.count];
                params       = new BridgeParamInfo[param.count];
            }
            else
            {
                param.data = nullptr;
                param.ranges = nullptr;
                params = nullptr;
            }

            break;
            Q_UNUSED(pIns);
            Q_UNUSED(pOuts);
        }

        case PluginBridgeProgramCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t count = argv[0]->i;

            CARLA_ASSERT(count >= 0);

            // delete old programs
            if (prog.count > 0)
            {
                for (uint32_t i=0; i < prog.count; i++)
                {
                    if (prog.names[i])
                        free((void*)prog.names[i]);
                }

                delete[] prog.names;
            }

            prog.count = count;

            // create new if needed
            if (prog.count > 0)
            {
                prog.names = new const char* [prog.count];

                for (uint32_t i=0; i < prog.count; i++)
                    prog.names[i] = nullptr;
            }
            else
                prog.names = nullptr;

            break;
        }

        case PluginBridgeMidiProgramCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t count = argv[0]->i;

            // delete old programs
            if (midiprog.count > 0)
            {
                for (uint32_t i=0; i < midiprog.count; i++)
                {
                    if (midiprog.data[i].name)
                        free((void*)midiprog.data[i].name);
                }

                delete[] midiprog.data;
            }

            // create new if needed
            midiprog.count = count;

            if (midiprog.count > 0)
                midiprog.data = new MidiProgramData[midiprog.count];
            else
                midiprog.data = nullptr;

            break;
        }
#endif

        case kPluginBridgePluginInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(7, "iissssh");

            const int32_t category  = argv[0]->i;
            const int32_t hints     = argv[1]->i;
            const char* const name  = (const char*)&argv[2]->s;
            const char* const label = (const char*)&argv[3]->s;
            const char* const maker = (const char*)&argv[4]->s;
            const char* const copyright = (const char*)&argv[5]->s;
            const int64_t uniqueId      = argv[6]->h;

            CARLA_ASSERT(category >= 0);
            CARLA_ASSERT(hints >= 0);
            CARLA_ASSERT(name);
            CARLA_ASSERT(label);
            CARLA_ASSERT(maker);
            CARLA_ASSERT(copyright);

            fHints = hints | PLUGIN_IS_BRIDGE;

            fInfo.category = static_cast<PluginCategory>(category);
            fInfo.uniqueId = uniqueId;

            fInfo.name  = name;
            fInfo.label = label;
            fInfo.maker = maker;
            fInfo.copyright = copyright;

            if (fName.isEmpty())
                fName = kData->engine->getNewUniquePluginName(name);

            break;
        }

#if 0
        case PluginBridgeParameterInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iss");

            const int32_t index    = argv[0]->i;
            const char* const name = (const char*)&argv[1]->s;
            const char* const unit = (const char*)&argv[2]->s;

            CARLA_ASSERT(index >= 0 && index < (int32_t)param.count);
            CARLA_ASSERT(name);
            CARLA_ASSERT(unit);

            if (index >= 0 && index < (int32_t)param.count)
            {
                params[index].name = QString(name);
                params[index].unit = QString(unit);
            }

            break;
        }

        case PluginBridgeParameterData:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(6, "iiiiii");

            const int32_t index   = argv[0]->i;
            const int32_t type    = argv[1]->i;
            const int32_t rindex  = argv[2]->i;
            const int32_t hints   = argv[3]->i;
            const int32_t channel = argv[4]->i;
            const int32_t cc      = argv[5]->i;

            CARLA_ASSERT(index >= 0 && index < (int32_t)param.count);
            CARLA_ASSERT(type >= 0);
            CARLA_ASSERT(rindex >= 0);
            CARLA_ASSERT(hints >= 0);
            CARLA_ASSERT(channel >= 0 && channel < 16);
            CARLA_ASSERT(cc >= -1);

            if (index >= 0 && index < (int32_t)param.count)
            {
                param.data[index].type    = (ParameterType)type;
                param.data[index].index   = index;
                param.data[index].rindex  = rindex;
                param.data[index].hints   = hints;
                param.data[index].midiChannel = channel;
                param.data[index].midiCC  = cc;
            }

            break;
        }

        case PluginBridgeParameterRanges:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(7, "idddddd");

            const int32_t index = argv[0]->i;
            const double def    = argv[1]->d;
            const double min    = argv[2]->d;
            const double max    = argv[3]->d;
            const double step   = argv[4]->d;
            const double stepSmall = argv[5]->d;
            const double stepLarge = argv[6]->d;

            CARLA_ASSERT(index >= 0 && index < (int32_t)param.count);
            CARLA_ASSERT(min < max);
            CARLA_ASSERT(def >= min);
            CARLA_ASSERT(def <= max);

            if (index >= 0 && index < (int32_t)param.count)
            {
                param.ranges[index].def  = def;
                param.ranges[index].min  = min;
                param.ranges[index].max  = max;
                param.ranges[index].step = step;
                param.ranges[index].stepSmall = stepSmall;
                param.ranges[index].stepLarge = stepLarge;
                params[index].value = def;
            }

            break;
        }

        case PluginBridgeProgramInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "is");

            const int32_t index    = argv[0]->i;
            const char* const name = (const char*)&argv[1]->s;

            CARLA_ASSERT(index >= 0 && index < (int32_t)prog.count);
            CARLA_ASSERT(name);

            if (index >= 0 && index < (int32_t)prog.count)
            {
                if (prog.names[index])
                    free((void*)prog.names[index]);

                prog.names[index] = strdup(name);
            }

            break;
        }

        case PluginBridgeMidiProgramInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(4, "iiis");

            const int32_t index    = argv[0]->i;
            const int32_t bank     = argv[1]->i;
            const int32_t program  = argv[2]->i;
            const char* const name = (const char*)&argv[3]->s;

            CARLA_ASSERT(index >= 0 && index < (int32_t)midiprog.count);
            CARLA_ASSERT(bank >= 0);
            CARLA_ASSERT(program >= 0 && program < 128);
            CARLA_ASSERT(name);

            if (index >= 0 && index < (int32_t)midiprog.count)
            {
                if (midiprog.data[index].name)
                    free((void*)midiprog.data[index].name);

                midiprog.data[index].bank    = bank;
                midiprog.data[index].program = program;
                midiprog.data[index].name    = strdup(name);
            }

            break;
        }

        case PluginBridgeConfigure:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "ss");

            const char* const key   = (const char*)&argv[0]->s;
            const char* const value = (const char*)&argv[1]->s;

            CARLA_ASSERT(key);
            CARLA_ASSERT(value);

            if (! (key && value))
            {
                // invalid
                pass();
            }
            else if (std::strcmp(key, CARLA_BRIDGE_MSG_HIDE_GUI) == 0)
            {
                x_engine->callback(CALLBACK_SHOW_GUI, m_id, 0, 0, 0.0, nullptr);
            }
            else if (std::strcmp(key, CARLA_BRIDGE_MSG_SAVED) == 0)
            {
                m_saved = true;
            }

            break;
        }

        case PluginBridgeSetParameterValue:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "id");

            const int32_t index = argv[0]->i;
            const double  value = argv[1]->d;

            CARLA_ASSERT(index != PARAMETER_NULL);

            setParameterValueByRIndex(index, value, false, true, true);

            break;
        }

        case PluginBridgeSetDefaultValue:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "id");

            const int32_t index = argv[0]->i;
            const double  value = argv[1]->d;

            CARLA_ASSERT(index >= 0 && index < (int32_t)param.count);

            if (index >= 0 && index < (int32_t)param.count)
                param.ranges[index].def = value;

            break;
        }

        case PluginBridgeSetProgram:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t index = argv[0]->i;

            CARLA_ASSERT(index < (int32_t)prog.count);

            setProgram(index, false, true, true, true);

            break;
        }

        case PluginBridgeSetMidiProgram:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t index = argv[0]->i;

            CARLA_ASSERT(index < (int32_t)midiprog.count);

            setMidiProgram(index, false, true, true, true);

            break;
        }

        case PluginBridgeSetCustomData:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "sss");

            const char* const type  = (const char*)&argv[0]->s;
            const char* const key   = (const char*)&argv[1]->s;
            const char* const value = (const char*)&argv[2]->s;

            CARLA_ASSERT(type);
            CARLA_ASSERT(key);
            CARLA_ASSERT(value);

            setCustomData(type, key, value, false);

            break;
        }

        case PluginBridgeSetChunkData:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "s");

            const char* const chunkFileChar = (const char*)&argv[0]->s;

            CARLA_ASSERT(chunkFileChar);

            QString chunkFileStr(chunkFileChar);

#ifndef Q_OS_WIN
            // Using Wine, fix temp dir
            if (m_binary == BINARY_WIN32 || m_binary == BINARY_WIN64)
            {
                // Get WINEPREFIX
                QString wineDir;
                if (const char* const WINEPREFIX = getenv("WINEPREFIX"))
                    wineDir = QString(WINEPREFIX);
                else
                    wineDir = QDir::homePath() + "/.wine";

                QStringList chunkFileStrSplit1 = chunkFileStr.split(":/");
                QStringList chunkFileStrSplit2 = chunkFileStrSplit1.at(1).split("\\");

                QString wineDrive = chunkFileStrSplit1.at(0).toLower();
                QString wineTMP   = chunkFileStrSplit2.at(0);
                QString baseName  = chunkFileStrSplit2.at(1);

                chunkFileStr  = wineDir;
                chunkFileStr += "/drive_";
                chunkFileStr += wineDrive;
                chunkFileStr += "/";
                chunkFileStr += wineTMP;
                chunkFileStr += "/";
                chunkFileStr += baseName;
                chunkFileStr  = QDir::toNativeSeparators(chunkFileStr);
            }
#endif

            QFile chunkFile(chunkFileStr);

            if (chunkFile.open(QIODevice::ReadOnly))
            {
                info.chunk = chunkFile.readAll();
                chunkFile.close();
                chunkFile.remove();
            }

            break;
        }
#endif

        case kPluginBridgeUpdateNow:
        {
            fInitiated = true;
            break;
        }

        case kPluginBridgeError:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "s");

            const char* const error = (const char*)&argv[0]->s;

            CARLA_ASSERT(error != nullptr);

            kData->engine->setLastError(error);

            fInitError = true;
            fInitiated = true;
            break;
        }
        }

        return 0;
    }

#if 0
    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        CARLA_ASSERT(parameterId < param.count);

        params[parameterId].value = fixParameterValue(value, param.ranges[parameterId]);

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
    {
        CARLA_ASSERT(type);
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);

        if (sendGui)
        {
            // TODO - if type is chunk|binary, store it in a file and send path instead
            QString cData;
            cData  = type;
            cData += "·";
            cData += key;
            cData += "·";
            cData += value;
            osc_send_configure(&osc.data, CARLA_BRIDGE_MSG_SET_CUSTOM, cData.toUtf8().constData());
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setChunkData(const char* const stringData)
    {
        CARLA_ASSERT(m_hints & PLUGIN_USES_CHUNKS);
        CARLA_ASSERT(stringData);

        QString filePath;
        filePath  = QDir::tempPath();
        filePath += "/.CarlaChunk_";
        filePath += m_name;

        filePath = QDir::toNativeSeparators(filePath);

        QFile file(filePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out << stringData;
            file.close();
            osc_send_configure(&osc.data, CARLA_BRIDGE_MSG_SET_CHUNK, filePath.toUtf8().constData());
        }
    }
#endif

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo)
    {
        if (yesNo)
            osc_send_show(&kData->osc.data);
        else
            osc_send_hide(&kData->osc.data);
    }

    void idleGui()
    {
        if (! kData->osc.thread.isRunning())
            carla_stderr2("TESTING: Bridge has closed!");

        CarlaPlugin::idleGui();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        carla_debug("BridgePlugin::reload() - start");
        CARLA_ASSERT(kData->engine != nullptr);

        if (kData->engine == nullptr)
            return;

        const ProcessMode processMode(kData->engine->getProccessMode());

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        deleteBuffers();

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        if (fInfo.aIns > 0)
        {
            kData->audioIn.createNew(fInfo.aIns);
        }

        if (fInfo.aOuts > 0)
        {
            kData->audioOut.createNew(fInfo.aOuts);
            needsCtrlIn = true;
        }

        if (fInfo.mIns > 0)
            needsCtrlIn = true;

        if (fInfo.mOuts > 0)
            needsCtrlOut = true;

        const uint  portNameSize = kData->engine->maxPortNameSize();
        CarlaString portName;

        // Audio Ins
        for (uint32_t j=0; j < fInfo.aIns; j++)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            if (fInfo.aIns > 1)
            {
                portName += "input_";
                portName += CarlaString(j+1);
            }
            else
                portName += "input";
            portName.truncate(portNameSize);

            kData->audioIn.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, true);
            kData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (uint32_t j=0; j < fInfo.aOuts; j++)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            if (fInfo.aOuts > 1)
            {
                portName += "output_";
                portName += CarlaString(j+1);
            }
            else
                portName += "output";
            portName.truncate(portNameSize);

            kData->audioOut.ports[j].port   = (CarlaEngineAudioPort*)kData->client->addPort(kEnginePortTypeAudio, portName, false);
            kData->audioOut.ports[j].rindex = j;
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "event-in";
            portName.truncate(portNameSize);

            kData->event.portIn = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, true);
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = fName;
                portName += ":";
            }

            portName += "event-out";
            portName.truncate(portNameSize);

            kData->event.portOut = (CarlaEngineEventPort*)kData->client->addPort(kEnginePortTypeEvent, portName, false);
        }

        //bufferSizeChanged(kData->engine->getBufferSize());
        //reloadPrograms(true);

        carla_debug("BridgePlugin::reload() - end");
    }

#if 0
    void prepareForSave()
    {
        m_saved = false;
        osc_send_configure(&osc.data, CARLA_BRIDGE_MSG_SAVE_NOW, "");

        for (int i=0; i < 200; i++)
        {
            if (m_saved)
                break;
            carla_msleep(50);
        }

        if (! m_saved)
            carla_stderr("BridgePlugin::prepareForSave() - Timeout while requesting save state");
        else
            carla_debug("BridgePlugin::prepareForSave() - success!");
    }
#endif

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames)
    {
        uint32_t i, k;

        // --------------------------------------------------------------------------------------------------------
        // Check if active

        if (! kData->active)
        {
            // disable any output sound
            for (i=0; i < kData->audioOut.count; i++)
                carla_zeroFloat(outBuffer[i], frames);

            //kData->activeBefore = kData->active;
            return;
        }

        for (i=0; i < fInfo.aIns; ++i)
            carla_copyFloat(fShmAudioPool.data + i * frames, inBuffer[i], frames);

        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeProcess);
        rdwr_commitWrite(&fShmControl.data->ringBuffer);

        waitForServer();

        for (i=0; i < fInfo.aOuts; ++i)
            carla_copyFloat(outBuffer[i], fShmAudioPool.data + (i+fInfo.aIns) * frames, frames);
    }

    // -------------------------------------------------------------------
    // Post-poned events

    void uiParameterChange(const uint32_t index, const float value)
    {
        CARLA_ASSERT(index < kData->param.count);

        if (index >= kData->param.count)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        osc_send_control(&kData->osc.data, kData->param.data[index].rindex, value);
    }

    void uiProgramChange(const uint32_t index)
    {
        CARLA_ASSERT(index < kData->prog.count);

        if (index >= kData->prog.count)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        osc_send_program(&kData->osc.data, index);
    }

    void uiMidiProgramChange(const uint32_t index)
    {
        CARLA_ASSERT(index < kData->midiprog.count);

        if (index >= kData->midiprog.count)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        osc_send_midi_program(&kData->osc.data, index);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(note < MAX_MIDI_NOTE);
        CARLA_ASSERT(velo > 0 && velo < MAX_MIDI_VALUE);

        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (note >= MAX_MIDI_NOTE)
            return;
        if (velo >= MAX_MIDI_VALUE)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_ON + channel;
        midiData[2] = note;
        midiData[3] = velo;

        osc_send_midi(&kData->osc.data, midiData);
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note)
    {
        CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);
        CARLA_ASSERT(note < MAX_MIDI_NOTE);

        if (channel >= MAX_MIDI_CHANNELS)
            return;
        if (note >= MAX_MIDI_NOTE)
            return;
        if (kData->osc.data.target == nullptr)
            return;

        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
        midiData[2] = note;

        osc_send_midi(&kData->osc.data, midiData);
    }

    void bufferSizeChanged(const uint32_t newBufferSize)
    {
        resizeAudioPool(newBufferSize);
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label, const char* const bridgeBinary)
    {
        CARLA_ASSERT(kData->engine != nullptr);
        CARLA_ASSERT(kData->client == nullptr);
        CARLA_ASSERT(filename != nullptr);

        // ---------------------------------------------------------------
        // first checks

        if (kData->engine == nullptr)
        {
            return false;
        }

        if (kData->client != nullptr)
        {
            kData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr)
        {
            kData->engine->setLastError("null filename");
            return false;
        }

        // ---------------------------------------------------------------
        // set info

        if (name != nullptr)
            fName = kData->engine->getNewUniquePluginName(name);

        fFilename = filename;

        // ---------------------------------------------------------------
        // register client

        kData->client = kData->engine->addClient(this);

        if (kData->client == nullptr || ! kData->client->isOk())
        {
            kData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // SHM Audio Pool
        {
            char tmpFileBase[60];

            std::srand(std::time(NULL));
            std::sprintf(tmpFileBase, "/carla-bridge_shm_XXXXXX");

            fShmAudioPool.shm = shm_mkstemp(tmpFileBase);

            if (! carla_is_shm_valid(fShmAudioPool.shm))
            {
                //_cleanup();
                carla_stdout("Failed to open or create shared memory file #1");
                return false;
            }

            fShmAudioPool.filename = tmpFileBase;
        }

        // ---------------------------------------------------------------
        // SHM Control
        {
            char tmpFileBase[60];

            std::sprintf(tmpFileBase, "/carla-bridge_shc_XXXXXX");

            fShmControl.shm = shm_mkstemp(tmpFileBase);

            if (! carla_is_shm_valid(fShmControl.shm))
            {
                //_cleanup();
                carla_stdout("Failed to open or create shared memory file #2");
                return false;
            }

            fShmControl.filename = tmpFileBase;

            if (! carla_shm_map<BridgeShmControl>(fShmControl.shm, fShmControl.data))
            {
                //_cleanup();
                carla_stdout("Failed to mmap shared memory file");
                return false;
            }

            CARLA_ASSERT(fShmControl.data != nullptr);

            std::memset(fShmControl.data, 0, sizeof(BridgeShmControl));
            std::strcpy(fShmControl.data->ringBuffer.buf, "This thing is actually working!!!!");

            if (sem_init(&fShmControl.data->runServer, 1, 0) != 0)
            {
                //_cleanup();
                carla_stdout("Failed to initialize shared memory semaphore #1");
                return false;
            }

            if (sem_init(&fShmControl.data->runClient, 1, 0) != 0)
            {
                //_cleanup();
                carla_stdout("Failed to initialize shared memory semaphore #2");
                return false;
            }
        }

        // initial values
        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeBufferSize);
        rdwr_writeInt(&fShmControl.data->ringBuffer, kData->engine->getBufferSize());

        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeSampleRate);
        rdwr_writeFloat(&fShmControl.data->ringBuffer, kData->engine->getSampleRate());

        rdwr_commitWrite(&fShmControl.data->ringBuffer);

        // register plugin now so we can receive OSC (and wait for it)
        fHints |= PLUGIN_IS_BRIDGE;
        registerEnginePlugin(kData->engine, fId, this);

        // init OSC
        {
            char shmIdStr[12+1] = { 0 };
            std::strncpy(shmIdStr, &fShmAudioPool.filename[fShmAudioPool.filename.length()-6], 6);
            std::strncat(shmIdStr, &fShmControl.filename[fShmControl.filename.length()-6], 6);

            kData->osc.thread.setOscData(bridgeBinary, label, getPluginTypeAsString(fPluginType), shmIdStr);
            kData->osc.thread.start();
            //kData->osc.thread.waitForStarted();
        }

        for (int i=0; i < 200; i++)
        {
            if (fInitiated || ! kData->osc.thread.isRunning())
                break;
            carla_msleep(50);
        }

        if (! fInitiated)
        {
            // unregister so it gets handled properly
            registerEnginePlugin(kData->engine, fId, nullptr);

            if (kData->osc.thread.isRunning())
                kData->osc.thread.terminate();

            kData->engine->setLastError("Timeout while waiting for a response from plugin-bridge\n(or the plugin crashed on initialization?)");
            return false;
        }
        else if (fInitError)
        {
            // unregister so it gets handled properly
            registerEnginePlugin(kData->engine, fId, nullptr);

            kData->osc.thread.quit();
            // last error was set before
            return false;
        }

        resizeAudioPool(kData->engine->getBufferSize());

        rdwr_writeOpcode(&fShmControl.data->ringBuffer, kPluginBridgeOpcodeReadyWait);
        rdwr_writeInt(&fShmControl.data->ringBuffer, fShmAudioPool.size);
        rdwr_commitWrite(&fShmControl.data->ringBuffer);
        waitForServer();

        return true;
    }

private:
    const BinaryType fBinaryType;
    const PluginType fPluginType;

    bool fInitiated;
    bool fInitError;
    bool fSaved;

    struct BridgeAudioPool {
        CarlaString filename;
        float* data;
        size_t size;
        shm_t shm;

        BridgeAudioPool()
            : data(nullptr),
              size(0)
        {
            carla_shm_init(shm);
        }
    } fShmAudioPool;

    struct BridgeControl {
        CarlaString filename;
        BridgeShmControl* data;
        shm_t shm;

        BridgeControl()
            : data(nullptr)
        {
            carla_shm_init(shm);
        }

    } fShmControl;

    struct Info {
        uint32_t aIns, aOuts;
        uint32_t mIns, mOuts;
        PluginCategory category;
        long uniqueId;
        CarlaString name;
        CarlaString label;
        CarlaString maker;
        CarlaString copyright;
        //QByteArray chunk;

        Info()
            : aIns(0),
              aOuts(0),
              mIns(0),
              mOuts(0),
              category(PLUGIN_CATEGORY_NONE),
              uniqueId(0) {}
    } fInfo;

    BridgeParamInfo* fParams;

    void _cleanup()
    {
        if (fShmAudioPool.filename.isNotEmpty())
            fShmAudioPool.filename.clear();

        if (fShmControl.filename.isNotEmpty())
            fShmControl.filename.clear();

        if (fShmAudioPool.data != nullptr)
        {
            carla_shm_unmap(fShmAudioPool.shm, fShmAudioPool.data, fShmAudioPool.size);
            fShmAudioPool.data = nullptr;
        }

        fShmAudioPool.size = 0;

        // and again
        if (fShmControl.data != nullptr)
        {
            carla_shm_unmap(fShmControl.shm, fShmControl.data, sizeof(BridgeShmControl));
            fShmControl.data = nullptr;
        }

        if (carla_is_shm_valid(fShmAudioPool.shm))
            carla_shm_close(fShmAudioPool.shm);

        if (carla_is_shm_valid(fShmControl.shm))
            carla_shm_close(fShmControl.shm);
    }

    void resizeAudioPool(const uint32_t bufferSize)
    {
        if (fShmAudioPool.data != nullptr)
            carla_shm_unmap(fShmAudioPool.shm, fShmAudioPool.data, fShmAudioPool.size);

        fShmAudioPool.size = (fInfo.aIns+fInfo.aOuts)*bufferSize*sizeof(float);

        if (fShmAudioPool.size > 0)
            fShmAudioPool.data = (float*)carla_shm_map(fShmAudioPool.shm, fShmAudioPool.size);
        else
            fShmAudioPool.data = nullptr;
    }

    void waitForServer()
    {
        sem_post(&fShmControl.data->runServer);

        timespec ts_timeout;
#ifdef CARLA_OS_WIN
        timeval now;
        gettimeofday(&now, nullptr);
        ts_timeout.tv_sec = now.tv_sec;
        ts_timeout.tv_nsec = now.tv_usec * 1000;
#else
        clock_gettime(CLOCK_REALTIME, &ts_timeout);
#endif
        ts_timeout.tv_sec += 5;

        if (sem_timedwait(&fShmControl.data->runClient, &ts_timeout) != 0)
            kData->active = false; // TODO
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BridgePlugin)
};

CARLA_BACKEND_END_NAMESPACE

#endif // ! BUILD_BRIDGE

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newBridge(const Initializer& init, BinaryType btype, PluginType ptype, const char* const bridgeBinary)
{
    carla_debug("CarlaPlugin::newBridge({%p, \"%s\", \"%s\", \"%s\"}, %s, %s, \"%s\")", init.engine, init.filename, init.name, init.label, BinaryType2Str(btype), PluginType2Str(ptype), bridgeBinary);

#ifndef BUILD_BRIDGE
    if (bridgeBinary == nullptr)
    {
        init.engine->setLastError("Bridge not possible, bridge-binary not found");
        return nullptr;
    }

    BridgePlugin* const plugin = new BridgePlugin(init.engine, init.id, btype, ptype);

    if (! plugin->init(init.filename, init.name, init.label, bridgeBinary))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

    return plugin;
#else
    init.engine->setLastError("Plugin bridge support not available");
    return nullptr;

    // unused
    (void)bridgeBinary;
#endif
}

#ifndef BUILD_BRIDGE
// -------------------------------------------------------------------
// Bridge Helper

int CarlaPluginSetOscBridgeInfo(CarlaPlugin* const plugin, const PluginBridgeInfoType type,
                                const int argc, const lo_arg* const* const argv, const char* const types)
{
    return ((BridgePlugin*)plugin)->setOscPluginBridgeInfo(type, argc, argv, types);
}
#endif

CARLA_BACKEND_END_NAMESPACE
