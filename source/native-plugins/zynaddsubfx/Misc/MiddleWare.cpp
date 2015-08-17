#include "MiddleWare.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <rtosc/undo-history.h>
#include <rtosc/thread-link.h>
#include <rtosc/ports.h>
#include <rtosc/typed-message.h>
#include <lo/lo.h>

#include <unistd.h>

#include "../UI/Connection.h"
#include "../UI/Fl_Osc_Interface.h"

#include <map>

#include "Util.h"
#include "TmpFileMgr.h"
#include "Master.h"
#include "Part.h"
#include "PresetExtractor.h"
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
#ifndef PLUGINVERSION
extern rtosc::ThreadLink *the_bToU;//XXX
#endif

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
    } else if(buffer[0]=='/' && rindex(buffer, '/')[1])
        mw->transmitMsg(buffer);

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

void preparePadSynth(string path, PADnoteParameters *p, rtosc::ThreadLink *uToB)
{
    //printf("preparing padsynth parameters\n");
    assert(!path.empty());
    path += "sample";

    unsigned max = 0;
    p->sampleGenerator([&max,&path,uToB]
            (unsigned N, PADnoteParameters::Sample &s)
            {
            max = max<N ? N : max;
            //printf("sending info to '%s'\n", (path+to_s(N)).c_str());
            uToB->write((path+to_s(N)).c_str(), "ifb",
                s.size, s.basefreq, sizeof(float*), &s.smp);
            }, []{return false;});
    //clear out unused samples
    for(unsigned i = max+1; i < PAD_MAX_SAMPLES; ++i) {
        uToB->write((path+to_s(i)).c_str(), "ifb",
                0, 440.0f, sizeof(float*), NULL);
    }
}

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
void refreshBankView(const Bank &bank, unsigned loc, std::function<void(const char*)> cb)
{
    if(loc >= BANK_SIZE)
        return;

    char response[2048];
    if(!rtosc_message(response, 1024, "/bankview", "iss",
                loc, bank.ins[loc].name.c_str(),
                bank.ins[loc].filename.c_str()))
        errx(1, "Failure to handle bank update properly...");


    if(cb)
        cb(response);
}

void bankList(Bank &bank, std::function<void(const char*)> cb)
{
    char response[2048];
    int i = 0;

    for(auto &elm : bank.banks) {
        if(!rtosc_message(response, 2048, "/bank-list", "iss",
                    i++, elm.name.c_str(), elm.dir.c_str()))
            errx(1, "Failure to handle bank update properly...");
        if(cb)
            cb(response);
    }
}

void rescanForBanks(Bank &bank, std::function<void(const char*)> cb)
{
    bank.rescanforbanks();
    bankList(bank, cb);
}

void loadBank(Bank &bank, int pos, std::function<void(const char*)> cb)
{
    if(bank.bankpos != pos) {
        bank.bankpos = pos;
        bank.loadbank(bank.banks[pos].dir);
        for(int i=0; i<BANK_SIZE; ++i)
            refreshBankView(bank, i, cb);
    }
}

void bankPos(Bank &bank, std::function<void(const char *)> cb)
{
    char response[2048];

    if(!rtosc_message(response, 2048, "/loadbank", "i", bank.bankpos))
        errx(1, "Failure to handle bank update properly...");
    if(cb)
        cb(response);
}

/*****************************************************************************
 *                      Data Object for Non-RT Class Dispatch                *
 *****************************************************************************/

class DummyDataObj:public rtosc::RtData
{
    public:
        DummyDataObj(char *loc_, size_t loc_size_, void *obj_, MiddleWareImpl *mwi_,
                rtosc::ThreadLink *uToB_)
        {
            memset(loc_, 0, sizeof(loc_size_));
            buffer = new char[4*4096];
            memset(buffer, 0, 4*4096);
            loc      = loc_;
            loc_size = loc_size_;
            obj      = obj_;
            mwi      = mwi_;
            uToB     = uToB_;
        }
        ~DummyDataObj(void)
        {
            delete[] buffer;
        }

        virtual void reply(const char *path, const char *args, ...)
        {
            //printf("reply building '%s'\n", path);
            va_list va;
            va_start(va,args);
            if(!strcmp(path, "/forward")) { //forward the information to the backend
                args++;
                path = va_arg(va, const char *);
                //fprintf(stderr, "forwarding information to the backend on '%s'<%s>\n",
                //        path, args);
                rtosc_vmessage(buffer,4*4096,path,args,va);
                uToB->raw_write(buffer);
            } else {
                //printf("path = '%s' args = '%s'\n", path, args);
                //printf("buffer = '%p'\n", buffer);
                rtosc_vmessage(buffer,4*4096,path,args,va);
                //printf("buffer = '%s'\n", buffer);
                reply(buffer);
            }
            va_end(va);
        }
        virtual void reply(const char *msg);
        //virtual void broadcast(const char *path, const char *args, ...){(void)path;(void)args;};
        //virtual void broadcast(const char *msg){(void)msg;};
    private:
        char *buffer;
        MiddleWareImpl   *mwi;
        rtosc::ThreadLink *uToB;
};


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
    using rtosc::rtMsg;
    rtosc::Ports ports = {
        {"sink-list:", 0, 0, [](const char *msg, rtosc::RtData &d) {
                auto list = Nio::getSinks();
                char *ret = rtosc_splat(d.loc, list);
                d.reply(ret);
                delete [] ret;
            }},
        {"source-list:", 0, 0, [](const char *msg, rtosc::RtData &d) {
                auto list = Nio::getSources();
                char *ret = rtosc_splat(d.loc, list);
                d.reply(ret);
                delete [] ret;
            }},
        {"source::s", 0, 0, [](const char *msg, rtosc::RtData &d) {
                if(rtosc_narguments(msg) == 0)
                    d.reply(d.loc, "s", Nio::getSource().c_str());
                else if(rtMsg<const char*> m{msg})
                    Nio::setSource(get<0>(m));}},
        {"sink::s", 0, 0, [](const char *msg, rtosc::RtData &d) {
                if(rtosc_narguments(msg) == 0)
                    d.reply(d.loc, "s", Nio::getSink().c_str());
                else if(rtMsg<const char*> m{msg})
                    Nio::setSink(get<0>(m));}},
    };
}

/* Implementation */
class MiddleWareImpl : TmpFileMgr
{
    MiddleWare *parent;

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

    Config* const config;
    
public:
    MiddleWareImpl(MiddleWare *mw, SYNTH_T synth, Config* config,
                   int preferred_port);
    ~MiddleWareImpl(void);

    void warnMemoryLeaks(void);

    //Apply function while parameters are write locked
    void doReadOnlyOp(std::function<void()> read_only_fn);


    void saveBankSlot(int npart, int nslot, Master *master, Fl_Osc_Interface *osc)
    {
        int err = 0;
        doReadOnlyOp([master,nslot,npart,&err](){
                err = master->bank.savetoslot(nslot, master->part[npart]);});
        if(err) {
            char buffer[1024];
            rtosc_message(buffer, 1024, "/alert", "s",
                    "Failed To Save To Bank Slot, please check file permissions");
            GUI::raiseUi(ui, buffer);
        }
    }

    void renameBankSlot(int slot, string name, Master *master, Fl_Osc_Interface *osc)
    {
        int err = master->bank.setname(slot, name, -1);
        if(err) {
            char buffer[1024];
            rtosc_message(buffer, 1024, "/alert", "s",
                    "Failed To Rename Bank Slot, please check file permissions");
            GUI::raiseUi(ui, buffer);
        }
    }

    void swapBankSlot(int slota, int slotb, Master *master, Fl_Osc_Interface *osc)
    {
        int err = master->bank.swapslot(slota, slotb);
        if(err) {
            char buffer[1024];
            rtosc_message(buffer, 1024, "/alert", "s",
                    "Failed To Swap Bank Slots, please check file permissions");
            GUI::raiseUi(ui, buffer);
        }
    }

    void clearBankSlot(int slot, Master *master, Fl_Osc_Interface *osc)
    {
        int err = master->bank.clearslot(slot);
        if(err) {
            char buffer[1024];
            rtosc_message(buffer, 1024, "/alert", "s",
                    "Failed To Clear Bank Slot, please check file permissions");
            GUI::raiseUi(ui, buffer);
        }
    }

    void saveMaster(const char *filename)
    {
        //Copy is needed as filename WILL get trashed during the rest of the run
        std::string fname = filename;
        //printf("saving master('%s')\n", filename);
        doReadOnlyOp([this,fname](){
                int res = master->saveXML(fname.c_str());
                (void)res;
                /*printf("results: '%s' '%d'\n",fname.c_str(), res);*/});
    }

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

    void loadPart(int npart, const char *filename, Master *master, Fl_Osc_Interface *osc)
    {
        actual_load[npart]++;

        if(actual_load[npart] != pending_load[npart])
            return;
        assert(actual_load[npart] <= pending_load[npart]);

        auto alloc = std::async(std::launch::async,
                [master,filename,this,npart](){
                Part *p = new Part(*master->memory, synth,
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

        obj_store.extractPart(p, npart);
        kits.extractPart(p, npart);

        //Give it to the backend and wait for the old part to return for
        //deallocation
        uToB->write("/load-part", "ib", npart, sizeof(Part*), &p);
        GUI::raiseUi(ui, "/damage", "s", ("/part"+to_s(npart)+"/").c_str());
    }

    //Load a new cleared Part instance
    void loadClearPart(int npart)
    {
        if(npart == -1)
            return;
        Part *p = new Part(*master->memory, synth,
                           config->cfg.GzipCompression,
                           config->cfg.Interpolation,
                           &master->microtonal, master->fft);
        p->applyparameters();
        obj_store.extractPart(p, npart);
        kits.extractPart(p, npart);

        //Give it to the backend and wait for the old part to return for
        //deallocation
        uToB->write("/load-part", "ib", npart, sizeof(Part*), &p);
        GUI::raiseUi(ui, "/damage", "s", ("/part"+to_s(npart)+"/").c_str());
        //if(osc)
        //    osc->damage(("/part"+to_s(npart)+"/").c_str());
    }

    //Well, you don't get much crazier than changing out all of your RT
    //structures at once... TODO error handling
    void loadMaster(const char *filename)
    {
        Master *m = new Master(synth, config);
        m->uToB = uToB;
        m->bToU = bToU;
        if(filename) {
            m->loadXML(filename);
            m->applyparameters();
        }

        //Update resource locator table
        updateResources(m);

        master = m;

        //Give it to the backend and wait for the old part to return for
        //deallocation
        uToB->write("/load-master", "b", sizeof(Master*), &m);
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
    //If accepting undo events as user driven
    bool recording_undo = true;
    void bToUhandle(const char *rtmsg, bool dummy=false);

    void tick(void)
    {
        while(lo_server_recv_noblock(server, 0));
        while(bToU->hasNext()) {
            const char *rtmsg = bToU->read();
            bToUhandle(rtmsg);
        }
    }

    bool handlePAD(string path, const char *msg, void *v)
    {
        if(!v)
            return true;
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        DummyDataObj d(buffer, 1024, v, this, uToB);
        strcpy(buffer, path.c_str());

        PADnoteParameters::ports.dispatch(msg, d);
        if(!d.matches) {
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
            fprintf(stderr, "Unknown location '%s%s'<%s>\n",
                    path.c_str(), msg, rtosc_argument_string(msg));
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
        }

        return true;
    }

    void handlePresets(const char *msg)
    {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        DummyDataObj d(buffer, 1024, (void*)parent, this, uToB);
        strcpy(buffer, "/presets/");

        //012345678
        ///presets/
        real_preset_ports.dispatch(msg+9, d);
        //printf("Something <%s>\n", msg+9);
        if(strstr(msg, "paste") && rtosc_argument_string(msg)[0] == 's') {
            char buffer[1024];
            rtosc_message(buffer, 1024, "/damage", "s",
                    rtosc_argument(msg, 0).s);
            GUI::raiseUi(ui, buffer);
        }


        if(!d.matches) {
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
            fprintf(stderr, "Unknown location '%s'<%s>\n",
                    msg, rtosc_argument_string(msg));
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
        }
    }

    void handleIo(const char *msg)
    {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        DummyDataObj d(buffer, 1024, (void*)&config, this, uToB);
        strcpy(buffer, "/io/");

        Nio::ports.dispatch(msg+4, d);
        if(!d.matches) {
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
            fprintf(stderr, "Unknown location '%s'<%s>\n",
                    msg, rtosc_argument_string(msg));
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
        }
    }

    void handleConfig(const char *msg)
    {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        DummyDataObj d(buffer, 1024, (void*)config, this, uToB);
        strcpy(buffer, "/config/");

        Config::ports.dispatch(msg+8, d);
        if(!d.matches) {
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
            fprintf(stderr, "Unknown location '%s'<%s>\n",
                    msg, rtosc_argument_string(msg));
            fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
        }
    }

    bool handleOscil(string path, const char *msg, void *v);

    void kitEnable(const char *msg);
    void kitEnable(int part, int kit, int type);

    // Handle an event with special cases
    void handleMsg(const char *msg);


    void write(const char *path, const char *args, ...);
    void write(const char *path, const char *args, va_list va);


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

    rtosc::UndoHistory undo;

    //Link To the Realtime
    rtosc::ThreadLink *bToU;
    rtosc::ThreadLink *uToB;

    //LIBLO
    lo_server server;
    string last_url, curr_url;

    //Synthesis Rate Parameters
    const SYNTH_T synth;
    
    PresetsStore presetsstore;
};

MiddleWareImpl::MiddleWareImpl(MiddleWare *mw, SYNTH_T synth_,
    Config* config, int preferrred_port)
    :parent(mw), config(config), ui(nullptr), synth(std::move(synth_)),
    presetsstore(*config)
{
    bToU = new rtosc::ThreadLink(4096*2,1024);
    uToB = new rtosc::ThreadLink(4096*2,1024);
    if(preferrred_port != -1)
        server = lo_server_new_with_proto(to_s(preferrred_port).c_str(),
                                          LO_UDP, liblo_error_cb);
    else
        server = lo_server_new_with_proto(NULL, LO_UDP, liblo_error_cb);
    lo_server_add_method(server, NULL, NULL, handler_function, mw);
    fprintf(stderr, "lo server running on %d\n", lo_server_get_port(server));

#ifndef PLUGINVERSION
    if(!isPlugin()) {
        clean_up_tmp_nams();
        create_tmp_file((unsigned)lo_server_get_port(server));
    }
#endif

    //dummy callback for starters
    cb = [](void*, const char*){};
    idle = 0;
    idle_ptr = 0;

#ifndef PLUGINVERSION
    the_bToU = bToU;
#endif
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

void DummyDataObj::reply(const char *msg)
{
    mwi->bToUhandle(msg, true);
}
MiddleWareImpl::~MiddleWareImpl(void)
{
    remove(get_tmp_nam().c_str());

    warnMemoryLeaks();

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

void MiddleWareImpl::bToUhandle(const char *rtmsg, bool dummy)
{
    assert(strcmp(rtmsg, "/part0/kit0/Ppadenableda"));
    assert(strcmp(rtmsg, "/ze_state"));
    //Dump Incomming Events For Debugging
    if(strcmp(rtmsg, "/vu-meter") && false) {
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 1 + 30, 0 + 40);
        fprintf(stdout, "frontend: '%s'<%s>\n", rtmsg,
                rtosc_argument_string(rtmsg));
        fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
    }

    //Activity dot
    //printf(".");fflush(stdout);

    if(!strcmp(rtmsg, "/echo")
            && !strcmp(rtosc_argument_string(rtmsg),"ss")
            && !strcmp(rtosc_argument(rtmsg,0).s, "OSC_URL"))
        curr_url = rtosc_argument(rtmsg,1).s;
    else if(!strcmp(rtmsg, "/free")
            && !strcmp(rtosc_argument_string(rtmsg),"sb")) {
        deallocate(rtosc_argument(rtmsg, 0).s, *((void**)rtosc_argument(rtmsg, 1).b.data));
    } else if(!strcmp(rtmsg, "/request-memory")) {
        //Generate out more memory for the RT memory pool
        //5MBi chunk
        size_t N  = 5*1024*1024;
        void *mem = malloc(N);
        uToB->write("/add-rt-memory", "bi", sizeof(void*), &mem, N);
    } else if(!strcmp(rtmsg, "/setprogram")
            && !strcmp(rtosc_argument_string(rtmsg),"cc")) {
        loadPart(rtosc_argument(rtmsg,0).i, master->bank.ins[rtosc_argument(rtmsg,1).i].filename.c_str(), master, osc);
    } else if(!strcmp("/undo_pause", rtmsg)) {
        recording_undo = false;
    } else if(!strcmp("/undo_resume", rtmsg)) {
        recording_undo = true;
    } else if(!strcmp("undo_change", rtmsg) && recording_undo) {
        undo.recordEvent(rtmsg);
    } else if(!strcmp(rtmsg, "/broadcast")) {
        broadcast = true;
    } else if(broadcast) {
        broadcast = false;
#ifdef PLUGINVERSION
        if (!curr_url.empty()) // falktx: check added
            cb(ui, rtmsg);

        // falktx: changed curr_url to last_url
        if(last_url != "GUI") {
            lo_message msg  = lo_message_deserialise((void*)rtmsg,
                    rtosc_message_length(rtmsg, bToU->buffer_size()), NULL);

            //Send to known url
            if(!last_url.empty()) {
                lo_address addr = lo_address_new_from_url(last_url.c_str());
                lo_send_message(addr, rtmsg, msg);
            }
        }
#else
        cb(ui, rtmsg);

        if(curr_url != "GUI") {
            lo_message msg  = lo_message_deserialise((void*)rtmsg,
                    rtosc_message_length(rtmsg, bToU->buffer_size()), NULL);

            //Send to known url
            if(!curr_url.empty()) {
                lo_address addr = lo_address_new_from_url(curr_url.c_str());
                lo_send_message(addr, rtmsg, msg);
            }
        }
#endif
    } else if((dummy?last_url:curr_url) == "GUI" || !strcmp(rtmsg, "/close-ui")) {
        cb(ui, rtmsg);
    } else{
        lo_message msg  = lo_message_deserialise((void*)rtmsg,
                rtosc_message_length(rtmsg, bToU->buffer_size()), NULL);

        //Send to known url
        if(!curr_url.empty()) {
            lo_address addr = lo_address_new_from_url(dummy?last_url.c_str():curr_url.c_str());
            lo_send_message(addr, rtmsg, msg);
        }
    }
}

bool MiddleWareImpl::handleOscil(string path, const char *msg, void *v)
{
    //printf("handleOscil...\n");
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    DummyDataObj d(buffer, 1024, v, this, uToB);
    strcpy(buffer, path.c_str());
    if(!v)
        return true;

    //Paste To Non-Realtime Parameters and then forward
    if(strstr(msg, "paste") && !strstr(msg, "padpars")) {
    }

    if(!strstr(msg, "padpars")) {
        for(auto &p:OscilGen::ports.ports) {
            if(strstr(p.name,msg) && strstr(p.metadata, "realtime") &&
                    !strcmp("b", rtosc_argument_string(msg))) {
                //printf("sending along packet '%s'...\n", msg);
                return false;
            }
        }
    }

    OscilGen::ports.dispatch(msg, d);
    if(!d.matches) {
        //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
        //fprintf(stderr, "Unknown location '%s%s'<%s>\n",
        //        path.c_str(), msg, rtosc_argument_string(msg));
        //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
    }

    return true;
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

    if(type == -1)
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

/* BASE/part#/kititem#
 * BASE/part#/kit#/adpars/voice#/oscil/\*
 * BASE/part#/kit#/adpars/voice#/mod-oscil/\*
 * BASE/part#/kit#/padpars/prepare
 * BASE/part#/kit#/padpars/oscil/\*
 */
void MiddleWareImpl::handleMsg(const char *msg)
{
    assert(msg && *msg && rindex(msg, '/')[1]);
    assert(strstr(msg,"free") == NULL || strstr(rtosc_argument_string(msg), "b") == NULL);
    assert(strcmp(msg, "/part0/Psysefxvol"));
    assert(strcmp(msg, "/Penabled"));
    assert(strcmp(msg, "part0/part0/Ppanning"));
    assert(strcmp(msg, "sysefx0sysefx0/preset"));
    assert(strcmp(msg, "/sysefx0preset"));
    assert(strcmp(msg, "Psysefxvol0/part0"));
    //fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 6 + 30, 0 + 40);
    //fprintf(stdout, "middleware: '%s':%s\n", msg, rtosc_argument_string(msg));
    //fprintf(stdout, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
    const char *last_path = rindex(msg, '/');
    if(!last_path)
        return;


    //printf("watching '%s' go by\n", msg);
    //Get the object resource locator
    string obj_rl(msg, last_path+1);
    int npart = -1;
    char testchr = 0;

    std::function<void(const char*)> bank_cb;
    if(last_url == "GUI")
        bank_cb = [this](const char *msg){if(osc)osc->tryLink(msg);};
    else
        bank_cb = [this](const char *msg){this->bToUhandle(msg, 1);};

    if(!strcmp(msg, "/refresh_bank") && !strcmp(rtosc_argument_string(msg), "i")) {
        refreshBankView(master->bank, rtosc_argument(msg,0).i, bank_cb);
    } else if(!strcmp(msg, "/bank-list") && !strcmp(rtosc_argument_string(msg), "")) {
        bankList(master->bank, bank_cb);
    } else if(!strcmp(msg, "/rescanforbanks") && !strcmp(rtosc_argument_string(msg), "")) {
        rescanForBanks(master->bank, bank_cb);
    } else if(!strcmp(msg, "/loadbank") && !strcmp(rtosc_argument_string(msg), "i")) {
        loadBank(master->bank, rtosc_argument(msg, 0).i, bank_cb);
    } else if(!strcmp(msg, "/loadbank") && !strcmp(rtosc_argument_string(msg), "")) {
        bankPos(master->bank, bank_cb);
    } else if(obj_store.has(obj_rl)) {
        //try some over simplified pattern matching
        if(strstr(msg, "oscilgen/") || strstr(msg, "FMSmp/") || strstr(msg, "OscilSmp/")) {
            if(!handleOscil(obj_rl, last_path+1, obj_store.get(obj_rl)))
                uToB->raw_write(msg);
            //else if(strstr(obj_rl.c_str(), "kititem"))
            //    handleKitItem(obj_rl, objmap[obj_rl],atoi(rindex(msg,'m')+1),rtosc_argument(msg,0).T);
        } else if(strstr(msg, "padpars/prepare"))
            preparePadSynth(obj_rl,(PADnoteParameters *) obj_store.get(obj_rl), uToB);
        else if(strstr(msg, "padpars")) {
            if(!handlePAD(obj_rl, last_path+1, obj_store.get(obj_rl)))
                uToB->raw_write(msg);
        } else //just forward the message
            uToB->raw_write(msg);
    } else if(strstr(msg, "/save_xmz") && !strcmp(rtosc_argument_string(msg), "s")) {
        saveMaster(rtosc_argument(msg,0).s);
    } else if(strstr(msg, "/save_xiz") && !strcmp(rtosc_argument_string(msg), "is")) {
        savePart(rtosc_argument(msg,0).i,rtosc_argument(msg,1).s);
    } else if(strstr(msg, "/load_xmz") && !strcmp(rtosc_argument_string(msg), "s")) {
        loadMaster(rtosc_argument(msg,0).s);
    } else if(strstr(msg, "/reset_master") && !strcmp(rtosc_argument_string(msg), "")) {
        loadMaster(NULL);
    } else if(!strcmp(msg, "/load_xiz") && !strcmp(rtosc_argument_string(msg), "is")) {
        pending_load[rtosc_argument(msg,0).i]++;
        loadPart(rtosc_argument(msg,0).i, rtosc_argument(msg,1).s, master, osc);
    } else if(strstr(msg, "load-part") && !strcmp(rtosc_argument_string(msg), "is")) {
        pending_load[rtosc_argument(msg,0).i]++;
        loadPart(rtosc_argument(msg,0).i, rtosc_argument(msg,1).s, master, osc);
    } else if(!strcmp(msg, "/setprogram")
            && !strcmp(rtosc_argument_string(msg),"c")) {
        pending_load[0]++;
        loadPart(0, master->bank.ins[rtosc_argument(msg,0).i].filename.c_str(), master, osc);
    } else if(strstr(msg, "save-bank-part") && !strcmp(rtosc_argument_string(msg), "ii")) {
        saveBankSlot(rtosc_argument(msg,0).i, rtosc_argument(msg,1).i, master, osc);
    } else if(strstr(msg, "bank-rename") && !strcmp(rtosc_argument_string(msg), "is")) {
        renameBankSlot(rtosc_argument(msg,0).i, rtosc_argument(msg,1).s, master, osc);
    } else if(strstr(msg, "swap-bank-slots") && !strcmp(rtosc_argument_string(msg), "ii")) {
        swapBankSlot(rtosc_argument(msg,0).i, rtosc_argument(msg,1).i, master, osc);
    } else if(strstr(msg, "clear-bank-slot") && !strcmp(rtosc_argument_string(msg), "i")) {
        clearBankSlot(rtosc_argument(msg,0).i, master, osc);
    } else if(strstr(msg, "/config/")) {
        handleConfig(msg);
    } else if(strstr(msg, "/presets/")) {
        handlePresets(msg);
    } else if(strstr(msg, "/io/")) {
        handleIo(msg);
    } else if(strstr(msg, "Padenabled") || strstr(msg, "Ppadenabled") || strstr(msg, "Psubenabled")) {
        kitEnable(msg);
        uToB->raw_write(msg);
    } else if(sscanf(msg, "/part%d/clea%c", &npart, &testchr) == 2 && testchr == 'r') {
        loadClearPart(npart);
    } else if(!strcmp(msg, "/undo")) {
        undo.seekHistory(-1);
    } else if(!strcmp(msg, "/redo")) {
        undo.seekHistory(+1);
    } else
        uToB->raw_write(msg);
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

void MiddleWareImpl::warnMemoryLeaks(void)
{}

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

void MiddleWare::transmitMsg(const char *path, const char *args, va_list va)
{
    char buffer[1024];
    if(rtosc_vmessage(buffer, 1024, path, args, va))
        transmitMsg(buffer);
    else
        fprintf(stderr, "Error in transmitMsg(va)n");
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
    return lo_server_get_url(impl->server);
}

const PresetsStore& MiddleWare::getPresetsStore() const
{
    return impl->presetsstore;
}

PresetsStore& MiddleWare::getPresetsStore()
{
    return impl->presetsstore;
}
