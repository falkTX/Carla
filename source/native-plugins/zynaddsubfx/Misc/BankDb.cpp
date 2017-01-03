#include "BankDb.h"
#include "XMLwrapper.h"
#include "Util.h"
#include "../globals.h"
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

#define INSTRUMENT_EXTENSION ".xiz"

using std::string;
typedef BankDb::svec svec;
typedef BankDb::bvec bvec;

BankEntry::BankEntry(void)
    :id(0), add(false), pad(false), sub(false), time(0)
{}

bool platform_strcasestr(const char *hay, const char *needle)
{
    int n = strlen(hay);
    int m = strlen(needle);
    for(int i=0; i<n; i++) {
        int good = 1;
        for(int j=0; j<m; ++j) {
            if(toupper(hay[i+j]) != toupper(needle[j])) {
                good = 0;
                break;
            }

        }
        if(good)
            return 1;
    }
    return 0;

}

bool sfind(std::string hay, std::string needle)
{
    //return strcasestr(hay.c_str(), needle.c_str());
    return platform_strcasestr(hay.c_str(), needle.c_str());
    return false;
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

bool BankEntry::operator<(const BankEntry &b) const
{
    return this->file < b.file;
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

    std::sort(vec.begin(), vec.end());

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

static std::string getCacheName(void)
{
    char name[512] = {0};
    snprintf(name, sizeof(name), "%s%s", getenv("HOME"),
            "/.zynaddsubfx-bank-cache.xml");
    return name;
}

static bvec loadCache(void)
{
    bvec cache;
    XMLwrapper xml;
    xml.loadXMLfile(getCacheName());
    if(xml.enterbranch("bank-cache")) {
        auto nodes = xml.getBranch();

        for(auto node:nodes) {
            BankEntry be;
#define bind(x,y) if(node.has(#x)) {be.x = y(node[#x].c_str());}
            bind(file, string);
            bind(bank, string);
            bind(name, string);
            bind(comments, string);
            bind(author, string);
            bind(type, atoi);
            bind(id, atoi);
            bind(add, atoi);
            bind(pad, atoi);
            bind(sub, atoi);
            bind(time, atoi);
#undef bind
            cache.push_back(be);
        }
    }
    return cache;
}

static void saveCache(bvec vec)
{
    XMLwrapper xml;
    xml.beginbranch("bank-cache");
    for(auto value:vec) {
        XmlNode binding("instrument-entry");
#define bind(x) binding[#x] = to_s(value.x);
        bind(file);
        bind(bank);
        bind(name);
        bind(comments);
        bind(author);
        bind(type);
        bind(id);
        bind(add);
        bind(pad);
        bind(sub);
        bind(time);
#undef bind
        xml.add(binding);
    }
    xml.endbranch();
    xml.saveXMLfile(getCacheName(), 0);
}

void BankDb::scanBanks(void)
{
    fields.clear();
    bvec cache = loadCache();
    bmap cc;
    for(auto c:cache)
        cc[c.bank + c.file] = c;

    bvec ncache;
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

            auto xiz = processXiz(filename, bank, cc);
            fields.push_back(xiz);
            ncache.push_back(xiz);
        }

        closedir(dir);

    }
    saveCache(ncache);
}

BankEntry BankDb::processXiz(std::string filename,
        std::string bank, bmap &cache) const
{
    string fname = bank+filename;

#ifdef WIN32
    int ret, time = 0;
#else
    //Grab a timestamp
    struct stat st;
    int ret  = lstat(fname.c_str(), &st);
    int time = 0;
    if(ret != -1)
        time = st.st_mtim.tv_sec;
#endif

    //quickly check if the file exists in the cache and if it is up-to-date
    if(cache.find(fname) != cache.end() &&
            cache[fname].time == time)
        return cache[fname];



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
    entry.time = time;

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
    ret = xml.loadXMLfile(fname);
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
    //printf("\tbank   - %s\n", entry.bank.c_str());
    //printf("\tadd/pad/sub - %d/%d/%d\n", entry.add, entry.pad, entry.sub);

    return entry;
}
