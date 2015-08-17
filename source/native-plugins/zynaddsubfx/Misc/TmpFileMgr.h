#pragma once

/**
    This file provides routines for using zyn's tmp files.
*/
class TmpFileMgr
{
    static constexpr const char* tmp_nam_prefix = "/tmp/zynaddsubfx_";

public:
    //! returns file name to where UDP port is saved
    std::string get_tmp_nam() const;

    //! creates a tmp file with given UDP port information
    void create_tmp_file(unsigned server_port) const;

    //! cleans up as many tmp files as possible
    void clean_up_tmp_nams() const;
};
