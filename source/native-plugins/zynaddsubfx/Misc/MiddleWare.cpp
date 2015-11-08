#include "MiddleWare.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <rtosc/undo-history.h>
#include <rtosc/thread-link.h>
#include <rtosc/ports.h>
#include <lo/lo.h>

#include <unistd.h>

#include "../UI/Connection.h"
#include "../UI/Fl_Osc_Interface.h"

#include <map>

#include "Util.h"
#include "Master.h"
#include "Part.h"
#include "PresetExtractor.h"
#include "../Containers/MultiPseudoStack.h"
#include "../Params/PresetsStore.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/SUBnoteParameters.h"
#include "../Params/PADnoteParameters.h"
#include "../DSP/FFTwrapper.h"
#include "../Synth/OscilGen.h"
#include "../Nio/Nio.h"

#include <string>
#include <future>
#include <atomic>
#include <list>

#ifndef errx
#include <err.h>
#endif

using std::string;

/******************************************************************************
 *                        LIBLO And Reflection Code                           *
 *                                                                            *
 * All messages that are handled are handled in a serial fashion.             *
 * Thus, changes in the current interface sending messages can be encoded     *
 * into the stream via events which simply echo back the active interface     *
 ******************************************************************************/
static void liblo_error_cb(int i, const char *m, const char *loc)
{
    fprintf(stderr, "liblo :-( %d-%s@%s\n",i,m,loc);
}

void path_search(const char *m, const char *url)
{
    using rtosc::Ports;
    using rtosc::Port;

    //assumed upper bound of 32 ports (may need to be resized)
    char         types[129];
    rtosc_arg_t  args[128];
    size_t       pos    = 0;
    const Ports *ports  = NULL;
    const char  *str    = rtosc_argument(m,0).s;
    const char  *needle = rtosc_argument(m,1).s;

    //zero out data
    memset(types, 0, sizeof(types));
    memset(args,  0, sizeof(args));

    if(!*str) {
        ports = &Master::ports;
    } else {
        const Port *port = Master::ports.apropos(rtosc_argument(m,0).s);
        if(port)
            ports = port->ports;
    }

    if(ports) {
        //RTness not confirmed here
        for(const Port &p:*ports) {
            if(strstr(p.name, needle)!=p.name)
                continue;
            types[pos]    = 's';
            args[pos++].s = p.name;
            types[pos]    = 'b';
            if(p.metadata && *p.metadata) {
                args[pos].b.data = (unsigned char*) p.metadata;
                auto tmp = rtosc::Port::MetaContainer(p.metadata);
                args[pos++].b.len  = tmp.length();
            } else {
                args[pos].b.data = (unsigned char*) NULL;
                args[pos++].b.len  = 0;
            }
        }
    }


    //Reply to requester [wow, these messages are getting huge...]
    char buffer[1024*20];
    size_t length = rtosc_amessage(buffer, sizeof(buffer), "/paths", types, args);
    if(length) {
        lo_message msg  = lo_message_deserialise((void*)buffer, length, NULL);
        lo_address addr = lo_address_new_from_url(url);
        if(addr)
            lo_send_message(addr, buffer, msg);
    }
}

static int handler_function(const char *path, const char *types, lo_arg **argv,
        int argc, lo_message msg, void *user_data)
{
    (void) types;
    (void) argv;
    (void) argc;
    MiddleWare *mw = (MiddleWare*)user_data;
    lo_address addr = lo_message_get_source(msg);
    if(addr) {
        const char *tmp = lo_address_get_url(addr);
        if(tmp != mw->activeUrl()) {
            mw->transmitMsg("/echo", "ss", "OSC_URL", tmp);
            mw->activeUrl(tmp);
        }

    }

    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    size_t size = 2048;
    lo_message_serialise(msg, path, buffer, &size);
    if(!strcmp(buffer, "/path-search") && !strcmp("ss", rtosc_argument_string(buffer))) {
        path_search(buffer, mw->activeUrl().c_str());
    } else if(buffer[0]=='/' && rindex(buffer, '/')[1]) {
        mw->transmitMsg(rtosc::Ports::collapsePath(buffer));
    }

    return 0;
}

typedef void(*cb_t)(void*,const char*);


/*****************************************************************************
 *                    Memory Deallocation                                    *
 *****************************************************************************/
void deallocate(const char *str, void *v)
{
    //printf("deallocating a '%s' at '%p'\n", str, v);
    if(!strcmp(str, "Part"))
        delete (Part*)v;
    else if(!strcmp(str, "Master"))
        delete (Master*)v;
    else if(!strcmp(str, "fft_t"))
        delete[] (fft_t*)v;
    else
        fprintf(stderr, "Unknown type '%s', leaking pointer %p!!\n", str, v);
}


/*****************************************************************************
 *                    PadSynth Setup                                         *
 *****************************************************************************/

void preparePadSynth(string path, PADnoteParameters *p, rtosc::RtData &d)
{
    //printf("preparing padsynth parameters\n");
    assert(!path.empty());
    path += "sample";

    unsigned max = 0;
    p->sampleGenerator([&max,&path,&d]
            (unsigned N, PADnoteParameters::Sample &s)
            {
            max = max<N ? N : max;
            //printf("sending info to '%s'\n", (path+to_s(N)).c_str());
            d.chain((path+to_s(N)).c_str(), "ifb",
                s.size, s.basefreq, sizeof(float*), &s.smp);
            }, []{return false;});
    //clear out unused samples
    for(unsigned i = max+1; i < PAD_MAX_SAMPLES; ++i) {
        d.chain((path+to_s(i)).c_str(), "ifb",
                0, 440.0f, sizeof(float*), NULL);
    }
}

/******************************************************************************
 *                      MIDI Serialization                                    *
 *                                                                            *
 ******************************************************************************/
void saveMidiLearn(XMLwrapper &xml, const rtosc::MidiMappernRT &midi)
{
    xml.beginbranch("midi-learn");
    for(auto value:midi.inv_map) {
        XmlNode binding("midi-binding");
        auto biject = std::get<3>(value.second);
        binding["osc-path"]  = value.first;
        binding["coarse-CC"] = to_s(std::get<1>(value.second));
        binding["fine-CC"]   = to_s(std::get<2>(value.second));
        binding["type"]      = "i";
        binding["minimum"]   = to_s(biject.min);
        binding["maximum"]   = to_s(biject.max);
        xml.add(binding);
    }
    xml.endbranch();
}

void loadMidiLearn(XMLwrapper &xml, rtosc::MidiMappernRT &midi)
{
    using rtosc::Port;
    if(xml.enterbranch("midi-learn")) {
        auto nodes = xml.getBranch();

        //TODO clear mapper

        for(auto node:nodes) {
            if(node.name != "midi-binding" ||
                    !node.has("osc-path") ||
                    !node.has("coarse-CC"))
                continue;
            const string path = node["osc-path"];
            const int    CC   = atoi(node["coarse-CC"].c_str());
            const Port  *p    = Master::ports.apropos(path.c_str());
            if(p) {
                printf("loading midi port...\n");
                midi.addNewMapper(CC, *p, path);
            } else {
                printf("unknown midi bindable <%s>\n", path.c_str());
            }
        }
        xml.exitbranch();
    } else
        printf("cannot find 'midi-learn' branch...\n");
}

/******************************************************************************
 *                      Non-RealTime Object Store                             *
 *                                                                            *
 *                                                                            *
 * Storage For Objects which need to be interfaced with outside the realtime  *
 * thread (aka they have long lived operations which can be done out-of-band) *
 *                                                                            *
 * - OscilGen instances as prepare() cannot be done in realtime and PAD       *
 *   depends on these instances                                               *
 * - PADnoteParameter instances as applyparameters() cannot be done in        *
 *   realtime                                                                 *
 *                                                                            *
 * These instances are collected on every part change and kit change          *
 ******************************************************************************/
struct NonRtObjStore
{
    std::map<std::string, void*> objmap;

    void extractMaster(Master *master)
    {
        for(int i=0; i < NUM_MIDI_PARTS; ++i) {
            extractPart(master->part[i], i);
        }
    }

    void extractPart(Part *part, int i)
    {
        for(int j=0; j < NUM_KIT_ITEMS; ++j) {
            auto &obj = part->kit[j];
            extractAD(obj.adpars, i, j);
            extractPAD(obj.padpars, i, j);
        }
    }

    void extractAD(ADnoteParameters *adpars, int i, int j)
    {
        std::string base = "/part"+to_s(i)+"/kit"+to_s(j)+"/";
        for(int k=0; k<NUM_VOICES; ++k) {
            std::string nbase = base+"adpars/VoicePar"+to_s(k)+"/";
            if(adpars) {
                auto &nobj = adpars->VoicePar[k];
                objmap[nbase+"OscilSmp/"]     = nobj.OscilSmp;
                objmap[nbase+"FMSmp/"] = nobj.FMSmp;
            } else {
                objmap[nbase+"OscilSmp/"] = nullptr;
                objmap[nbase+"FMSmp/"]    = nullptr;
            }
        }
    }

    void extractPAD(PADnoteParameters *padpars, int i, int j)
    {
        std::string base = "/part"+to_s(i)+"/kit"+to_s(j)+"/";
        for(int k=0; k<NUM_VOICES; ++k) {
            if(padpars) {
                objmap[base+"padpars/"]       = padpars;
                objmap[base+"padpars/oscilgen/"] = padpars->oscilgen;
            } else {
                objmap[base+"padpars/"]       = nullptr;
                objmap[base+"padpars/oscilgen/"] = nullptr;
            }
        }
    }

    void clear(void)
    {
        objmap.clear();
    }

    bool has(std::string loc)
    {
        return objmap.find(loc) != objmap.end();
    }

    void *get(std::string loc)
    {
        return objmap[loc];
    }

    void handleOscil(const char *msg, rtosc::RtData &d) {
        string obj_rl(d.message, msg);
        void *osc = get(obj_rl);
        assert(osc);
        strcpy(d.loc, obj_rl.c_str());
        d.obj = osc;
        OscilGen::non_realtime_ports.dispatch(msg, d);
    }
    void handlePad(const char *msg, rtosc::RtData &d) {
        string obj_rl(d.message, msg);
        void *pad = get(obj_rl);
        if(!strcmp(msg, "prepare"))
            preparePadSynth(obj_rl, (PADnoteParameters*)pad, d);
        else {
            assert(pad);
            strcpy(d.loc, obj_rl.c_str());
            d.obj = pad;
            PADnoteParameters::non_realtime_ports.dispatch(msg, d);
        }
    }
};

/******************************************************************************
 *                      Realtime Parameter Store                              *
 *                                                                            *
 * Storage for AD/PAD/SUB parameters which are allocated as needed by kits.   *
 * Two classes of events affect this:                                         *
 * 1. When a message to enable a kit is observed, then the kit is allocated   *
 *    and sent prior to the enable message.                                   *
 * 2. When a part is allocated all part information is rebuilt                *
 *                                                                            *
 * (NOTE pointers aren't really needed here, just booleans on whether it has  *
 * been  allocated)                                                           *
 * This may be later utilized for copy/paste support                          *
 ******************************************************************************/
struct ParamStore
{
    ParamStore(void)
    {
        memset(add, 0, sizeof(add));
        memset(pad, 0, sizeof(pad));
        memset(sub, 0, sizeof(sub));
    }

    void extractPart(Part *part, int i)
    {
        for(int j=0; j < NUM_KIT_ITEMS; ++j) {
            auto kit = part->kit[j];
            add[i][j] = kit.adpars;
            sub[i][j] = kit.subpars;
            pad[i][j] = kit.padpars;
        }
    }

    ADnoteParameters  *add[NUM_MIDI_PARTS][NUM_KIT_ITEMS];
    SUBnoteParameters *sub[NUM_MIDI_PARTS][NUM_KIT_ITEMS];
    PADnoteParameters *pad[NUM_MIDI_PARTS][NUM_KIT_ITEMS];
};

//XXX perhaps move this to Nio
//(there needs to be some standard Nio stub file for this sort of stuff)
namespace Nio
{
    using std::get;
    rtosc::Ports ports = {
        {"sink-list:", 0, 0, [](const char *, rtosc::RtData &d) {
                auto list = Nio::getSinks();
                char *ret = rtosc_splat(d.loc, list);
                d.reply(ret);
                delete [] ret;
            }},
        {"source-list:", 0, 0, [](const char *, rtosc::RtData &d) {
                auto list = Nio::getSources();
                char *ret = rtosc_splat(d.loc, list);
                d.reply(ret);
                delete [] ret;
            }},
        {"source::s", 0, 0, [](const char *msg, rtosc::RtData &d) {
                if(rtosc_narguments(msg) == 0)
                    d.reply(d.loc, "s", Nio::getSource().c_str());
                else
                    Nio::setSource(rtosc_argument(msg,0).s);}},
        {"sink::s", 0, 0, [](const char *msg, rtosc::RtData &d) {
                if(rtosc_narguments(msg) == 0)
                    d.reply(d.loc, "s", Nio::getSink().c_str());
                else
                    Nio::setSink(rtosc_argument(msg,0).s);}},
    };
}


/* Implementation */
class MiddleWareImpl
{
    public:
    MiddleWare *parent;
    private:

    //Detect if the name of the process is 'zynaddsubfx'
    bool isPlugin() const
    {
        std::string proc_file = "/proc/" + to_s(getpid()) + "/comm";
        std::ifstream ifs(proc_file);
        if(ifs.good()) {
            std::string comm_name;
            ifs >> comm_name;
            return comm_name != "zynaddsubfx";
        }
        return true;
    }

public:
    Config* const config;
    MiddleWareImpl(MiddleWare *mw, SYNTH_T synth, Config* config,
                   int preferred_port);
    ~MiddleWareImpl(void);

    //Apply function while parameters are write locked
    void doReadOnlyOp(std::function<void()> read_only_fn);

    void savePart(int npart, const char *filename)
    {
        //Copy is needed as filename WILL get trashed during the rest of the run
        std::string fname = filename;
        //printf("saving part(%d,'%s')\n", npart, filename);
        doReadOnlyOp([this,fname,npart](){
                int res = master->part[npart]->saveXML(fname.c_str());
                (void)res;
                /*printf("results: '%s' '%d'\n",fname.c_str(), res);*/});
    }

    void loadPendingBank(int par, Bank &bank)
    {
        if(((unsigned int)par < bank.banks.size())
           && (bank.banks[par].dir != bank.bankfiletitle))
            bank.loadbank(bank.banks[par].dir);
    }

    void loadPart(int npart, const char *filename, Master *master)
    {
        actual_load[npart]++;

        if(actual_load[npart] != pending_load[npart])
            return;
        assert(actual_load[npart] <= pending_load[npart]);

        //load part in async fashion when possible
#if HAVE_ASYNC
        auto alloc = std::async(std::launch::async,
                [master,filename,this,npart](){
                Part *p = new Part(*master->memory, synth,
                                   master->time,
                                   config->cfg.GzipCompression,
                                   config->cfg.Interpolation,
                                   &master->microtonal, master->fft);
                if(p->loadXMLinstrument(filename))
                    fprintf(stderr, "Warning: failed to load part<%s>!\n", filename);

                auto isLateLoad = [this,npart]{
                return actual_load[npart] != pending_load[npart];
                };

                p->applyparameters(isLateLoad);
                return p;});

        //Load the part
        if(idle) {
            while(alloc.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                idle(idle_ptr);
            }
        }

        Part *p = alloc.get();
#else
        Part *p = new Part(*master->memory, synth, master->time,
                config->cfg.GzipCompression,
                config->cfg.Interpolation,
                &master->microtonal, master->fft);
        if(p->loadXMLinstrument(filename))
            fprintf(stderr, "Warning: failed to load part<%s>!\n", filename);

        auto isLateLoad = [this,npart]{
            return actual_load[npart] != pending_load[npart];
        };

        p->applyparameters(isLateLoad);
#endif

        obj_store.extractPart(p, npart);
        kits.extractPart(p, npart);

        //Give it to the backend and wait for the old part to return for
        //deallocation
        parent->transmitMsg("/load-part", "ib", npart, sizeof(Part*), &p);
        GUI::raiseUi(ui, "/damage", "s", ("/part"+to_s(npart)+"/").c_str());
    }

    //Load a new cleared Part instance
    void loadClearPart(int npart)
    {
        if(npart == -1)
            return;
        Part *p = new Part(*master->memory, synth,
                           master->time,
                           config->cfg.GzipCompression,
                           config->cfg.Interpolation,
                           &master->microtonal, master->fft);
        p->applyparameters();
        obj_store.extractPart(p, npart);
        kits.extractPart(p, npart);

        //Give it to the backend and wait for the old part to return for
        //deallocation
        parent->transmitMsg("/load-part", "ib", npart, sizeof(Part*), &p);
        GUI::raiseUi(ui, "/damage", "s", ("/part"+to_s(npart)+"/").c_str());
    }

    //Well, you don't get much crazier than changing out all of your RT
    //structures at once... TODO error handling
    void loadMaster(const char *filename)
    {
        Master *m = new Master(synth, config);
        m->uToB = uToB;
        m->bToU = bToU;
        if(filename) {
            if ( m->loadXML(filename) ) {
                delete m;
                return;
            }
            m->applyparameters();
        }

        //Update resource locator table
        updateResources(m);

        master = m;

        //Give it to the backend and wait for the old part to return for
        //deallocation
        parent->transmitMsg("/load-master", "b", sizeof(Master*), &m);
    }

    void updateResources(Master *m)
    {
        obj_store.clear();
        obj_store.extractMaster(m);
        for(int i=0; i<NUM_MIDI_PARTS; ++i)
            kits.extractPart(m->part[i], i);
    }

    //If currently broadcasting messages
    bool broadcast = false;
    //If message should be forwarded through snoop ports
    bool forward   = false;
    //if message is in order or out-of-order execution
    bool in_order  = false;
    //If accepting undo events as user driven
    bool recording_undo = true;
    void bToUhandle(const char *rtmsg);

    void tick(void)
    {
        if(server)
            while(lo_server_recv_noblock(server, 0));
        while(bToU->hasNext()) {
            const char *rtmsg = bToU->read();
            bToUhandle(rtmsg);
        }
        while(auto *m = multi_thread_source.read()) {
            handleMsg(m->memory);
            multi_thread_source.free(m);
        }
    }


    void kitEnable(const char *msg);
    void kitEnable(int part, int kit, int type);

    // Handle an event with special cases
    void handleMsg(const char *msg);

    void write(const char *path, const char *args, ...);
    void write(const char *path, const char *args, va_list va);


    // Send a message to a remote client
    void sendToRemote(const char *msg, std::string dest);
    // Send a message to the current remote client
    void sendToCurrentRemote(const char *msg)
    {
        sendToRemote(msg, in_order ? curr_url : last_url);
    }
    // Broadcast a message to all listening remote clients
    void broadcastToRemote(const char *msg);


    /*
     * Provides a mapping for non-RT objects stored inside the backend
     * - Oscilgen almost all parameters can be safely set
     * - Padnote can have anything set on its oscilgen and a very small set
     *   of general parameters
     */
    NonRtObjStore obj_store;

    //This code will own the pointer to master, be prepared for odd things if
    //this assumption is broken
    Master *master;

    //The ONLY means that any chunk of UI code should have for interacting with the
    //backend
    Fl_Osc_Interface *osc;
    //Synth Engine Parameters
    ParamStore kits;

    //Callback When Waiting on async events
    void(*idle)(void*);
    void* idle_ptr;

    //General UI callback
    cb_t cb;
    //UI handle
    void *ui;

    std::atomic_int pending_load[NUM_MIDI_PARTS];
    std::atomic_int actual_load[NUM_MIDI_PARTS];

    //Undo/Redo
    rtosc::UndoHistory undo;

    //MIDI Learn
    rtosc::MidiMappernRT midi_mapper;

    //Link To the Realtime
    rtosc::ThreadLink *bToU;
    rtosc::ThreadLink *uToB;

    //Link to the unknown
    MultiQueue multi_thread_source;

    //LIBLO
    lo_server server;
    string last_url, curr_url;

    //Synthesis Rate Parameters
    const SYNTH_T synth;

    PresetsStore presetsstore;
};

/*****************************************************************************
 *                      Data Object for Non-RT Class Dispatch                *
 *****************************************************************************/

class MwDataObj:public rtosc::RtData
{
    public:
        MwDataObj(MiddleWareImpl *mwi_)
        {
            loc_size = 1024;
            loc = new char[loc_size];
            memset(loc, 0, loc_size);
            buffer = new char[4*4096];
            memset(buffer, 0, 4*4096);
            obj       = mwi_;
            mwi       = mwi_;
            forwarded = false;
        }

        ~MwDataObj(void)
        {
            delete[] buffer;
        }

        //Replies and broadcasts go to the remote

        //Chain calls repeat the call into handle()

        //Forward calls send the message directly to the realtime
        virtual void reply(const char *path, const char *args, ...)
        {
            //printf("reply building '%s'\n", path);
            va_list va;
            va_start(va,args);
            if(!strcmp(path, "/forward")) { //forward the information to the backend
                args++;
                path = va_arg(va, const char *);
                rtosc_vmessage(buffer,4*4096,path,args,va);
            } else {
                rtosc_vmessage(buffer,4*4096,path,args,va);
                reply(buffer);
            }
            va_end(va);
        }
        virtual void reply(const char *msg){
            mwi->sendToCurrentRemote(msg);
        };
        //virtual void broadcast(const char *path, const char *args, ...){(void)path;(void)args;};
        //virtual void broadcast(const char *msg){(void)msg;};

        virtual void chain(const char *msg) override
        {
            assert(msg);
            // printf("chain call on <%s>\n", msg);
            mwi->handleMsg(msg);
        }

        virtual void chain(const char *path, const char *args, ...) override
        {
            assert(path);
            va_list va;
            va_start(va,args);
            rtosc_vmessage(buffer,4*4096,path,args,va);
            chain(buffer);
            va_end(va);
        }

        virtual void forward(const char *) override
        {
            forwarded = true;
        }

        bool forwarded;
    private:
        char *buffer;
        MiddleWareImpl   *mwi;
};




static int extractInt(const char *msg)
{
    const char *mm = msg;
    while(*mm && !isdigit(*mm)) ++mm;
    if(isdigit(*mm))
        return atoi(mm);
    return -1;
}

static const char *chomp(const char *msg)
{
    while(*msg && *msg!='/') ++msg; \
    msg = *msg ? msg+1 : msg;
    return msg;
};

using rtosc::RtData;
#define rObject Bank
#define rBegin [](const char *msg, RtData &d) { (void)msg;(void)d;\
    rObject &impl = *((rObject*)d.obj);(void)impl;
#define rEnd }
/*****************************************************************************
 *                       Instrument Banks                                    *
 *                                                                           *
 * Banks and presets in general are not classed as realtime safe             *
 *                                                                           *
 * The supported operations are:                                             *
 * - Load Names                                                              *
 * - Load Bank                                                               *
 * - Refresh List of Banks                                                   *
 *****************************************************************************/
rtosc::Ports bankPorts = {
    {"rescan:", 0, 0,
        rBegin;
        impl.rescanforbanks();
        //Send updated banks
        int i = 0;
        for(auto &elm : impl.banks)
            d.reply("/bank/bank_select", "iss", i++, elm.name.c_str(), elm.dir.c_str());

        rEnd},
    {"slot#1024:", 0, 0,
        rBegin;
        const int loc = extractInt(msg);
        if(loc >= BANK_SIZE)
            return;

        d.reply("/bankview", "iss",
                loc, impl.ins[loc].name.c_str(),
                impl.ins[loc].filename.c_str());
        rEnd},
    {"banks:", 0, 0,
        rBegin;
        int i = 0;
        for(auto &elm : impl.banks)
            d.reply("/bank/bank_select", "iss", i++, elm.name.c_str(), elm.dir.c_str());
        rEnd},
    {"bank_select::i", 0, 0,
        rBegin
           if(rtosc_narguments(msg)) {
               const int pos = rtosc_argument(msg, 0).i;
               d.reply(d.loc, "i", pos);
               if(impl.bankpos != pos) {
                   impl.bankpos = pos;
                   impl.loadbank(impl.banks[pos].dir);

                   //Reload bank slots
                   for(int i=0; i<BANK_SIZE; ++i)
                       d.reply("/bankview", "iss",
                                   i, impl.ins[i].name.c_str(),
                                   impl.ins[i].filename.c_str());
               }
            } else
                d.reply("/bank/bank_select", "i", impl.bankpos);
        rEnd},
    {"rename_slot:is", 0, 0,
        rBegin;
        const int   slot = rtosc_argument(msg, 0).i;
        const char *name = rtosc_argument(msg, 1).s;
        const int err = impl.setname(slot, name, -1);
        if(err) {
            d.reply("/alert", "s",
                    "Failed To Rename Bank Slot, please check file permissions");
        }
        rEnd},
    {"swap_slots:ii", 0, 0,
        rBegin;
        const int slota = rtosc_argument(msg, 0).i;
        const int slotb = rtosc_argument(msg, 1).i;
        const int err = impl.swapslot(slota, slotb);
        if(err)
            d.reply("/alert", "s",
                    "Failed To Swap Bank Slots, please check file permissions");
        rEnd},
    {"clear_slot:i", 0, 0,
        rBegin;
        const int slot = rtosc_argument(msg, 0).i;
        const int err  = impl.clearslot(slot);
        if(err)
            d.reply("/alert", "s",
                    "Failed To Clear Bank Slot, please check file permissions");
        rEnd},
    {"msb:i", 0, 0,
        rBegin;
        impl.setMsb(rtosc_argument(msg, 0).i);
        rEnd},
    {"lsb:i", 0, 0,
        rBegin;
        impl.setLsb(rtosc_argument(msg, 0).i);
        rEnd},
};

/******************************************************************************
 *                       MiddleWare Snooping Ports                            *
 *                                                                            *
 * These ports handle:                                                        *
 * - Events going to the realtime thread which cannot be safely handled       *
 *   there                                                                    *
 * - Events generated by the realtime thread which are not destined for a     *
 *   user interface                                                           *
 ******************************************************************************/

#undef  rObject
#define rObject MiddleWareImpl
/*
 * BASE/part#/kititem#
 * BASE/part#/kit#/adpars/voice#/oscil/\*
 * BASE/part#/kit#/adpars/voice#/mod-oscil/\*
 * BASE/part#/kit#/padpars/prepare
 * BASE/part#/kit#/padpars/oscil/\*
 */
static rtosc::Ports middwareSnoopPorts = {
    {"part#16/kit#8/adpars/VoicePar#8/OscilSmp/", 0, &OscilGen::non_realtime_ports,
        rBegin;
        impl.obj_store.handleOscil(chomp(chomp(chomp(chomp(chomp(msg))))), d);
        rEnd},
    {"part#16/kit#8/adpars/VoicePar#8/FMSmp/", 0, &OscilGen::non_realtime_ports,
        rBegin
        impl.obj_store.handleOscil(chomp(chomp(chomp(chomp(chomp(msg))))), d);
        rEnd},
    {"part#16/kit#8/padpars/", 0, &PADnoteParameters::non_realtime_ports,
        rBegin
        impl.obj_store.handlePad(chomp(chomp(chomp(msg))), d);
        rEnd},
    {"bank/", 0, &bankPorts,
        rBegin;
        d.obj = &impl.master->bank;
        bankPorts.dispatch(chomp(msg),d);
        rEnd},
    {"bank/save_to_slot:ii", 0, 0,
        rBegin;
        const int part_id = rtosc_argument(msg, 0).i;
        const int slot    = rtosc_argument(msg, 1).i;

        int err = 0;
        impl.doReadOnlyOp([&impl,slot,part_id,&err](){
                err = impl.master->bank.savetoslot(slot, impl.master->part[part_id]);});
        if(err) {
            char buffer[1024];
            rtosc_message(buffer, 1024, "/alert", "s",
                    "Failed To Save To Bank Slot, please check file permissions");
            GUI::raiseUi(impl.ui, buffer);
        }
        rEnd},
    {"config/", 0, &Config::ports,
        rBegin;
        d.obj = impl.config;
        Config::ports.dispatch(chomp(msg), d);
        rEnd},
    {"presets/", 0,  &real_preset_ports,          [](const char *msg, RtData &d) {
        MiddleWareImpl *obj = (MiddleWareImpl*)d.obj;
        d.obj = (void*)obj->parent;
        real_preset_ports.dispatch(chomp(msg), d);
        if(strstr(msg, "paste") && rtosc_argument_string(msg)[0] == 's')
            d.reply("/damage", "s", rtosc_argument(msg, 0).s);
        }},
    {"io/", 0, &Nio::ports,               [](const char *msg, RtData &d) {
        Nio::ports.dispatch(chomp(msg), d);}},
    {"part*/kit*/{Padenabled,Ppadenabled,Psubenabled}:T:F", 0, 0,
        rBegin;
        impl.kitEnable(msg);
        d.forward();
        rEnd},
    {"save_xlz:s", 0, 0,
        rBegin;
        const char *file = rtosc_argument(msg, 0).s;
        XMLwrapper xml;
        saveMidiLearn(xml, impl.midi_mapper);
        xml.saveXMLfile(file, impl.master->gzip_compression);
        rEnd},
    {"load_xlz:s", 0, 0,
        rBegin;
        const char *file = rtosc_argument(msg, 0).s;
        XMLwrapper xml;
        xml.loadXMLfile(file);
        loadMidiLearn(xml, impl.midi_mapper);
        rEnd},
    {"save_xmz:s", 0, 0,
        rBegin;
        const char *file = rtosc_argument(msg, 0).s;
        //Copy is needed as filename WILL get trashed during the rest of the run
        impl.doReadOnlyOp([&impl,file](){
                int res = impl.master->saveXML(file);
                (void)res;});
        rEnd},
    {"save_xiz:is", 0, 0,
        rBegin;
        const int   part_id = rtosc_argument(msg,0).i;
        const char *file    = rtosc_argument(msg,1).s;
        impl.savePart(part_id, file);
        rEnd},
    {"load_xmz:s", 0, 0,
        rBegin;
        const char *file = rtosc_argument(msg, 0).s;
        impl.loadMaster(file);
        rEnd},
    {"reset_master:", 0, 0,
        rBegin;
        impl.loadMaster(NULL);
        rEnd},
    {"load_xiz:is", 0, 0,
        rBegin;
        const int part_id = rtosc_argument(msg,0).i;
        const char *file  = rtosc_argument(msg,1).s;
        impl.pending_load[part_id]++;
        impl.loadPart(part_id, file, impl.master);
        rEnd},
    {"load-part:is", 0, 0,
        rBegin;
        const int part_id = rtosc_argument(msg,0).i;
        const char *file  = rtosc_argument(msg,1).s;
        impl.pending_load[part_id]++;
        impl.loadPart(part_id, file, impl.master);
        rEnd},
    {"setprogram:i:c", 0, 0,
        rBegin;
        Bank &bank     = impl.master->bank;
        const int slot = rtosc_argument(msg, 0).i + 128*bank.bank_lsb;
        if(slot < BANK_SIZE) {
            impl.pending_load[0]++;
            impl.loadPart(0, impl.master->bank.ins[slot].filename.c_str(), impl.master);
        }
        rEnd},
    {"part#16/clear:", 0, 0,
        rBegin;
        impl.loadClearPart(extractInt(msg));
        rEnd},
    {"undo:", 0, 0,
        rBegin;
        impl.undo.seekHistory(-1);
        rEnd},
    {"redo:", 0, 0,
        rBegin;
        impl.undo.seekHistory(+1);
        rEnd},
    {"learn:s", 0, 0,
        rBegin;
        string addr = rtosc_argument(msg, 0).s;
        auto &midi  = impl.midi_mapper;
        auto map    = midi.getMidiMappingStrings();
        if(map.find(addr) != map.end())
            midi.map(addr.c_str(), false);
        else
            midi.map(addr.c_str(), true);
        rEnd},
    //drop this message into the abyss
    {"ui/title:", 0, 0, [](const char *msg, RtData &d) {}}
};

static rtosc::Ports middlewareReplyPorts = {
    {"echo:ss", 0, 0,
        rBegin;
        const char *type = rtosc_argument(msg, 0).s;
        const char *url  = rtosc_argument(msg, 1).s;
        if(!strcmp(type, "OSC_URL"))
            impl.curr_url = url;
        rEnd},
    {"free:sb", 0, 0,
        rBegin;
        const char *type = rtosc_argument(msg, 0).s;
        void       *ptr  = *(void**)rtosc_argument(msg, 1).b.data;
        deallocate(type, ptr);
        rEnd},
    {"request_memory:", 0, 0,
        rBegin;
        //Generate out more memory for the RT memory pool
        //5MBi chunk
        size_t N  = 5*1024*1024;
        void *mem = malloc(N);
        impl.uToB->write("/add-rt-memory", "bi", sizeof(void*), &mem, N);
        rEnd},
    {"setprogram:cc:ii", 0, 0,
        rBegin;
        const int part    = rtosc_argument(msg, 0).i;
        const int program = rtosc_argument(msg, 1).i;
        impl.loadPart(part, impl.master->bank.ins[program].filename.c_str(), impl.master);
        rEnd},
    {"undo_pause:", 0, 0, rBegin; impl.recording_undo = false; rEnd},
    {"undo_resume:", 0, 0, rBegin; impl.recording_undo = true; rEnd},
    {"undo_change", 0, 0,
        rBegin;
        if(impl.recording_undo)
            impl.undo.recordEvent(msg);
        rEnd},
    {"midi-use-CC:i", 0, 0,
        rBegin;
        impl.midi_mapper.useFreeID(rtosc_argument(msg, 0).i);
        rEnd},
    {"broadcast:", 0, 0, rBegin; impl.broadcast = true; rEnd},
    {"forward:", 0, 0, rBegin; impl.forward = true; rEnd},
};
#undef rBegin
#undef rEnd

/******************************************************************************
 *                         MiddleWare Implementation                          *
 ******************************************************************************/

MiddleWareImpl::MiddleWareImpl(MiddleWare *mw, SYNTH_T synth_,
    Config* config, int preferrred_port)
    :parent(mw), config(config), ui(nullptr), synth(std::move(synth_)),
    presetsstore(*config)
{
    bToU = new rtosc::ThreadLink(4096*2,1024);
    uToB = new rtosc::ThreadLink(4096*2,1024);
    midi_mapper.base_ports = &Master::ports;
    midi_mapper.rt_cb      = [this](const char *msg){handleMsg(msg);};
    if(preferrred_port != -1)
        server = lo_server_new_with_proto(to_s(preferrred_port).c_str(),
                                          LO_UDP, liblo_error_cb);
    else
        server = lo_server_new_with_proto(NULL, LO_UDP, liblo_error_cb);

    if(server) {
        lo_server_add_method(server, NULL, NULL, handler_function, mw);
        fprintf(stderr, "lo server running on %d\n", lo_server_get_port(server));
    } else
        fprintf(stderr, "lo server could not be started :-/\n");


    //dummy callback for starters
    cb = [](void*, const char*){};
    idle = 0;
    idle_ptr = 0;

    master = new Master(synth, config);
    master->bToU = bToU;
    master->uToB = uToB;
    osc    = GUI::genOscInterface(mw);

    //Grab objects of interest from master
    updateResources(master);

    //Null out Load IDs
    for(int i=0; i < NUM_MIDI_PARTS; ++i) {
        pending_load[i] = 0;
        actual_load[i] = 0;
    }

    //Setup Undo
    undo.setCallback([this](const char *msg) {
           // printf("undo callback <%s>\n", msg);
            char buf[1024];
            rtosc_message(buf, 1024, "/undo_pause","");
            handleMsg(buf);
            handleMsg(msg);
            rtosc_message(buf, 1024, "/undo_resume","");
            handleMsg(buf);
            });
}

MiddleWareImpl::~MiddleWareImpl(void)
{

    if(server)
        lo_server_free(server);

    delete master;
    delete osc;
    delete bToU;
    delete uToB;

}

/** Threading When Saving
 *  ----------------------
 *
 * Procedure Middleware:
 *   1) Middleware sends /freeze_state to backend
 *   2) Middleware waits on /state_frozen from backend
 *      All intervening commands are held for out of order execution
 *   3) Aquire memory
 *      At this time by the memory barrier we are guarenteed that all old
 *      writes are done and assuming the freezing logic is sound, then it is
 *      impossible for any other parameter to change at this time
 *   3) Middleware performs saving operation
 *   4) Middleware sends /thaw_state to backend
 *   5) Restore in order execution
 *
 * Procedure Backend:
 *   1) Observe /freeze_state and disable all mutating events (MIDI CC)
 *   2) Run a memory release to ensure that all writes are complete
 *   3) Send /state_frozen to Middleware
 *   time...
 *   4) Observe /thaw_state and resume normal processing
 */

void MiddleWareImpl::doReadOnlyOp(std::function<void()> read_only_fn)
{
    assert(uToB);
    uToB->write("/freeze_state","");

    std::list<const char *> fico;
    int tries = 0;
    while(tries++ < 10000) {
        if(!bToU->hasNext()) {
            usleep(500);
            continue;
        }
        const char *msg = bToU->read();
        if(!strcmp("/state_frozen", msg))
            break;
        size_t bytes = rtosc_message_length(msg, bToU->buffer_size());
        char *save_buf = new char[bytes];
        memcpy(save_buf, msg, bytes);
        fico.push_back(save_buf);
    }

    assert(tries < 10000);//if this happens, the backend must be dead

    std::atomic_thread_fence(std::memory_order_acquire);

    //Now it is safe to do any read only operation
    read_only_fn();

    //Now to resume normal operations
    uToB->write("/thaw_state","");
    for(auto x:fico) {
        uToB->raw_write(x);
        delete [] x;
    }
}

void MiddleWareImpl::broadcastToRemote(const char *rtmsg)
{
    //Always send to the local UI
    sendToRemote(rtmsg, "GUI");

    //Send to remote UI if there's one listening
    if(curr_url != "GUI")
        sendToRemote(rtmsg, curr_url);

    broadcast = false;
}

void MiddleWareImpl::sendToRemote(const char *rtmsg, std::string dest)
{
    //printf("sendToRemote(%s:%s,%s)\n", rtmsg, rtosc_argument_string(rtmsg),
    //        dest.c_str());
    if(dest == "GUI") {
        cb(ui, rtmsg);
    } else if(!dest.empty()) {
        lo_message msg  = lo_message_deserialise((void*)rtmsg,
                rtosc_message_length(rtmsg, bToU->buffer_size()), NULL);

        //Send to known url
        lo_address addr = lo_address_new_from_url(dest.c_str());
        if(addr)
            lo_send_message(addr, rtmsg, msg);
    }
}

/**
 * Handle all events coming from the backend
 *
 * This includes forwarded events which need to be retransmitted to the backend
 * after the snooping code inspects the message
 */
void MiddleWareImpl::bToUhandle(const char *rtmsg)
{
    //Verify Message isn't a known corruption bug
    assert(strcmp(rtmsg, "/part0/kit0/Ppadenableda"));
    assert(strcmp(rtmsg, "/ze_state"));

    //Dump Incomming Events For Debugging
    if(strcmp(rtmsg, "/vu-meter") && false) {
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 1 + 30, 0 + 40);
        fprintf(stdout, "frontend[%c]: '%s'<%s>\n", forward?'f':broadcast?'b':'N',
                rtmsg, rtosc_argument_string(rtmsg));
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
    }

    //Activity dot
    //printf(".");fflush(stdout);

    MwDataObj d(this);
    middlewareReplyPorts.dispatch(rtmsg, d, true);

    in_order = true;
    //Normal message not captured by the ports
    if(d.matches == 0) {
        if(forward) {
            forward = false;
            handleMsg(rtmsg);
        } if(broadcast)
            broadcastToRemote(rtmsg);
        else
            sendToCurrentRemote(rtmsg);
    }
    in_order = false;

}

//Allocate kits on a as needed basis
void MiddleWareImpl::kitEnable(const char *msg)
{
    const string argv = rtosc_argument_string(msg);
    if(argv != "T")
        return;
    //Extract fields from:
    //BASE/part#/kit#/Pxxxenabled
    int type = -1;
    if(strstr(msg, "Padenabled"))
        type = 0;
    else if(strstr(msg, "Ppadenabled"))
        type = 1;
    else if(strstr(msg, "Psubenabled"))
        type = 2;
    else
        return;

    const char *tmp = strstr(msg, "part");

    if(tmp == NULL)
        return;

    const int part = atoi(tmp+4);

    tmp = strstr(msg, "kit");

    if(tmp == NULL)
        return;

    const int kit = atoi(tmp+3);

    kitEnable(part, kit, type);
}

void MiddleWareImpl::kitEnable(int part, int kit, int type)
{
    //printf("attempting a kit enable<%d,%d,%d>\n", part, kit, type);
    string url = "/part"+to_s(part)+"/kit"+to_s(kit)+"/";
    void *ptr = NULL;
    if(type == 0 && kits.add[part][kit] == NULL) {
        ptr = kits.add[part][kit] = new ADnoteParameters(synth, master->fft);
        url += "adpars-data";
        obj_store.extractAD(kits.add[part][kit], part, kit);
    } else if(type == 1 && kits.pad[part][kit] == NULL) {
        ptr = kits.pad[part][kit] = new PADnoteParameters(synth, master->fft);
        url += "padpars-data";
        obj_store.extractPAD(kits.pad[part][kit], part, kit);
    } else if(type == 2 && kits.sub[part][kit] == NULL) {
        ptr = kits.sub[part][kit] = new SUBnoteParameters();
        url += "subpars-data";
    }

    //Send the new memory
    if(ptr)
        uToB->write(url.c_str(), "b", sizeof(void*), &ptr);
}


/*
 * Handle all messages traveling to the realtime side.
 */
void MiddleWareImpl::handleMsg(const char *msg)
{
    //Check for known bugs
    assert(msg && *msg && rindex(msg, '/')[1]);
    assert(strstr(msg,"free") == NULL || strstr(rtosc_argument_string(msg), "b") == NULL);
    assert(strcmp(msg, "/part0/Psysefxvol"));
    assert(strcmp(msg, "/Penabled"));
    assert(strcmp(msg, "part0/part0/Ppanning"));
    assert(strcmp(msg, "sysefx0sysefx0/preset"));
    assert(strcmp(msg, "/sysefx0preset"));
    assert(strcmp(msg, "Psysefxvol0/part0"));

    if(strcmp("/get-vu", msg) && false) {
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 6 + 30, 0 + 40);
        fprintf(stdout, "middleware: '%s':%s\n", msg, rtosc_argument_string(msg));
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
    }

    const char *last_path = rindex(msg, '/');
    if(!last_path) {
        printf("Bad message in handleMsg() <%s>\n", msg);
        assert(false);
        return;
    }

    MwDataObj d(this);
    middwareSnoopPorts.dispatch(msg, d, true);

    //A message unmodified by snooping
    if(d.matches == 0 || d.forwarded) {
        //if(strcmp("/get-vu", msg)) {
        //    printf("Message Continuing on<%s:%s>...\n", msg, rtosc_argument_string(msg));
        //}
        uToB->raw_write(msg);
    } else {
        //printf("Message Handled<%s:%s>...\n", msg, rtosc_argument_string(msg));
    }
}

void MiddleWareImpl::write(const char *path, const char *args, ...)
{
    //We have a free buffer in the threadlink, so use it
    va_list va;
    va_start(va, args);
    write(path, args, va);
    va_end(va);
}

void MiddleWareImpl::write(const char *path, const char *args, va_list va)
{
    //printf("is that a '%s' I see there?\n", path);
    char *buffer = uToB->buffer();
    unsigned len = uToB->buffer_size();
    bool success = rtosc_vmessage(buffer, len, path, args, va);
    //printf("working on '%s':'%s'\n",path, args);

    if(success)
        handleMsg(buffer);
    else
        warnx("Failed to write message to '%s'", path);
}

/******************************************************************************
 *                         MidleWare Forwarding Stubs                         *
 ******************************************************************************/
MiddleWare::MiddleWare(SYNTH_T synth, Config* config,
                       int preferred_port)
:impl(new MiddleWareImpl(this, std::move(synth), config, preferred_port))
{}
MiddleWare::~MiddleWare(void)
{
    delete impl;
}
void MiddleWare::updateResources(Master *m)
{
    impl->updateResources(m);
}
Master *MiddleWare::spawnMaster(void)
{
    assert(impl->master);
    assert(impl->master->uToB);
    return impl->master;
}
Fl_Osc_Interface *MiddleWare::spawnUiApi(void)
{
    return impl->osc;
}
void MiddleWare::tick(void)
{
    impl->tick();
}

void MiddleWare::doReadOnlyOp(std::function<void()> fn)
{
    impl->doReadOnlyOp(fn);
}

void MiddleWare::setUiCallback(void(*cb)(void*,const char *), void *ui)
{
    impl->cb = cb;
    impl->ui = ui;
}

void MiddleWare::setIdleCallback(void(*cb)(void*), void *ptr)
{
    impl->idle     = cb;
    impl->idle_ptr = ptr;
}

void MiddleWare::transmitMsg(const char *msg)
{
    impl->handleMsg(msg);
}

void MiddleWare::transmitMsg(const char *path, const char *args, ...)
{
    char buffer[1024];
    va_list va;
    va_start(va,args);
    if(rtosc_vmessage(buffer,1024,path,args,va))
        transmitMsg(buffer);
    else
        fprintf(stderr, "Error in transmitMsg(...)\n");
    va_end(va);
}

void MiddleWare::transmitMsg_va(const char *path, const char *args, va_list va)
{
    char buffer[1024];
    if(rtosc_vmessage(buffer, 1024, path, args, va))
        transmitMsg(buffer);
    else
        fprintf(stderr, "Error in transmitMsg(va)n");
}

void MiddleWare::messageAnywhere(const char *path, const char *args, ...)
{
    auto *mem = impl->multi_thread_source.alloc();
    if(!mem)
        fprintf(stderr, "Middleware::messageAnywhere memory pool out of memory...\n");

    va_list va;
    va_start(va,args);
    if(rtosc_vmessage(mem->memory,mem->size,path,args,va))
        impl->multi_thread_source.write(mem);
    else {
        fprintf(stderr, "Middleware::messageAnywhere message too big...\n");
        impl->multi_thread_source.free(mem);
    }
}


void MiddleWare::pendingSetBank(int bank)
{
    impl->bToU->write("/setbank", "c", bank);
}
void MiddleWare::pendingSetProgram(int part, int program)
{
    impl->pending_load[part]++;
    impl->bToU->write("/setprogram", "cc", part, program);
}

std::string MiddleWare::activeUrl(void)
{
    return impl->last_url;
}

void MiddleWare::activeUrl(std::string u)
{
    impl->last_url = u;
}

const SYNTH_T &MiddleWare::getSynth(void) const
{
    return impl->synth;
}

const char* MiddleWare::getServerAddress(void) const
{
    if(impl->server)
        return lo_server_get_url(impl->server);
    else
        return "NULL";
}

const PresetsStore& MiddleWare::getPresetsStore() const
{
    return impl->presetsstore;
}

PresetsStore& MiddleWare::getPresetsStore()
{
    return impl->presetsstore;
}
