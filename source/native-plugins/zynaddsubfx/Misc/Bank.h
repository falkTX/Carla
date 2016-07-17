/*
  ZynAddSubFX - a software synthesizer

  Bank.h - Instrument Bank
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef BANK_H
#define BANK_H

#include <string>
#include <vector>
#include "../globals.h"
#include "Config.h"

//entries in a bank
#define BANK_SIZE 160

/**The instrument Bank*/
class Bank
{
    public:
        /**Constructor*/
        Bank(Config* config);
        ~Bank();
        std::string getname(unsigned int ninstrument);
        std::string getnamenumbered(unsigned int ninstrument);
        //if newslot==-1 then this is ignored, else it will be put on that slot
        int setname(unsigned int ninstrument,
                     const std::string &newname,
                     int newslot);

        /**returns true when slot is empty*/
        bool emptyslot(unsigned int ninstrument);

        /**Empties out the selected slot*/
        int clearslot(unsigned int ninstrument);
        /**Saves the given Part to slot*/
        int savetoslot(unsigned int ninstrument, class Part * part);
        /**Loads the given slot into a Part*/
        int loadfromslot(unsigned int ninstrument, class Part * part);

        /**Swaps Slots*/
        int swapslot(unsigned int n1, unsigned int n2);

        int loadbank(std::string bankdirname) NONREALTIME;
        int newbank(std::string newbankdirname) NONREALTIME;

        std::string bankfiletitle; //this is shown on the UI of the bank (the title of the window)
        int locked();

        void rescanforbanks();

        void setMsb(uint8_t msb);
        void setLsb(uint8_t lsb);

        struct bankstruct {
            bool operator<(const bankstruct &b) const;
            std::string dir;
            std::string name;
        };

        std::vector<bankstruct> banks;
        int bankpos;

        struct ins_t {
            ins_t(void);
            std::string name;
            //All valid instruments must have a non-empty filename
            std::string filename;
        } ins[BANK_SIZE];

        std::vector<std::string> search(std::string) const;

    private:

        //it adds a filename to the bank
        //if pos is -1 it try to find a position
        //returns -1 if the bank is full, or 0 if the instrument was added
        int addtobank(int pos, std::string filename, std::string name);

        void deletefrombank(int pos);

        void clearbank();

        std::string defaultinsname;
        std::string dirname;

        void scanrootdir(std::string rootdir); //scans a root dir for banks

        /** Expends ~ prefix in dirname, if any */
        void expanddirname(std::string &dirname);

        /** Ensure that the directory name is suffixed by a
         * directory separator */
        void normalizedirsuffix(std::string &dirname) const;

        Config* const config;
        class BankDb *db;

    public:
        uint8_t bank_msb;
        uint8_t bank_lsb;
};

#endif
