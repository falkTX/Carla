#include "Fl_Osc_ListView.H"
#include <cstdio>
#include <rtosc/rtosc.h>

Fl_Osc_ListView::Fl_Osc_ListView(int x,int y, int w, int h, const char *label)
    :Fl_Browser(x,y,w,h,label), data(0)
{}

Fl_Osc_ListView::~Fl_Osc_ListView(void) 
{
    delete data;
};
        
void Fl_Osc_ListView::init(const char *path_)
{
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    assert(pane);
    osc = pane->osc;
    loc = pane->base;
    assert(osc);
    path = path_;
    data = new Osc_SimpleListModel(osc);
    data->callback = [this](Osc_SimpleListModel::list_t l){this->doUpdate(l);};
    data->update(loc+path_);
}

void Fl_Osc_ListView::doUpdate(Osc_SimpleListModel::list_t l)
{
    this->clear();
    for(int i=0; i<(int)l.size(); ++i) {
        this->add(l[i].c_str());
    }
}
void Fl_Osc_ListView::update(void)
{
    data->update(loc+path);
}

void Fl_Osc_ListView::insert(std::string s, int offset)
{
    assert(offset);
    data->list.insert(data->list.begin()+offset-1, s);
    data->apply();
    //fprintf(stderr, "UNIMPLEMENTED\n");
}
void Fl_Osc_ListView::append(std::string s)
{
    data->list.push_back(s);
    data->apply();
}
void Fl_Osc_ListView::doMove(int i, int j)
{
    assert(i);
    assert(j);
    auto &list = data->list;
    std::string value = list[j-1];
    list.erase(list.begin()+j-1);
    list.insert(list.begin()+i-1, value);
    //std::swap(data->list[i-1], data->list[j-1]);
    data->apply();
}
void Fl_Osc_ListView::doRemove(int offset)
{
    assert(offset);
    data->list.erase(data->list.begin()+offset-1);
    data->apply();
}
void Fl_Osc_ListView::sendUpdate() const
{
}
