#pragma once
#include "Fl_Osc_Widget.H"
#include <functional>
#include <vector>
#include <rtosc/rtosc.h>

class Osc_ListModel:public Fl_Osc_Widget
{
    public:
        Osc_ListModel(Fl_Osc_Interface *osc_)
            :Fl_Osc_Widget("", osc_), list_size(0)
        {
            assert(osc);
        }

        typedef std::vector<std::tuple<std::string, std::string, std::string>> list_t;
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
        
        //Raw messages
        virtual void OSC_raw(const char *msg)
        {
            std::string args = rtosc_argument_string(msg);
            if(args == "i") {
                list_size = rtosc_argument(msg, 0).i;
                list.clear();
                list.resize(list_size);
            } else if(args == "isss") {
                int idx = rtosc_argument(msg,0).i;
                std::get<0>(list[idx]) = rtosc_argument(msg,1).s;
                std::get<1>(list[idx]) = rtosc_argument(msg,2).s;
                std::get<2>(list[idx]) = rtosc_argument(msg,3).s;
                if(idx == (int)list_size-1 && callback)
                    callback(list);
            }
        }
};
