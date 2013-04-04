/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNative.h"

#include "audio_decoder/ad.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PROGRAM_COUNT 16

#ifdef _WIN32
# define OS_SEP '\\'
#else
# define OS_SEP '/'
#endif

typedef struct adinfo   ADInfo;
typedef pthread_mutex_t Mutex;
typedef pthread_t       Thread;

typedef struct _AudioFilePool {
    float*   buffer[2];
    uint32_t startFrame;
    uint32_t size;

} AudioFilePool;

typedef struct _AudioFilePrograms {
    uint32_t    current;
    const char* fullNames[PROGRAM_COUNT];
    const char* shortNames[PROGRAM_COUNT];
} AudioFilePrograms;

typedef struct _AudioFileInstance {
    HostDescriptor* host;

    void*  filePtr;
    ADInfo fileNfo;

    uint32_t lastFrame;
    uint32_t maxFrame;
    AudioFilePool pool;
    AudioFilePrograms programs;

    bool loopMode;
    bool needsRead;
    bool doProcess;
    bool doQuit;

    Mutex  mutex;
    Thread thread;

} AudioFileInstance;

// ------------------------------------------------------------------------------------------

void zeroFloat(float* data, unsigned size)
{
    for (unsigned i=0; i < size; ++i)
        *data++ = 0.0f;
}

void audiofile_read_poll(AudioFileInstance* const handlePtr)
{
    if (handlePtr == NULL)
    {
        fprintf(stderr, "R: invalid instance\n");
        return;
    }

    if (handlePtr->fileNfo.frames == 0 || handlePtr->maxFrame == 0)
    {
        //fprintf(stderr, "R: no song loaded\n");
        handlePtr->needsRead = false;
        return;
    }

    int64_t lastFrame = handlePtr->lastFrame;
    int64_t readFrame = lastFrame;

    if (lastFrame >= handlePtr->maxFrame)
    {
        if (handlePtr->loopMode)
        {
            //fprintf(stderr, "R: DEBUG read loop, lastFrame:%i, maxFrame:%i\n", handlePtr->lastFrame, handlePtr->maxFrame);

            if (handlePtr->maxFrame >= handlePtr->pool.size)
            {
                readFrame %= handlePtr->maxFrame;
            }
            else
            {
                readFrame  = 0;
                lastFrame -= lastFrame % handlePtr->maxFrame;
            }
        }
        else
        {
            //fprintf(stderr, "R: transport out of bounds\n");
            handlePtr->needsRead = false;
            return;
        }
    }

    // temp data buffer
    const uint32_t tmpSize = handlePtr->pool.size * handlePtr->fileNfo.channels;

    float tmpData[tmpSize];
    zeroFloat(tmpData, tmpSize);

    {
        fprintf(stderr, "R: poll data - reading at %li:%02li\n", readFrame/44100/60, (readFrame/44100) % 60);

        ad_seek(handlePtr->filePtr, readFrame);
        ssize_t i, j, rv = ad_read(handlePtr->filePtr, tmpData, tmpSize);
        i = j = 0;

        // lock, and put data asap
        pthread_mutex_lock(&handlePtr->mutex);

        for (; i < handlePtr->pool.size && j < rv; j++)
        {
            if (handlePtr->fileNfo.channels == 1)
            {
                handlePtr->pool.buffer[0][i] = tmpData[j];
                handlePtr->pool.buffer[1][i] = tmpData[j];
                i++;
            }
            else
            {
                if (j % 2 == 0)
                {
                    handlePtr->pool.buffer[0][i] = tmpData[j];
                }
                else
                {
                    handlePtr->pool.buffer[1][i] = tmpData[j];
                    i++;
                }
            }
        }

        if (handlePtr->loopMode && i < handlePtr->pool.size)
        {
            while (i < handlePtr->pool.size)
            {
                for (j=0; i < handlePtr->pool.size && j < rv; j++)
                {
                    if (handlePtr->fileNfo.channels == 1)
                    {
                        handlePtr->pool.buffer[0][i] = tmpData[j];
                        handlePtr->pool.buffer[1][i] = tmpData[j];
                        i++;
                    }
                    else
                    {
                        if (j % 2 == 0)
                        {
                            handlePtr->pool.buffer[0][i] = tmpData[j];
                        }
                        else
                        {
                            handlePtr->pool.buffer[1][i] = tmpData[j];
                            i++;
                        }
                    }
                }
            }
        }
        else
        {
            for (; i < handlePtr->pool.size; i++)
            {
                handlePtr->pool.buffer[0][i] = 0.0f;
                handlePtr->pool.buffer[1][i] = 0.0f;
            }
        }

        handlePtr->pool.startFrame = lastFrame;

        // done
        pthread_mutex_unlock(&handlePtr->mutex);
    }

    handlePtr->needsRead = false;
}

static void audiofile_thread_idle(void* ptr)
{
    AudioFileInstance* const handlePtr = (AudioFileInstance*)ptr;

    while (! handlePtr->doQuit)
    {
        if (handlePtr->needsRead || handlePtr->lastFrame - handlePtr->pool.startFrame >= handlePtr->pool.size*3/4)
            audiofile_read_poll(handlePtr);
        else
            usleep(50*1000);
    }

    pthread_exit(NULL);
}

void audiofile_load_filename(AudioFileInstance* const handlePtr, const char* const filename)
{
    // wait for jack processing to end
    handlePtr->doProcess = false;
    pthread_mutex_lock(&handlePtr->mutex);

    handlePtr->maxFrame = 0;
    handlePtr->pool.startFrame = 0;

    // stop thread
    if (! handlePtr->doQuit)
    {
        handlePtr->doQuit = true;
        pthread_join(handlePtr->thread, NULL);
    }

    pthread_mutex_unlock(&handlePtr->mutex);

    // clear old data
    if (handlePtr->filePtr != NULL)
    {
        ad_close(handlePtr->filePtr);
        handlePtr->filePtr = NULL;
    }
    ad_clear_nfo(&handlePtr->fileNfo);

    if (filename == NULL)
        return;

    // open new
    handlePtr->filePtr = ad_open(filename, &handlePtr->fileNfo);

    if (handlePtr->filePtr != NULL)
    {
        ad_dump_nfo(99, &handlePtr->fileNfo);

        if (handlePtr->fileNfo.frames == 0)
            fprintf(stderr, "L: filename \"%s\" has 0 frames\n", filename);

        if ((handlePtr->fileNfo.channels == 1 || handlePtr->fileNfo.channels == 2) && handlePtr->fileNfo.frames > 0)
        {
            handlePtr->maxFrame = handlePtr->fileNfo.frames;
            audiofile_read_poll(handlePtr);
            handlePtr->doProcess = true;

            // start thread
            handlePtr->doQuit = false;
            pthread_create(&handlePtr->thread, NULL, (void*)&audiofile_thread_idle, handlePtr);
        }
        else
        {
            ad_close(handlePtr->filePtr);
            handlePtr->filePtr = NULL;
            ad_clear_nfo(&handlePtr->fileNfo);
        }
    }
}

// ------------------------------------------------------------------------------------------

static bool gADInitiated = false;

// ------------------------------------------------------------------------------------------

static PluginHandle audiofile_instantiate(const PluginDescriptor* _this_, HostDescriptor* host)
{
    AudioFileInstance* const handlePtr = (AudioFileInstance*)malloc(sizeof(AudioFileInstance));

    if (handlePtr == NULL)
        return NULL;

    if (! gADInitiated)
    {
        ad_init();
        gADInitiated = true;
    }

    // init
    handlePtr->host = host;
    handlePtr->filePtr   = NULL;
    handlePtr->lastFrame = 0;
    handlePtr->maxFrame  = 0;
    handlePtr->pool.buffer[0]  = NULL;
    handlePtr->pool.buffer[1]  = NULL;
    handlePtr->pool.startFrame = 0;
    handlePtr->pool.size = 0;
    handlePtr->programs.current = 0;

    handlePtr->loopMode  = true;
    handlePtr->needsRead = false;
    handlePtr->doProcess = false;
    handlePtr->doQuit    = true;

    ad_clear_nfo(&handlePtr->fileNfo);
    pthread_mutex_init(&handlePtr->mutex, NULL);

    for (uint32_t i=0; i < PROGRAM_COUNT; i++)
    {
        handlePtr->programs.fullNames[i]  = NULL;
        handlePtr->programs.shortNames[i] = NULL;
    }

    // create audio pool
    handlePtr->pool.size = host->get_sample_rate(host->handle) * 6; // 6 secs

    handlePtr->pool.buffer[0] = (float*)malloc(sizeof(float) * handlePtr->pool.size);
    if (handlePtr->pool.buffer[0] == NULL)
    {
        free(handlePtr);
        return NULL;
    }

    handlePtr->pool.buffer[1] = (float*)malloc(sizeof(float) * handlePtr->pool.size);
    if (handlePtr->pool.buffer[1] == NULL)
    {
        free(handlePtr->pool.buffer[0]);
        free(handlePtr);
        return NULL;
    }

    zeroFloat(handlePtr->pool.buffer[0], handlePtr->pool.size);
    zeroFloat(handlePtr->pool.buffer[1], handlePtr->pool.size);

    return handlePtr;

    // unused
    (void)_this_;
}

static void audiofile_cleanup(PluginHandle handle)
{
    AudioFileInstance* const handlePtr = (AudioFileInstance*)handle;

    // wait for processing to end
    handlePtr->doProcess = false;
    pthread_mutex_lock(&handlePtr->mutex);

    // stop thread
    if (! handlePtr->doQuit)
    {
        handlePtr->doQuit = true;
        pthread_join(handlePtr->thread, NULL);
    }

    pthread_mutex_unlock(&handlePtr->mutex);
    pthread_mutex_destroy(&handlePtr->mutex);

    if (handlePtr->filePtr != NULL)
        ad_close(handlePtr->filePtr);

    if (handlePtr->pool.buffer[0] != NULL)
        free(handlePtr->pool.buffer[0]);

    if (handlePtr->pool.buffer[1] != NULL)
        free(handlePtr->pool.buffer[1]);

    for (uint32_t i=0; i < PROGRAM_COUNT; i++)
    {
        if (handlePtr->programs.fullNames[i] != NULL)
            free((void*)handlePtr->programs.fullNames[i]);
        if (handlePtr->programs.shortNames[i] != NULL)
            free((void*)handlePtr->programs.shortNames[i]);
    }

    free(handlePtr);
}

static uint32_t audiofile_get_parameter_count(PluginHandle handle)
{
    return 1;

    // unused
    (void)handle;
}

static const Parameter* audiofile_get_parameter_info(PluginHandle handle, uint32_t index)
{
    if (index != 0)
        return NULL;

    static Parameter param;

    param.name  = "Loop Mode";
    param.unit  = NULL;
    param.hints = PARAMETER_IS_ENABLED|PARAMETER_IS_BOOLEAN;
    param.ranges.def = 1.0f;
    param.ranges.min = 0.0f;
    param.ranges.max = 1.0f;
    param.ranges.step = 1.0f;
    param.ranges.stepSmall = 1.0f;
    param.ranges.stepLarge = 1.0f;
    param.scalePointCount = 0;
    param.scalePoints     = NULL;

    return &param;

    // unused
    (void)handle;
}

static float audiofile_get_parameter_value(PluginHandle handle, uint32_t index)
{
    AudioFileInstance* const handlePtr = (AudioFileInstance*)handle;

    if (index != 0)
        return 0.0f;

    return handlePtr->loopMode ? 1.0f : 0.0f;

    // unused
    (void)handle;
}

static uint32_t audiofile_get_program_count(PluginHandle handle)
{
    return PROGRAM_COUNT;

    // unused
    (void)handle;
}

const MidiProgram* audiofile_get_program_info(PluginHandle handle, uint32_t index)
{
    AudioFileInstance* const handlePtr = (AudioFileInstance*)handle;

    if (index >= PROGRAM_COUNT)
        return NULL;

    static MidiProgram midiProgram;

    midiProgram.bank    = 0;
    midiProgram.program = index;
    midiProgram.name    = handlePtr->programs.shortNames[index];

    if (midiProgram.name == NULL)
        midiProgram.name = "";

    return &midiProgram;
}

static void audiofile_set_parameter_value(PluginHandle handle, uint32_t index, float value)
{
    AudioFileInstance* const handlePtr = (AudioFileInstance*)handle;

    if (index != 0)
        return;

    handlePtr->loopMode  = (value > 0.5f);
    handlePtr->needsRead = true;
}

static void audiofile_set_program(PluginHandle handle, uint32_t bank, uint32_t program)
{
    AudioFileInstance* const handlePtr = (AudioFileInstance*)handle;

    if (bank != 0 || program >= PROGRAM_COUNT)
        return;

    if (handlePtr->programs.current != program)
    {
        audiofile_load_filename(handlePtr, handlePtr->programs.fullNames[program]);
        handlePtr->programs.current = program;
    }
}

static void audiofile_set_custom_data(PluginHandle handle, const char* key, const char* value)
{
    AudioFileInstance* const handlePtr = (AudioFileInstance*)handle;

    if (strncmp(key, "file", 4) != 0)
        return;
    if (key[4] < '0' || key[4] > '9')
        return;
    if (key[5] < '0' || key[5] > '9')
        return;

    uint8_t tens = key[4]-'0';
    uint8_t nums = key[5]-'0';

    uint32_t program = tens*10 + nums;

    if (program >= PROGRAM_COUNT)
        return;

    if (handlePtr->programs.fullNames[program] != NULL)
        free((void*)handlePtr->programs.fullNames[program]);
    if (handlePtr->programs.shortNames[program] != NULL)
        free((void*)handlePtr->programs.shortNames[program]);

    handlePtr->programs.fullNames[program] = strdup(value);

    {
        const char* shortName1 = strrchr(value, OS_SEP)+1;
        //const char* shortName2 = strchr(shortName1, '.');

        handlePtr->programs.shortNames[program] = strdup(shortName1);
    }

    if (handlePtr->programs.current == program)
        audiofile_load_filename(handlePtr, value);
}

static void audiofile_ui_show(PluginHandle handle, bool show)
{
    AudioFileInstance* const handlePtr = (AudioFileInstance*)handle;

    if (! show)
        return;

    const char* const filename = handlePtr->host->ui_open_file(handlePtr->host->handle, false, "Open Audio File", "");

    if (filename != NULL)
    {
        char fileStr[4+2+1] = { 'f', 'i', 'l', 'e', 0, 0, 0 };
        fileStr[4] = '0' + (handlePtr->programs.current / 10);
        fileStr[5] = '0' + (handlePtr->programs.current % 10);

        handlePtr->host->ui_custom_data_changed(handlePtr->host->handle, fileStr, filename);
    }

    handlePtr->host->ui_closed(handlePtr->host->handle);
}

static void audiofile_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents)
{
    AudioFileInstance* const handlePtr = (AudioFileInstance*)handle;

    const TimeInfo* const timePos = handlePtr->host->get_time_info(handlePtr->host->handle);

    float* out1 = outBuffer[0];
    float* out2 = outBuffer[1];

    if (! handlePtr->doProcess)
    {
        //fprintf(stderr, "P: no process\n");
        handlePtr->lastFrame = timePos->frame;

        zeroFloat(out1, frames);
        zeroFloat(out2, frames);
        return;
    }

    // not playing
    if (! timePos->playing)
    {
        //fprintf(stderr, "P: not rolling\n");

        if (timePos->frame == 0 && handlePtr->lastFrame > 0)
            handlePtr->needsRead = true;

        handlePtr->lastFrame = timePos->frame;

        zeroFloat(out1, frames);
        zeroFloat(out2, frames);
        return;
    }

    pthread_mutex_lock(&handlePtr->mutex);

    // out of reach
    if (timePos->frame + frames < handlePtr->pool.startFrame || (timePos->frame >= handlePtr->maxFrame && ! handlePtr->loopMode))
    {
        //fprintf(stderr, "P: non-continuous playback, out of reach %u vs %u\n", timePos->frame + frames, handlePtr->maxFrame);
        handlePtr->lastFrame = timePos->frame;
        handlePtr->needsRead = true;
        pthread_mutex_unlock(&handlePtr->mutex);

        zeroFloat(out1, frames);
        zeroFloat(out2, frames);
        return;
    }

    int64_t poolFrame = (int64_t)timePos->frame - handlePtr->pool.startFrame;
    int64_t poolSize  = handlePtr->pool.size;

    for (uint32_t i=0; i < frames; i++, poolFrame++)
    {
        if (poolFrame >= 0 && poolFrame < poolSize)
        {
            out1[i] = handlePtr->pool.buffer[0][poolFrame];
            out2[i] = handlePtr->pool.buffer[1][poolFrame];

            // reset
            handlePtr->pool.buffer[0][poolFrame] = 0.0f;
            handlePtr->pool.buffer[1][poolFrame] = 0.0f;
        }
        else
        {
            out1[i] = 0.0f;
            out2[i] = 0.0f;
        }
    }

    handlePtr->lastFrame = timePos->frame;
    pthread_mutex_unlock(&handlePtr->mutex);

    return;

    // unused
    (void)inBuffer;
    (void)midiEventCount;
    (void)midiEvents;
}

// -----------------------------------------------------------------------

static const PluginDescriptor audiofileDesc = {
    .category  = PLUGIN_CATEGORY_UTILITY,
    .hints     = PLUGIN_IS_RTSAFE|PLUGIN_HAS_GUI,
    .audioIns  = 0,
    .audioOuts = 2,
    .midiIns   = 0,
    .midiOuts  = 0,
    .parameterIns  = 1,
    .parameterOuts = 0,
    .name      = "Audio File",
    .label     = "audiofile",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = audiofile_instantiate,
    .cleanup     = audiofile_cleanup,

    .get_parameter_count = audiofile_get_parameter_count,
    .get_parameter_info  = audiofile_get_parameter_info,
    .get_parameter_value = audiofile_get_parameter_value,
    .get_parameter_text  = NULL,

    .get_midi_program_count = audiofile_get_program_count,
    .get_midi_program_info  = audiofile_get_program_info,

    .set_parameter_value = audiofile_set_parameter_value,
    .set_midi_program    = audiofile_set_program,
    .set_custom_data     = audiofile_set_custom_data,

    .ui_show = audiofile_ui_show,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = audiofile_process,

    .get_state = NULL,
    .set_state = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_audiofile()
{
    carla_register_native_plugin(&audiofileDesc);
}

// -----------------------------------------------------------------------
// amalgamated build

#include "audio_decoder/ad_ffmpeg.c"
#include "audio_decoder/ad_plugin.c"
#include "audio_decoder/ad_soundfile.c"

// -----------------------------------------------------------------------
