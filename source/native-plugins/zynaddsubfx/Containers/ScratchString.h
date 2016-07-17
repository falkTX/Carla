#pragma once
#define SCRATCH_SIZE 128

//Fixed Size String Substitute
struct ScratchString
{
    ScratchString(void);
    ScratchString(int num);
    ScratchString(unsigned char num);
    ScratchString(const char *str);

    ScratchString operator+(const ScratchString s);

    //operator const char*() const;

    char c_str[SCRATCH_SIZE];
};
