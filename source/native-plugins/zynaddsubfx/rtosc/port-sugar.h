/*
 * Copyright (c) 2016 Mark McCurry
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

#include <assert.h>
#include <type_traits>

#ifndef RTOSC_PORT_SUGAR
#define RTOSC_PORT_SUGAR

//Hack to workaround old incomplete decltype implementations
#ifdef __GNUC__
#ifndef __clang__
#if    __GNUC__ < 4 ||  (__GNUC__ == 4 && __GNUC_MINOR__ <= 7)
template<typename T>
struct rtosc_hack_decltype_t
{
    typedef T type;
};

#define decltype(expr) rtosc_hack_decltype_t<decltype(expr)>::type
#endif
#endif
#endif

//General macro utilities
#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY2(a)

//Helper for documenting varargs
#define IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,N, ...) N
#define LAST_IMP(...) IMPL(__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0)
#define DOC_IMP9(a,b,c,d,e,f,g,h,i) a b c d e f g h rDoc(i)
#define DOC_IMP8(a,b,c,d,e,f,g,h)   a b c d e f g rDoc(h)
#define DOC_IMP7(a,b,c,d,e,f,g)     a b c d e f rDoc(g)
#define DOC_IMP6(a,b,c,d,e,f)       a b c d e rDoc(f)
#define DOC_IMP5(a,b,c,d,e)         a b c d rDoc(e)
#define DOC_IMP4(a,b,c,d)           a b c rDoc(d)
#define DOC_IMP3(a,b,c)             a b rDoc(c)
#define DOC_IMP2(a,b)               a rDoc(b)
#define DOC_IMP1(a)                 rDoc(a)
#define DOC_IMP0() YOU_MUST_DOCUMENT_YOUR_PORTS
#define DOC_IMP(count, ...) DOC_IMP ##count(__VA_ARGS__)
#define DOC_I(count, ...) DOC_IMP(count,__VA_ARGS__)
#define DOC(...) DOC_I(LAST_IMP(__VA_ARGS__), __VA_ARGS__)


#define rINC(x) rINC_ ## x
#define rINC_0 1
#define rINC_1 2
#define rINC_2 3
#define rINC_3 4
#define rINC_4 5
#define rINC_5 6
#define rINC_6 7
#define rINC_7 8
#define rINC_8 9
#define rINC_9 10
#define rINC_10 11
#define rINC_11 12
#define rINC_12 13
#define rINC_13 14
#define rINC_14 15
#define rINC_15 16

//Helper for applying macro on varargs
//arguments: counting offset, macro, macro args
#define MAC_EACH_0(o, m, x, ...) INSUFFICIENT_ARGUMENTS_PROVIDED_TO_MAC_EACH
#define MAC_EACH_1(o, m, x, ...) m(o, x)
#define MAC_EACH_2(o, m, x, ...) m(o, x) MAC_EACH_1(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_3(o, m, x, ...) m(o, x) MAC_EACH_2(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_4(o, m, x, ...) m(o, x) MAC_EACH_3(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_5(o, m, x, ...) m(o, x) MAC_EACH_4(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_6(o, m, x, ...) m(o, x) MAC_EACH_5(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_7(o, m, x, ...) m(o, x) MAC_EACH_6(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_8(o, m, x, ...) m(o, x) MAC_EACH_7(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_9(o, m, x, ...) m(o, x) MAC_EACH_8(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_10(o, m, x, ...) m(o, x) MAC_EACH_9(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_11(o, m, x, ...) m(o, x) MAC_EACH_10(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_12(o, m, x, ...) m(o, x) MAC_EACH_11(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_13(o, m, x, ...) m(o, x) MAC_EACH_12(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_14(o, m, x, ...) m(o, x) MAC_EACH_13(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_15(o, m, x, ...) m(o, x) MAC_EACH_14(rINC(o), m, __VA_ARGS__)
#define MAC_EACH_16(o, m, x, ...) m(o, x) MAC_EACH_15(rINC(o), m, __VA_ARGS__)

#define MAC_EACH_IMP(off, mac, count, ...) \
    MAC_EACH_ ##count(off, mac,__VA_ARGS__)
#define MAC_EACH_I(off, mac, count, ...) \
    MAC_EACH_IMP(off, mac, count, __VA_ARGS__)
#define MAC_EACH_OFF(off, mac, ...) \
    MAC_EACH_I(off, mac, LAST_IMP(__VA_ARGS__), __VA_ARGS__)
#define MAC_EACH(mac, ...) MAC_EACH_OFF(0, mac, __VA_ARGS__)

//                    1 2 3 4 5 6 7 8 910111213141516
#define OPTIONS_IMP16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g) \
    rOpt(7,h) rOpt(8,i) rOpt(9,j) rOpt(10,k)rOpt(11,l)rOpt(12,m)rOpt(13,n)\
    rOpt(14,o)rOpt(15,p)
#define OPTIONS_IMP15(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g) \
    rOpt(7,h) rOpt(8,i) rOpt(9,j) rOpt(10,k)rOpt(11,l)rOpt(12,m)rOpt(13,n)\
    rOpt(14,o)
#define OPTIONS_IMP14(a,b,c,d,e,f,g,h,i,j,k,l,m,n) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g) \
    rOpt(7,h) rOpt(8,i) rOpt(9,j) rOpt(10,k)rOpt(11,l)rOpt(12,m)rOpt(13,n)
#define OPTIONS_IMP13(a,b,c,d,e,f,g,h,i,j,k,l,m) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g) \
    rOpt(7,h) rOpt(8,i) rOpt(9,j) rOpt(10,k)rOpt(11,l)rOpt(12,m)
#define OPTIONS_IMP12(a,b,c,d,e,f,g,h,i,j,k,l) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g) \
    rOpt(7,h) rOpt(8,i) rOpt(9,j) rOpt(10,k)rOpt(11,l)
#define OPTIONS_IMP11(a,b,c,d,e,f,g,h,i,j,k) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g) \
    rOpt(7,h) rOpt(8,i) rOpt(9,j) rOpt(10,k)
#define OPTIONS_IMP10(a,b,c,d,e,f,g,h,i,j) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g) \
    rOpt(7,h) rOpt(8,i) rOpt(9,j)
#define OPTIONS_IMP9(a,b,c,d,e,f,g,h,i) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g) \
    rOpt(7,h) rOpt(8,i)
#define OPTIONS_IMP8(a,b,c,d,e,f,g,h) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g) \
    rOpt(7,h)
#define OPTIONS_IMP7(a,b,c,d,e,f,g) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g)
#define OPTIONS_IMP6(a,b,c,d,e,f) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f)
#define OPTIONS_IMP5(a,b,c,d,e) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e)
#define OPTIONS_IMP4(a,b,c,d) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d)
#define OPTIONS_IMP3(a,b,c) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c)
#define OPTIONS_IMP2(a,b) \
    rOpt(0,a) rOpt(1,b)
#define OPTIONS_IMP1(a) \
    rOpt(0,a)
#define OPTIONS_IMP0() YOU_MUST_PROVIDE_OPTIONS
#define OPTIONS_IMP(count, ...) OPTIONS_IMP ##count(__VA_ARGS__)
#define OPTIONS_I(count, ...) OPTIONS_IMP(count, __VA_ARGS__)
#define OPTIONS(...) OPTIONS_I(LAST_IMP(__VA_ARGS__), __VA_ARGS__)

//Additional Change Callback (after parameters have been changed)
//This can be used to queue up interpolation or parameter regen
#define rChangeCb

//Normal parameters
#define rParam(name, ...) \
  {STRINGIFY(name) "::c",  rProp(parameter) rMap(min, 0) rMap(max, 127) DOC(__VA_ARGS__), NULL, rParamCb(name)}
#define rParamF(name, ...) \
  {STRINGIFY(name) "::f",  rProp(parameter) DOC(__VA_ARGS__), NULL, rParamFCb(name)}
#define rParamI(name, ...) \
  {STRINGIFY(name) "::i",  rProp(parameter) DOC(__VA_ARGS__), NULL, rParamICb(name)}
#define rToggle(name, ...) \
  {STRINGIFY(name) "::T:F",rProp(parameter) DOC(__VA_ARGS__), NULL, rToggleCb(name)}
#define rOption(name, ...) \
  {STRINGIFY(name) "::i:c:S",rProp(parameter) rProp(enumerated) DOC(__VA_ARGS__), NULL, rOptionCb(name)}

//Array operators
#define rArrayF(name, length, ...) \
{STRINGIFY(name) "#" STRINGIFY(length) "::f", rProp(parameter) DOC(__VA_ARGS__), NULL, rArrayFCb(name)}
#define rArray(name, length, ...) \
{STRINGIFY(name) "#" STRINGIFY(length) "::c:i", rProp(parameter) DOC(__VA_ARGS__), NULL, rArrayCb(name)}
#define rArrayT(name, length, ...) \
{STRINGIFY(name) "#" STRINGIFY(length) "::T:F", rProp(parameter) DOC(__VA_ARGS__), NULL, rArrayTCb(name)}
#define rArrayI(name, length, ...) \
{STRINGIFY(name) "#" STRINGIFY(length) "::i", rProp(parameter) DOC(__VA_ARGS__), NULL, rArrayICb(name)}
#define rArrayOption(name, length, ...) \
{STRINGIFY(name) "#" STRINGIFY(length) "::i:c:S", rProp(parameter) DOC(__VA_ARGS__), NULL, rArrayOptionCb(name)}


//Method callback Actions
#define rAction(name, ...) \
{STRINGIFY(name) ":", DOC(__VA_ARGS__), NULL, rActionCb(name)}
#define rActioni(name, ...) \
{STRINGIFY(name) ":i", DOC(__VA_ARGS__), NULL, rActioniCb(name)}


//Alias operators
#define rParams(name, length, ...) \
rArray(name, length, __VA_ARGS__), \
{STRINGIFY(name) ":", rProp(alias) rDoc("get all data from aliased array"), NULL, rParamsCb(name, length)}


template<class T> constexpr T spice(T*t) {return *t;}

//Recursion [two ports in one for pointer manipulation]
#define rRecur(name, ...) \
    {STRINGIFY(name) "/", DOC(__VA_ARGS__), &decltype(rObject::name)::ports, rRecurCb(name)}, \
    {STRINGIFY(name) ":", rProp(internal) rDoc("get obj pointer"), NULL, rRecurPtrCb(name)}

#define rRecurp(name, ...) \
    {STRINGIFY(name) "/", DOC(__VA_ARGS__), \
        &decltype(spice(rObject::name))::ports, \
        rRecurpCb(name)}

#define rRecurs(name, length, ...) \
    {STRINGIFY(name) "#" STRINGIFY(length)"/", DOC(__VA_ARGS__), \
        &decltype(spice(&rObject::name[0]))::ports, \
        rRecursCb(name, length)}

//Technically this is a pointer pointer method...
#define rRecursp(name, length, ...) \
    {STRINGIFY(name)"#" STRINGIFY(length) "/", DOC(__VA_ARGS__), \
        &decltype(spice(rObject::name[0]))::ports, \
        rRecurspCb(name)}

//{STRINGIFY(name) ":", rProp(internal), NULL, rRecurPtrCb(name)}

//let this recurring parameter depend on another port
#define rEnabledBy(portname) rMap(enabled by, portname)
#define rEnabledByCondition(cond_name) rEnabledBy(cond_name)
#define rEnabledCondition(cond_name, condition) \
    {STRINGIFY(cond_name) ":", rProp(internal), NULL, rEnabledIfCb(condition)}
#define rEnabledIfCb(condition) rBOIL_BEGIN \
    assert(!rtosc_narguments(msg)); \
    data.reply(loc, (condition)?"T":"F"); \
    rBOIL_END \

#define rSelf(type, ...) \
{"self:", rProp(internal) rMap(class, type) __VA_ARGS__ rDoc("port metadata"), 0, \
    [](const char *, rtosc::RtData &d){ \
        d.reply(d.loc, "b", sizeof(d.obj), &d.obj);}}\

//Misc
#define rDummy(name, ...) {STRINGIFY(name), rProp(dummy), NULL, [](msg_t, rtosc::RtData &){}}
#define rString(name, len, ...) \
    {STRINGIFY(name) "::s", rMap(length, len) rProp(parameter) DOC(__VA_ARGS__), NULL, rStringCb(name,len)}

//General property operators
#define rMap(name, value) ":" STRINGIFY(name) "\0=" STRINGIFY(value) "\0"
#define rProp(name)       ":" STRINGIFY(name) "\0"

//Scaling property
#define rLinear(min_, max_) rMap(min, min_) rMap(max, max_) rMap(scale, linear)
#define rLog(min_, max_) rMap(min, min_) rMap(max, max_) rMap(scale, logarithmic)

//Special values
#define rSpecial(doc) ":special\0" STRINGIFY(doc) "\0"
#define rCentered ":centered\0"

//Default values
#define rDefault(default_value_) rMap(default, default_value_)
#define rDefaultId(default_value_) ":default\0=\"" STRINGIFY(default_value_) "\"S\0"
//#define rDefaultArr(default_value_, idx_) rMap(default[idx_], default_value_)
#define rPreset(no, default_value) \
    ":default " STRINGIFY(no) "\0=" STRINGIFY(default_value) "\0"
#define rPresetsAt(offs, ...) MAC_EACH_OFF(offs, rPreset, __VA_ARGS__)
#define rPresets(...) rPresetsAt(0, __VA_ARGS__)
#define rDefaultDepends(dep_path_) rMap(default depends, dep_path_)
#define rDefaultMissing "" // macro to denote yet missing default values
//#define rNoDefaults() ":no defaults\0" //!< this port (and all children) have no defaults
//! @brief Denote that this port and its children must always be skipped from
//!        port-walking if a runtime is being given.
#define rNoWalk rProp(no walk)

//Misc properties
#define rDoc(doc) ":documentation\0=" doc "\0"
#define rOpt(numeric,symbolic) rMap(map numeric, symbolic)
#define rOptions(...) OPTIONS(__VA_ARGS__)

//Zest Metadata
#define rShort(name) ":shortname\0=" name "\0"


//Callback Implementations
#define rBOIL_BEGIN [](const char *msg, rtosc::RtData &data) { \
        (void) msg; (void) data; \
        rObject *obj = (rObject*) data.obj;(void) obj; \
        const char *args = rtosc_argument_string(msg); (void) args;\
        const char *loc = data.loc; (void) loc;\
        auto prop = data.port->meta(); (void) prop;

#define rBOIL_END }

#define rLIMIT(var, convert) \
    if(prop["min"] && var < (decltype(var)) convert(prop["min"])) \
        var = convert(prop["min"]);\
    if(prop["max"] && var > (decltype(var)) convert(prop["max"])) \
        var = convert(prop["max"]);

#define rTYPE(n) decltype(obj->n)

//#define rAPPLY(n,t) if(obj->n != var) data.reply("undo_change", "s" #t #t, data.loc, obj->n, var); obj->n = var;
#define rCAPPLY(getcode, t, setcode) if(getcode != var) data.reply("undo_change", "s" #t #t, data.loc, getcode, var); setcode;
#define rAPPLY(n,t) rCAPPLY(obj->n, t, obj->n = var)

#define rParamCb(name) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "c", obj->name); \
        } else { \
            rTYPE(name) var = rtosc_argument(msg, 0).i; \
            rLIMIT(var, atoi) \
            rAPPLY(name, c) \
            data.broadcast(loc, "c", obj->name);\
            rChangeCb \
        } rBOIL_END

#define rParamFCb(name) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "f", obj->name); \
        } else { \
            rTYPE(name) var = rtosc_argument(msg, 0).f; \
            rLIMIT(var, atof) \
            rAPPLY(name, f) \
            data.broadcast(loc, "f", obj->name);\
            rChangeCb \
        } rBOIL_END

#define rParamICb(name) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "i", obj->name); \
        } else { \
            rTYPE(name) var = rtosc_argument(msg, 0).i; \
            rLIMIT(var, atoi) \
            rAPPLY(name, i) \
            data.broadcast(loc, "i", obj->name);\
            rChangeCb \
        } rBOIL_END

#define rCOptionCb_(getcode, setcode) { \
            if(!strcmp("", args)) {\
                data.reply(loc, "i", getcode); \
            } else if(!strcmp("s", args) || !strcmp("S", args)) { \
                auto var = \
                    enum_key(prop, rtosc_argument(msg, 0).s); \
                /* make sure we have no out-of-bound options */ \
                assert(!prop["min"] || \
                       var >= atoi(prop["min"])); \
                assert(!prop["max"] || \
                       var <= atoi(prop["max"])); \
                rCAPPLY(getcode, i, setcode) \
                data.broadcast(loc, "i", getcode); \
                rChangeCb \
            } else {\
                auto var = \
                    rtosc_argument(msg, 0).i; \
                rLIMIT(var, atoi) \
                rCAPPLY(getcode, i, setcode) \
                data.broadcast(loc, rtosc_argument_string(msg), getcode);\
                rChangeCb \
            } \
        }

#define rOptionCb_(name) rCOptionCb_(obj->name, obj->name = var)

#define rOptionCb(name) rBOIL_BEGIN \
    rOptionCb_(name) \
    rBOIL_END

#define rCOptionCb(getcode, setcode) rBOIL_BEGIN \
    rCOptionCb_(getcode, setcode) \
    rBOIL_END


#define rToggleCb(name) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, obj->name ? "T" : "F"); \
        } else { \
            if(obj->name != rtosc_argument(msg, 0).T) { \
                data.broadcast(loc, args);\
                obj->name = rtosc_argument(msg, 0).T; \
                rChangeCb \
            } \
        } rBOIL_END

#define SNIP \
    while(*msg && *msg!='/') ++msg; \
    msg = *msg ? msg+1 : msg;

#define rRecurCb(name) rBOIL_BEGIN \
    data.obj = &obj->name; \
    SNIP \
    decltype(obj->name)::ports.dispatch(msg, data); \
    rBOIL_END

#define rRecurPtrCb(name) rBOIL_BEGIN \
    void *ptr = &obj->name; \
    data.reply(loc, "b", sizeof(void*), &ptr); \
    rBOIL_END

#define rRecurpCb(name) rBOIL_BEGIN \
    data.obj = obj->name; \
    if(obj->name == NULL) return; \
    SNIP \
    decltype(spice(rObject::name))::ports.dispatch(msg, data); \
    rBOIL_END

#define rRecursCb(name, length) rBOILS_BEGIN \
    data.obj = &obj->name[idx]; \
    SNIP \
    decltype(spice(rObject::name))::ports.dispatch(msg, data); \
    rBOILS_END

#define rRecurspCb(name) rBOILS_BEGIN \
    data.obj = obj->name[idx]; \
    SNIP \
    decltype(spice(rObject::name[0]))::ports.dispatch(msg, data); \
    rBOILS_END

#define rActionCb(name) rBOIL_BEGIN obj->name(); rBOIL_END
#define rActioniCb(name) rBOIL_BEGIN \
    obj->name(rtosc_argument(msg,0).i); rBOIL_END

//Array ops

#define rBOILS_BEGIN rBOIL_BEGIN \
            const char *mm = msg; \
            while(*mm && !isdigit(*mm)) ++mm; \
            unsigned idx = atoi(mm);

#define rBOILS_END rBOIL_END


#define rArrayCb(name) rBOILS_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "c", obj->name[idx]); \
        } else { \
            char var = rtosc_argument(msg, 0).i; \
            rLIMIT(var, atoi) \
            rAPPLY(name[idx], c) \
            data.broadcast(loc, "c", obj->name[idx]);\
            rChangeCb \
        } rBOILS_END

#define rArrayFCb(name) rBOILS_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "f", obj->name[idx]); \
        } else { \
            float var = rtosc_argument(msg, 0).f; \
            rLIMIT(var, atof) \
            rAPPLY(name[idx], f) \
            data.broadcast(loc, "f", obj->name[idx]);\
        } rBOILS_END

#define rArrayTCb(name) rBOILS_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, obj->name[idx] ? "T" : "F"); \
        } else { \
            if(obj->name[idx] != rtosc_argument(msg, 0).T) { \
                data.broadcast(loc, args);\
                rChangeCb \
            } \
            obj->name[idx] = rtosc_argument(msg, 0).T; \
        } rBOILS_END

#define rArrayTCbMember(name, member) rBOILS_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, obj->name[idx].member ? "T" : "F"); \
        } else { \
            if(obj->name[idx].member != rtosc_argument(msg, 0).T) { \
                data.broadcast(loc, args);\
                rChangeCb \
            } \
            obj->name[idx].member = rtosc_argument(msg, 0).T; \
        } rBOILS_END


#define rArrayICb(name) rBOILS_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "i", obj->name[idx]); \
        } else { \
            char var = rtosc_argument(msg, 0).i; \
            rLIMIT(var, atoi) \
            rAPPLY(name[idx], i) \
            data.broadcast(loc, "i", obj->name[idx]);\
            rChangeCb \
        } rBOILS_END


#define rArrayOptionCb(name) rBOILS_BEGIN \
    rOptionCb_(name[idx]) \
    rBOILS_END

#define rParamsCb(name, length) rBOIL_BEGIN \
    data.reply(loc, "b", length, obj->name); rBOIL_END

#define rStringCb(name, length) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "s", obj->name); \
        } else { \
            strncpy(obj->name, rtosc_argument(msg, 0).s, length-1); \
            obj->name[length-1] = '\0'; \
            data.broadcast(loc, "s", obj->name);\
            rChangeCb \
        } rBOIL_END


#endif
