
/*******************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                                     */
/*                                                                             */
/* This program is free software; you can redistribute it and/or modify it     */
/* under the terms of the GNU General Public License as published by the       */
/* Free Software Foundation; either version 2 of the License, or (at your      */
/* option) any later version.                                                  */
/*                                                                             */
/* This program is distributed in the hope that it will be useful, but WITHOUT */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   */
/* more details.                                                               */
/*                                                                             */
/* You should have received a copy of the GNU General Public License along     */
/* with This program; see the file COPYING.  If not,write to the Free Software */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */
/*******************************************************************************/


#include "NSM.H"

#include "../Nio/Nio.h"

#if defined(FLTK_UI) || defined(NTK_UI)
#include <FL/Fl.H>
#endif
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

extern int Pexitprogram;
#if defined(FLTK_UI) || defined(NTK_UI)
#include "MasterUI.h"
extern MasterUI *ui;
#endif

extern NSM_Client *nsm;
extern char       *instance_name;

NSM_Client::NSM_Client(MiddleWare *m)
    :project_filename(0),
     display_name(0),
     middleware(m)
{
}

int command_open(const char *name,
                 const char *display_name,
                 const char *client_id,
                 char **out_msg);
int command_save(char **out_msg);

int
NSM_Client::command_save(char **out_msg)
{
    (void) out_msg;
    int r = ERR_OK;

    middleware->transmitMsg("/save_xmz", "s", project_filename);

    return r;
}

int
NSM_Client::command_open(const char *name,
                         const char *display_name,
                         const char *client_id,
                         char **out_msg)
{
    Nio::stop();

    if(instance_name)
        free(instance_name);

    instance_name = strdup(client_id);

    Nio::start();

    char *new_filename;

    //if you're on windows enjoy the undefined behavior...
#ifndef WIN32
    asprintf(&new_filename, "%s.xmz", name);
#endif

    struct stat st;

    int r = ERR_OK;

    if(0 == stat(new_filename, &st))
        middleware->transmitMsg("/load_xmz", "s", new_filename);
    else
        middleware->transmitMsg("/reset_master", "");

    if(project_filename)
        free(project_filename);

    if(this->display_name)
        free(this->display_name);

    project_filename = new_filename;

    this->display_name = strdup(display_name);

    return r;
}

#if defined(FLTK_UI) || defined(NTK_UI)
static void save_callback(Fl_Widget *, void *v)
{
    MasterUI *ui = static_cast<MasterUI*>(v);
    ui->do_save_master();
}
#endif

void
NSM_Client::command_active(bool active)
{
#if defined(FLTK_UI) || defined(NTK_UI)
    if(active) {
        Fl_Menu_Item *m;
        //TODO see if there is a cleaner way of doing this without voiding
        //constness
        if((m=const_cast<Fl_Menu_Item *>(ui->mastermenu->find_item(
                                       "&File/&Open Parameters..."))))
            m->label("&Import Parameters...");
        if((m=const_cast<Fl_Menu_Item *>(ui->simplemastermenu->find_item(
                                       "&File/&Open Parameters..."))))
            m->label("&Import Parameters...");

        //TODO get this menu entry inserted at the right point
        if((m=const_cast<Fl_Menu_Item *>(ui->mastermenu->find_item("&File/&Export Parameters..."))))
            m->show();
        else
            ui->mastermenu->add("&File/&Export Parameters...",0,save_callback,ui);

        if((m=const_cast<Fl_Menu_Item *>(ui->simplemastermenu->find_item("&File/&Export Parameters..."))))
            m->show();
        else
            ui->simplemastermenu->add("&File/&Export Parameters...",0,save_callback,ui);

        ui->sm_indicator1->value(1);
        ui->sm_indicator2->value(1);
        ui->sm_indicator1->tooltip(session_manager_name());
        ui->sm_indicator2->tooltip(session_manager_name());
    }
    else {
        Fl_Menu_Item *m;
        if((m=const_cast<Fl_Menu_Item *>(ui->mastermenu->find_item(
                                       "&File/&Import Parameters..."))))
            m->label("&Open Parameters...");
        if((m=const_cast<Fl_Menu_Item *>(ui->simplemastermenu->find_item(
                                       "&File/&Open Parameters..."))))
            m->label("&Open Parameters...");

        if((m=const_cast<Fl_Menu_Item *>(ui->mastermenu->find_item("&File/&Export Parameters..."))))
            m->hide();
        if((m=const_cast<Fl_Menu_Item *>(ui->simplemastermenu->find_item("&File/&Export Parameters..."))))
            m->hide();

        ui->sm_indicator1->value(0);
        ui->sm_indicator2->value(0);
        ui->sm_indicator1->tooltip(NULL);
        ui->sm_indicator2->tooltip(NULL);
    }
#endif
}
