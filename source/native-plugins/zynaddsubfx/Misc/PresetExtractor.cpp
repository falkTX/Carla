/**
 * Extract Presets from realtime data
 */

#include "../Params/PresetsStore.h"

#include "../Misc/Master.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Effects/EffectMgr.h"
#include "../Synth/OscilGen.h"
#include "../Synth/Resonance.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/EnvelopeParams.h"
#include "../Params/FilterParams.h"
#include "../Params/LFOParams.h"
#include "../Params/PADnoteParameters.h"
#include "../Params/Presets.h"
#include "../Params/PresetsArray.h"
#include "../Params/SUBnoteParameters.h"
#include "../Misc/MiddleWare.h"
#include "PresetExtractor.h"
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include <string>
using std::string;

static void dummy(const char *, rtosc::RtData&) {}

const rtosc::Ports real_preset_ports =
{
    {"scan-for-presets:", 0, 0,
        [](const char *msg, rtosc::RtData &d) {
            MiddleWare &mw = *(MiddleWare*)d.obj;
            mw.getPresetsStore().scanforpresets();
            auto &pre = mw.getPresetsStore().presets;
            d.reply(d.loc, "i", pre.size());
            for(unsigned i=0; i<pre.size();++i)
                d.reply(d.loc, "isss", i,
                        pre[i].file.c_str(),
                        pre[i].name.c_str(),
                        pre[i].type.c_str());

        }},
    {"copy:s:ss:si:ssi", 0, 0,
        [](const char *msg, rtosc::RtData &d) {
            MiddleWare &mw = *(MiddleWare*)d.obj;
            std::string args = rtosc_argument_string(msg);
            d.reply(d.loc, "s", "clipboard copy...");
            printf("\nClipboard Copy...\n");
            if(args == "s")
                presetCopy(mw, rtosc_argument(msg, 0).s, "");
            else if(args == "ss")
                presetCopy(mw, rtosc_argument(msg, 0).s,
                            rtosc_argument(msg, 1).s);
            else if(args == "si")
                presetCopyArray(mw, rtosc_argument(msg, 0).s,
                            rtosc_argument(msg, 1).i, "");
            else if(args == "ssi")
                presetCopyArray(mw, rtosc_argument(msg, 0).s,
                            rtosc_argument(msg, 2).i, rtosc_argument(msg, 1).s);
            else
                assert(false && "bad arguments");
        }},
    {"paste:s:ss:si:ssi", 0, 0,
        [](const char *msg, rtosc::RtData &d) {
            MiddleWare &mw = *(MiddleWare*)d.obj;
            std::string args = rtosc_argument_string(msg);
            d.reply(d.loc, "s", "clipboard paste...");
            printf("\nClipboard Paste...\n");
            if(args == "s")
                presetPaste(mw, rtosc_argument(msg, 0).s, "");
            else if(args == "ss")
                presetPaste(mw, rtosc_argument(msg, 0).s,
                            rtosc_argument(msg, 1).s);
            else if(args == "si")
                presetPasteArray(mw, rtosc_argument(msg, 0).s,
                            rtosc_argument(msg, 1).i, "");
            else if(args == "ssi")
                presetPasteArray(mw, rtosc_argument(msg, 0).s,
                            rtosc_argument(msg, 2).i, rtosc_argument(msg, 1).s);
            else
                assert(false && "bad arguments");
        }},
    {"clipboard-type:", 0, 0,
        [](const char *msg, rtosc::RtData &d) {
            const MiddleWare &mw = *(MiddleWare*)d.obj;
            d.reply(d.loc, "s", mw.getPresetsStore().clipboard.type.c_str());
        }},
    {"delete:s", 0, 0,
        [](const char *msg, rtosc::RtData &d) {
            MiddleWare &mw = *(MiddleWare*)d.obj;
            mw.getPresetsStore().deletepreset(rtosc_argument(msg,0).s);
        }},

};


const rtosc::Ports preset_ports
{
    {"scan-for-presets:", rDoc("Scan For Presets"), 0, dummy},
    {"copy:s:ss:si:ssi",  rDoc("Copy <s> URL to <s> Name/Clipboard from subfield <i>"), 0, dummy},
    {"paste:s:ss:si:ssi", rDoc("Paste <s> URL to <s> File-Name/Clipboard from subfield <i>"), 0, dummy},
    {"clipboard-type:",   rDoc("Type Stored In Clipboard"), 0, dummy},
    {"delete:s", rDoc("Delete the given preset file"), 0, dummy},
};

//Relevant types to keep in mind
//Effects/EffectMgr.cpp:    setpresettype("Peffect");
//Params/ADnoteParameters.cpp:    setpresettype("Padsynth");
//Params/EnvelopeParams.cpp:    //setpresettype("Penvamplitude");
//Params/EnvelopeParams.cpp:    //setpresettype("Penvamplitude");
//Params/EnvelopeParams.cpp:    //setpresettype("Penvfrequency");
//Params/EnvelopeParams.cpp:    //setpresettype("Penvfilter");
//Params/EnvelopeParams.cpp:    //setpresettype("Penvbandwidth");
//Params/FilterParams.cpp:    //setpresettype("Pfilter");
//Params/LFOParams.cpp:    //        setpresettype("Plfofrequency");
//Params/LFOParams.cpp:    //        setpresettype("Plfoamplitude");
//Params/LFOParams.cpp:    //        setpresettype("Plfofilter");
//Params/PADnoteParameters.cpp:    setpresettype("Ppadsynth");
//Params/SUBnoteParameters.cpp:    setpresettype("Psubsynth");
//Synth/OscilGen.cpp:    setpresettype("Poscilgen");
//Synth/Resonance.cpp:    setpresettype("Presonance");


//Translate newer symbols to old preset types
std::vector<string> translate_preset_types(std::string metatype)
{
    std::vector<string> results;
    return results;
}


/*****************************************************************************
 *                     Implementation Methods                                *
 *****************************************************************************/
class Capture:public rtosc::RtData
{
    public:
        Capture(void *obj_)
        {
            matches = 0;
            memset(locbuf, 0, sizeof(locbuf));
            loc      = locbuf;
            loc_size = sizeof(locbuf);
            obj      = obj_;
        }

        virtual void reply(const char *path, const char *args, ...)
        {
            printf("reply(%p)(%s)(%s)...\n", msgbuf, path, args);
            //printf("size is %d\n", sizeof(msgbuf));
            va_list va;
            va_start(va,args);
            char *buffer = msgbuf;
            rtosc_vmessage(buffer,sizeof(msgbuf),path,args,va);
            va_end(va);
        }
        char msgbuf[1024];
        char locbuf[1024];
};

template <class T>
T capture(Master *m, std::string url);

template <>
std::string capture(Master *m, std::string url)
{
    Capture c(m);
    char query[1024];
    rtosc_message(query, 1024, url.c_str(), "");
    Master::ports.dispatch(query+1,c);
    if(rtosc_message_length(c.msgbuf, sizeof(c.msgbuf))) {
        if(rtosc_type(c.msgbuf, 0) == 's')
            return rtosc_argument(c.msgbuf,0).s;
    }

    return "";
}

template <>
void *capture(Master *m, std::string url)
{
    Capture c(m);
    char query[1024];
    rtosc_message(query, 1024, url.c_str(), "");
    Master::ports.dispatch(query+1,c);
    if(rtosc_message_length(c.msgbuf, sizeof(c.msgbuf))) {
        if(rtosc_type(c.msgbuf, 0) == 'b' &&
                rtosc_argument(c.msgbuf, 0).b.len == sizeof(void*))
            return *(void**)rtosc_argument(c.msgbuf,0).b.data;
    }

    return NULL;
}

template<class T>
std::string doCopy(MiddleWare &mw, string url, string name)
{
    XMLwrapper xml;
    mw.doReadOnlyOp([&xml, url, name, &mw](){
        Master *m = mw.spawnMaster();
        //Get the pointer
        T *t = (T*)capture<void*>(m, url+"self");
        //Extract Via mxml
        //t->add2XML(&xml);
        t->copy(mw.getPresetsStore(), name.empty()? NULL:name.c_str());
    });

    return "";//xml.getXMLdata();
}

template<class T, typename... Ts>
void doPaste(MiddleWare &mw, string url, string type, XMLwrapper &xml, Ts&&... args)
{
    //Generate a new object
    T *t = new T(std::forward<Ts>(args)...);
    
    if(xml.enterbranch(type) == 0)
        return;

    t->getfromXML(&xml);

    //Send the pointer
    string path = url+"paste";
    char buffer[1024];
    rtosc_message(buffer, 1024, path.c_str(), "b", sizeof(void*), &t);
    if(!Master::ports.apropos(path.c_str()))
        fprintf(stderr, "Warning: Missing Paste URL: '%s'\n", path.c_str());
    printf("Sending info to '%s'\n", buffer);
    mw.transmitMsg(buffer);

    //Let the pointer be reclaimed later
}

template<class T>
std::string doArrayCopy(MiddleWare &mw, int field, string url, string name)
{
    XMLwrapper xml;
    printf("Getting info from '%s'<%d>\n", url.c_str(), field);
    mw.doReadOnlyOp([&xml, url, field, name, &mw](){
        Master *m = mw.spawnMaster();
        //Get the pointer
        T *t = (T*)capture<void*>(m, url+"self");
        //Extract Via mxml
        t->copy(mw.getPresetsStore(), field, name.empty()?NULL:name.c_str());
    });

    return "";//xml.getXMLdata();
}

template<class T, typename... Ts>
void doArrayPaste(MiddleWare &mw, int field, string url, string type,
        XMLwrapper &xml, Ts&&... args)
{
    //Generate a new object
    T *t = new T(std::forward<Ts>(args)...);

    if(xml.enterbranch(type+"n") == 0) {
        delete t;
        return;
    }
    t->defaults(field);
    t->getfromXMLsection(&xml, field);
    xml.exitbranch();

    //Send the pointer
    string path = url+"paste-array";
    char buffer[1024];
    rtosc_message(buffer, 1024, path.c_str(), "bi", sizeof(void*), &t, field);
    if(!Master::ports.apropos(path.c_str()))
        fprintf(stderr, "Warning: Missing Paste URL: '%s'\n", path.c_str());
    printf("Sending info to '%s'<%d>\n", buffer, field);
    mw.transmitMsg(buffer);

    //Let the pointer be reclaimed later
}

/*
 * Dispatch to class specific operators
 *
 * Oscilgen and PADnoteParameters have mixed RT/non-RT parameters and require
 * extra handling.
 * See MiddleWare.cpp for these specifics
 */
void doClassPaste(std::string type, std::string type_, MiddleWare &mw, string url, XMLwrapper &data)
{
    printf("Class Paste\n");
    if(type == "EnvelopeParams")
        doPaste<EnvelopeParams>(mw, url, type_, data);
    else if(type == "LFOParams")
        doPaste<LFOParams>(mw, url, type_, data);
    else if(type == "FilterParams")
        doPaste<FilterParams>(mw, url, type_, data);
    else if(type == "ADnoteParameters")
        doPaste<ADnoteParameters>(mw, url, type_, data, mw.getSynth(), (FFTwrapper*)NULL);
    else if(type == "PADnoteParameters")
        doPaste<PADnoteParameters>(mw, url, type_, data, mw.getSynth(), (FFTwrapper*)NULL);
    else if(type == "SUBnoteParameters")
        doPaste<SUBnoteParameters>(mw, url, type_, data);
    else if(type == "OscilGen")
        doPaste<OscilGen>(mw, url, type_, data, mw.getSynth(), (FFTwrapper*)NULL, (Resonance*)NULL);
    else if(type == "Resonance")
        doPaste<Resonance>(mw, url, type_, data);
    else if(type == "EffectMgr")
        doPaste<EffectMgr>(mw, url, type_, data, DummyAlloc, mw.getSynth(), false);
    else {
        fprintf(stderr, "Warning: Unknown type<%s> from url<%s>\n", type.c_str(), url.c_str());
    }
}

std::string doClassCopy(std::string type, MiddleWare &mw, string url, string name)
{
    if(type == "EnvelopeParams")
        return doCopy<EnvelopeParams>(mw, url, name);
    else if(type == "LFOParams")
        return doCopy<LFOParams>(mw, url, name);
    else if(type == "FilterParams")
        return doCopy<FilterParams>(mw, url, name);
    else if(type == "ADnoteParameters")
        return doCopy<ADnoteParameters>(mw, url, name);
    else if(type == "PADnoteParameters")
        return doCopy<PADnoteParameters>(mw, url, name);
    else if(type == "SUBnoteParameters")
        return doCopy<SUBnoteParameters>(mw, url, name);
    else if(type == "OscilGen")
        return doCopy<OscilGen>(mw, url, name);
    else if(type == "Resonance")
        return doCopy<Resonance>(mw, url, name);
    else if(type == "EffectMgr")
        doCopy<EffectMgr>(mw, url, name);
    return "UNDEF";
}

void doClassArrayPaste(std::string type, std::string type_, int field, MiddleWare &mw, string url,
        XMLwrapper &data)
{
    if(type == "FilterParams")
        doArrayPaste<FilterParams>(mw, field, url, type_, data);
    else if(type == "ADnoteParameters")
        doArrayPaste<ADnoteParameters>(mw, field, url, type_, data, mw.getSynth(), (FFTwrapper*)NULL);
}

std::string doClassArrayCopy(std::string type, int field, MiddleWare &mw, string url, string name)
{
    if(type == "FilterParams")
        return doArrayCopy<FilterParams>(mw, field, url, name);
    else if(type == "ADnoteParameters")
        return doArrayCopy<ADnoteParameters>(mw, field, url, name);
    return "UNDEF";
}

//This is an abuse of the readonly op, but one that might look reasonable from a
//user perspective...
std::string getUrlPresetType(std::string url, MiddleWare &mw)
{
    std::string result;
    mw.doReadOnlyOp([url, &result, &mw](){
        Master *m = mw.spawnMaster();
        //Get the pointer
        result = capture<std::string>(m, url+"preset-type");
    });
    printf("preset type = %s\n", result.c_str());
    return result;
}

std::string getUrlType(std::string url)
{
    assert(!url.empty());
    printf("Searching for '%s'\n", (url+"self").c_str());
    auto self = Master::ports.apropos((url+"self").c_str());
    if(!self)
        fprintf(stderr, "Warning: URL Metadata Not Found For '%s'\n", url.c_str());

    if(self)
        return self->meta()["class"];
    else
        return "";
}


/*****************************************************************************
 *                            API Stubs                                      *
 *****************************************************************************/

#if 0
Clipboard clipboardCopy(MiddleWare &mw, string url)
{
    //Identify The Self Type of the Object
    string type = getUrlType(url);
    printf("Copying a '%s' object", type.c_str());

    //Copy The Object
    string data = doClassCopy(type, mw, url);
    printf("Object Information '%s'\n", data.c_str());

    return {type, data};
}

void clipBoardPaste(const char *url, Clipboard clip)
{
    (void) url;
    (void) clip;
}
#endif

void presetCopy(MiddleWare &mw, std::string url, std::string name)
{
    (void) name;
    doClassCopy(getUrlType(url), mw, url, name);
    printf("PresetCopy()\n");
}
void presetPaste(MiddleWare &mw, std::string url, std::string name)
{
    (void) name;
    printf("PresetPaste()\n");
    string data = "";
    XMLwrapper xml;
    if(name.empty()) {
        data = mw.getPresetsStore().clipboard.data;
        if(data.length() < 20)
            return;
        if(!xml.putXMLdata(data.c_str()))
            return;
    } else {
        if(xml.loadXMLfile(name))
            return;
    }

    doClassPaste(getUrlType(url), getUrlPresetType(url, mw), mw, url, xml);
}
void presetCopyArray(MiddleWare &mw, std::string url, int field, std::string name)
{
    (void) name;
    printf("PresetArrayCopy()\n");
    doClassArrayCopy(getUrlType(url), field, mw, url, name);
}
void presetPasteArray(MiddleWare &mw, std::string url, int field, std::string name)
{
    (void) name;
    printf("PresetArrayPaste()\n");
    string data = "";
    XMLwrapper xml;
    if(name.empty()) {
        data = mw.getPresetsStore().clipboard.data;
        if(data.length() < 20)
            return;
        if(!xml.putXMLdata(data.c_str()))
            return;
    } else {
        if(xml.loadXMLfile(name))
            return;
    }
    printf("Performing Paste...\n");
    doClassArrayPaste(getUrlType(url), getUrlPresetType(url, mw), field, mw, url, xml);
}
#if 0
void presetPaste(std::string url, int)
{
    printf("PresetPaste()\n");
    doClassPaste(getUrlType(url), *middlewarepointer, url, presetsstore.clipboard.data);
}
#endif
void presetDelete(int)
{
    printf("PresetDelete()\n");
}
void presetRescan()
{
    printf("PresetRescan()\n");
}
std::string presetClipboardType()
{
    printf("PresetClipboardType()\n");
    return "dummy";
}
bool presetCheckClipboardType()
{
    printf("PresetCheckClipboardType()\n");
    return true;
}
