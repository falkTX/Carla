#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <dirent.h>

#include "Util.h"
#include "TmpFileMgr.h"

std::string TmpFileMgr::get_tmp_nam() const
{
     return tmp_nam_prefix + to_s(getpid());
}

void TmpFileMgr::create_tmp_file(unsigned server_port) const
{
    std::string tmp_nam = get_tmp_nam();
    if(0 == access(tmp_nam.c_str(), F_OK)) {
        fprintf(stderr, "Error: Cannot overwrite file %s. "
                "You should probably remove it.", tmp_nam.c_str());
        exit(EXIT_FAILURE);
    }
    FILE* tmp_fp = fopen(tmp_nam.c_str(), "w");
    if(!tmp_fp)
        fprintf(stderr, "Warning: could not create new file %s.\n",
                tmp_nam.c_str());
    else
        fprintf(tmp_fp, "%u", server_port);
    fclose(tmp_fp);
}

void TmpFileMgr::clean_up_tmp_nams() const
{
    DIR *dir;
    struct dirent *entry;
    if ((dir = opendir ("/tmp/")) != nullptr)
    {
        while ((entry = readdir (dir)) != nullptr)
        {
            std::string name = std::string("/tmp/") + entry->d_name;
            if(!name.compare(0, strlen(tmp_nam_prefix),tmp_nam_prefix))
            {
                std::string pid = name.substr(strlen(tmp_nam_prefix));
                std::string proc_file = "/proc/" + std::move(pid) +
                                        "/comm";

                std::ifstream ifs(proc_file);
                bool remove = false;

                if(!ifs.good())
                {
                    fprintf(stderr, "Note: trying to remove %s - the "
                                    "process does not exist anymore.\n",
                            name.c_str());
                    remove = true;
                }
                else
                {
                    std::string comm_name;
                    ifs >> comm_name;
                    if(comm_name == "zynaddsubfx")
                        fprintf(stderr, "Note: detected running "
                                        "zynaddsubfx with PID %s.\n",
                                name.c_str() + strlen(tmp_nam_prefix));
                    else {
                        fprintf(stderr, "Note: trying to remove %s - the "
                                        "PID is owned by\n  another "
                                        "process: %s\n",
                                name.c_str(), comm_name.c_str());
                        remove = true;
                    }
                }


                if(remove)
                {
                    // make sure this file contains only one unsigned
                    unsigned udp_port;
                    std::ifstream ifs2(name);
                    if(!ifs2.good())
                        fprintf(stderr, "Warning: could not open %s.\n",
                                name.c_str());
                    else
                    {
                        ifs2 >> udp_port;
                        if(ifs.good())
                            fprintf(stderr, "Warning: could not remove "
                                            "%s, \n  it has not been "
                                            "written by zynaddsubfx\n",
                                    name.c_str());
                        else
                        {
                            if(std::remove(name.c_str()) != 0)
                                fprintf(stderr, "Warning: can not remove "
                                                "%s.\n", name.c_str());
                        }
                    }
                }

                /* one might want to connect to zyn here,
                   but it is not necessary:
                lo_address la = lo_address_new(nullptr, udp_port.c_str());
                if(lo_send(la, "/echo", nullptr) <= 0)
                    fputs("Note: found crashed file %s\n", stderr);
                lo_address_free(la);*/
            }
        }
        closedir (dir);
    } else {
        fputs("Warning: can not read /tmp.\n", stderr);
    }
}
