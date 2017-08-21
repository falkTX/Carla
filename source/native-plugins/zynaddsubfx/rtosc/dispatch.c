#include <rtosc/rtosc.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>


#include <stdio.h>

static bool rtosc_match_number(const char **pattern, const char **msg)
{
    //Verify both hold digits
    if(!isdigit(**pattern) || !isdigit(**msg))
        return false;

    //Read in both numeric values
    unsigned max = atoi(*pattern);
    unsigned val = atoi(*msg);

    ////Advance pointers
    while(isdigit(**pattern))++*pattern;
    while(isdigit(**msg))++*msg;

    //Match iff msg number is strictly less than pattern
    return val < max;
}

// pattern = /previous/{A,B,C,D,E}/after
//                     ^
// message = /previous/C/after
//                     ^
const char *rtosc_match_options(const char *pattern, const char **msg)
{
    const char *preserve = *msg;
    assert(*pattern == '{');
    pattern++;

retry:

    while(1) {
        //Check for special characters
        if(*pattern == ',' || *pattern == '}') {
            goto advance_until_end;
        } else if((*pattern == **msg)) { //verbatim compare
            if(**msg)
                ++pattern, ++*msg;
            else
                goto try_next;
        } else
            goto try_next;
    }

advance_until_end:
    while(*pattern && *pattern != '}') pattern++;
    if(*pattern == '}')
        pattern++;
    return pattern;
try_next:
    *msg = preserve;
    while(*pattern && *pattern != '}' && *pattern != ',') pattern++;
    if(*pattern == ',') {
        pattern++;
        goto retry;
    }

    return NULL;
}

const char *rtosc_match_path(const char *pattern,
                             const char *msg, const char** path_end)
{
    if(!path_end)
        path_end = &msg; // writing *path_end = msg later will have no effect
    while(1) {
        //Check for special characters
        if(*pattern == ':' && !*msg)
            return *path_end = msg, pattern;
        else if(*pattern == '{') {
            pattern = rtosc_match_options(pattern, &msg);
            if(!pattern)
                return NULL;
        } else if(*pattern == '*') {
            //advance message and pattern to '/' or ':' and '\0'
            while(*pattern && *pattern != '/' && *pattern != ':')
                pattern++;
            if(*pattern == '/' || *pattern == ':')
                while(*msg && *msg != '/')
                    msg++;
        } else if(*pattern == '/' && *msg == '/') {
            ++pattern;
            ++msg;
            if(*pattern == '\0' || *pattern == ':')
                return *path_end = msg, pattern;
        } else if(*pattern == '#') {
            ++pattern;
            if(!rtosc_match_number(&pattern, &msg))
                return NULL;
        } else if((*pattern == *msg)) { //verbatim compare
            if(*msg)
                ++pattern, ++msg;
            else
                return *path_end = msg, pattern;
        } else
            return NULL;
    }
}

//Match the arg string or fail
static bool rtosc_match_args(const char *pattern, const char *msg)
{
    //match anything if now arg restriction is present (ie the ':')
    if(*pattern++ != ':')
        return true;

    const char *arg_str = rtosc_argument_string(msg);
    bool      arg_match = *pattern || *pattern == *arg_str;

    while(*pattern && *pattern != ':')
        arg_match &= (*pattern++==*arg_str++);

    if(*pattern==':') {
        if(arg_match && !*arg_str)
            return true;
        else
            return rtosc_match_args(pattern, msg); //retry
    }

    return arg_match;
}

bool rtosc_match(const char *pattern,
                 const char *msg, const char** path_end)
{
    const char *arg_pattern = rtosc_match_path(pattern, msg, path_end);
    if(!arg_pattern)
        return false;
    else if(*arg_pattern == ':')
        return rtosc_match_args(arg_pattern, msg);
    return true;
}



/*
 * Special characters from the specification:
 * ' '  space               32
 *  #   number sign         35
 *  *   asterisk            42
 *  ,   comma               44
 *  /   forward slash       47
 *  ?   question mark       63
 *  [   open bracket        91
 *  ]   close bracket       93
 *  {   open curly brace    123
 *  }   close curly brace   125
 */

#if 0
QUOTE FROM OSC 1.0 SPEC

'?' in the OSC Address Pattern matches any single character
'*' in the OSC Address Pattern matches any sequence of zero or more characters
A string of characters in square brackets (e.g., "[string]") in the OSC Address Pattern matches any character in the string.
Inside square brackets, the minus sign (-) and exclamation point (!) have special meanings:
    two characters separated by a minus sign indicate the range of characters between the given two
in ASCII collating sequence. (A minus sign at the end of the string has no special meaning.)
    An exclamation point at the beginning of a bracketed string negates the sense of the list,
    meaning that the list matches any character not in the list.
(An exclamation point anywhere besides the first character after the open bracket has no special meaning.)
    A comma-separated list of strings enclosed in curly braces (e.g., "{foo,bar}") in the OSC Address Pattern
    matches any of the strings in the list.
#endif




//for literal string X
//for X+?+[]         Y
//for Y+single{}     Z
//for numeric string N
//assume a is of the form X
//assume b is a pattern of the form:
//*   (1)
//Y   (2)
//Y*  (3)
//*X* (4)
//Z   (5)
//Z*  (6)
//Y#N (7)
#define RTOSC_MATCH_ALL             1
#define RTOSC_MATCH_CHAR            2
#define RTOSC_MATCH_PARTIAL_CHAR    3
#define RTOSC_MATCH_SUBSTRING       4
#define RTOSC_MATCH_OPTIONS         5
#define RTOSC_MATCH_PARTIAL_OPTIONS 6
#define RTOSC_MATCH_ENUMERATED      7
static bool is_charwise(uint8_t c)
{
    return c<=0x7f && c != ' ' && c != '#' &&
        c != '/' && c != '{' && c != '}';
}

int rtosc_subpath_pat_type(const char *pattern)
{
    int charwise_only = 1;
    const char *last_star = strrchr(pattern, '*');
    const char *pound = strchr(pattern, '#');
    if(!strcmp("*", pattern))
        return RTOSC_MATCH_ALL;

    for(const char *p = pattern;*p;++p)
        charwise_only &= is_charwise(*p);
    if(charwise_only && !last_star)
        return RTOSC_MATCH_CHAR;
    if(pound)
        return RTOSC_MATCH_ENUMERATED;


    return 2;
}

static bool rtosc_match_char(const char **path, const char **pattern)
{
    //printf("rtosc_match_char('%s','%s')\n", *path, *pattern);
    if(**path == **pattern && **path) {
        ++*path;
        ++*pattern;
        return true;
    } else if(**pattern == '?' && *path) {
        ++*path;
        ++*pattern;
        return true;
    } else if(**pattern == '[') {
        bool matched    = false;
        bool negation   = false;
        char last_range = '\0';
        char to_match   = **path;
        ++*pattern;
        if(**pattern == '!') {
            negation = true;
            ++*pattern;
        }
        while(**pattern && **pattern != ']') {
            last_range = **pattern;
            if(**pattern == to_match) {
                matched = true;
            } else if(**pattern == '-') {//range
                ++*pattern;
                char range_high = **pattern;
                if(range_high == ']' || !range_high)
                    break;
                if(to_match <= range_high && to_match >= last_range)
                    matched = true;
            }
            ++*pattern;
        }
        if(**pattern == ']')
            ++*pattern;
        ++*path;
        return negation ^ matched;
    }
    return false;
}

bool rtosc_match_partial(const char *a, const char *b)
{
    //assume a is of the form X
    //assume b is a pattern of the form: (1..6)
    //This is done to avoid backtracking of any kind
    //This is an OSC serialization library, not a regex
    //implementation

    char patternbuf[256];
    (void) patternbuf;
    int type = rtosc_subpath_pat_type(b);

    if(type == RTOSC_MATCH_ALL)
        return true;
    else if(type == RTOSC_MATCH_CHAR || type == RTOSC_MATCH_PARTIAL_CHAR) {
        while(rtosc_match_char(&a,&b));
        if(!*a && !*b)
            return true;
        else if(*a && *b=='*' && b[1] == '\0')
            return true;
        else
            return false;
    } else if(type == 4) {
        //extract substring
        const char *sub=NULL;
        return strstr(a,sub);
    } else if(type == RTOSC_MATCH_OPTIONS || type == 6) {
        return false;
    } else if(type == RTOSC_MATCH_ENUMERATED) {
        while(rtosc_match_char(&a,&b));
        if(*a && *b=='#' && b[1] != '\0')
            return atoi(a) < atoi(b+1);
        return false;
    } else
        return 0;
    assert(false);
}

int rtosc_matchable_path(const char *pattern)
{
    (void) pattern;
    return 0;

}

int chunk_path(const char *a, int b, const char *c)
{
    (void) a;
    (void) b;
    (void) c;
    return 0;
}
void advance_path(const char **a)
{
    (void) a;
}

bool rtosc_match_full_path(const char *pattern, const char *message)
{
    assert(false && "This API is a WIP");
    char subpattern[256];
    char submessage[256];
    const char *p = pattern;
    const char *m = message;

step:
    if(*p != *m)
        return 0;
    if(chunk_path(subpattern, sizeof(subpattern), p))
        return 0;
    if(chunk_path(submessage, sizeof(submessage), m))
        return 0;

    advance_path(&p);
    advance_path(&m);

    if(*p == 0 && *m == 0)
        return 1;
    else
        goto step;
}
