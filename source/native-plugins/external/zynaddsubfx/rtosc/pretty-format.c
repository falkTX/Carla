#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include <rtosc/rtosc.h>
#include <rtosc/pretty-format.h>

/** Call snprintf and assert() that it did fit into the buffer */
static int asnprintf(char* str, size_t size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int written = vsnprintf(str, size, format, args);
    assert(written >= 0); // no error
    // snprintf(3) says that a return value of size or more bytes means
    // that the output has been truncated
    assert((size_t)written < size);
    va_end(args);
    return written;
}

static const rtosc_print_options default_print_options
 = ((rtosc_print_options) { true, 2, " ", 80});

/**
 * Return the char that represents the escape sequence
 *
 * @param c The escape sequence, e.g. '\n'
 * @param chr True if the character appears in a single character
 *    (vs in a string)
 * @return The character describing the escape sequence, e.g. 'n';
 *   -1 if none exists
 */
static int as_escaped_char(int c, int chr)
{
    switch(c)
    {
        case '\a': return 'a';
        case '\b': return 'b';
        case '\t': return 't';
        case '\n': return 'n';
        case '\v': return 'v';
        case '\f': return 'f';
        case '\r': return 'r';
        case '\\': return '\\';
        default:
            if(chr && c == '\'')
                return '\'';
            else if(!chr && c == '"')
                return '"';
            else return -1;
    }
}

// internal function for rtosc_print_arg_val
static void break_string(char** buffer, size_t bs, int wrt, int* cols_used)
{
    // unquoted, this means: "\<newline>    "
    int tmp = asnprintf(*buffer, bs, "\"\\\n    \"");
    wrt += tmp;
    *buffer += tmp;
    *cols_used = 5;
}

size_t rtosc_print_arg_val(const rtosc_arg_val_t *arg,
                           char *buffer, size_t bs,
                           const rtosc_print_options* opt,
                           int *cols_used)
{
    size_t wrt = 0;
    if(!opt)
        opt = &default_print_options;
    assert(arg);
    const rtosc_arg_t* val = &arg->val;

    switch(arg->type) {
        case 'T':
            assert(bs>4);
            strncpy(buffer, "true", bs);
            wrt = 4;
            break;
        case 'F':
            assert(bs>5);
            strncpy(buffer, "false", bs);
            wrt = 5;
            break;
        case 'N':
            assert(bs>3);
            strncpy(buffer, "nil", bs);
            wrt = 3;
            break;
        case 'I':
            assert(bs>3);
            strncpy(buffer, "inf", bs);
            wrt = 3;
            break;
        case 'h':
            wrt = asnprintf(buffer, bs, "%"PRId64"h", val->h);
            break;
        case 't': // write to ISO 8601 date
        {
            if(val->t == 1)
                wrt = asnprintf(buffer, bs, "immediately");
            else
            {
                time_t t = (time_t)(val->t >> 32);
                int32_t secfracs = val->t & (0xffffffff);
                struct tm* m_tm = localtime(&t);

                const char* strtimefmt = (secfracs || m_tm->tm_sec)
                                  ? "%Y-%m-%d %H:%M:%S"
                                  : (m_tm->tm_hour || m_tm->tm_min)
                                    ? "%Y-%m-%d %H:%M"
                                    : "%Y-%m-%d";

                wrt = strftime(buffer, bs, strtimefmt, m_tm);
                assert(wrt);

                if(secfracs)
                {
                    int rd = 0;
                    int prec = opt->floating_point_precision;
                    assert(prec>=0);
                    assert(prec<100);

                    // convert fractions -> float
                    char lossless[16];
                    asnprintf(lossless, 16, "0x%xp-32", secfracs);
                    float flt;
                    sscanf(lossless, "%f%n", &flt, &rd);
                    assert(rd);

                    // append float
                    char fmtstr[8];
                    asnprintf(fmtstr, 5, "%%.%df", prec);
                    int lastwrt = wrt;
                    wrt += asnprintf(buffer + wrt, bs - wrt,
                                     fmtstr, flt);
                    // snip part before separator
                    const char* sep = strchr(buffer + lastwrt, '.');
                    assert(sep);
                    memmove(buffer + lastwrt, sep, strlen(sep)+1);
                    wrt -= (sep - (buffer + lastwrt));

                    if(opt->lossless)
                        wrt += asnprintf(buffer + wrt, bs - wrt,
                                         " (...+%as)", flt);
                } // if secfracs
            } // else
            break;
        }
        case 'r':
            wrt = asnprintf(buffer, bs, "#%02x%02x%02x%02x",
                      (val->i >> 24) & 0xff,
                      (val->i >> 16) & 0xff,
                      (val->i >>  8) & 0xff,
                       val->i        & 0xff
                      );
            break;
        case 'd':
        case 'f':
        {
            int prec = opt->floating_point_precision;
            assert(prec>=0);
            assert(prec<100);
            char fmtstr[9];
            if(arg->type == 'f')
            {
                // e.g. "%.42f" or "%.0f.":
                asnprintf(fmtstr, 6, "%%#.%df", prec);
                wrt = asnprintf(buffer, bs, fmtstr, val->f);
                if(opt->lossless)
                    wrt += asnprintf(buffer + wrt, bs - wrt,
                                     " (%a)", val->f);
            }
            else
            {
                // e.g. "%.42lfd" or "%.0lf.d"
                asnprintf(fmtstr, 8, "%%#.%dlfd", prec);
                wrt = asnprintf(buffer, bs, fmtstr, val->d);
                if(opt->lossless)
                    wrt += asnprintf(buffer + wrt, bs - wrt,
                                     " (%la)", val->d);
            }
            break;
        }
        case 'c':
        {
            int is_esc = (as_escaped_char(val->i, true) != -1);
            wrt = asnprintf(buffer, bs, "'%s%c'",
                            is_esc ? "\\" : "",
                            is_esc ? as_escaped_char(val->i, true)
                                   : val->i);
            break;
        }
        case 'i':
            wrt = asnprintf(buffer, bs, "%"PRId32, val->i);
            break;
        case 'm':
            wrt = asnprintf(buffer, bs, "MIDI [0x%02x 0x%02x 0x%02x 0x%02x]",
                            val->m[0], val->m[1], val->m[2], val->m[3]);
            break;
        case 's':
        case 'S':
        {
            bool plain; // e.g. without quotes
            if(arg->type == 'S')
            {
                plain = true; // "Symbol": are quotes required?
                if(*val->s != '_' && !isalpha(*val->s))
                    plain = false;
                else for(const char* s = val->s + 1; *s && plain; ++s)
                    plain = (*s == '_' || (isalnum(*s)));
            }
            else plain = false;

            if(plain)
            {
                wrt = asnprintf(buffer, bs, "%s", val->s);
                break;
            }
            else
            {
                char* b = buffer;
                *b++ = '"';
                for(const char* s = val->s; *s; ++s)
                {
                    if(*cols_used >= opt->linelength - 2)
                        break_string(&b, bs, wrt, cols_used);
                    assert(bs);
                    int as_esc = as_escaped_char(*s, false);
                    if(as_esc != -1) {
                        assert(bs-1);
                        *b++ = '\\';
                        *b++ = as_esc;
                        *cols_used += 2;
                        if(as_esc == 'n')
                            break_string(&b, bs, wrt, cols_used);
                    }
                    else {
                        *b++ = *s;
                        ++*cols_used;
                    }
                }
                assert(bs >= 2);
                *b++ = '"';
                if(arg->type == 'S') {
                    assert(bs >= 2);
                    *b++ = 'S';
                }
                *b = 0;
                wrt += (b-buffer);
                break;
            }
        }
        case 'b':
            wrt = asnprintf(buffer, bs, "[%d ", val->b.len);
            *cols_used += wrt;
            buffer += wrt;
            for(int32_t i = 0; i < val->b.len; ++i)
            {
                if(*cols_used >= opt->linelength - 6)
                {
                    int tmp = asnprintf(buffer-1, bs+1, "\n    ");
                    wrt += (tmp-1);
                    buffer += (tmp-1);
                    *cols_used = 4;
                }
                wrt += asnprintf(buffer, bs, "0x%02x ", val->b.data[i]);
                bs -= 5;
                buffer += 5;
                *cols_used += 5;
            }
            buffer[-1] = ']';
            break;
        default:
            ;
    }

    switch(arg->type)
    {
        case 's':
        case 'b':
            // these can break the line, so they compute *cols_used themselves
            break;
        default:
            *cols_used += wrt;
    }

    return wrt;
}

size_t rtosc_print_arg_vals(const rtosc_arg_val_t *args, size_t n,
                            char *buffer, size_t bs,
                            const rtosc_print_options *opt, int cols_used)
{
    size_t wrt=0;
    int args_written_this_line = (cols_used) ? 1 : 0;
    if(!opt)
        opt = &default_print_options;
    size_t sep_len = strlen(opt->sep);
    char* last_sep = buffer - 1;
    for(size_t i = 0; i < n; ++i)
    {
        size_t tmp = rtosc_print_arg_val(args++, buffer, bs, opt, &cols_used);

        wrt += tmp;
        buffer += tmp;
        bs -= tmp;
        ++args_written_this_line;
        // did we break the line length,
        // and this is not the first arg written in this line?
        if(cols_used > opt->linelength && (args_written_this_line > 1))
        {
            // insert "\n    "
            *last_sep = '\n';
            assert(bs >= 4);
            memmove(last_sep+5, last_sep+1, tmp);
            last_sep[1] = last_sep[2] = last_sep[3] = last_sep[4] = ' ';
            cols_used = 4 + wrt;
            wrt += 4;
            buffer += 4;
            bs -= 4;
            args_written_this_line = 0;
        }
        if(i<n-1)
        {
            assert(sep_len < bs);
            last_sep = buffer;
            strncpy(buffer, opt->sep, bs);
            cols_used += sep_len;
            wrt += sep_len;
            buffer += sep_len;
            bs -= sep_len;
        }
    }
    return wrt;
}

size_t rtosc_print_message(const char* address,
                           const rtosc_arg_val_t *args, size_t n,
                           char *buffer, size_t bs,
                           const rtosc_print_options* opt,
                           int cols_used)
{
    size_t wrt = asnprintf(buffer, bs, "%s ", address);
    cols_used += wrt;
    buffer += wrt;
    bs -= wrt;
    wrt += rtosc_print_arg_vals(args, n, buffer, bs, opt, cols_used);
    return wrt;
}

/**
 * Increase @p s while property @property is true and the end has not yet
 * been reached
 */
static void skip_while(const char** s, int (*property)(int))
{
    for(;**s && (*property)(**s);++*s);
}

/**
 * Parse the string pointed to by @p src conforming to the format string @p
 * @param src Pointer to the input string
 * @param fmt Format string for sscanf(). Must suppress all assignments,
 *   except the last one, which must be "%n" and be at the string's end.
 * @return The number of bytes skipped from the string pointed to by @p src
 */
static int skip_fmt(const char** src, const char* fmt)
{
    assert(!strncmp(fmt + strlen(fmt) - 2, "%n", 2));
    int rd = 0;
    sscanf(*src, fmt, &rd);
    *src += rd;
    return rd;
}

/**
 * Behave like skip_fmt() , but set *src to NULL if the string didn't match
 * @see skip_fmt
 */
static int skip_fmt_null(const char** src, const char* fmt)
{
    int result = skip_fmt(src, fmt);
    if(!result)
        *src = NULL;
    return result;
}

/** Helper function for scanf_fmtstr() */
static const char* try_fmt(const char* src, int exp, const char* fmt,
                           char* typesrc, char type)
{
    int rd = 0;
    sscanf(src, fmt, &rd);
    if(rd == exp)
    {
        *typesrc = type;
        return fmt;
    }
    else
        return NULL;
}

/**
 * Return the right format string for skipping the next numeric value
 *
 * This can be used to find out how to sscanf() ints, floats and doubles.
 * If the string needs to be read, too, see scanf_fmtstr_scan() below.
 * @param src The beginning of the numeric value to scan
 * @param type If non-NULL, the corresponding rtosc argument character
 *   (h,i,f,d) will be written here
 * @return The format string to use or NULL
 */
static const char* scanf_fmtstr(const char* src, char* type)
{
    const char* end = src;
    // skip to string end, word end or a closing paranthesis
    for(;*end && !isspace(*end) && (*end != ')');++end);

    int exp = end - src;

    // store type byte in the parameter, or in a temporary variable?
    char tmp;
    char* _type = type ? type : &tmp;

    const char i32[] = "%*"PRIi32"%n";

    const char* r; // result
    int ok = (r = try_fmt(src, exp, "%*"PRIi64"h%n", _type, 'h'))
          || (r = try_fmt(src, exp, "%*d%n", _type, 'i'))
          || (r = try_fmt(src, exp, "%*"PRIi32"i%n", _type, 'i'))
          || (r = try_fmt(src, exp, i32, _type, 'i'))
          || (r = try_fmt(src, exp, "%*lfd%n", _type, 'd'))
          || (r = try_fmt(src, exp, "%*ff%n", _type, 'f'))
          || (r = try_fmt(src, exp, "%*f%n", _type, 'f'));
    (void)ok;
    if(r == i32)
        r = "%*x%n";
    return r;
}

/** Return the right format string for reading the next numeric value */
static const char* scanf_fmtstr_scan(const char* src, char* bytes8,
                                     char* type)
{
    const char *buf = scanf_fmtstr(src, type);
    assert(buf);
    assert(bytes8);
    strncpy(bytes8, buf, 8);
    *++bytes8 = '%'; // transform "%*" to "%"
    return bytes8;
}

/** Skip the next numeric at @p src */
static size_t skip_numeric(const char** src, char* type)
{
    const char* scan_str = scanf_fmtstr(*src, type);
    if(!scan_str) return 0;
    else return skip_fmt(src, scan_str);
}

/**
 * Return the escape sequence that's generated with the given character
 * @param c The character, e.g. 'n'
 * @param chr True if the character appears in a single character
 *   (vs in a string)
 * @return The escape sequence generated by the character, e.g. '\n';
 *   0 if none exists
 */
static char get_escaped_char(char c, int chr)
{
    switch(c)
    {
        case 'a': return '\a';
        case 'b': return '\b';
        case 't': return '\t';
        case 'n': return '\n';
        case 'v': return '\v';
        case 'f': return '\f';
        case 'r': return '\r';
        case '\\': return '\\';
        default:
            if(chr && c == '\'')
                return '\'';
            else if(!chr && c == '"')
                return '"';
            else
                return 0;
    }
}

/** Called inside a string at @p src, skips until after the closing quote */
static const char* end_of_printed_string(const char* src)
{
    bool escaped = false;
    ++src;
    bool cont;
    do
    {
        for(; *src && (escaped || *src != '"') ; ++src)
        {
            if(escaped) // last char introduced an escape sequence?
            {
                if(!get_escaped_char(*src, false))
                    return NULL; // bad escape sequence
            }
            escaped = (*src == '\\') ? (!escaped) : false;
        }
        if(*src == '"' && src[1] == '\\') {
            skip_fmt_null(&src, "\"\\ \"%n");
            cont = true;
        }
        else
            cont = false;
    } while(cont);
    if(!src || !*src)
        return NULL;
    return ++src;
}

/**
 * Skips string @p exp at the current string pointed to by @p str,
 * but only if it's a separated word, i.e. if there's a char after the string,
 * it mus be a slash or whitespace.
 *
 * @return The position after the word, or NULL if the word was not present
 *   at @p str .
 */
static const char* skip_word(const char* exp, const char** str)
{
    size_t explen = strlen(exp);
    const char* cur = *str;
    int match = (!strncmp(exp, cur, explen) &&
            (!cur[explen] || cur[explen] == '/' || isspace(cur[explen])));
    if(match) {
        *str += explen;
        return *str;
    }
    else return NULL;
}

/**
 * Tries to skip the next identifier beginning at @p str
 *
 * @return The position after the identifier, or NULL if there's no identifier
 *   at @p str .
 */
static const char* skip_identifier(const char* str)
{
    if(!isalpha(*str) && *str != '_')
        return NULL;
    else
    {
        ++str;
        for(; isalnum(*str) || *str == '_'; ++str) ;
        return str;
    }
}

const char* rtosc_skip_next_printed_arg(const char* src)
{
    switch(*src)
    {
        case 't':
            if(!skip_word("true", &src))
                src = skip_identifier(src);
            break;
        case 'f':
            if(!skip_word("false", &src))
                src = skip_identifier(src);
            break;
        case 'n':
            if(!skip_word("nil", &src))
            if(!skip_word("now", &src))
                src = skip_identifier(src);
            break;
        case 'i':
            if(!skip_word("inf", &src))
            if(!skip_word("immediately", &src))
                src = skip_identifier(src);
            break;
        case '#':
            for(size_t i = 0; i<8; ++i)
            {
                ++src;
                if(!isxdigit(*src)) {
                    src = NULL;
                    i = 8;
                }
            }
            if(src) ++src;
            break;
        case '\'':
        {
            int esc = -1;
            if(strlen(src) < 3)
                return NULL;
            // type 1: '<noslash>' => normal char
            // type 2: '\<noquote>' => escaped char
            // type 3: '\'' => escaped quote
            // type 4: '\' => mistyped backslash
            if(src[1] == '\\') {
                if(src[2] == '\'' && (!src[3] || isspace(src[3])))
                {
                    // type 4
                    // the user inputs '\', which is wrong, but
                    // we accept it as a backslash anyways
                }
                else
                {
                    ++src; // type 2 or 3
                    esc = get_escaped_char(src[1], 1);
                }
            }
            // if the last char was no single quote,
            // or we had an invalid escape sequence, return NULL
            src = (!esc || src[2] != '\'') ? NULL : (src + 3);
            break;
        }
        case '"':
            src = end_of_printed_string(src);
            if(src && *src == 'S')
                ++src;
            break;
        case 'M':
            if(!strncmp("MIDI", src, 4) && (isspace(src[4]) || src[4] == '['))
                skip_fmt_null(&src, "MIDI [ 0x%*x 0x%*x 0x%*x 0x%*x ]%n");
            else
                src = skip_identifier(src);
            break;
        case '[':
        {
            int rd = 0, blobsize = 0;
            sscanf(src, "[ %i %n", &blobsize, &rd);
            src = rd ? (src + rd) : NULL;
            for(;src && *src == '0';) // i.e. 0x...
            {
                skip_fmt_null(&src, "0x%*x %n");
                blobsize--;
            }
            if(blobsize)
                src = NULL;
            if(src)
                src = (*src == ']') ? (src + 1) : NULL;
            break;
        }
        default:
        {
            // is it an identifier?
            if(*src == '_' || isalpha(*src))
            {
                for(; *src == '_' || isalnum(*src); ++src) ;
            }
            // is it a date? (vs a numeric)
            else if(skip_fmt(&src, "%*4d-%*1d%*1d-%*1d%*1d%n"))
            {
                if(skip_fmt(&src, " %*2d:%*1d%*1d%n"))
                if(skip_fmt(&src, ":%*1d%*1d%n"))
                if(skip_fmt(&src, ".%*d%n"))
                {
                    if(skip_fmt(&src, " ( ... + 0x%n"))
                    {
                        skip_fmt(&src, "%*x.%n");
                        if(skip_fmt(&src, "%*xp%n"))
                        {
                            int rd = 0, expm;
                            sscanf(src, "-%d s )%n", &expm, &rd);
                            if(rd && expm > 0 && expm <= 32)
                            {
                                // ok
                                src += rd;
                            }
                            else
                                src = NULL;

                        }
                        else
                            src = NULL;
                    }
                }
            }
            else
            {
                char type;
                int rd = skip_numeric(&src, &type);
                if(!rd) { src = NULL; break; }
                const char* after_num = src;
                skip_while(&after_num, isspace);
                if(*after_num == '(')
                {
                    if (type == 'f' || type =='d')
                    {
                        // skip lossless representation
                        src = ++after_num;
                        skip_while(&src, isspace);
                        rd = skip_numeric(&src, NULL);
                        if(!rd) { src = NULL; break; }
                        skip_fmt_null(&src, " )%n");
                    }
                    else
                        src = NULL;
                }
            }
        }
    }
    return src;
}

int rtosc_count_printed_arg_vals(const char* src)
{
    int num = 0;

    skip_while(&src, isspace);
    while (*src == '%')
        skip_fmt(&src, "%*[^\n] %n");

    for(; src && *src && *src != '/'; ++num)
    {
        src = rtosc_skip_next_printed_arg(src);
        if(src) // parse error
        {
            skip_while(&src, isspace);
            if(*src && !isspace(*src))
            {
                while (*src == '%')
                    skip_fmt(&src, "%*[^\n] %n");
            }
        }
    }
    return src ? num : -num;
}

int rtosc_count_printed_arg_vals_of_msg(const char* msg)
{
    skip_while(&msg, isspace);
    while (*msg == '%')
        skip_fmt(&msg, "%*[^\n] %n");

    if (*msg == '/') {
        for(; *msg && !isspace(*msg); ++msg);
        return rtosc_count_printed_arg_vals(msg);
    }
    else if(!*msg)
        return INT_MIN;
    else
        return -1;
}

//! Tries to parse an identifier at @p src and stores it in @p arg
const char* parse_identifier(const char* src, rtosc_arg_val_t *arg,
                             char* buffer_for_strings,
                             size_t* bufsize)
{
    if(*src == '_' || isalpha(*src))
    {
        arg->type = 'S';
        arg->val.s = buffer_for_strings;
        for(; *src == '_' || isalnum(*src); ++src)
        {
            --*bufsize;
            assert(*bufsize);
            *buffer_for_strings = *src;
            ++buffer_for_strings;
        }
        --*bufsize;
        assert(*bufsize);
        *buffer_for_strings = 0;
        ++buffer_for_strings;
    }
    return src;
}

size_t rtosc_scan_arg_val(const char* src,
                          rtosc_arg_val_t *arg,
                          char* buffer_for_strings, size_t* bufsize)
{
    int rd = 0;
    const char* start = src;
    switch(*src)
    {
        case 't':
        case 'f':
        case 'n':
        case 'i':
        {
            const char* src_backup = src;
            // timestamps "immediately" or "now"?
            if(skip_word("immediately", &src) || skip_word("now", &src))
            {
                arg->type = 't';
                arg->val.t = 1;
            }
            else if(skip_word("nil", &src)   ||
                    skip_word("inf", &src)   ||
                    skip_word("true", &src)  ||
                    skip_word("false", &src) )
            {
                arg->type = arg->val.T = toupper(*src_backup);
            }
            else
            {
                // no reserved keyword => identifier
                src = parse_identifier(src, arg, buffer_for_strings, bufsize);
            }
            break;
        }
        case '#':
        {
            arg->type = 'r';
            sscanf(++src, "%x", &arg->val.i);
            src+=8;
            break;
        }
        case '\'':
            // type 1: '<noslash>' => normal char
            // type 2: '\<noquote>' => escaped char
            // type 3: '\'' => escaped quote
            // type 4: '\'<isspace> => mistyped backslash
            arg->type = 'c';
            if(*++src == '\\')
            {
                if(src[2] && !isspace(src[2])) // escaped and 4 chars
                    arg->val.i = get_escaped_char(*++src, true);
                else // escaped, but only 3 chars: type 4
                    arg->val.i = '\\';
            }
            else // non-escaped
                arg->val.i = *src;
            src+=2;
            break;
        case '"':
        {
            ++src; // skip obligatory '"'
            char* dest = buffer_for_strings;
            bool cont;
            do
            {
                while(*src != '"')
                {
                    (*bufsize)--;
                    assert(*bufsize);
                    if(*src == '\\') {
                        *dest++ = get_escaped_char(*++src, false);
                        ++src;
                    }
                    else
                        *dest++ = *src++;
                }
                if(src[1] == '\\')
                {
                    skip_fmt(&src, "\"\\ \"%n");
                    cont = true;
                }
                else
                    cont = false;
            } while (cont);
            *dest = 0;
            ++src; // skip final '"'
            (*bufsize)--;
            arg->val.s = buffer_for_strings;
            if(*src == 'S')
            {
                ++src;
                (*bufsize)--;
                arg->type = 'S';
            }
            else
                arg->type = 's';
            break;
        }
        case 'M':
        {
            if(!strncmp("MIDI", src, 4) && (isspace(src[4]) || src[4] == '['))
            {
                arg->type = 'm';
                int32_t tmp[4];
                sscanf(src, "MIDI [ 0x%"PRIx32" 0x%"PRIx32
                                  " 0x%"PRIx32" 0x%"PRIx32" ]%n",
                       tmp, tmp + 1, tmp + 2, tmp + 3, &rd); src+=rd;
                for(size_t i = 0; i < 4; ++i)
                    arg->val.m[i] = tmp[i]; // copy to 8 bit array
            }
            else
                src = parse_identifier(src, arg, buffer_for_strings, bufsize);
            break;
        }
        case '[': // blob
        {
            arg->type = 'b';
            while( isspace(*++src) ) ;
            sscanf(src, "%"PRIi32" %n", &arg->val.b.len, &rd);
            src +=rd;

            assert(*bufsize >= (size_t)arg->val.b.len);
            *bufsize -= (size_t)arg->val.b.len;

            arg->val.b.data = (uint8_t*)buffer_for_strings;
            for(int32_t i = 0; i < arg->val.b.len; ++i)
            {
                int32_t tmp;
                int rd;
                sscanf(src, "0x%x %n", &tmp, &rd);
                arg->val.b.data[i] = tmp;
                src+=rd;
            }

            ++src; // skip ']'
            break;
        }
        default:
            // is it an identifier?
            if(*src == '_' || isalpha(*src))
            {
                arg->type = 'S';
                arg->val.s = buffer_for_strings;
                for(; *src == '_' || isalnum(*src); ++src)
                {
                    --*bufsize;
                    assert(*bufsize);
                    *buffer_for_strings = *src;
                    ++buffer_for_strings;
                }
                --*bufsize;
                assert(*bufsize);
                *buffer_for_strings = 0;
                ++buffer_for_strings;
            }
            // "YYYY-" => it's a date
            else if(src[0] && src[1] && src[2] && src[3] && src[4] == '-')
            {
                arg->val.t = 0;

                struct tm m_tm;
                m_tm.tm_hour = 0;
                m_tm.tm_min = 0;
                m_tm.tm_sec = 0;
                sscanf(src, "%4d-%2d-%2d%n",
                       &m_tm.tm_year, &m_tm.tm_mon, &m_tm.tm_mday, &rd);
                src+=rd;
                float secfracsf;

                rd = 0;
                sscanf(src, " %2d:%2d%n", &m_tm.tm_hour, &m_tm.tm_min, &rd);
                if(rd)
                 src+=rd;

                rd = 0;
                sscanf(src, ":%2d%n", &m_tm.tm_sec, &rd);
                if(rd)
                 src+=rd;

                // lossless format is appended in parantheses?
                //  => take it directly from there
                if(skip_fmt(&src, "%*f (%n"))
                {

                    sscanf(src, " ... + 0x%8"PRIx64"p-32 s )%n",
                           &arg->val.t, &rd);
                    src += rd;
                }
                // float number, but not lossless?
                //  => convert it to fractions of seconds
                else if(*src == '.')
                {
                    sscanf(src, "%f%n", &secfracsf, &rd);
                    src += rd;

                    // convert float -> secfracs
                    char secfracs_as_hex[16];
                    asnprintf(secfracs_as_hex, 16, "%a", secfracsf);
                    assert(secfracs_as_hex[3]=='.'); // 0x?.
                    secfracs_as_hex[3] = secfracs_as_hex[2]; // remove '.'
                    uint64_t secfracs;
                    int exp;
                    sscanf(secfracs_as_hex + 3,
                           "%"PRIx64"p-%i", &secfracs, &exp);
                    const char* p = strchr(secfracs_as_hex, 'p');
                    assert(p);
                    int lshift = 32-exp-((int)(p-(secfracs_as_hex+4))<<2);
                    assert(lshift > 0);
                    secfracs <<= lshift;
                    assert((secfracs & 0xFFFFFFFF) == secfracs);

                    arg->val.t = secfracs;
                }
                else
                {
                    // no fractional / floating seconds part
                }

                // adjust ranges to be POSIX conform
                m_tm.tm_year -= 1900;
                --m_tm.tm_mon;
                // don't mess around with Daylight Saving Time
                m_tm.tm_isdst = -1;

                arg->val.t |= (((uint64_t)mktime(&m_tm)) << ((uint64_t)32));
                arg->type = 't';
            }
            else
            {
                char bytes8[8];
                char type = arg->type = 0;

                bool repeat_once = false;
                do
                {
                    rd = 0;

                    const char *fmtstr = scanf_fmtstr_scan(src, bytes8,
                                                           &type);
                    if(!arg->type) // the first occurence determins the type
                     arg->type = type;

                    switch(type)
                    {
                        case 'h':
                            sscanf(src, fmtstr, &arg->val.h, &rd); break;
                        case 'i':
                            sscanf(src, fmtstr, &arg->val.i, &rd); break;
                        case 'f':
                            sscanf(src, fmtstr, &arg->val.f, &rd); break;
                        case 'd':
                            sscanf(src, fmtstr, &arg->val.d, &rd); break;
                    }
                    src += rd;

                    if(repeat_once)
                    {
                        // we have read the lossless part. skip spaces and ')'
                        skip_fmt(&src, " )%n");
                        repeat_once = false;
                    }
                    else
                    {
                        // is a lossless part appended in parantheses?
                        const char* after_num = src;
                        skip_while(&after_num, isspace);
                        if(*after_num == '(') {
                            ++after_num;
                            skip_while(&after_num, isspace);
                            src = after_num;
                            repeat_once = true;
                        }
                    }
                } while(repeat_once);
            } // date vs integer
        // case ident
    } // switch
    return (size_t)(src-start);
}

size_t rtosc_scan_arg_vals(const char* src,
                           rtosc_arg_val_t *args, size_t n,
                           char* buffer_for_strings, size_t bufsize)
{
    size_t last_bufsize;
    size_t rd=0;
    for(size_t i = 0; i < n; ++i)
    {
        last_bufsize = bufsize;
        size_t tmp = rtosc_scan_arg_val(src, args + i,
                                        buffer_for_strings, &bufsize);
        src += tmp;
        rd += tmp;

        size_t written = last_bufsize - bufsize;
        buffer_for_strings += written;

        do
        {
            rd += skip_fmt(&src, " %n");
            while(*src == '%')
                rd += skip_fmt(&src, "%*[^\n]%n");
        } while(isspace(*src));
    }
    return rd;
}

size_t rtosc_scan_message(const char* src,
                          char* address, size_t adrsize,
                          rtosc_arg_val_t *args, size_t n,
                          char* buffer_for_strings, size_t bufsize)
{
    size_t rd = 0;
    for(;*src && isspace(*src); ++src) ++rd;
    while (*src == '%')
        rd += skip_fmt(&src, "%*[^\n] %n");

    assert(*src == '/');
    for(; *src && !isspace(*src) && rd < adrsize; ++rd)
        *address++ = *src++;
    assert(rd < adrsize); // otherwise, the address was too long
    *address = 0;

    for(;*src && isspace(*src); ++src) ++rd;

    rd += rtosc_scan_arg_vals(src, args, n, buffer_for_strings, bufsize);

    return rd;
}

