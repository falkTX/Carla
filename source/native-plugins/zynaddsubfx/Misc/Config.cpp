/*
  ZynAddSubFX - a software synthesizer

  Config.cpp - Configuration file functions
  Copyright (C) 2003-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>

#include "Config.h"
#include "globals.h"
#include "XMLwrapper.h"

#define rStdString(name, len, ...) \
    {STRINGIFY(name) "::s", rMap(length, len) DOC(__VA_ARGS__), NULL, rStringCb(name,len)}
#define rStdStringCb(name, length) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "s", obj->name); \
        } else { \
            strncpy(obj->name, rtosc_argument(msg, 0).s, length); \
            data.broadcast(loc, "s", obj->name);\
            rChangeCb \
        } rBOIL_END




#if 1
#define rObject Config
static const rtosc::Ports ports = {
    //rString(cfg.LinuxOSSWaveOutDev),
    //rString(cfg.LinuxOSSSeqInDev),
    rParamI(cfg.SampleRate, "samples of audio per second"),
    rParamI(cfg.SoundBufferSize, "Size of processed audio buffer"),
    rParamI(cfg.OscilSize, "Size Of Oscillator Wavetable"),
    rToggle(cfg.SwapStereo, "Swap Left And Right Channels"),
    rToggle(cfg.BankUIAutoClose, "Automatic Closing of BackUI After Patch Selection"),
    rParamI(cfg.GzipCompression, "Level of Gzip Compression For Save Files"),
    rParamI(cfg.Interpolation, "Level of Interpolation, Linear/Cubic"),
    {"cfg.presetsDirList", rProp(parameter) rDoc("list of preset search directories"), 0,
        [](const char *msg, rtosc::RtData &d)
        {
            Config &c = *(Config*)d.obj;
            if(rtosc_narguments(msg) != 0) {
                std::string args = rtosc_argument_string(msg);

                //clear everything
                c.clearpresetsdirlist();
                for(int i=0; i<(int)args.size(); ++i)
                    if(args[i] == 's')
                        c.cfg.presetsDirList[i] = rtosc_argument(msg, i).s;
            }

            char         types[MAX_BANK_ROOT_DIRS+1];
            rtosc_arg_t  args[MAX_BANK_ROOT_DIRS];
            size_t       pos    = 0;

            //zero out data
            memset(types, 0, sizeof(types));
            memset(args,  0, sizeof(args));

            for(int i=0; i<MAX_BANK_ROOT_DIRS; ++i) {
                if(!c.cfg.presetsDirList[i].empty()) {
                    types[pos] = 's';
                    args[pos].s  = c.cfg.presetsDirList[i].c_str();
                    pos++;
                }
            }
            char buffer[1024*5];
            rtosc_amessage(buffer, sizeof(buffer), d.loc, types, args);
            d.reply(buffer);
        }},
    {"cfg.bankRootDirList", rProp(parameter) rDoc("list of bank search directories"), 0,
        [](const char *msg, rtosc::RtData &d)
        {
            Config &c = *(Config*)d.obj;
            if(rtosc_narguments(msg) != 0) {
                std::string args = rtosc_argument_string(msg);

                //clear everything
                c.clearbankrootdirlist();
                for(int i=0; i<(int)args.size(); ++i)
                    if(args[i] == 's')
                        c.cfg.bankRootDirList[i] = rtosc_argument(msg, i).s;
            }

            char         types[MAX_BANK_ROOT_DIRS+1];
            rtosc_arg_t  args[MAX_BANK_ROOT_DIRS];
            size_t       pos    = 0;

            //zero out data
            memset(types, 0, sizeof(types));
            memset(args,  0, sizeof(args));

            for(int i=0; i<MAX_BANK_ROOT_DIRS; ++i) {
                if(!c.cfg.bankRootDirList[i].empty()) {
                    types[pos] = 's';
                    args[pos].s  = c.cfg.bankRootDirList[i].c_str();
                    pos++;
                }
            }
            char buffer[1024*5];
            rtosc_amessage(buffer, sizeof(buffer), d.loc, types, args);
            d.reply(buffer);
        }},

    //rArrayS(cfg.bankRootDirList,MAX_BANK_ROOT_DIRS),
    //rString(cfg.currentBankDir),
    //rArrayS(cfg.presetsDirList,MAX_BANK_ROOT_DIRS),
    rToggle(cfg.CheckPADsynth, "Old Check For PADsynth functionality within a patch"),
    rToggle(cfg.IgnoreProgramChange, "Ignore MIDI Program Change Events"),
    rParamI(cfg.UserInterfaceMode,   "Beginner/Advanced Mode Select"),
    rParamI(cfg.VirKeybLayout,       "Keyboard Layout For Virtual Piano Keyboard"),
    //rParamS(cfg.LinuxALSAaudioDev),
    //rParamS(cfg.nameTag)
    {"cfg.OscilPower::i", rDoc("Size Of Oscillator Wavetable"), 0,
        [](const char *msg, rtosc::RtData &d)
        {
            Config &c = *(Config*)d.obj;
            if(rtosc_narguments(msg) == 0) {
                d.reply(d.loc, "i", (int)(log(c.cfg.OscilSize*1.0)/log(2.0)));
                return;
            }
            float val = powf(2.0, rtosc_argument(msg, 0).i);
            c.cfg.OscilSize = val;
            d.broadcast(d.loc, "i", (int)(log(c.cfg.OscilSize*1.0)/log(2.0)));
        }},
};
const rtosc::Ports &Config::ports = ::ports;
#endif

Config::Config()
{}

void Config::init()
{
    maxstringsize = MAX_STRING_SIZE; //for ui
    //defaults
    cfg.SampleRate      = 44100;
    cfg.SoundBufferSize = 256;
    cfg.OscilSize  = 1024;
    cfg.SwapStereo = 0;

    cfg.oss_devs.linux_wave_out = new char[MAX_STRING_SIZE];
    snprintf(cfg.oss_devs.linux_wave_out, MAX_STRING_SIZE, "/dev/dsp");
    cfg.oss_devs.linux_seq_in = new char[MAX_STRING_SIZE];
    snprintf(cfg.oss_devs.linux_seq_in, MAX_STRING_SIZE, "/dev/sequencer");

    cfg.WindowsWaveOutId = 0;
    cfg.WindowsMidiInId  = 0;

    cfg.BankUIAutoClose = 0;

    cfg.GzipCompression = 3;

    cfg.Interpolation = 0;
    cfg.CheckPADsynth = 1;
    cfg.IgnoreProgramChange = 0;

    cfg.UserInterfaceMode = 0;
    cfg.VirKeybLayout     = 1;
    winwavemax = 1;
    winmidimax = 1;
    //try to find out how many input midi devices are there
    winmididevices = new winmidionedevice[winmidimax];
    for(int i = 0; i < winmidimax; ++i) {
        winmididevices[i].name = new char[MAX_STRING_SIZE];
        for(int j = 0; j < MAX_STRING_SIZE; ++j)
            winmididevices[i].name[j] = '\0';
    }


//get the midi input devices name
    cfg.currentBankDir = "./testbnk";

    char filename[MAX_STRING_SIZE];
    getConfigFileName(filename, MAX_STRING_SIZE);
    readConfig(filename);

    if(cfg.bankRootDirList[0].empty()) {
        //banks
        cfg.bankRootDirList[0] = "~/banks";
        cfg.bankRootDirList[1] = "./";
        cfg.bankRootDirList[2] = "/usr/share/zynaddsubfx/banks";
        cfg.bankRootDirList[3] = "/usr/local/share/zynaddsubfx/banks";
#ifdef __APPLE__
        cfg.bankRootDirList[4] = "../Resources/banks";
#else
        cfg.bankRootDirList[4] = "../banks";
#endif
        cfg.bankRootDirList[5] = "banks";
    }

    if(cfg.presetsDirList[0].empty()) {
        //presets
        cfg.presetsDirList[0] = "./";
#ifdef __APPLE__
        cfg.presetsDirList[1] = "../Resources/presets";
#else
        cfg.presetsDirList[1] = "../presets";
#endif
        cfg.presetsDirList[2] = "presets";
        cfg.presetsDirList[3] = "/usr/share/zynaddsubfx/presets";
        cfg.presetsDirList[4] = "/usr/local/share/zynaddsubfx/presets";
    }
    cfg.LinuxALSAaudioDev = "default";
    cfg.nameTag = "";
}

Config::~Config()
{
    delete [] cfg.oss_devs.linux_wave_out;
    delete [] cfg.oss_devs.linux_seq_in;

    for(int i = 0; i < winmidimax; ++i)
        delete [] winmididevices[i].name;
    delete [] winmididevices;
}


void Config::save() const
{
    char filename[MAX_STRING_SIZE];
    getConfigFileName(filename, MAX_STRING_SIZE);
    saveConfig(filename);
}

void Config::clearbankrootdirlist()
{
    for(int i = 0; i < MAX_BANK_ROOT_DIRS; ++i)
        cfg.bankRootDirList[i].clear();
}

void Config::clearpresetsdirlist()
{
    for(int i = 0; i < MAX_BANK_ROOT_DIRS; ++i)
        cfg.presetsDirList[i].clear();
}

void Config::readConfig(const char *filename)
{
    XMLwrapper xmlcfg;
    if(xmlcfg.loadXMLfile(filename) < 0)
        return;
    if(xmlcfg.enterbranch("CONFIGURATION")) {
        cfg.SampleRate = xmlcfg.getpar("sample_rate",
                                       cfg.SampleRate,
                                       4000,
                                       1024000);
        cfg.SoundBufferSize = xmlcfg.getpar("sound_buffer_size",
                                            cfg.SoundBufferSize,
                                            16,
                                            8192);
        cfg.OscilSize = xmlcfg.getpar("oscil_size",
                                      cfg.OscilSize,
                                      MAX_AD_HARMONICS * 2,
                                      131072);
        cfg.SwapStereo = xmlcfg.getpar("swap_stereo",
                                       cfg.SwapStereo,
                                       0,
                                       1);
        cfg.BankUIAutoClose = xmlcfg.getpar("bank_window_auto_close",
                                            cfg.BankUIAutoClose,
                                            0,
                                            1);

        cfg.GzipCompression = xmlcfg.getpar("gzip_compression",
                                            cfg.GzipCompression,
                                            0,
                                            9);

        cfg.currentBankDir = xmlcfg.getparstr("bank_current", "");
        cfg.Interpolation  = xmlcfg.getpar("interpolation",
                                           cfg.Interpolation,
                                           0,
                                           1);

        cfg.CheckPADsynth = xmlcfg.getpar("check_pad_synth",
                                          cfg.CheckPADsynth,
                                          0,
                                          1);

        cfg.IgnoreProgramChange = xmlcfg.getpar("ignore_program_change",
                                          cfg.IgnoreProgramChange,
                                          0,
                                          1);


        cfg.UserInterfaceMode = xmlcfg.getpar("user_interface_mode",
                                              cfg.UserInterfaceMode,
                                              0,
                                              2);
        cfg.VirKeybLayout = xmlcfg.getpar("virtual_keyboard_layout",
                                          cfg.VirKeybLayout,
                                          0,
                                          10);

        //get bankroot dirs
        for(int i = 0; i < MAX_BANK_ROOT_DIRS; ++i)
            if(xmlcfg.enterbranch("BANKROOT", i)) {
                cfg.bankRootDirList[i] = xmlcfg.getparstr("bank_root", "");
                xmlcfg.exitbranch();
            }

        //get preset root dirs
        for(int i = 0; i < MAX_BANK_ROOT_DIRS; ++i)
            if(xmlcfg.enterbranch("PRESETSROOT", i)) {
                cfg.presetsDirList[i] = xmlcfg.getparstr("presets_root", "");
                xmlcfg.exitbranch();
            }

        //linux stuff
        xmlcfg.getparstr("linux_oss_wave_out_dev",
                         cfg.oss_devs.linux_wave_out,
                         MAX_STRING_SIZE);
        xmlcfg.getparstr("linux_oss_seq_in_dev",
                         cfg.oss_devs.linux_seq_in,
                         MAX_STRING_SIZE);

        //windows stuff
        cfg.WindowsWaveOutId = xmlcfg.getpar("windows_wave_out_id",
                                             cfg.WindowsWaveOutId,
                                             0,
                                             winwavemax);
        cfg.WindowsMidiInId = xmlcfg.getpar("windows_midi_in_id",
                                            cfg.WindowsMidiInId,
                                            0,
                                            winmidimax);

        xmlcfg.exitbranch();
    }

    cfg.OscilSize = (int) powf(2, ceil(logf(cfg.OscilSize - 1.0f) / logf(2.0f)));
}

void Config::saveConfig(const char *filename) const
{
    XMLwrapper *xmlcfg = new XMLwrapper();

    xmlcfg->beginbranch("CONFIGURATION");

    xmlcfg->addpar("sample_rate", cfg.SampleRate);
    xmlcfg->addpar("sound_buffer_size", cfg.SoundBufferSize);
    xmlcfg->addpar("oscil_size", cfg.OscilSize);
    xmlcfg->addpar("swap_stereo", cfg.SwapStereo);
    xmlcfg->addpar("bank_window_auto_close", cfg.BankUIAutoClose);

    xmlcfg->addpar("gzip_compression", cfg.GzipCompression);

    xmlcfg->addpar("check_pad_synth", cfg.CheckPADsynth);
    xmlcfg->addpar("ignore_program_change", cfg.IgnoreProgramChange);

    xmlcfg->addparstr("bank_current", cfg.currentBankDir);

    xmlcfg->addpar("user_interface_mode", cfg.UserInterfaceMode);
    xmlcfg->addpar("virtual_keyboard_layout", cfg.VirKeybLayout);


    for(int i = 0; i < MAX_BANK_ROOT_DIRS; ++i)
        if(!cfg.bankRootDirList[i].empty()) {
            xmlcfg->beginbranch("BANKROOT", i);
            xmlcfg->addparstr("bank_root", cfg.bankRootDirList[i]);
            xmlcfg->endbranch();
        }

    for(int i = 0; i < MAX_BANK_ROOT_DIRS; ++i)
        if(!cfg.presetsDirList[i].empty()) {
            xmlcfg->beginbranch("PRESETSROOT", i);
            xmlcfg->addparstr("presets_root", cfg.presetsDirList[i]);
            xmlcfg->endbranch();
        }

    xmlcfg->addpar("interpolation", cfg.Interpolation);

    //linux stuff
    xmlcfg->addparstr("linux_oss_wave_out_dev", cfg.oss_devs.linux_wave_out);
    xmlcfg->addparstr("linux_oss_seq_in_dev", cfg.oss_devs.linux_seq_in);

    //windows stuff
    xmlcfg->addpar("windows_wave_out_id", cfg.WindowsWaveOutId);
    xmlcfg->addpar("windows_midi_in_id", cfg.WindowsMidiInId);

    xmlcfg->endbranch();

    // for some reason (which one?), the gzip compression is bashed to 0
    xmlcfg->saveXMLfile(filename, 0);

    delete (xmlcfg);
}

void Config::getConfigFileName(char *name, int namesize) const
{
    name[0] = 0;
    snprintf(name, namesize, "%s%s", getenv("HOME"), "/.zynaddsubfxXML.cfg");
}
