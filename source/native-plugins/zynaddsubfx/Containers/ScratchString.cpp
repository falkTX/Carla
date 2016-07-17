#include "ScratchString.h"
#include <cstring>
#include <cstdio>

ScratchString::ScratchString(void)
{
    memset(c_str, 0, sizeof(c_str));
}

ScratchString::ScratchString(int num)
{
    snprintf(c_str, SCRATCH_SIZE, "%d", num);
}
    
ScratchString::ScratchString(unsigned char num)
{
    snprintf(c_str, SCRATCH_SIZE, "%d", num);
}

ScratchString::ScratchString(const char *str)
{
    if(str)
        strncpy(c_str, str, SCRATCH_SIZE);
    else
        memset(c_str, 0, sizeof(c_str));
}

ScratchString ScratchString::operator+(const ScratchString s)
{
    ScratchString ss;
    strncpy(ss.c_str, c_str, SCRATCH_SIZE);
    strncat(ss.c_str, s.c_str, SCRATCH_SIZE-strlen(c_str));
    return ss;
}

//ScratchString::operator const char*() const
//{
//    return c_str;
//}
