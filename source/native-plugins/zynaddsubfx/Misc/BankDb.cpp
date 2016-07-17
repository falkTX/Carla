#include "BankDb.h"
#include "XMLwrapper.h"
#include "../globals.h"
#include <cstring>
#include <dirent.h>

#define INSTRUMENT_EXTENSION ".xiz"

using std::string;
typedef BankDb::svec svec;
typedef BankDb::bvec bvec;

BankEntry::BankEntry(void)
    :id(0), add(false), pad(false), sub(false)
{}

bool sfind(std::string hay, std::string needle)
{
    return strcasestr(hay.c_str(), needle.c_str());
}

bool BankEntry::match(string s) const
{
    if(s == "#pad")
        return pad;
    else if(s == "#sub")
        return sub;
    else if(s == "#add")
        return add;
    return sfind(file,s) || sfind(name,s) || sfind(bank, s) ||
        sfind(type, s) || sfind(comments,s) || sfind(author,s);
}

static svec split(string s)
{
    svec vec;
    string ss;
    for(char c:s) {
        if(isspace(c) && !ss.empty()) {
            vec.push_back(ss);
            ss.clear();
        } else if(!isspace(c)) {
            ss.push_back(c);
        }
    }
    if(!ss.empty())
        vec.push_back(ss);

    return vec;
}

static string line(string s)
{
    string ss;
    for(char c:s) {
        if(c != '\n')
            ss.push_back(c);
        else
            return ss;
    }
    return ss;
}

bvec BankDb::search(std::string ss) const
{
    bvec vec;
    const svec sterm = split(ss);
    for(auto field:fields) {
        bool match = true;
        for(auto s:sterm)
            match &= field.match(s);
        if(match)
            vec.push_back(field);
    }

    return vec;
}

void BankDb::addBankDir(std::string bnk)
{
    bool repeat = false;
    for(auto b:banks)
        repeat |= b == bnk;

    if(!repeat)
        banks.push_back(bnk);
}

void BankDb::clear(void)
{
    banks.clear();
    fields.clear();
}

void BankDb::scanBanks(void)
{
    fields.clear();
    for(auto bank:banks)
    {
        DIR *dir = opendir(bank.c_str());

        if(!dir)
            continue;

        struct dirent *fn;

        while((fn = readdir(dir))) {
            const char *filename = fn->d_name;

            //check for extension
            if(!strstr(filename, INSTRUMENT_EXTENSION))
                continue;

            auto xiz = processXiz(filename, bank);
            fields.push_back(xiz);
        }

        closedir(dir);
    }
}

BankEntry BankDb::processXiz(std::string filename, std::string bank) const
{
    //verify if the name is like this NNNN-name (where N is a digit)
    int no = 0;
    unsigned int startname = 0;

    for(unsigned int i = 0; i < 4; ++i) {
        if(filename.length() <= i)
            break;

        if(isdigit(filename[i])) {
            no = no * 10 + (filename[i] - '0');
            startname++;
        }
    }

    if(startname + 1 < filename.length())
        startname++;  //to take out the "-"

    std::string name = filename;

    //remove the file extension
    for(int i = name.size() - 1; i >= 2; i--) {
        if(name[i] == '.') {
            name = name.substr(0, i);
            break;
        }
    }


    BankEntry entry;
    entry.file = filename;
    entry.bank = bank;
    entry.id   = no;

    if(no != 0) //the instrument position in the bank is found
        entry.name = name.substr(startname);
    else
        entry.name = name;

    const char *types[] = {
        "None",
        "Piano",
        "Chromatic Percussion",
        "Organ",
        "Guitar",
        "Bass",
        "Solo Strings",
        "Ensemble",
        "Brass",
        "Reed",
        "Pipe",
        "Synth Lead",
        "Synth Pad",
        "Synth Effects",
        "Ethnic",
        "Percussive",
        "Sound Effects",
    };

    //Try to obtain other metadata (expensive)
    XMLwrapper xml;
    string fname = bank+filename;
    int ret = xml.loadXMLfile(fname);
    if(xml.enterbranch("INSTRUMENT")) {
        if(xml.enterbranch("INFO")) {
            char author[1024];
            char comments[1024];
            int  type = 0;
            xml.getparstr("author", author, 1024);
            xml.getparstr("comments", comments, 1024);
            type = xml.getpar("type", 0, 0, 16);
            entry.author   = author;
            entry.comments = comments;
            entry.type     = types[type];
            xml.exitbranch();
        }
        if(xml.enterbranch("INSTRUMENT_KIT")) {
            for(int i = 0; i < NUM_KIT_ITEMS; ++i) {
                if(xml.enterbranch("INSTRUMENT_KIT_ITEM", i) == 0) {
                    entry.add |= xml.getparbool("add_enabled", false);
                    entry.sub |= xml.getparbool("sub_enabled", false);
                    entry.pad |= xml.getparbool("pad_enabled", false);
                    xml.exitbranch();
                }
            }
            xml.exitbranch();
        }
        xml.exitbranch();
    }

    //printf("Bank Entry:\n");
    //printf("\tname   - %s\n", entry.name.c_str());
    //printf("\tauthor - %s\n", line(entry.author).c_str());
    //printf("\tadd/pad/sub - %d/%d/%d\n", entry.add, entry.pad, entry.sub);

    return entry;
}
