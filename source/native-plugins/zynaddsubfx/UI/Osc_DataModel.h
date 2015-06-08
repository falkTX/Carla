#pragma once
#include "Fl_Osc_Widget.H"
#include <functional>
#include <vector>
#include <rtosc/rtosc.h>

class Osc_DataModel:public Fl_Osc_Widget
{
    public:
        Osc_DataModel(Fl_Osc_Interface *osc_)
            :Fl_Osc_Widget("", osc_)
        {
            assert(osc);
        }

        typedef std::string value_t;
        value_t value;
        std::function<void(value_t)> callback;

        void update(std::string url)
        {
            if(!ext.empty())
                osc->removeLink(this);
            ext = url;
            value = "";

            oscRegister(ext.c_str());
        }
        
        //Raw messages
        virtual void OSC_raw(const char *msg)
        {
            std::string args = rtosc_argument_string(msg);
            if(args == "s") {
                value = rtosc_argument(msg, 0).s;
                if(callback)
                    callback(value);
            }
        }
};
