#pragma once
#include "Fl_Osc_Widget.H"
#include <functional>
#include <vector>
#include <rtosc/rtosc.h>

class Osc_SimpleListModel:public Fl_Osc_Widget
{
    public:
        Osc_SimpleListModel(Fl_Osc_Interface *osc_)
            :Fl_Osc_Widget("", osc_), list_size(0)
        {
            assert(osc);
        }

        typedef std::vector<std::string> list_t;
        list_t list;
        std::function<void(list_t)> callback;
        unsigned list_size;

        void update(std::string url)
        {
            if(!ext.empty())
                osc->removeLink(this);
            ext = url;

            oscRegister(ext.c_str());
        }

        void apply()
        {
            if(list.size() == 0) {
                oscWrite("", "I");
            }
            char         types[list.size()+1];
            rtosc_arg_t  args[list.size()];

            //zero out data
            memset(types, 0, sizeof(types));
            memset(args,  0, sizeof(args));

            for(int i=0; i<(int)list.size(); ++i) {
                types[i]  = 's';
                args[i].s = list[i].c_str();
            }
            char buffer[1024*5];
            rtosc_amessage(buffer, sizeof(buffer), ext.c_str(), types, args);
            osc->writeRaw(buffer);
        }
        
        //Raw messages
        virtual void OSC_raw(const char *msg)
        {
            std::string args = rtosc_argument_string(msg);
            const int list_size = args.length();
            for(int i=0; i<list_size; ++i)
                if(args[i] != 's')
                    return;

            list.clear();
            list.resize(list_size);

            for(int idx=0; idx<list_size; ++idx)
                list[idx] = rtosc_argument(msg, idx).s;
            if(callback)
                callback(list);
        }
};
