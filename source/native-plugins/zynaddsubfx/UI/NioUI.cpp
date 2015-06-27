#include "NioUI.h"
#include "../Nio/Nio.h"
#include <cstdio>
#include <iostream>
#include <cstring>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Spinner.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Display.H>
#include "Osc_SimpleListModel.h"

using namespace std;

static void callback_fn_choice_nio(Fl_Widget *w, void *);
class Fl_Osc_StrChoice:public Fl_Choice, public Fl_Osc_Widget
{
    public:
        Fl_Osc_StrChoice(int X, int Y, int W, int H, const char *label = NULL)
        :Fl_Choice(X,Y,W,H, label), Fl_Osc_Widget(this), cb_data(NULL, NULL)
        {
            Fl_Choice::callback(callback_fn_choice_nio, NULL);
        }

        virtual ~Fl_Osc_StrChoice(void) {};
        void init(std::string path_)
        {
            ext = path_;
            Fl_Osc_Pane *pane = fetch_osc_pane(this);
            assert(pane);
            assert(pane->osc);
            osc = pane->osc;
            oscRegister(path_.c_str());
        };


        void OSC_value(const char *S) override
        {
            for(int i=0; i<size()-1; ++i) {
                printf("size = %d, i=%d, text='%s'\n", size(), i, text(i));
                if(!strcmp(S, text(i)))
                    value(i);
            }
        }

        //Refetch parameter information
        void update(void)
        {
            assert(osc);
            oscWrite(ext);
        }
        void callback(Fl_Callback *cb, void *p = NULL)
        {
            cb_data.first = cb;
            cb_data.second = p;
        }
        
        void cb(void)
        {
            assert(osc);
            if(text(value()))
                oscWrite(ext, "s", text(value()));
            if(cb_data.first)
                cb_data.first(this, cb_data.second);
        }
    private:
        int min;
        std::pair<Fl_Callback*, void*> cb_data;
};
static void callback_fn_choice_nio(Fl_Widget *w, void *)
{
    ((Fl_Osc_StrChoice*)w)->cb();
}

NioUI::NioUI(Fl_Osc_Interface *osc)
    :Fl_Window(200, 100, 400, 400, "New IO Controls")
{
    //hm, I appear to be leaking memory
    Fl_Osc_Group *settings = new Fl_Osc_Group(0, 20, 400, 400 - 35, "Settings");
    {
        settings->osc = osc;
        audio = new Fl_Osc_StrChoice(60, 80, 100, 25, "Audio");
        //audio->callback(audioCallback);
        audio->init("/io/sink");
        midi = new Fl_Osc_StrChoice(60, 100, 100, 25, "Midi");
        //midi->callback(midiCallback);
        midi->init("/io/source");
    }
    settings->end();

    //Get options
    midi_opt  = new Osc_SimpleListModel(osc);
    audio_opt = new Osc_SimpleListModel(osc);

    using list_t = Osc_SimpleListModel::list_t;

    //initialize midi list
    midi_opt->callback = [this](list_t list) {
        printf("midi list updating...\n");
        midi->clear();
        for(auto io:list)
            midi->add(io.c_str());
    };

    //initialize audio list
    audio_opt->callback = [this](list_t list) {
        audio->clear();
        for(auto io:list)
            audio->add(io.c_str());
    };
    
    midi_opt->update("/io/source-list");
    audio_opt->update("/io/sink-list");

    resizable(this);
    size_range(400, 300);
}

NioUI::~NioUI()
{}

void NioUI::refresh()
{
    midi_opt->update("/io/source-list");
    audio_opt->update("/io/sink-list");
    midi->update();
    audio->update();
}

void NioUI::midiCallback(Fl_Widget *c)
{
    //bool good = Nio::setSource(static_cast<Fl_Choice *>(c)->text());
    //static_cast<Fl_Choice *>(c)->textcolor(fl_rgb_color(255 * !good, 0, 0));
}

void NioUI::audioCallback(Fl_Widget *c)
{
    //bool good = Nio::setSink(static_cast<Fl_Choice *>(c)->text());
    //static_cast<Fl_Choice *>(c)->textcolor(fl_rgb_color(255 * !good, 0, 0));
}
