/*
 * Copyright (c) 2012 Mark McCurry
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * @file ports.h
 */

#ifndef RTOSC_PORTS
#define RTOSC_PORTS

#include <vector>
#include <functional>
#include <initializer_list>
#include <rtosc/rtosc.h>
#include <rtosc/rtosc-version.h>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <string>

namespace rtosc {

//First define all types
typedef const char *msg_t;

struct Port;
struct Ports;

//! data object for the dispatch routine
struct RtData
{
    RtData(void);

    /**
     * @brief location of where the dispatch routine is currently being called
     *
     * If non-NULL, the dispatch routine will update the port name here while
     * walking through the Ports tree
     */
    char *loc;
    size_t loc_size;
    void *obj;        //!< runtime object to dispatch this object to
    int  matches;     //!< number of matches returned from dispatch routine
    const Port *port; //!< dispatch will write the matching port's pointer here
    //! @brief Will be set to point to the full OSC message in case of
    //!   a base dispatch
    const char *message;

    int idx[16];
    void push_index(int ind);
    void pop_index(void);

    virtual void replyArray(const char *path, const char *args,
            rtosc_arg_t *vals);
    virtual void reply(const char *path, const char *args, ...);
    //!Reply if information has been requested
    virtual void reply(const char *msg);
    virtual void chain(const char *path, const char *args, ...);
    //!Bypass message to some kind of backend if the message can not be handled
    virtual void chain(const char *msg);
    virtual void chainArray(const char *path, const char *args,
            rtosc_arg_t *vals);
    //!Transmit initialization/change of a value to all listeners
    virtual void broadcast(const char *path, const char *args, ...);
    virtual void broadcast(const char *msg);
    virtual void broadcastArray(const char *path, const char *args,
            rtosc_arg_t *vals);

    virtual void forward(const char *rational=NULL);
};


/**
 * Port in rtosc dispatching hierarchy
 */
struct Port {
    const char  *name;    //!< Pattern for messages to match
    const char  *metadata;//!< Statically accessable data about port
    const Ports *ports;   //!< Pointer to further ports
    std::function<void(msg_t, RtData&)> cb;//!< Callback for matching functions

    class MetaIterator
    {
        public:
            MetaIterator(const char *str);

            //A bit odd to return yourself, but it seems to work for this
            //context
            const MetaIterator& operator*(void) const {return *this;}
            const MetaIterator* operator->(void) const {return this;}
            bool operator==(MetaIterator a) {return title == a.title;}
            bool operator!=(MetaIterator a) {return title != a.title;}
            MetaIterator& operator++(void);
            operator bool() const;

            const char *title;
            const char *value;
    };

    class MetaContainer
    {
        public:
            MetaContainer(const char *str_);

            MetaIterator begin(void) const;
            MetaIterator end(void) const;

            MetaIterator find(const char *str) const;
            size_t length(void) const;
            //!Return the key to the value @p str, or NULL if the key is
            //!invalid or if there's no value for that key.
            const char *operator[](const char *str) const;

            const char *str_ptr;
    };

    MetaContainer meta(void) const
    {
        if(metadata && *metadata == ':')
            return MetaContainer(metadata+1);
        else
            return MetaContainer(metadata);
    }

};

/**
 * Ports - a dispatchable collection of Port entries
 *
 * This structure makes it somewhat easier to perform actions on collections of
 * port entries and it is responsible for the dispatching of OSC messages to
 * their respective ports.
 * That said, it is a very simple structure, which uses a stl container to store
 * all data in a simple dispatch table.
 * All methods post-initialization are RT safe (assuming callbacks are RT safe)
 */
struct Ports
{
    std::vector<Port> ports;
    std::function<void(msg_t, RtData&)> default_handler;

    typedef std::vector<Port>::const_iterator itr_t;

    /**Forwards to builtin container*/
    itr_t begin() const {return ports.begin();}

    /**Forwards to builtin container*/
    itr_t end() const {return ports.end();}

    /**Forwards to builtin container*/
    size_t size() const {return ports.size();}

    /**Forwards to builtin container*/
    const Port &operator[](unsigned i) const {return ports[i];}

    Ports(std::initializer_list<Port> l);
    ~Ports(void);

    Ports(const Ports&) = delete;

    /**
     * Dispatches message to all matching ports.
     * This uses simple pattern matching available in rtosc::match.
     *
     * @param m A valid OSC message. Note that the address part shall not
     *          contain any type specifier.
     * @param d The RtData object shall contain a path buffer (or null), the length of
     *          the buffer, a pointer to data. It must also contain the location
     *          if an answer is being expected.
     * @param base_dispatch Whether the OSC path is to be interpreted as a full
     *                      OSC path beginning at the root
     */
    void dispatch(const char *m, RtData &d, bool base_dispatch=false) const;

    /**
     * Retrieve local port by name
     * TODO implement full matching
     */
    const Port *operator[](const char *name) const;


    /**
     * Find the best match for a given path
     *
     * @parameter path partial OSC path
     * @returns first path prefixed by the argument
     *
     * Example usage:
     * @code
     *    Ports p = {{"foo",0,0,dummy_method},
     *               {"flam",0,0,dummy_method},
     *               {"bar",0,0,dummy_method}};
     *    p.apropos("/b")->name;//bar
     *    p.apropos("/f")->name;//foo
     *    p.apropos("/fl")->name;//flam
     *    p.apropos("/gg");//NULL
     * @endcode
     */
    const Port *apropos(const char *path) const;

    /**
     * Collapse path with parent path identifiers "/.."
     *
     * e.g. /foo/bar/../baz => /foo/baz
     */
    static char *collapsePath(char *p);

    protected:
    void refreshMagic(void);
    private:
    //Performance hacks
    class Port_Matcher *impl;
    unsigned elms;
};

struct ClonePort
{
    const char *name;
    std::function<void(msg_t, RtData&)> cb;
};

struct ClonePorts:public Ports
{
    ClonePorts(const Ports &p,
               std::initializer_list<ClonePort> c);
};

struct MergePorts:public Ports
{
    MergePorts(std::initializer_list<const Ports*> c);
};

/**
 * @brief Returns a port's default value
 *
 * Returns the default value of a given port, if any exists, as a string.
 * For the parameters, see the overloaded function.
 * @note The recursive parameter should never be specified.
 * @return The default value(s), pretty-printed, or NULL if there is no
 *     valid default annotation
 */
const char* get_default_value(const char* port_name, const Ports& ports,
                              void* runtime, const Port* port_hint = NULL,
                              int32_t idx = -1, int recursive = 1);

/**
 * @brief Returns a port's default value
 *
 * Returns the default value of a given port, if any exists, as an array of
 * rtosc_arg_vals . The values in the resulting array are being canonicalized,
 * i.e. mapped values are being converted to integers; see
 * canonicalize_arg_vals() .
 *
 * @param port_name the port's OSC path.
 * @param port_args the port's arguments, e.g. '::i:c:S'
 * @param ports the ports where @a portname is to be searched
 * @param runtime object holding @a ports . Optional. Helps finding
 *        default values dependent on others, such as presets.
 * @param port_hint The port itself corresponding to portname (including
 *        the args). If not specified, will be found using @p portname .
 * @param idx If the port is an array (using the '#' notation), this specifies
 *        the index required for the default value
 * @param n Size of the output parameter @res . This size can not be known,
 *        so you should provide a large enough array.
 * @param res The output parameter for the argument values.
 * @param strbuf String buffer for storing pretty printed strings and blobs.
 * @param strbufsize Size of @p strbuf
 * @return The actual number of aruments written to @p res (can be smaller
 *         than @p n) or -1 if there is no valid default annotation
 */
int get_default_value(const char* port_name, const char *port_args,
                      const Ports& ports,
                      void* runtime, const Port* port_hint,
                      int32_t idx,
                      size_t n, rtosc_arg_val_t* res,
                      char *strbuf, size_t strbufsize);


/**
 * @brief Return a string list of all changed values
 *
 * Return a human readable list of the value that changed
 * corresponding to the rDefault macro
 * @param ports The static ports structure
 * @param runtime The runtime object
 * @return The list of ports and their changed values, linewise
 */
std::string get_changed_values(const Ports& ports, void* runtime);

//! @brief Class to modify messages loaded from savefiles
//!
//! Object of this class shall be passed to savefile loading routines. You can
//! inherit to change the behaviour, e.g. to modify or discard such messages.
class savefile_dispatcher_t
{
    const Ports* ports;
    void* runtime;
    char loc[1024];

protected:
    enum proceed {
        abort = -2, //!< the message shall lead to abort the savefile loading
        discard = -1 //!< the message shall not be dispatched
    };

    enum dependency_t {
        no_dependencies,  //! default values don't depend on others
        has_dependencies, //! default values do depend on others
        not_specified     //! it's not know which of the other enum values fit
    };

    rtosc_version rtosc_filever, //!< rtosc versinon savefile was written with
                  rtosc_curver, //!< rtosc version of this library
                  app_filever, //!< app version savefile was written with
                  app_curver; //!< current app version

    //! call this to dispatch a message
    void operator()(const char* msg);

    static int default_response(size_t nargs, bool first_round,
                                dependency_t dependency);

private:
    //! callback for when a message shall be dispatched
    //! implement this if you need to change a message
    virtual int on_dispatch(size_t portname_max, char* portname,
                            size_t maxargs, size_t nargs,
                            rtosc_arg_val_t* args,
                            bool round2, dependency_t dependency);

    friend int dispatch_printed_messages(const char* messages,
                                         const Ports& ports, void* runtime,
                                         savefile_dispatcher_t *dispatcher);

    friend int load_from_file(const char* file_content,
                              const Ports& ports, void* runtime,
                              const char* appname,
                              rtosc_version appver,
                              savefile_dispatcher_t* dispatcher);
};

/**
 * @brief Scan OSC messages from human readable format and dispatch them
 *
 * @param messages The OSC messages, whitespace-separated
 * @param ports The static ports structure
 * @param runtime The runtime object
 * @param dispatcher Object to modify messages prior to dispatching, or NULL.
 *   You can overwrite its virtual functions, and you should specify any of the
 *   version structs if needed. All other members shall not be initialized.
 * @return The number of messages read, or, if there was a read error,
 *   the number of bytes read until the read error occured minus one
 */
int dispatch_printed_messages(const char* messages,
                              const Ports& ports, void* runtime,
                              savefile_dispatcher_t *dispatcher = NULL);

/**
 * @brief Return a savefile containing all values that differ from the default
 *   values.
 *
 * @param ports The static ports structure
 * @param runtime The runtime object
 * @param appname Name of the application calling this function
 * @param appver Version of the application calling this function
 * @return The resulting savefile as an std::sting
 */
std::string save_to_file(const Ports& ports, void* runtime,
                         const char* appname, rtosc_version appver);

/**
 * @brief Read save file and dispatch contained parameters
 *
 * @param file_content The file as a C string
 * @param ports The static ports structure
 * @param runtime The runtime object
 * @param appname Name of the application calling this function; must
 *   match the file's application name
 * @param appver Version of the application calling this function
 * @param dispatcher Modifier for the messages; NULL if no modifiers are needed
 * @return The number of messages read, or, if there was a read error,
 *   the negated number of bytes read until the read error occured minus one
 */
int load_from_file(const char* file_content,
                   const Ports& ports, void* runtime,
                   const char* appname,
                   rtosc_version appver,
                   savefile_dispatcher_t* dispatcher = NULL);

/**
 * @brief Convert given argument values to their canonical representation, e.g.
 * to the ports first (or-wise) argument types.
 *
 * E.g. if passing two 'S' argument values, the
 * port could be `portname::ii:cc:SS` or `portname::ii:t`.
 *
 * @param av The input and output argument values
 * @param n The size of @p av
 * @param port_args The port arguments string, e.g. `::i:c:s`. The first
 *   non-colon letter sequence marks the canonical types
 * @param meta The port's metadata container
 * @return The number of argument values that should have need conversion,
 *   but failed, e.g. because of values missing in rMap.
 */
int canonicalize_arg_vals(rtosc_arg_val_t* av, size_t n,
                          const char* port_args, Port::MetaContainer meta);

/**
 * @brief Converts each of the given arguments to their mapped symbol, if
 *        possible
 * @param av The input and output argument values
 * @param n The size of @p av
 * @param meta The port's metadata container
 */
void map_arg_vals(rtosc_arg_val_t* av, size_t n,
                  Port::MetaContainer meta);

/*********************
 * Port walking code *
 *********************/
//typedef std::function<void(const Port*,const char*)> port_walker_t;
/**
 * @brief function pointer type for port walking
 *
 * accepts:
 * - the currently walked port
 * - the port's absolute location
 * - the part of the location which makes up the port; this is usually the
 *   location's substring after the last slash, but it can also contain multiple
 *   slashes
 * - the port's base, i.e. it's parent Ports struct
 * - the custom data supplied to walk_ports
 * - the runtime object (which may be NULL if not known)
 */
typedef void(*port_walker_t)(const Port*,const char*,const char*,
                             const Ports&,void*,void*);

/**
 * @brief Call a function on all ports and subports
 * @param base The base port of traversing
 * @param name_buffer Buffer which will be filled with the port name; must be
 *   reset to zero over the full length!
 * @param buffer_size Size of name_buffer
 * @param data Data that should be available in the callback
 * @param walker Callback function
 */
void walk_ports(const Ports *base,
                char          *name_buffer,
                size_t         buffer_size,
                void          *data,
                port_walker_t  walker,
                void *runtime = NULL);

/**
 * @brief Return the index with value @p value from the metadata's enumeration
 * @param meta The metadata
 * @param value The value to search the key for
 * @return The first key holding value, or `std::numeric_limits<int>::min()`
 *   if none was found
 */
int enum_key(Port::MetaContainer meta, const char* value);

/*********************
 * Port Dumping code *
 *********************/

struct OscDocFormatter
{
    const Ports *p;
    std::string prog_name;
    std::string uri;
    std::string doc_origin;
    std::string author_first;
    std::string author_last;
    //TODO extend this some more
};

std::ostream &operator<<(std::ostream &o, OscDocFormatter &formatter);
};
#endif
