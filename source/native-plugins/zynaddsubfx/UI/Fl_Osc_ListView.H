#pragma once
#include <string>
#include <vector>
#include "Fl_Osc_Widget.H"
#include <FL/Fl_Browser.H>

class Fl_Osc_ListView:public Fl_Browser, public Fl_Osc_Widget
{
    public:
        Fl_Osc_ListView(int x,int y, int w, int h, const char *label=0);
        virtual ~Fl_Osc_ListView(void);
        
        void init(const char *path_);
        void OSC_raw(const char *msg);
        void update(void);

        void insert(std::string s, int offset);
        void append(std::string s);
        void doMove(int i, int j);
        void doRemove(int offset);
    private:
        void sendUpdate() const;
        std::string path;
        std::vector<std::string> data;
};
