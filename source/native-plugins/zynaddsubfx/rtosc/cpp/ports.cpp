#include "../ports.h"
#include "../rtosc.h"
#include "../pretty-format.h"

#include <cinttypes>
#include <ostream>
#include <cassert>
#include <limits>
#include <cstring>
#include <string>

/* Compatibility with non-clang compilers */
#ifndef __has_feature
# define __has_feature(x) 0
#endif
#ifndef __has_extension
# define __has_extension __has_feature
#endif

/* Check for C++11 support */
#if defined(HAVE_CPP11_SUPPORT)
# if HAVE_CPP11_SUPPORT
#  define DISTRHO_PROPER_CPP11_SUPPORT
# endif
#elif __cplusplus >= 201103L || (defined(__GNUC__) && defined(__GXX_EXPERIMENTAL_CXX0X__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 405) || __has_extension(cxx_noexcept)
# define DISTRHO_PROPER_CPP11_SUPPORT
# if (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) < 407 && ! defined(__clang__)) || (defined(__clang__) && ! __has_extension(cxx_override_control))
#  define override // gcc4.7+ only
#  define final    // gcc4.7+ only
# endif
#endif

using namespace rtosc;

static inline void scat(char *dest, const char *src)
{
    while(*dest) dest++;
    while(*src && *src!=':') *dest++ = *src++;
    *dest = 0;
}

RtData::RtData(void)
    :loc(NULL), loc_size(0), obj(NULL), matches(0), message(NULL)
{
    for(int i=0; i<(int)(sizeof(idx)/sizeof(int)); ++i)
        idx[i] = 0;
}

void RtData::push_index(int ind)
{
    for(int i=1; i<(int)(sizeof(idx)/sizeof(int)); ++i)
        idx[i] = idx[i-1];
    idx[0] = ind;
}

void RtData::pop_index(void)
{
    int n = sizeof(idx)/sizeof(int);
    for(int i=n-2; i >= 0; --i)
        idx[i] = idx[i+1];
    idx[n-1] = 0;
}

void RtData::replyArray(const char *path, const char *args,
        rtosc_arg_t *vals)
{
    (void) path;
    (void) args;
    (void) vals;
}
void RtData::reply(const char *path, const char *args, ...)
{
    va_list va;
    va_start(va,args);
    char buffer[1024];
    rtosc_vmessage(buffer,1024,path,args,va);
    reply(buffer);
    va_end(va);
}
void RtData::reply(const char *msg)
{(void)msg;}
void RtData::chain(const char *path, const char *args, ...)
{
    (void) path;
    (void) args;
}

void RtData::chain(const char *msg)
{
    (void) msg;
}
void RtData::chainArray(const char *path, const char *args,
        rtosc_arg_t *vals)
{
    (void) path;
    (void) args;
    (void) vals;
};
void RtData::broadcast(const char *path, const char *args, ...)
{
    va_list va;
    va_start(va,args);
    char buffer[1024];
    rtosc_vmessage(buffer,1024,path,args,va);
    broadcast(buffer);
    va_end(va);
}
void RtData::broadcast(const char *msg)
{reply(msg);};
void RtData::broadcastArray(const char *path, const char *args,
        rtosc_arg_t *vals)
{
    (void) path;
    (void) args;
    (void) vals;
}

void RtData::forward(const char *rational)
{
    (void) rational;
}

void metaiterator_advance(const char *&title, const char *&value)
{
    if(!title || !*title) {
        value = NULL;
        return;
    }

    //Try to find "\0=" after title string
    value = title;
    while(*value)
        ++value;
    if(*++value != '=')
        value = NULL;
    else
        value++;
}

Port::MetaIterator::MetaIterator(const char *str)
    :title(str), value(NULL)
{
    metaiterator_advance(title, value);
}

Port::MetaIterator& Port::MetaIterator::operator++(void)
{
    if(!title || !*title) {
        title = NULL;
        return *this;
    }
    //search for next parameter start
    //aka "\0:" unless "\0\0" is seen
    char prev = 0;
    while(prev || (*title && *title != ':'))
        prev = *title++;

    if(!*title)
        title = NULL;
    else
        ++title;

    metaiterator_advance(title, value);
    return *this;
}

Port::MetaIterator::operator bool(void) const
{
    return title;
}

Port::MetaContainer::MetaContainer(const char *str_)
:str_ptr(str_)
{}

Port::MetaIterator Port::MetaContainer::begin(void) const
{
    if(str_ptr && *str_ptr == ':')
        return Port::MetaIterator(str_ptr+1);
    else
        return Port::MetaIterator(str_ptr);
}

Port::MetaIterator Port::MetaContainer::end(void) const
{
    return MetaIterator(NULL);
}

Port::MetaIterator Port::MetaContainer::find(const char *str) const
{
    for(const auto x : *this)
        if(!strcmp(x.title, str))
            return x;
    return NULL;
}

size_t Port::MetaContainer::length(void) const
{
        if(!str_ptr || !*str_ptr)
            return 0;
        char prev = 0;
        const char *itr = str_ptr;
        while(prev || *itr)
            prev = *itr++;
        return 2+(itr-str_ptr);
}

const char *Port::MetaContainer::operator[](const char *str) const
{
    for(const auto x : *this)
        if(!strcmp(x.title, str))
            return x.value;
    return NULL;
}
//Match the arg string or fail
inline bool arg_matcher(const char *pattern, const char *args)
{
    //match anything if now arg restriction is present (ie the ':')
    if(*pattern++ != ':')
        return true;

    const char *arg_str = args;
    bool      arg_match = *pattern || *pattern == *arg_str;

    while(*pattern && *pattern != ':')
        arg_match &= (*pattern++==*arg_str++);

    if(*pattern==':') {
        if(arg_match && !*arg_str)
            return true;
        else
            return arg_matcher(pattern, args); //retry
    }

    return arg_match;
}

inline bool scmp(const char *a, const char *b)
{
    while(*a && *a == *b) a++, b++;
    return a[0] == b[0];
}

typedef std::vector<std::string>  words_t;
typedef std::vector<std::string>  svec_t;
typedef std::vector<const char *> cvec_t;
typedef std::vector<int> ivec_t;
typedef std::vector<int> tuple_t;
typedef std::vector<tuple_t> tvec_t;

namespace rtosc{
class Port_Matcher
{
    public:
        bool *enump;
        svec_t fixed;
        cvec_t arg_spec;
        ivec_t pos;
        ivec_t assoc;
        ivec_t remap;

        bool rtosc_match_args(const char *pattern, const char *msg)
        {
            //match anything if no arg restriction is present
            //(ie the ':')
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

        bool hard_match(int i, const char *msg)
        {
            if(strncmp(msg, fixed[i].c_str(), fixed[i].length()))
                return false;
            if(arg_spec[i])
                return rtosc_match_args(arg_spec[i], msg);
            else
                return true;
        }
};

}


tvec_t do_hash(const words_t &strs, const ivec_t &pos)
{
    tvec_t  tvec;
    for(auto &s:strs) {
        tuple_t tuple;
        tuple.push_back(s.length());
        for(const auto &p:pos)
            if(p < (int)s.size())
                tuple.push_back(s[p]);
        tvec.push_back(std::move(tuple));
    }
    return tvec;
}

template<class T>
int count_dups(std::vector<T> &t)
{
    int dups = 0;
    int N = t.size();
    bool mark[t.size()];
    memset(mark, 0, N);
    for(int i=0; i<N; ++i) {
        if(mark[i])
            continue;
        for(int j=i+1; j<N; ++j) {
            if(t[i] == t[j]) {
                dups++;
                mark[j] = true;
            }
        }
    }
    return dups;
}

template<class T, class Z>
bool has(T &t, Z&z)
{
    for(auto tt:t)
        if(tt==z)
            return true;
    return false;
}

static int int_max(int a, int b) { return a<b?b:a;}

static ivec_t find_pos(words_t &strs)
{
    ivec_t pos;
    int current_dups = strs.size();
    int N = 0;
    for(auto w:strs)
        N = int_max(N,w.length());

    int pos_best = -1;
    int pos_best_val = std::numeric_limits<int>::max();
    while(true)
    {
        for(int i=0; i<N; ++i) {
            ivec_t npos = pos;
            if(has(pos, i))
                continue;
            npos.push_back(i);
            auto hashed = do_hash(strs, npos);
            int d = count_dups(hashed);
            if(d < pos_best_val) {
                pos_best_val = d;
                pos_best = i;
            }
        }
        if(pos_best_val >= current_dups)
            break;
        current_dups = pos_best_val;
        pos.push_back(pos_best);
    }
    auto hashed = do_hash(strs, pos);
    int d = count_dups(hashed);
    //printf("Total Dups: %d\n", d);
    if(d != 0)
        pos.clear();
    return pos;
}

static ivec_t do_hash(const words_t &strs, const ivec_t &pos, const ivec_t &assoc)
{
    ivec_t ivec;
    ivec.reserve(strs.size());
    for(auto &s:strs) {
        int t = s.length();
        for(auto p:pos)
            if(p < (int)s.size())
                t += assoc[s[p]];
        ivec.push_back(t);
    }
    return ivec;
}

static ivec_t find_assoc(const words_t &strs, const ivec_t &pos)
{
    ivec_t assoc;
    int current_dups = strs.size();
    int N = 127;
    std::vector<char> useful_chars;
    for(auto w:strs)
        for(auto c:w)
            if(!has(useful_chars, c))
                useful_chars.push_back(c);

    for(int i=0; i<N; ++i)
        assoc.push_back(0);

    int assoc_best = -1;
    int assoc_best_val = std::numeric_limits<int>::max();;
    for(int k=0; k<4; ++k)
    {
        for(int i:useful_chars) {
            assoc_best_val = std::numeric_limits<int>::max();
            for(int j=0; j<100; ++j) {
                //printf(".");
                assoc[i] = j;
                auto hashed = do_hash(strs, pos, assoc);
                //for(int i=0; i<hashed.size(); ++i)
                //    printf("%d ", hashed[i]);
                //printf("\n");
                int d = count_dups(hashed);
                //printf("dup %d\n",d);
                if(d < assoc_best_val) {
                    assoc_best_val = d;
                    assoc_best = j;
                }
            }
            assoc[i] = assoc_best;
        }
        if(assoc_best_val >= current_dups)
            break;
        current_dups = assoc_best_val;
    }
    auto hashed = do_hash(strs, pos, assoc);
    //int d = count_dups(hashed);
    //printf("Total Dups Assoc: %d\n", d);
    return assoc;
}

static ivec_t find_remap(words_t &strs, ivec_t &pos, ivec_t &assoc)
{
    ivec_t remap;
    auto hashed = do_hash(strs, pos, assoc);
    //for(int i=0; i<strs.size(); ++i)
    //    printf("%d) '%s'\n", hashed[i], strs[i].c_str());
    int N = 0;
    for(auto h:hashed)
        N = int_max(N,h+1);
    for(int i=0; i<N; ++i)
        remap.push_back(0);
    for(int i=0; i<(int)hashed.size(); ++i)
        remap[hashed[i]] = i;

    return remap;
}

static void generate_minimal_hash(std::vector<std::string> str, Port_Matcher &pm)
{
    if(str.empty())
        return;
    pm.pos   = find_pos(str);
    if(pm.pos.empty()) {
        fprintf(stderr, "rtosc: Failed to generate minimal hash\n");
        return;
    }
    pm.assoc = find_assoc(str, pm.pos);
    pm.remap = find_remap(str, pm.pos, pm.assoc);
}

static void generate_minimal_hash(Ports &p, Port_Matcher &pm)
{
    svec_t keys;
    cvec_t args;

    bool enump = false;
    for(unsigned i=0; i<p.ports.size(); ++i)
        if(strchr(p.ports[i].name, '#'))
            enump = true;
    if(enump)
        return;
    for(unsigned i=0; i<p.ports.size(); ++i)
    {
        std::string tmp = p.ports[i].name;
        const char *arg = NULL;
        int idx = tmp.find(':');
        if(idx > 0) {
            arg = p.ports[i].name+idx;
            tmp = tmp.substr(0,idx);
        }
        keys.push_back(tmp);
        args.push_back(arg);

    }
    pm.fixed    = keys;
    pm.arg_spec = args;

    generate_minimal_hash(keys, pm);
}

Ports::Ports(std::initializer_list<Port> l)
    :ports(l), impl(NULL)
{
    refreshMagic();
}

Ports::~Ports()
{
    delete []impl->enump;
    delete impl;
}

#if !defined(__GNUC__)
#define __builtin_expect(a,b) a
#endif

void Ports::dispatch(const char *m, rtosc::RtData &d, bool base_dispatch) const
{
    if(!strcmp(m, "pointer"))
    {
        // rRecur*Cb have already set d.loc to the pointer we need,
        // so we just return
        return;
    }

    void *obj = d.obj;

    //handle the first dispatch layer
    if(base_dispatch) {
        d.matches = 0;
        d.message = m;
        if(m && *m == '/')
            m++;
        if(d.loc)
            d.loc[0] = 0;
    }

    //simple case
    if(!d.loc || !d.loc_size) {
        for(const Port &port: ports) {
            if(rtosc_match(port.name,m, NULL))
                d.port = &port, port.cb(m,d), d.obj = obj;
        }
    } else {

        //TODO this function is certainly buggy at the moment, some tests
        //are needed to make it clean
        //XXX buffer_size is not properly handled yet
        if(__builtin_expect(d.loc[0] == 0, 0)) {
            memset(d.loc, 0, d.loc_size);
            d.loc[0] = '/';
        }

        char *old_end = d.loc;
        while(*old_end) ++old_end;

        if(impl->pos.empty()) { //No perfect minimal hash function
            for(unsigned i=0; i<elms; ++i) {
                const Port &port = ports[i];
                const char* m_end;
                if(!rtosc_match(port.name, m, &m_end))
                    continue;
                if(!port.ports)
                    d.matches++;

                //Append the path
                if(strchr(port.name,'#')) {
                    const char *msg = m;
                    char       *pos = old_end;
                    while(*msg && msg != m_end)
                        *pos++ = *msg++;
                    *pos = '\0';
                } else
                    scat(d.loc, port.name);

                d.port = &port;

                //Apply callback
                port.cb(m,d), d.obj = obj;

                //Remove the rest of the path
                char *tmp = old_end;
                while(*tmp) *tmp++=0;
            }
        } else {

            //Define string to be hashed
            unsigned len=0;
            const char *tmp = m;

            while(*tmp && *tmp != '/')
                tmp++;
            if(*tmp == '/')
                tmp++;
            len = tmp-m;

            //Compute the hash
            int t = len;
            for(auto p:impl->pos)
                if(p < (int)len)
                    t += impl->assoc[m[p]];
            if(t >= (int)impl->remap.size() && !default_handler)
                return;
            else if(t >= (int)impl->remap.size() && default_handler) {
                d.matches++;
                default_handler(m,d), d.obj = obj;
                return;
            }

            int port_num = impl->remap[t];

            //Verify the chosen port is correct
            if(__builtin_expect(impl->hard_match(port_num, m), 1)) {
                const Port &port = ports[impl->remap[t]];
                if(!port.ports)
                    d.matches++;

                //Append the path
                if(impl->enump[port_num]) {
                    const char *msg = m;
                    char       *pos = old_end;
                    while(*msg && *msg != '/')
                        *pos++ = *msg++;
                    if(strchr(port.name, '/'))
                        *pos++ = '/';
                    *pos = '\0';
                } else
                    memcpy(old_end, impl->fixed[port_num].c_str(),
                            impl->fixed[port_num].length()+1);

                d.port = &port;

                //Apply callback
                port.cb(m,d), d.obj = obj;

                //Remove the rest of the path
                old_end[0] = '\0';
            } else if(default_handler) {
                d.matches++;
                default_handler(m,d), d.obj = obj;
            }
        }
    }
}

/*
 * Returning values from runtime
 */

//! RtData subclass to capture argument values pretty-printed from
//! a runtime object
class CapturePretty : public RtData
{
    char* buffer;
    std::size_t buffersize;
    int cols_used;    

    void reply(const char *) override { assert(false); }

/*    void replyArray(const char*, const char *args,
                    rtosc_arg_t *vals)
    {
        size_t cur_idx = 0;
        for(const char* ptr = args; *ptr; ++ptr, ++cur_idx)
        {
            assert(cur_idx < max_args);
            arg_vals[cur_idx].type = *ptr;
            arg_vals[cur_idx].val  = vals[cur_idx];
        }

        // TODO: refactor code, also with Capture ?
        size_t wrt = rtosc_print_arg_vals(arg_vals, cur_idx,
                                          buffer, buffersize, NULL,
                                          cols_used);
        assert(wrt);
    }*/

    void reply(const char *, const char *args, ...) override
    {        
        va_list va;
        va_start(va,args);

        size_t nargs = strlen(args);
        rtosc_arg_val_t arg_vals[nargs];

        rtosc_v2argvals(arg_vals, nargs, args, va);

        size_t wrt = rtosc_print_arg_vals(arg_vals, nargs,
                                          buffer, buffersize, NULL,
                                          cols_used);
        (void) wrt;
        va_end(va);
        assert(wrt);
    }

public:
    //! Return the argument values, pretty-printed
    const char* value() const { return buffer; }
    CapturePretty(char* buffer, std::size_t size, int cols_used) :
        buffer(buffer), buffersize(size), cols_used(cols_used) {}
};

/**
 * @brief Returns a port's value pretty-printed from a runtime object. The
 *        port object must not be known.
 *
 * For the parameters, see the overloaded function
 * @return The argument values, pretty-printed
 */
static const char* get_value_from_runtime(void* runtime,
                                          const Ports& ports,
                                          size_t loc_size,
                                          char* loc,
                                          char* buffer_with_port,
                                          std::size_t buffersize,
                                          int cols_used)
{
    std::size_t addr_len = strlen(buffer_with_port);

    // use the port buffer to print the result, but do not overwrite the
    // port name
    CapturePretty d(buffer_with_port + addr_len, buffersize - addr_len,
                    cols_used);
    d.obj = runtime;
    d.loc_size = loc_size;
    d.loc = loc;
    d.matches = 0;

    // does the message at least fit the arguments?
    assert(buffersize - addr_len >= 8);
    // append type
    memset(buffer_with_port + addr_len, 0, 8); // cover string end and arguments
    buffer_with_port[addr_len + (4-addr_len%4)] = ',';

    d.message = buffer_with_port;

    // buffer_with_port is a message in this call:
    ports.dispatch(buffer_with_port, d, false);

    return d.value();
}

//! RtData subclass to capture argument values from a runtime object
class Capture : public RtData
{
    size_t max_args;
    rtosc_arg_val_t* arg_vals;
    int nargs;

    void chain(const char *path, const char *args, ...) override
    {
        nargs = 0;
    }

    void chain(const char *msg) override
    {
        nargs = 0;
    }

    void reply(const char *) override { assert(false); }

    void replyArray(const char*, const char *args,
                    rtosc_arg_t *vals) override
    {
        size_t cur_idx = 0;
        for(const char* ptr = args; *ptr; ++ptr, ++cur_idx)
        {
            assert(cur_idx < max_args);
            arg_vals[cur_idx].type = *ptr;
            arg_vals[cur_idx].val  = vals[cur_idx];
        }
        nargs = cur_idx;
    }

    void reply(const char *, const char *args, ...) override
    {
        va_list va;
        va_start(va,args);

        nargs = strlen(args);
        assert((size_t)nargs <= max_args);

        rtosc_v2argvals(arg_vals, nargs, args, va);

        va_end(va);
    }
public:
    //! Return the number of argument values stored
    int size() const { return nargs; }
    Capture(std::size_t max_args, rtosc_arg_val_t* arg_vals) :
        max_args(max_args), arg_vals(arg_vals), nargs(-1) {}
};

/**
 * @brief Returns a port's current value(s)
 *
 * This function returns the value(s) of a known port object and stores them as
 * rtosc_arg_val_t.
 * @param runtime The runtime object
 * @param port the port where the value shall be retrieved
 * @param loc A buffer where dispatch can write down the currently dispatched
 *   path
 * @param loc_size Size of loc
 * @param portname_from_base The name of the port, relative to its base
 * @param buffer_with_port A buffer which already contains the port.
 *   This buffer will be modified and must at least have space for 8 more bytes.
 * @param buffersize Size of @p buffer_with_port
 * @param max_args Maximum capacity of @p arg_vals
 * @param arg_vals Argument buffer for returned argument values
 * @return The number of argument values stored in @p arg_vals
 */
static size_t get_value_from_runtime(void* runtime,
                                     const Port& port,
                                     size_t loc_size,
                                     char* loc,
                                     const char* portname_from_base,
                                     char* buffer_with_port,
                                     std::size_t buffersize,
                                     std::size_t max_args,
                                     rtosc_arg_val_t* arg_vals)
{
    strncpy(buffer_with_port, portname_from_base, buffersize);
    std::size_t addr_len = strlen(buffer_with_port);

    Capture d(max_args, arg_vals);
    d.obj = runtime;
    d.loc_size = loc_size;
    d.loc = loc;
    d.port = &port;
    d.matches = 0;
    assert(*loc);

    // does the message at least fit the arguments?
    assert(buffersize - addr_len >= 8);
    // append type
    memset(buffer_with_port + addr_len, 0, 8); // cover string end and arguments
    buffer_with_port[addr_len + (4-addr_len%4)] = ',';

    // TODO? code duplication

    // buffer_with_port is a message in this call:
    d.message = buffer_with_port;
    port.cb(buffer_with_port, d);

    assert(d.size() >= 0);
    return d.size();
}

/*
 * default values
 */

const char* rtosc::get_default_value(const char* port_name, const Ports& ports,
                                     void* runtime, const Port* port_hint,
                                     int32_t idx, int recursive)
{
    constexpr std::size_t buffersize = 1024;
    char buffer[buffersize];
    char loc[buffersize] = "";

    assert(recursive >= 0); // forbid recursing twice

    char default_annotation[20] = "default";
//    if(idx > 0)
//        snprintf(default_annotation + 7, 13, "[%" PRId32 "]", idx);
    const char* const dependent_annotation = "default depends";
    const char* return_value = nullptr;

    if(!port_hint)
        port_hint = ports.apropos(port_name);
    assert(port_hint); // port must be found
    const Port::MetaContainer metadata = port_hint->meta();

    // Let complex cases depend upon a marker variable
    // If the runtime is available the exact preset number can be found
    // This generalizes to envelope types nicely if envelopes have a read
    // only port which indicates if they're amplitude/frequency/etc
    const char* dependent = metadata[dependent_annotation];
    if(dependent)
    {
        char* dependent_port = buffer;
        *dependent_port = 0;

        assert(strlen(port_name) + strlen(dependent_port) + 5 < buffersize);
        strncat(dependent_port, port_name,
                buffersize - strlen(dependent_port) - 1);
        strncat(dependent_port, "/../",
                buffersize - strlen(dependent_port) - 1);
        strncat(dependent_port, dependent,
                buffersize - strlen(dependent_port) - 1);
        dependent_port = Ports::collapsePath(dependent_port);

        // TODO: collapsePath bug?
        // Relative paths should not start with a slash after collapsing ...
        if(*dependent_port == '/')
            ++dependent_port;

        const char* dependent_value =
            runtime
            ? get_value_from_runtime(runtime, ports,
                                     buffersize, loc,
                                     dependent_port,
                                     buffersize-1, 0)
            : get_default_value(dependent_port, ports,
                                runtime, NULL, recursive-1);

        assert(strlen(dependent_value) < 16); // must be an int

        char* default_variant = buffer;
        *default_variant = 0;
        assert(strlen(default_annotation) + 1 + 16 < buffersize);
        strncat(default_variant, default_annotation,
                buffersize - strlen(default_variant));
        strncat(default_variant, " ", buffersize - strlen(default_variant));
        strncat(default_variant, dependent_value,
                buffersize - strlen(default_variant));

        return_value = metadata[default_variant];
    }

    // If return_value is NULL, this can have two meanings:
    //   1. there was no depedent annotation
    //     => check for a direct (non-dependent) default value
    //        (a non existing direct default value is OK)
    //   2. there was a dependent annotation, but the dependent value has no
    //      mapping (mapping for default_variant was NULL)
    //     => check for the direct default value, which acts as a default
    //        mapping for all presets; a missing default value indicates an
    //        error in the metadata
    if(!return_value)
    {
        return_value = metadata[default_annotation];
        assert(!dependent || return_value);
    }

    return return_value;
}

int rtosc::canonicalize_arg_vals(rtosc_arg_val_t* av, size_t n,
                                 const char* port_args,
                                 Port::MetaContainer meta)
{
    const char* first = port_args;
    int errors_found = 0;

    for( ; *first && (*first == ':' || *first == '[' || *first == ']');
           ++first) ;

    for(size_t i = 0; i < n; ++i, ++first, ++av)
    {
        for( ; *first && (*first == '[' || *first == ']'); ++first) ;

        if(!*first || *first == ':')
        {
            // (n-i) arguments left, but we have no recipe to convert them
            return n-i;
        }

        if(av->type == 'S' && *first == 'i')
        {
            int val = enum_key(meta, av->val.s);
            if(val == std::numeric_limits<int>::min())
                ++errors_found;
            else
            {
                av->type = 'i';
                av->val.i = val;
            }
        }
    }
    return errors_found;
}

void rtosc::map_arg_vals(rtosc_arg_val_t* av, size_t n,
                         Port::MetaContainer meta)
{
    char mapbuf[20] = "map ";

    for(size_t i = 0; i < n; ++i, ++av)
    {
        if(av->type == 'i')
        {
            snprintf(mapbuf + 4, 16, "%d", av->val.i);
            const char* val = meta[mapbuf];
            if(val)
            {
                av->type = 'S';
                av->val.s = val;
            }
        }
    }
}

int rtosc::get_default_value(const char* port_name, const char* port_args,
                             const Ports& ports,
                             void* runtime, const Port* port_hint,
                             int32_t idx,
                             size_t n, rtosc_arg_val_t* res,
                             char* strbuf, size_t strbufsize)
{
    const char* pretty = get_default_value(port_name, ports, runtime, port_hint,
                                           idx, 0);

    int nargs;
    if(pretty)
    {
        nargs = rtosc_count_printed_arg_vals(pretty);
        assert(nargs > 0); // parse error => error in the metadata?
        assert((size_t)nargs < n);

        rtosc_scan_arg_vals(pretty, res, nargs, strbuf, strbufsize);

        {
            int errs_found = canonicalize_arg_vals(res,
                                                   nargs,
                                                   port_args,
                                                   port_hint->meta());
            if(errs_found)
                fprintf(stderr, "Could not canonicalize %s\n", pretty);
            assert(!errs_found); // error in the metadata?
        }
    }
    else
        nargs = -1;

    return nargs;
}

std::string rtosc::get_changed_values(const Ports& ports, void* runtime)
{
    std::string res;
    constexpr std::size_t buffersize = 1024;
    char port_buffer[buffersize];
    memset(port_buffer, 0, buffersize); // requirement for walk_ports

    const size_t max_arg_vals = 256;

    auto on_reach_port =
            [](const Port* p, const char* port_buffer,
               const char* port_from_base, const Ports& base,
               void* data, void* runtime)
    {
        assert(runtime);
        const Port::MetaContainer meta = p->meta();

        if((p->name[strlen(p->name)-1] != ':' && !strstr(p->name, "::"))
            || meta.find("parameter") == meta.end())
        {
            // runtime information can not be retrieved,
            // thus, it can not be compared with the default value
            return;
        }

        char loc[buffersize] = "";
        rtosc_arg_val_t arg_vals_default[max_arg_vals];
        rtosc_arg_val_t arg_vals_runtime[max_arg_vals];
        char buffer_with_port[buffersize];
        char cur_value_pretty[buffersize] = " ";
        char strbuf[buffersize]; // temporary string buffer for pretty-printing

        std::string* res = (std::string*)data;
        assert(strlen(port_buffer) + 1 < buffersize);
        strncpy(loc, port_buffer, buffersize); // TODO: +-1?

        strncpy(buffer_with_port, port_from_base, buffersize);
        const char* portargs = strchr(p->name, ':');
        if(!portargs)
            portargs = p->name + strlen(p->name);

#if 0 // debugging stuff
        if(!strncmp(port_buffer, "/part1/Penabled", 5) &&
           !strncmp(port_buffer+6, "/Penabled", 9))
        {
            printf("runtime: %ld\n", (long int)runtime);
        }
#endif
// TODO: p->name: duplicate to p
        int nargs_default = get_default_value(p->name,
                                              portargs,
                                              base,
                                              runtime,
                                              p,
                                              -1,
                                              max_arg_vals,
                                              arg_vals_default,
                                              strbuf,
                                              buffersize);
        size_t nargs_runtime = get_value_from_runtime(runtime,
                                                      *p,
                                                      buffersize, loc,
                                                      port_from_base,
                                                      buffer_with_port,
                                                      buffersize,
                                                      max_arg_vals,
                                                      arg_vals_runtime);

        if(nargs_default == (int) nargs_runtime)
        {
            canonicalize_arg_vals(arg_vals_default, nargs_default,
                                  strchr(p->name, ':'), meta);
            if(!rtosc_arg_vals_eq(arg_vals_default,
                                  arg_vals_runtime,
                                  nargs_default,
                                  nargs_runtime,
                                  NULL))
            {
                map_arg_vals(arg_vals_runtime, nargs_runtime, meta);
                rtosc_print_arg_vals(arg_vals_runtime, nargs_runtime,
                                     cur_value_pretty + 1, buffersize - 1,
                                     NULL, strlen(port_buffer) + 1);

                *res += port_buffer;
                *res += cur_value_pretty;
                *res += "\n";
            }
        }
    };

    walk_ports(&ports, port_buffer, buffersize, &res, on_reach_port,
               runtime);

    if(res.length()) // remove trailing newline
        res.resize(res.length()-1);
    return res;
}

void rtosc::savefile_dispatcher_t::operator()(const char* msg)
{
    *loc = 0;
    RtData d;
    d.obj = runtime;
    d.loc = loc; // we're always dispatching at the base
    d.loc_size = 1024;
    ports->dispatch(msg, d, true);
}

int savefile_dispatcher_t::default_response(size_t nargs,
                                            bool first_round,
                                            savefile_dispatcher_t::dependency_t
                                            dependency)
{
    // default implementation:
    // no dependencies  => round 0,
    // has dependencies => round 1,
    // not specified    => both rounds
    return (dependency == not_specified
            || !(dependency ^ first_round))
           ? nargs // argument number is not changed
           : (int)discard;
}

int savefile_dispatcher_t::on_dispatch(size_t, char *,
                                       size_t, size_t nargs,
                                       rtosc_arg_val_t *,
                                       bool round2,
                                       dependency_t dependency)
{
    return default_response(nargs, round2, dependency);
}

int rtosc::dispatch_printed_messages(const char* messages,
                                     const Ports& ports, void* runtime,
                                     savefile_dispatcher_t* dispatcher)
{
    constexpr std::size_t buffersize = 1024;
    char portname[buffersize], message[buffersize], strbuf[buffersize];
    int rd, rd_total = 0;
    int nargs;
    int msgs_read = 0;

    savefile_dispatcher_t dummy_dispatcher;
    if(!dispatcher)
        dispatcher = &dummy_dispatcher;
    dispatcher->ports = &ports;
    dispatcher->runtime = runtime;

    // scan all messages twice:
    //  * in the second round, only dispatch those with ports that depend on
    //    other ports
    //  * in the first round, only dispatch all others
    for(int round = 0; round < 2 && msgs_read >= 0; ++round)
    {
        msgs_read = 0;
        rd_total = 0;
        const char* msg_ptr = messages;
        while(*msg_ptr && (msgs_read >= 0))
        {
            nargs = rtosc_count_printed_arg_vals_of_msg(msg_ptr);
            if(nargs >= 0)
            {
                // 16 is usually too much, but it allows the user to add
                // arguments if necessary
                size_t maxargs = 16;
                rtosc_arg_val_t arg_vals[maxargs];
                rd = rtosc_scan_message(msg_ptr, portname, buffersize,
                                        arg_vals, nargs, strbuf, buffersize);
                rd_total += rd;

                const Port* port = ports.apropos(portname);
                savefile_dispatcher_t::dependency_t dependency =
                    (savefile_dispatcher_t::dependency_t)
                    (port
                    ? !!port->meta()["default depends"]
                    : (int)savefile_dispatcher_t::not_specified);

                // let the user modify the message and the args
                // the argument number may have changed, or the user
                // wants to discard the message or abort the savefile loading
                nargs = dispatcher->on_dispatch(buffersize, portname,
                                                maxargs, nargs, arg_vals,
                                                round, dependency);

                if(nargs == savefile_dispatcher_t::abort)
                    msgs_read = -rd_total-1; // => causes abort
                else
                {
                    if(nargs != savefile_dispatcher_t::discard)
                    {
                        rtosc_arg_t vals[nargs];
                        char argstr[nargs+1];
                        for(int i = 0; i < nargs; ++i) {
                            vals[i] = arg_vals[i].val;
                            argstr[i] = arg_vals[i].type;
                        }
                        argstr[nargs] = 0;

                        rtosc_amessage(message, buffersize, portname,
                                       argstr, vals);

                        (*dispatcher)(message);
                    }
                }

                msg_ptr += rd;
                ++msgs_read;
            }
            else if(nargs == std::numeric_limits<int>::min())
            {
                // this means the (rest of the) file is whitespace only
                // => don't increase msgs_read
                while(*++msg_ptr) ;
            }
            else {
                // overwrite meaning of msgs_read in order to
                // inform the user where the read error occurred
                msgs_read = -rd_total-1;
            }
        }
    }
    return msgs_read;
}

std::string rtosc::save_to_file(const Ports &ports, void *runtime,
                                const char *appname, rtosc_version appver)
{
    std::string res;
    char rtosc_vbuf[12], app_vbuf[12];

    {
        rtosc_version rtoscver = rtosc_current_version();
        rtosc_version_print_to_12byte_str(&rtoscver, rtosc_vbuf);
        rtosc_version_print_to_12byte_str(&appver, app_vbuf);
    }

    res += "% RT OSC v"; res += rtosc_vbuf; res += " savefile\n"
           "% "; res += appname; res += " v"; res += app_vbuf; res += "\n";
    res += get_changed_values(ports, runtime);

    return res;
}

int rtosc::load_from_file(const char* file_content,
                          const Ports& ports, void* runtime,
                          const char* appname,
                          rtosc_version appver,
                          savefile_dispatcher_t* dispatcher)
{
    char appbuf[128];
    int bytes_read = 0;

    if(dispatcher)
    {
        dispatcher->app_curver = appver;
        dispatcher->rtosc_curver = rtosc_current_version();
    }

    unsigned vma, vmi, vre;
    int n = 0;

    sscanf(file_content,
           "%% RT OSC v%u.%u.%u savefile%n ", &vma, &vmi, &vre, &n);
    if(n <= 0 || vma > 255 || vmi > 255 || vre > 255)
        return -bytes_read-1;
    if(dispatcher)
    {
        dispatcher->rtosc_filever.major = vma;
        dispatcher->rtosc_filever.minor = vmi;
        dispatcher->rtosc_filever.revision = vre;
    }
    file_content += n;
    bytes_read += n;
    n = 0;

    sscanf(file_content,
           "%% %128s v%u.%u.%u%n ", appbuf, &vma, &vmi, &vre, &n);
    if(n <= 0 || strcmp(appbuf, appname) || vma > 255 || vmi > 255 || vre > 255)
        return -bytes_read-1;

    if(dispatcher)
    {
        dispatcher->app_filever.major = vma;
        dispatcher->app_filever.minor = vmi;
        dispatcher->app_filever.revision = vre;
    }
    file_content += n;
    bytes_read += n;
    n = 0;

    int rval = dispatch_printed_messages(file_content,
                                         ports, runtime, dispatcher);
    return (rval < 0) ? (rval-bytes_read) : rval;
}

/*
 * Miscellaneous
 */

const Port *Ports::operator[](const char *name) const
{
    for(const Port &port:ports) {
        const char *_needle = name,
              *_haystack = port.name;
        while(*_needle && *_needle==*_haystack)_needle++,_haystack++;

        if(*_needle == 0 && (*_haystack == ':' || *_haystack == '\0')) {
            return &port;
        }
    }
    return NULL;
}

static msg_t snip(msg_t m)
{
    while(*m && *m != '/') ++m;
    return m+1;
}

const Port *Ports::apropos(const char *path) const
{
    if(path && path[0] == '/')
        ++path;

    for(const Port &port: ports)
        if(strchr(port.name,'/') && rtosc_match_path(port.name,path, NULL))
            return (strchr(path,'/')[1]==0) ? &port :
                port.ports->apropos(snip(path));

    //This is the lowest level, now find the best port
    for(const Port &port: ports)
        if(*path && (strstr(port.name, path)==port.name ||
                    rtosc_match_path(port.name, path, NULL)))
            return &port;

    return NULL;
}

static bool parent_path_p(char *read, char *start)
{
    if(read-start<2)
        return false;
    return read[0]=='.' && read[-1]=='.' && read[-2]=='/';
}

static void read_path(char *&r, char *start)
{
    while(1)
    {
        if(r<start)
            break;
        bool doBreak = *r=='/';
        r--;
        if(doBreak)
            break;
    }
}

static void move_path(char *&r, char *&w, char *start)
{
    while(1)
    {
        if(r<start)
            break;
        bool doBreak = *r=='/';
        *w-- = *r--;
        if(doBreak)
            break;
    }
}


char *Ports::collapsePath(char *p)
{
    //obtain the pointer to the last non-null char
    char *p_end = p;
    while(*p_end) p_end++;
    p_end--;

    //number of subpaths to consume
    int consuming = 0;

    char *write_pos = p_end;
    char *read_pos = p_end;
    while(read_pos >= p) {
        //per path chunk either
        //(1) find a parent ref and inc consuming
        //(2) find a normal ref and consume
        //(3) find a normal ref and write through
        bool ppath = parent_path_p(read_pos, p);
        if(ppath) {
            read_path(read_pos, p);
            consuming++;
        } else if(consuming) {
            read_path(read_pos, p);
            consuming--;
        } else
            move_path(read_pos, write_pos, p);
    }
    //return last written location, not next to write
    return write_pos+1;
};

void Ports::refreshMagic()
{
    delete impl;
    impl = new Port_Matcher;
    generate_minimal_hash(*this, *impl);
    impl->enump = new bool[ports.size()];
    for(int i=0; i<(int)ports.size(); ++i)
        impl->enump[i] = strchr(ports[i].name, '#');

    elms = ports.size();
}

ClonePorts::ClonePorts(const Ports &ports_,
        std::initializer_list<ClonePort> c)
    :Ports({})
{
    for(auto &to_clone:c) {
        const Port *clone_port = NULL;
        for(auto &p:ports_.ports)
            if(!strcmp(p.name, to_clone.name))
                clone_port = &p;
        if(!clone_port && strcmp("*", to_clone.name)) {
            fprintf(stderr, "Cannot find a clone port for '%s'\n",to_clone.name);
            assert(false);
        }

        if(clone_port) {
            ports.push_back({clone_port->name, clone_port->metadata,
                    clone_port->ports, to_clone.cb});
        } else {
            default_handler = to_clone.cb;
        }
    }

    refreshMagic();
}
MergePorts::MergePorts(std::initializer_list<const rtosc::Ports*> c)
    :Ports({})
{
    //XXX TODO remove duplicates in some sane and documented way
    //e.g. repeated ports override and remove older ones
    for(auto *to_clone:c) {
        assert(to_clone);
        for(auto &p:to_clone->ports) {
            bool already_there = false;
            for(auto &pp:ports)
                if(!strcmp(pp.name, p.name))
                    already_there = true;

            if(!already_there)
                ports.push_back(p);
        }
    }

    refreshMagic();
}

/**
 * @brief Check if the port @p port is enabled
 * @param port The port to be checked. Usually of type rRecur* or rSelf.
 * @param loc The absolute path of @p port
 * @param loc_size The maximum usable size of @p loc
 * @param ports The Ports object containing @p port
 * @param runtime TODO
 * @return TODO
 */
bool port_is_enabled(const Port* port, char* loc, size_t loc_size,
                     const Ports& base, void *runtime)
{
    if(port && runtime)
    {
        const char* enable_port = port->meta()["enabled by"];
        if(enable_port)
        {
            /*
                find out which Ports object to dispatch at
                (the current one or its child?)
             */
            const char* n = port->name;
            const char* e = enable_port;
            for( ; *n && (*n == *e) && *n != '/' && *e != '/'; ++n, ++e) ;

            bool subport = (*e == '/' && *n == '/');

            const char* ask_port_str = subport
                                       ? e+1
                                       : enable_port;

            const Ports& ask_ports = subport ? *base[port->name]->ports
                                             : base;

            assert(!strchr(ask_port_str, '/'));
            const Port* ask_port = ask_ports[ask_port_str];
            assert(ask_port);

            rtosc_arg_val_t rval;

            /*
                concatenate the location string
             */
            if(subport)
                strncat(loc, "/../", loc_size - strlen(loc) - 1);
            strncat(loc, enable_port, loc_size - strlen(loc) - 1);

            char* collapsed_loc = Ports::collapsePath(loc);
            loc_size -= (collapsed_loc - loc);

// TODO: collapse, use .. only in one case
            /*
                receive the "enabled" property
             */
            char buf[loc_size];
#ifdef NEW_CODE
            strncpy(buf, collapsed_loc, loc_size);
#else
            // TODO: try to use portname_from_base, since Ports might
            //       also be of type a#N/b
            const char* last_slash = strrchr(collapsed_loc, '/');
            strncpy(buf,
                    last_slash ? last_slash + 1 : collapsed_loc,
                    loc_size);
#endif
            get_value_from_runtime(runtime, *ask_port,
                                   loc_size, collapsed_loc, ask_port_str,
                                   buf, 0, 1, &rval);
            assert(rval.type == 'T' || rval.type == 'F');
            return rval.val.T == 'T';
        }
        else // Port has no "enabled" property, so it is always enabled
            return true;
    }
    else // no runtime provided, so run statically through all subports
        return true;
}

// TODO: copy the changes into walk_ports_2
void rtosc::walk_ports(const Ports  *base,
                       char         *name_buffer,
                       size_t        buffer_size,
                       void         *data,
                       port_walker_t walker,
                       void*         runtime)
{
    auto walk_ports_recurse = [](const Port& p, char* name_buffer,
                                 size_t buffer_size, const Ports& base,
                                 void* data, port_walker_t walker,
                                 void* runtime, const char* old_end)
    {
        // TODO: all/most of these checks must also be done for the
        // first, non-recursive call
        bool enabled = true;
        if(runtime)
        {
            enabled = (p.meta().find("no walk") == p.meta().end());
            if(enabled)
            {
                // get child runtime and check if it's NULL
                RtData r;
                r.obj = runtime;
                r.port = &p;

                char buf[1024];
                strncpy(buf, old_end, 1024);
                strncat(buf, "pointer", 1024 - strlen(buf) - 1);
                assert(1024 - strlen(buf) >= 8);
                strncpy(buf + strlen(buf) + 1, ",", 2);

                p.cb(buf, r);
                runtime = r.obj; // callback has stored the child pointer here
                // if there is runtime information, but the pointer is NULL,
                // the port is not enabled
                enabled = (bool) runtime;
                if(enabled)
                {
                    // check if the port is disabled by a switch
                    enabled = port_is_enabled(&p, name_buffer, buffer_size,
                                              base, runtime);
                }
            }
        }
        if(enabled)
            rtosc::walk_ports(p.ports, name_buffer, buffer_size,
                              data, walker, runtime);
    };

    //only walk valid ports
    if(!base)
        return;

    assert(name_buffer);
    //XXX buffer_size is not properly handled yet
    if(name_buffer[0] == 0)
        name_buffer[0] = '/';

    char *old_end         = name_buffer;
    while(*old_end) ++old_end;

    if(port_is_enabled((*base)["self:"], name_buffer, buffer_size, *base,
                       runtime))
    for(const Port &p: *base) {
        //if(strchr(p.name, '/')) {//it is another tree
        if(p.ports) {//it is another tree
            if(strchr(p.name,'#')) {
                const char *name = p.name;
                char       *pos  = old_end;
                while(*name != '#') *pos++ = *name++;
                const unsigned max = atoi(name+1);

                for(unsigned i=0; i<max; ++i)
                {
                    sprintf(pos,"%d",i);

                    //Ensure the result is a path
                    if(strrchr(name_buffer, '/')[1] != '/')
                        strcat(name_buffer, "/");

                    //Recurse
                    walk_ports_recurse(p, name_buffer, buffer_size,
                                       *base, data, walker, runtime, old_end);
                }
            } else {
                //Append the path
                const char* old_end = name_buffer + strlen(name_buffer);
                scat(name_buffer, p.name);

                //Recurse
                walk_ports_recurse(p, name_buffer, buffer_size,
                                   *base, data, walker, runtime, old_end);
            }
        } else {
            if(strchr(p.name,'#')) {
                const char *name = p.name;
                char       *pos  = old_end;
                while(*name != '#') *pos++ = *name++;
                const unsigned max = atoi(name+1);
                while(isdigit(*++name)) ;

                for(unsigned i=0; i<max; ++i)
                {
                    char* pos_after_num = pos + sprintf(pos,"%d",i);
                    const char* name2_2 = name;

                    // append everything behind the '#' (for cases like a#N/b)
                    while(*name2_2 && *name2_2 != ':')
                        *pos_after_num++ = *name2_2++;

                    //Apply walker function
                    walker(&p, name_buffer, old_end, *base, data, runtime);
                }
            } else {
                //Append the path
                scat(name_buffer, p.name);

                //Apply walker function
                walker(&p, name_buffer, old_end, *base, data, runtime);
            }
        }

        //Remove the rest of the path
        char *tmp = old_end;
        while(*tmp) *tmp++=0;
    }
}

void walk_ports2(const rtosc::Ports *base,
                 char         *name_buffer,
                 size_t        buffer_size,
                 void         *data,
                 rtosc::port_walker_t walker)
{
    if(!base)
        return;

    assert(name_buffer);
    //XXX buffer_size is not properly handled yet
    if(name_buffer[0] == 0)
        name_buffer[0] = '/';

    char *old_end         = name_buffer;
    while(*old_end) ++old_end;

    for(const rtosc::Port &p: *base) {
        if(strchr(p.name, '/')) {//it is another tree
            if(strchr(p.name,'#')) {
                const char *name = p.name;
                char       *pos  = old_end;
                while(*name != '#') *pos++ = *name++;
                const unsigned max = atoi(name+1);

                //for(unsigned i=0; i<max; ++i)
                {
                    sprintf(pos,"[0,%d]",max-1);

                    //Ensure the result is a path
                    if(strrchr(name_buffer, '/')[1] != '/')
                        strcat(name_buffer, "/");

                    //Recurse
                    walk_ports2(p.ports, name_buffer, buffer_size,
                            data, walker);
                }
            } else {
                //Append the path
                scat(name_buffer, p.name);

                //Recurse
                walk_ports2(p.ports, name_buffer, buffer_size,
                        data, walker);
            }
        } else {
            if(strchr(p.name,'#')) {
                const char *name = p.name;
                char       *pos  = old_end;
                while(*name != '#') *pos++ = *name++;
                const unsigned max = atoi(name+1);

                //for(unsigned i=0; i<max; ++i)
                {
                    sprintf(pos,"[0,%d]",max-1);

                    //Apply walker function
                    walker(&p, name_buffer, old_end, *base, data, nullptr);
                }
            } else {
                //Append the path
                scat(name_buffer, p.name);

                //Apply walker function
                walker(&p, name_buffer, old_end, *base, data, nullptr);
            }
        }

        //Remove the rest of the path
        char *tmp = old_end;
        while(*tmp) *tmp++=0;
    }
}

static void units(std::ostream &o, const char *u)
{
    if(!u)
        return;
    o << " units=\"" << u << "\"";
}

using std::ostream;
using std::string;
static int enum_min(Port::MetaContainer meta)
{
    int min = 0;
    for(auto m:meta)
        if(strstr(m.title, "map "))
            min = atoi(m.title+4);

    for(auto m:meta)
        if(strstr(m.title, "map "))
            min = min>atoi(m.title+4) ? atoi(m.title+4) : min;

    return min;
}

static int enum_max(Port::MetaContainer meta)
{
    int max = 0;
    for(auto m:meta)
        if(strstr(m.title, "map "))
            max = atoi(m.title+4);

    for(auto m:meta)
        if(strstr(m.title, "map "))
            max = max<atoi(m.title+4) ? atoi(m.title+4) : max;

    return max;
}

int rtosc::enum_key(Port::MetaContainer meta, const char* value)
{
    int result = std::numeric_limits<int>::min();

    for(auto m:meta)
    if(strstr(m.title, "map "))
    if(!strcmp(m.value, value))
    {
        result = atoi(m.title+4);
        break;
    }

    return result;
}

static ostream &add_options(ostream &o, Port::MetaContainer meta)
{
    string sym_names = "xyzabcdefghijklmnopqrstuvw";
    int sym_idx = 0;
    bool has_options = false;
    for(auto m:meta)
        if(strstr(m.title, "map "))
            has_options = true;
    for(auto m:meta)
        if(strcmp(m.title, "documentation") &&
                strcmp(m.title, "parameter") &&
                strcmp(m.title, "max") &&
                strcmp(m.title, "min"))
        printf("m.title = <%s>\n", m.title);

    if(!has_options)
        return o;

    o << "    <hints>\n";
    for(auto m:meta) {
        if(strstr(m.title, "map ")) {
            o << "      <point symbol=\"" << sym_names[sym_idx++] << "\" value=\"";
            o << m.title+4 << "\">" << m.value << "</point>\n";
        }
    }
    o << "    </hints>\n";

    return o;
}
static ostream &dump_t_f_port(ostream &o, string name, string doc)
{
    o << " <message_in pattern=\"" << name << "\" typetag=\"T\">\n";
    o << "  <desc>Enable " << doc << "</desc>\n";
    o << "  <param_T symbol=\"x\"/>\n";
    o << " </message_in>\n";
    o << " <message_in pattern=\"" << name << "\" typetag=\"F\">\n";
    o << "  <desc>Disable "  << doc << "</desc>\n";
    o << "  <param_F symbol=\"x\"/>\n";
    o << " </message_in>\n";
    o << " <message_in pattern=\"" << name << "\" typetag=\"\">\n";
    o << "  <desc>Get state of " << doc << "</desc>\n";
    o << " </message_in>\n";
    o << " <message_out pattern=\"" << name << "\" typetag=\"T\">\n";
    o << "  <desc>Value of " << doc << "</desc>\n";
    o << "  <param_T symbol=\"x\"/>";
    o << " </message_out>\n";
    o << " <message_out pattern=\"" << name << "\" typetag=\"F\">\n";
    o << "  <desc>Value of " <<  doc << "</desc>\n";
    o << "  <param_F symbol=\"x\"/>";
    o << " </message_out>\n";
    return o;
}
static ostream &dump_any_port(ostream &o, string name, string doc)
{
    o << " <message_in pattern=\"" << name << "\" typetag=\"*\">\n";
    o << "  <desc>" << doc << "</desc>\n";
    o << " </message_in>\n";
    return o;
}

static ostream &dump_generic_port(ostream &o, string name, string doc, string type)
{
    const char *t = type.c_str();
    string arg_names = "xyzabcdefghijklmnopqrstuvw";

    //start out with argument separator
    if(*t++ != ':')
        return o;
    //now real arguments (assume [] don't exist)
    string args;
    while(*t && *t != ':')
        args += *t++;

    o << " <message_in pattern=\"" << name << "\" typetag=\"" << args << "\">\n";
    o << "  <desc>" << doc << "</desc>\n";

    assert(args.length()<arg_names.length());
    for(unsigned i=0; i<args.length(); ++i)
        o << "  <param_" << args[i] << " symbol=\"" << arg_names[i] << "\"/>\n";
    o << " </message_in>\n";

    if(*t == ':')
        return dump_generic_port(o, name, doc, t);
    else
        return o;
}

void dump_ports_cb(const rtosc::Port *p, const char *name,const char*,
                   const Ports&,void *v, void*)
{
    std::ostream &o  = *(std::ostream*)v;
    auto meta        = p->meta();
    const char *args = strchr(p->name, ':');
    auto mparameter  = meta.find("parameter");
    auto mdoc        = meta.find("documentation");
    string doc;

    if(mdoc != p->meta().end())
        doc = mdoc.value;
    if(meta.find("internal") != meta.end()) {
        doc += "[INTERNAL]";
    }

    if(mparameter != p->meta().end()) {
        char type = 0;
        if(args) {
            if(strchr(args, 'f'))
                type = 'f';
            else if(strchr(args, 'i'))
                type = 'i';
            else if(strchr(args, 'c'))
                type = 'c';
            else if(strchr(args, 'T'))
                type = 't';
            else if(strchr(args, 's'))
                type = 's';
        }

        if(!type) {
            fprintf(stderr, "rtosc port dumper: Cannot handle '%s'\n", name);
            fprintf(stderr, "    args = <%s>\n", args);
            return;
        }

        if(type == 't') {
            dump_t_f_port(o, name, doc);
            return;
        }

        o << " <message_in pattern=\"" << name << "\" typetag=\"" << type << "\">\n";
        o << "  <desc>Set Value of " << doc << "</desc>\n";
        if(meta.find("min") != meta.end() && meta.find("max") != meta.end() && type != 'c') {
            o << "  <param_" << type << " symbol=\"x\"";
            units(o, meta["unit"]);
            o << ">\n";
            o << "   <range_min_max " << (type == 'f' ? "lmin=\"[\" lmax=\"]\"" : "");
            o << " min=\"" << meta["min"] << "\"  max=\"" << meta["max"] << "\"/>\n";
            o << "  </param_" << type << ">";
        } else if(meta.find("enumerated") != meta.end()) {
            o << "  <param_" << type << " symbol=\"x\">\n";
            o << "    <range_min_max min=\"" << enum_min(meta) << "\" max=\"";
            o << enum_max(meta) << "\">\n";
            add_options(o, meta);
            o << "    </range_min_max>\n";
            o << "  </param_" << type << ">\n";
        } else {
            o << "  <param_" << type << " symbol=\"x\"";
            units(o, meta["unit"]);
            o << "/>\n";
        }
        o << " </message_in>\n";
        o << " <message_in pattern=\"" << name << "\" typetag=\"\">\n";
        o << "  <desc>Get Value of " << doc << "</desc>\n";
        o << " </message_in>\n";
        o << " <message_out pattern=\"" << name << "\" typetag=\"" << type << "\">\n";
        o << "  <desc>Value of " << doc << "</desc>\n";
        if(meta.find("min") != meta.end() && meta.find("max") != meta.end() && type != 'c') {
            o << "  <param_" << type << " symbol=\"x\"";
            units(o, meta["unit"]);
            o << ">\n";
            o << "   <range_min_max " << (type == 'f' ? "lmin=\"[\" lmax=\"]\"" : "");
            o << " min=\"" << meta["min"] << "\"  max=\"" << meta["max"] << "\"/>\n";
            o << "  </param_" << type << ">\n";
        } else if(meta.find("enumerated") != meta.end()) {
            o << "  <param_" << type << " symbol=\"x\">\n";
            o << "    <range_min_max min=\"" << enum_min(meta) << "\" max=\"";
            o << enum_max(meta) << "\">\n";
            add_options(o, meta);
            o << "    </range_min_max>\n";
            o << "  </param_" << type << ">\n";
        } else {
            o << "  <param_" << type << " symbol=\"x\"";
            units(o, meta["unit"]);
            o << "/>\n";
        }
        o << " </message_out>\n";
    } else if(mdoc != meta.end() && (!args || args == std::string(""))) {
        dump_any_port(o, name, doc);
    } else if(mdoc != meta.end() && args) {
        dump_generic_port(o, name, doc, args);
    } else if(mdoc != meta.end()) {
        fprintf(stderr, "Skipping \"%s\"\n", name);
        if(args) {
            fprintf(stderr, "    type = %s\n", args);
        }
    } else
        fprintf(stderr, "Skipping [UNDOCUMENTED] \"%s\"\n", name);
}

std::ostream &rtosc::operator<<(std::ostream &o, rtosc::OscDocFormatter &formatter)
{
    o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    o << "<osc_unit format_version=\"1.0\">\n";
    o << " <meta>\n";
    o << "  <name>" << formatter.prog_name << "</name>\n";
    o << "  <uri>" << formatter.uri << "</uri>\n";
    o << "  <doc_origin>" << formatter.doc_origin << "</doc_origin>\n";
    o << "  <author><firstname>" << formatter.author_first;
    o << "</firstname><lastname>" << formatter.author_last << "</lastname></author>\n";
    o << " </meta>\n";
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    walk_ports2(formatter.p, buffer, 1024, &o, dump_ports_cb);
    o << "</osc_unit>\n";
    return o;
}

