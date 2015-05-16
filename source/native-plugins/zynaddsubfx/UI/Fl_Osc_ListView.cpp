#include "Fl_Osc_ListView.H"
#include <cstdio>
#include <rtosc/rtosc.h>

Fl_Osc_ListView::Fl_Osc_ListView(int x,int y, int w, int h, const char *label)
    :Fl_Browser(x,y,w,h,label)
{}

Fl_Osc_ListView::~Fl_Osc_ListView(void) {};
        
void Fl_Osc_ListView::init(const char *path_)
{
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    assert(pane);
    osc = pane->osc;
    loc = pane->base;
    assert(osc);
    path = path_;
    oscRegister(path_);
}
void Fl_Osc_ListView::OSC_raw(const char *msg)
{
    this->clear();
    for(int i=0; i<(int)rtosc_narguments(msg); ++i) {
        this->add(rtosc_argument(msg, i).s);
    }
}

void Fl_Osc_ListView::update(void)
{
    osc->requestValue(path.c_str());
}

void Fl_Osc_ListView::insert(std::string s, int offset)
{
    fprintf(stderr, "UNIMPLEMENTED\n");
}
void Fl_Osc_ListView::append(std::string s)
{
    fprintf(stderr, "UNIMPLEMENTED\n");
}
void Fl_Osc_ListView::doMove(int i, int j)
{
    fprintf(stderr, "UNIMPLEMENTED\n");
}
void Fl_Osc_ListView::doRemove(int offset)
{
    fprintf(stderr, "UNIMPLEMENTED\n");
}
void Fl_Osc_ListView::sendUpdate() const
{
    fprintf(stderr, "UNIMPLEMENTED\n");
}
