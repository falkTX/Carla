/*
  ZynAddSubFX - a software synthesizer

  guimain.cpp  -  Main file of synthesizer GUI
  Copyright (C) 2015 Mark McCurry

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

#include <lo/lo.h>
#include <string>

//GUI System
#include "Connection.h"
#include "NSM.H"

#include <sys/stat.h>
GUI::ui_handle_t gui = 0;
#if USE_NSM
NSM_Client *nsm = 0;
#endif
lo_server server;
std::string sendtourl;

/*
 * Program exit
 */
void exitprogram()
{
    GUI::destroyUi(gui);
}

bool fileexists(const char *filename)
{
    struct stat tmp;
    int result = stat(filename, &tmp);
    if(result >= 0)
        return true;

    return false;
}

int Pexitprogram = 0;


#include "Connection.h"
#include "Fl_Osc_Interface.h"
#include "../globals.h"
#include <map>
#include <cassert>

#include <rtosc/rtosc.h>
#include <rtosc/ports.h>

#include <FL/Fl.H>
#include "Fl_Osc_Tree.H"
#include "common.H"
#include "MasterUI.h"

#ifdef NTK_GUI
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Tiled_Image.H>
#include <FL/Fl_Dial.H>
#include <err.h>
#endif // NTK_GUI

#ifndef NO_UI
#include "Fl_Osc_Widget.H"
#endif

using namespace GUI;
class MasterUI *ui=0;

#ifdef NTK_GUI
static Fl_Tiled_Image *module_backdrop;
#endif

int undo_redo_handler(int)
{
    const bool undo_ = Fl::event_ctrl() && Fl::event_key() == 'z';
    const bool redo = Fl::event_ctrl() && Fl::event_key() == 'r';
    if(undo_)
        ui->osc->write("/undo", "");
    else if(redo)
        ui->osc->write("/redo", "");
    return undo_ || redo;
}

void
set_module_parameters ( Fl_Widget *o )
{
#ifdef NTK_GUI
    o->box( FL_DOWN_FRAME );
    o->align( o->align() | FL_ALIGN_IMAGE_BACKDROP );
    o->color( FL_BLACK );
    o->image( module_backdrop );
    o->labeltype( FL_SHADOW_LABEL );
    if(o->parent()) {
        o->parent()->labeltype(FL_NO_LABEL);
        o->parent()->box(FL_NO_BOX);
    }
#else
    o->box( FL_PLASTIC_UP_BOX );
    o->color( FL_CYAN );
    o->labeltype( FL_EMBOSSED_LABEL );
#endif
}

ui_handle_t GUI::createUi(Fl_Osc_Interface *osc, void *exit)
{
#ifdef NTK_GUI
    fl_register_images();

    Fl_Dial::default_style(Fl_Dial::PIXMAP_DIAL);

#ifdef CARLA_VERSION_STRING
    if(Fl_Shared_Image *img = Fl_Shared_Image::get(gUiPixmapPath + "/knob.png"))
        Fl_Dial::default_image(img);
#else
    if(Fl_Shared_Image *img = Fl_Shared_Image::get(PIXMAP_PATH "/knob.png"))
        Fl_Dial::default_image(img);
#endif
    else if(Fl_Shared_Image *img = Fl_Shared_Image::get(SOURCE_DIR "/pixmaps/knob.png"))
        Fl_Dial::default_image(img);
    else
        errx(1, "ERROR: Cannot find pixmaps/knob.png");


#ifdef CARLA_VERSION_STRING
    if(Fl_Shared_Image *img = Fl_Shared_Image::get(gUiPixmapPath + "/window_backdrop.png"))
        Fl::scheme_bg(new Fl_Tiled_Image(img));
#else
    if(Fl_Shared_Image *img = Fl_Shared_Image::get(PIXMAP_PATH "/window_backdrop.png"))
        Fl::scheme_bg(new Fl_Tiled_Image(img));
#endif
    else if(Fl_Shared_Image *img = Fl_Shared_Image::get(SOURCE_DIR "/pixmaps/window_backdrop.png"))
        Fl::scheme_bg(new Fl_Tiled_Image(img));
    else
        errx(1, "ERROR: Cannot find pixmaps/window_backdrop.png");

#ifdef CARLA_VERSION_STRING
    if(Fl_Shared_Image *img = Fl_Shared_Image::get(gUiPixmapPath + "/module_backdrop.png"))
        module_backdrop = new Fl_Tiled_Image(img);
#else
    if(Fl_Shared_Image *img = Fl_Shared_Image::get(PIXMAP_PATH "/module_backdrop.png"))
        module_backdrop = new Fl_Tiled_Image(img);
#endif
    else if(Fl_Shared_Image *img = Fl_Shared_Image::get(SOURCE_DIR "/pixmaps/module_backdrop.png"))
        module_backdrop = new Fl_Tiled_Image(img);
    else
        errx(1, "ERROR: Cannot find pixmaps/module_backdrop");

    Fl::background(50,  50,  50);
    Fl::background2(70, 70,  70);
    Fl::foreground(255, 255, 255);
#endif

    //Fl_Window *midi_win = new Fl_Double_Window(400, 400, "Midi connections");
    //Fl_Osc_Tree *tree   = new Fl_Osc_Tree(0,0,400,400);
    //midi_win->resizable(tree);
    //tree->root_ports    = &Master::ports;
    //tree->osc           = osc;
    //midi_win->show();

    Fl::add_handler(undo_redo_handler);
    return (void*) (ui = new MasterUI((int*)exit, osc));
}
void GUI::destroyUi(ui_handle_t ui)
{
    delete static_cast<MasterUI*>(ui);
}

#define BEGIN(x) {x,":non-realtime\0",NULL,[](const char *m, rtosc::RtData d){ \
    MasterUI *ui   = static_cast<MasterUI*>(d.obj); \
    rtosc_arg_t a0 = {0}, a1 = {0}; \
    if(rtosc_narguments(m) > 0) \
        a0 = rtosc_argument(m,0); \
    if(rtosc_narguments(m) > 1) \
        a1 = rtosc_argument(m,1); \
    (void)ui;(void)a1;(void)a0;

#define END }},

struct uiPorts {
    static rtosc::Ports ports;
};

//DSL based ports
rtosc::Ports uiPorts::ports = {
    BEGIN("show:i") {
        ui->showUI(a0.i);
    } END
    BEGIN("alert:s") {
        fl_alert("%s",a0.s);
    } END
    BEGIN("session-type:s") {
        if(strcmp(a0.s,"LASH"))
            return;
        ui->sm_indicator1->value(1);
        ui->sm_indicator2->value(1);
        ui->sm_indicator1->tooltip("LASH");
        ui->sm_indicator2->tooltip("LASH");
    } END
    BEGIN("save-master:s") {
        ui->do_save_master(a0.s);
    } END
    BEGIN("load-master:s") {
        ui->do_load_master(a0.s);
    } END
    BEGIN("vu-meter:bb") {
#ifdef DEBUG
        printf("Vu meter handler...\n");
#endif
        if(a0.b.len == sizeof(vuData) &&
                a1.b.len == sizeof(float)*NUM_MIDI_PARTS) {
#ifdef DEBUG
            printf("Normal behavior...\n");
#endif
            //Refresh the primary VU meters
            ui->simplemastervu->update((vuData*)a0.b.data);
            ui->mastervu->update((vuData*)a0.b.data);

            float *partvu = (float*)a1.b.data;
            for(int i=0; i<NUM_MIDI_PARTS; ++i)
                ui->panellistitem[i]->partvu->update(partvu[i]);
        }
    } END
    BEGIN("close-ui") {
        ui->close();
    } END
};


void GUI::raiseUi(ui_handle_t gui, const char *message)
{
    if(!gui)
        return;
    MasterUI *mui = (MasterUI*)gui;
    mui->osc->tryLink(message);
#ifdef DEBUG
    printf("got message for UI '%s:%s'\n", message, rtosc_argument_string(message));
#endif
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    rtosc::RtData d;
    d.loc = buffer;
    d.loc_size = 1024;
    d.obj = gui;
    uiPorts::ports.dispatch(message+1, d);
}

void GUI::raiseUi(ui_handle_t gui, const char *dest, const char *args, ...)
{
    char buffer[1024];
    va_list va;
    va_start(va,args);
    if(rtosc_vmessage(buffer,1024,dest,args,va))
        raiseUi(gui, buffer);
    va_end(va);
}

void GUI::tickUi(ui_handle_t)
{
    Fl::wait(0.02f);
}

/******************************************************************************
 *    OSC Interface For User Interface                                        *
 *                                                                            *
 *    This is a largely out of date section of code                           *
 *    Most type specific write methods are no longer used                     *
 *    See UI/Fl_Osc_* to see what is actually used in this interface          *
 ******************************************************************************/
class UI_Interface:public Fl_Osc_Interface
{
    public:
        UI_Interface()
        {}

        void transmitMsg(const char *path, const char *args, ...)
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

        void transmitMsg(const char *rtmsg)
        {
            //Send to known url
            if(!sendtourl.empty()) {
                lo_message msg  = lo_message_deserialise((void*)rtmsg,
                        rtosc_message_length(rtmsg, rtosc_message_length(rtmsg,-1)), NULL);
                lo_address addr = lo_address_new_from_url(sendtourl.c_str());
                lo_send_message(addr, rtmsg, msg);
            }
        }

        void requestValue(string s) override
        {
            //printf("Request Value '%s'\n", s.c_str());
            assert(s!="/Psysefxvol-1/part0");
            //Fl_Osc_Interface::requestValue(s);
            /*
            if(impl->activeUrl() != "GUI") {
                impl->transmitMsg("/echo", "ss", "OSC_URL", "GUI");
                impl->activeUrl("GUI");
            }*/

            transmitMsg(s.c_str(),"");
        }

        void write(string s, const char *args, ...) override
        {
            char buffer[4096];
            va_list va;
            va_start(va, args);
            rtosc_vmessage(buffer, sizeof(buffer), s.c_str(), args, va);
            //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 4 + 30, 0 + 40);
            ////fprintf(stderr, ".");
            //fprintf(stderr, "write(%s:%s)\n", s.c_str(), args);
            //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
            transmitMsg(buffer);
            va_end(va);
        }

        void writeRaw(const char *msg) override
        {
            //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 4 + 30, 0 + 40);
            ////fprintf(stderr, ".");
            //fprintf(stderr, "rawWrite(%s:%s)\n", msg, rtosc_argument_string(msg));
            //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
            transmitMsg(msg);
        }

        void writeValue(string s, string ss) override
        {
            //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 4 + 30, 0 + 40);
            //fprintf(stderr, "writevalue<string>(%s,%s)\n", s.c_str(),ss.c_str());
            //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
            transmitMsg(s.c_str(), "s", ss.c_str());
        }

        void writeValue(string s, char c) override
        {
            //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 4 + 30, 0 + 40);
            //fprintf(stderr, "writevalue<char>(%s,%d)\n", s.c_str(),c);
            //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
            transmitMsg(s.c_str(), "c", c);
        }

        void writeValue(string s, float f) override
        {
            //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 4 + 30, 0 + 40);
            //fprintf(stderr, "writevalue<float>(%s,%f)\n", s.c_str(),f);
            //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
            transmitMsg(s.c_str(), "f", f);
        }

        void createLink(string s, class Fl_Osc_Widget*w) override
        {
            assert(s.length() != 0);
            Fl_Osc_Interface::createLink(s,w);
            assert(!strstr(s.c_str(), "/part0/kit-1"));
            map.insert(std::pair<string,Fl_Osc_Widget*>(s,w));
        }

        void renameLink(string old, string newer, Fl_Osc_Widget *w) override
        {
            fprintf(stdout, "renameLink('%s','%s',%p)\n",
                    old.c_str(), newer.c_str(), w);
            removeLink(old, w);
            createLink(newer, w);
        }

        void removeLink(string s, class Fl_Osc_Widget*w) override
        {
            for(auto i = map.begin(); i != map.end(); ++i) {
                if(i->first == s && i->second == w) {
                    map.erase(i);
                    break;
                }
            }
            //printf("[%d] removing '%s' (%p)...\n", map.size(), s.c_str(), w);
        }

        virtual void removeLink(class Fl_Osc_Widget *w) override
        {
            bool processing = true;
            while(processing)
            {
                //Verify Iterator invalidation sillyness
                processing = false;//Exit if no new elements are found
                for(auto i = map.begin(); i != map.end(); ++i) {
                    if(i->second == w) {
                        //printf("[%d] removing '%s' (%p)...\n", map.size()-1,
                        //        i->first.c_str(), w);
                        map.erase(i);
                        processing = true;
                        break;
                    }
                }
            }
        }

        //A very simplistic implementation of a UI agnostic refresh method
        virtual void damage(const char *path) override
        {
            //printf("\n\nDamage(\"%s\")\n", path);
            for(auto pair:map) {
                if(strstr(pair.first.c_str(), path)) {
                    auto *tmp = dynamic_cast<Fl_Widget*>(pair.second);
                    //if(tmp)
                    //    printf("%x, %d %d [%s]\n", (int)pair.second, tmp->visible_r(), tmp->visible(), pair.first.c_str());
                    //else
                    //    printf("%x, (NULL)[%s]\n", (int)pair.second,pair.first.c_str());
                    if(!tmp || tmp->visible_r())
                        pair.second->update();
                }
            }
        }

        void tryLink(const char *msg) override
        {

            //DEBUG
            //if(strcmp(msg, "/vu-meter"))//Ignore repeated message
            //    printf("trying the link for a '%s'<%s>\n", msg, rtosc_argument_string(msg));
            const char *handle = rindex(msg,'/');
            if(handle)
                ++handle;

            int found_count = 0;

            auto range = map.equal_range(msg);
            for(auto itr = range.first; itr != range.second; ++itr) {
                auto widget = itr->second;
                found_count++;
                const char *arg_str = rtosc_argument_string(msg);

                //Always provide the raw message
                widget->OSC_raw(msg);

                if(!strcmp(arg_str, "b")) {
                    widget->OSC_value(rtosc_argument(msg,0).b.len,
                            rtosc_argument(msg,0).b.data,
                            handle);
                } else if(!strcmp(arg_str, "c")) {
                    widget->OSC_value((char)rtosc_argument(msg,0).i,
                            handle);
                } else if(!strcmp(arg_str, "s")) {
                    widget->OSC_value((const char*)rtosc_argument(msg,0).s,
                            handle);
                } else if(!strcmp(arg_str, "i")) {
                    widget->OSC_value((int)rtosc_argument(msg,0).i,
                            handle);
                } else if(!strcmp(arg_str, "f")) {
                    widget->OSC_value((float)rtosc_argument(msg,0).f,
                            handle);
                } else if(!strcmp(arg_str, "T") || !strcmp(arg_str, "F")) {
                    widget->OSC_value((bool)rtosc_argument(msg,0).T, handle);
                }
            }

            if(found_count == 0
                    && strcmp(msg, "/vu-meter")
                    && strcmp(msg, "undo_change")
                    && !strstr(msg, "parameter")
                    && !strstr(msg, "Prespoint")) {
                //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 1, 7 + 30, 0 + 40);
                //fprintf(stderr, "Unknown widget '%s'\n", msg);
                //fprintf(stderr, "%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
            }
        };

        void dumpLookupTable(void)
        {
            if(!map.empty()) {
                printf("Leaked controls:\n");
                for(auto i = map.begin(); i != map.end(); ++i) {
                    printf("Known control  '%s' (%p)...\n", i->first.c_str(), i->second);
                }
            }
        }


    private:
        std::multimap<string,Fl_Osc_Widget*> map;
};

Fl_Osc_Interface *GUI::genOscInterface(MiddleWare *)
{
    return new UI_Interface();
}

static void liblo_error_cb(int i, const char *m, const char *loc)
{
    fprintf(stderr, "liblo :-( %d-%s@%s\n",i,m,loc);
}

static int handler_function(const char *path, const char *types, lo_arg **argv,
        int argc, lo_message msg, void *user_data)
{
    (void) types;
    (void) argv;
    (void) argc;
    (void) user_data;
    char buffer[8192];
    memset(buffer, 0, sizeof(buffer));
    size_t size = sizeof(buffer);
    assert(lo_message_length(msg, path) <= sizeof(buffer));
    lo_message_serialise(msg, path, buffer, &size);
    assert(size <= sizeof(buffer));
    raiseUi(gui, buffer);

    return 0;
}

#ifndef CARLA_VERSION_STRING
int main(int argc, char *argv[])
{
    //Startup Liblo Link
    if(argc == 2) {
        server = lo_server_new_with_proto(NULL, LO_UDP, liblo_error_cb);
        lo_server_add_method(server, NULL, NULL, handler_function, 0);
        sendtourl = argv[1];
    }
    
    gui = GUI::createUi(new UI_Interface(), &Pexitprogram);

    GUI::raiseUi(gui, "/show",  "i", 1);
    while(Pexitprogram == 0) {
        if(server)
            while(lo_server_recv_noblock(server, 0));
        GUI::tickUi(gui);
    }

    exitprogram();
    return 0;
}
#endif
