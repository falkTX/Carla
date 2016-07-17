#pragma once
#include <string>
#include <vector>

struct BankEntry
{
    BankEntry(void);
    std::string file;
    std::string bank;
    std::string name;
    std::string comments;
    std::string author;
    std::string type;
    int         id;
    bool        add;
    bool        pad;
    bool        sub;
    typedef std::vector<std::string> svec;
    svec tags(void) const;
    bool match(std::string) const;
};

class BankDb
{
    public:
        typedef std::vector<std::string> svec;
        typedef std::vector<BankEntry>   bvec;

        //search for banks
        //uses a space separated list of keywords and
        //finds something that matches ALL keywords
        bvec search(std::string) const;

        //fully qualified paths only
        void addBankDir(std::string);

        //clear all known entries and banks
        void clear(void);

        //List of all tags
        svec tags(void) const;

        //scan banks
        void scanBanks(void);

    private:
        BankEntry processXiz(std::string, std::string) const;
        bvec fields;
        svec banks;
};
