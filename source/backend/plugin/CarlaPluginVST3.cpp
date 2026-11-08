// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

/* TODO list
 * noexcept safe calls
 * paramId vs index
 */
#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaVst3Utils.hpp"

#include "CarlaPluginUI.hpp"

#ifdef CARLA_OS_MAC
# include "CarlaMacUtils.hpp"
# import <Foundation/Foundation.h>
#endif

#include "water/files/File.h"
#include "water/misc/Time.h"

#if defined(V3_VIEW_PLATFORM_TYPE_NATIVE) && defined(_POSIX_VERSION)
# ifdef CARLA_OS_LINUX
#  define CARLA_VST3_POSIX_EPOLL
#  include <sys/epoll.h>
# else
#  include <sys/event.h>
#  include <sys/types.h>
# endif
#endif

#include <atomic>
#include <unordered_map>

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

static inline
size_t strlen_utf16(const int16_t* const str)
{
    size_t i = 0;

    while (str[i] != 0)
        ++i;

    return i;
}

// --------------------------------------------------------------------------------------------------------------------

static inline
void strncpy_utf8(char* const dst, const int16_t* const src, const size_t length)
{
    CARLA_SAFE_ASSERT_RETURN(length > 0,);

    if (const size_t len = std::min(strlen_utf16(src), length-1U))
    {
        for (size_t i=0; i<len; ++i)
        {
            // skip non-ascii chars, unsupported
            if (src[i] >= 0x80)
                continue;

            dst[i] = static_cast<char>(src[i]);
        }
        dst[len] = 0;
    }
    else
    {
        dst[0] = 0;
    }
}

// --------------------------------------------------------------------------------------------------------------------

struct v3HostCallback {
    virtual ~v3HostCallback() {}
    // v3_component_handler
    virtual v3_result v3BeginEdit(v3_param_id) = 0;
    virtual v3_result v3PerformEdit(v3_param_id, double) = 0;
    virtual v3_result v3EndEdit(v3_param_id) = 0;
    virtual v3_result v3RestartComponent(int32_t) = 0;
   #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
    // v3_plugin_frame
    virtual v3_result v3ResizeView(struct v3_plugin_view**, struct v3_view_rect*) = 0;
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

struct v3_var {
    char type;
    uint32_t size;
    union {
        int64_t i;
        double f;
        int16_t* s;
        void* b;
    } value;
};

static void v3_var_cleanup(v3_var& var)
{
    switch (var.type)
    {
    case 's':
        std::free(var.value.s);
        break;
    case 'b':
        std::free(var.value.b);
        break;
    }

    carla_zeroStruct(var);
}

struct carla_v3_attribute_list : v3_attribute_list_cpp {
    // std::atomic<int> refcounter;
    std::unordered_map<std::string, v3_var> vars;

    carla_v3_attribute_list()
    //     : refcounter(1)
    {
        query_interface = v3_query_interface_static<v3_attribute_list_iid>;
        ref = v3_ref_static;
        unref = v3_unref_static;
        attrlist.set_int = set_int;
        attrlist.get_int = get_int;
        attrlist.set_float = set_float;
        attrlist.get_float = get_float;
        attrlist.set_string = set_string;
        attrlist.get_string = get_string;
        attrlist.set_binary = set_binary;
        attrlist.get_binary = get_binary;
    }

    ~carla_v3_attribute_list()
    {
        for (std::unordered_map<std::string, v3_var>::iterator it = vars.begin(); it != vars.end(); ++it)
            v3_var_cleanup(it->second);
    }

    v3_result add(const char* const id, const v3_var& var)
    {
        const std::string sid(id);

        for (std::unordered_map<std::string, v3_var>::iterator it = vars.begin(); it != vars.end(); ++it)
        {
            if (it->first == sid)
            {
                v3_var_cleanup(it->second);
                break;
            }
        }

        vars[sid] = var;
        return V3_OK;
    }

    bool get(const char* const id, v3_var& var)
    {
        const std::string sid(id);

        for (std::unordered_map<std::string, v3_var>::iterator it = vars.begin(); it != vars.end(); ++it)
        {
            if (it->first == sid)
            {
                var = it->second;
                return true;
            }
        }

        return false;
    }

private:
    static v3_result V3_API set_int(void* const self, const char* const id, const int64_t value)
    {
        CARLA_SAFE_ASSERT_RETURN(id != nullptr, V3_INVALID_ARG);
        carla_v3_attribute_list* const attrlist = *static_cast<carla_v3_attribute_list**>(self);

        v3_var var = {};
        var.type = 'i';
        var.value.i = value;
        return attrlist->add(id, var);
    }

    static v3_result V3_API get_int(void* const self, const char* const id, int64_t* const value)
    {
        CARLA_SAFE_ASSERT_RETURN(id != nullptr, V3_INVALID_ARG);
        carla_v3_attribute_list* const attrlist = *static_cast<carla_v3_attribute_list**>(self);

        v3_var var = {};
        if (attrlist->get(id, var))
        {
            *value = var.value.i;
            return V3_OK;
        }

        return V3_INVALID_ARG;
    }

    static v3_result V3_API set_float(void* const self, const char* const id, const double value)
    {
        CARLA_SAFE_ASSERT_RETURN(id != nullptr, V3_INVALID_ARG);
        carla_v3_attribute_list* const attrlist = *static_cast<carla_v3_attribute_list**>(self);

        v3_var var = {};
        var.type = 'f';
        var.value.f = value;
        return attrlist->add(id, var);
    }

    static v3_result V3_API get_float(void* const self, const char* const id, double* const value)
    {
        CARLA_SAFE_ASSERT_RETURN(id != nullptr, V3_INVALID_ARG);
        carla_v3_attribute_list* const attrlist = *static_cast<carla_v3_attribute_list**>(self);

        v3_var var = {};
        if (attrlist->get(id, var))
        {
            *value = var.value.f;
            return V3_OK;
        }

        return V3_INVALID_ARG;
    }

    static v3_result V3_API set_string(void* const self, const char* const id, const int16_t* const string)
    {
        CARLA_SAFE_ASSERT_RETURN(id != nullptr, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(string != nullptr, V3_INVALID_ARG);
        carla_v3_attribute_list* const attrlist = *static_cast<carla_v3_attribute_list**>(self);

        const size_t size = sizeof(int16_t) * (strlen_utf16(string) + 1);
        int16_t* const s = static_cast<int16_t*>(std::malloc(size));
        CARLA_SAFE_ASSERT_RETURN(s != nullptr, V3_NOMEM);
        std::memcpy(s, string, size);

        v3_var var = {};
        var.type = 's';
        var.size = size;
        var.value.s = s;
        return attrlist->add(id, var);
    }

    static v3_result V3_API get_string(void* const self, const char* const id,
                                       int16_t* const string, const uint32_t size)
    {
        CARLA_SAFE_ASSERT_RETURN(id != nullptr, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(string != nullptr, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(size != 0, V3_INVALID_ARG);
        carla_v3_attribute_list* const attrlist = *static_cast<carla_v3_attribute_list**>(self);

        v3_var var = {};
        if (attrlist->get(id, var))
        {
            CARLA_SAFE_ASSERT_UINT2_RETURN(var.size >= size, var.size, size, V3_INVALID_ARG);
            std::memcpy(string, var.value.s, size);
            return V3_OK;
        }

        return V3_INVALID_ARG;
    }

    static v3_result V3_API set_binary(void* const self, const char* const id,
                                       const void* const data, const uint32_t size)
    {
        CARLA_SAFE_ASSERT_RETURN(id != nullptr, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(size != 0, V3_INVALID_ARG);
        carla_v3_attribute_list* const attrlist = *static_cast<carla_v3_attribute_list**>(self);

        void* const b = std::malloc(size);
        CARLA_SAFE_ASSERT_RETURN(b != nullptr, V3_NOMEM);
        std::memcpy(b, data, size);

        v3_var var = {};
        var.type = 'b';
        var.size = size;
        var.value.b = b;
        return attrlist->add(id, var);
    }

    static v3_result V3_API get_binary(void* const self, const char* const id,
                                       const void** const data, uint32_t* const size)
    {
        CARLA_SAFE_ASSERT_RETURN(id != nullptr, V3_INVALID_ARG);
        carla_v3_attribute_list* const attrlist = *static_cast<carla_v3_attribute_list**>(self);

        v3_var var = {};
        if (attrlist->get(id, var))
        {
            *data = var.value.b;
            *size = var.size;
            return V3_OK;
        }

        return V3_INVALID_ARG;
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_attribute_list)
    CARLA_PREVENT_HEAP_ALLOCATION
};

struct carla_v3_message : v3_message_cpp {
    std::atomic<int> refcounter;
    carla_v3_attribute_list attrlist;
    carla_v3_attribute_list* attrlistptr;
    const char* msgId;

    carla_v3_message()
        : refcounter(1),
          attrlistptr(&attrlist),
          msgId(nullptr)
    {
        query_interface = v3_query_interface<carla_v3_message, v3_message_iid>;
        ref = v3_ref<carla_v3_message>;
        unref = v3_unref<carla_v3_message>;
        msg.get_message_id = get_message_id;
        msg.set_message_id = set_message_id;
        msg.get_attributes = get_attributes;
    }

    ~carla_v3_message()
    {
        delete[] msgId;
    }

private:
    static const char* V3_API get_message_id(void* const self)
    {
        carla_v3_message* const msg = *static_cast<carla_v3_message**>(self);
        return msg->msgId;
    }

    static void V3_API set_message_id(void* const self, const char* const id)
    {
        carla_v3_message* const msg = *static_cast<carla_v3_message**>(self);
        delete[] msg->msgId;
        msg->msgId = id != nullptr ? carla_strdup(id) : nullptr;
    }

    static v3_attribute_list** V3_API get_attributes(void* const self)
    {
        carla_v3_message* const msg = *static_cast<carla_v3_message**>(self);
        return (v3_attribute_list**)&msg->attrlistptr;
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_message)
};

// --------------------------------------------------------------------------------------------------------------------

struct carla_v3_bstream : v3_bstream_cpp {
    // to be filled by class producer
    void* buffer;
    int64_t size;
    bool canRead, canWrite;

    // used by class consumer
    int64_t readPos;

    carla_v3_bstream()
        : buffer(nullptr),
          size(0),
          canRead(false),
          canWrite(false),
          readPos(0)
    {
        query_interface = v3_query_interface_static<v3_bstream_iid>;
        ref = v3_ref_static;
        unref = v3_unref_static;
        stream.read = read;
        stream.write = write;
        stream.seek = seek;
        stream.tell = tell;
    }

private:
    static v3_result V3_API read(void* const self, void* const buffer, int32_t num_bytes, int32_t* const bytes_read)
    {
        carla_v3_bstream* const stream = *static_cast<carla_v3_bstream**>(self);
        CARLA_SAFE_ASSERT_RETURN(buffer != nullptr, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(num_bytes > 0, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(stream->canRead, V3_INVALID_ARG);

        if (stream->readPos + num_bytes > stream->size)
            num_bytes = stream->size - stream->readPos;

        std::memcpy(buffer, static_cast<uint8_t*>(stream->buffer) + stream->readPos, num_bytes);
        stream->readPos += num_bytes;

        // this is nasty, some plugins do not care about incomplete reads!
        if (bytes_read != nullptr)
            *bytes_read = num_bytes;

        return V3_OK;
    }

    static v3_result V3_API write(void* const self,
                                  void* const buffer, const int32_t num_bytes, int32_t* const bytes_read)
    {
        carla_v3_bstream* const stream = *static_cast<carla_v3_bstream**>(self);
        CARLA_SAFE_ASSERT_RETURN(buffer != nullptr, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(num_bytes > 0, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(stream->canWrite, V3_INVALID_ARG);

        void* const newbuffer = std::realloc(stream->buffer, stream->size + num_bytes);
        CARLA_SAFE_ASSERT_RETURN(newbuffer != nullptr, V3_NOMEM);

        std::memcpy(static_cast<uint8_t*>(newbuffer) + stream->size, buffer, num_bytes);
        stream->buffer = newbuffer;
        stream->size += num_bytes;

        // this is nasty, some plugins do not care about incomplete writes!
        if (bytes_read != nullptr)
            *bytes_read = num_bytes;

        return V3_OK;
    }

    static v3_result V3_API seek(void* const self, const int64_t pos, const int32_t seek_mode, int64_t* const result)
    {
        carla_v3_bstream* const stream = *static_cast<carla_v3_bstream**>(self);
        CARLA_SAFE_ASSERT_RETURN(stream->canRead, V3_INVALID_ARG);

        switch (seek_mode)
        {
        case V3_SEEK_SET:
            CARLA_SAFE_ASSERT_INT2_RETURN(pos <= stream->size, pos, stream->size, V3_INVALID_ARG);
            stream->readPos = pos;
            break;
        case V3_SEEK_CUR:
            CARLA_SAFE_ASSERT_INT2_RETURN(stream->readPos + pos <= stream->size, pos, stream->size, V3_INVALID_ARG);
            stream->readPos = stream->readPos + pos;
            break;
        case V3_SEEK_END:
            CARLA_SAFE_ASSERT_INT2_RETURN(pos <= stream->size, pos, stream->size, V3_INVALID_ARG);
            stream->readPos = stream->size - pos;
            break;
        default:
            return V3_INVALID_ARG;
        }

        if (result != nullptr)
            *result = stream->readPos;

        return V3_OK;
    }

    static v3_result V3_API tell(void* const self, int64_t* const pos)
    {
        carla_v3_bstream* const stream = *static_cast<carla_v3_bstream**>(self);
        CARLA_SAFE_ASSERT_RETURN(pos != nullptr, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(stream->canRead, V3_INVALID_ARG);

        *pos = stream->readPos;
        return V3_OK;
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_bstream)
    CARLA_PREVENT_HEAP_ALLOCATION
};

// --------------------------------------------------------------------------------------------------------------------

struct carla_v3_host_application : v3_host_application_cpp {
    carla_v3_host_application()
    {
        query_interface = v3_query_interface_static<v3_host_application_iid>;
        ref = v3_ref_static;
        unref = v3_unref_static;
        app.get_name = get_name;
        app.create_instance = create_instance;
    }

private:
    static v3_result V3_API get_name(void*, v3_str_128 name)
    {
        static const char hostname[] = "Carla\0";

        for (size_t i=0; i<sizeof(hostname); ++i)
            name[i] = hostname[i];

        return V3_OK;
    }

    static v3_result V3_API create_instance(void*, v3_tuid cid, v3_tuid iid, void** const obj)
    {
        if (v3_tuid_match(cid, v3_message_iid) && (v3_tuid_match(iid, v3_message_iid) ||
                                                   v3_tuid_match(iid, v3_funknown_iid)))
        {
            *obj = v3_create_class_ptr<carla_v3_message>();
            return V3_OK;
        }

        carla_stdout("TODO carla_create_instance %s", tuid2str(cid));
        return V3_NOT_IMPLEMENTED;
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_host_application)
    CARLA_PREVENT_HEAP_ALLOCATION
};

// --------------------------------------------------------------------------------------------------------------------

struct carla_v3_input_param_value_queue : v3_param_value_queue_cpp {
    const v3_param_id paramId;
    int8_t numUsed;

    struct Point {
        int32_t offset;
        float value;
    } points[32];

    carla_v3_input_param_value_queue(const v3_param_id pId)
        : paramId(pId),
          numUsed(0)
    {
        query_interface = v3_query_interface_static<v3_param_value_queue_iid>;
        ref = v3_ref_static;
        unref = v3_unref_static;
        queue.get_param_id = get_param_id;
        queue.get_point_count = get_point_count;
        queue.get_point = get_point;
        queue.add_point = add_point;
    }

private:
    static v3_param_id V3_API get_param_id(void* self)
    {
        carla_v3_input_param_value_queue* const me = *static_cast<carla_v3_input_param_value_queue**>(self);
        return me->paramId;
    }

    static int32_t V3_API get_point_count(void* self)
    {
        carla_v3_input_param_value_queue* const me = *static_cast<carla_v3_input_param_value_queue**>(self);
        return me->numUsed;
    }

    static v3_result V3_API get_point(void* const self,
                                      const int32_t idx, int32_t* const sample_offset, double* const value)
    {
        carla_v3_input_param_value_queue* const me = *static_cast<carla_v3_input_param_value_queue**>(self);
        CARLA_SAFE_ASSERT_INT2_RETURN(idx < me->numUsed, idx, me->numUsed, V3_INVALID_ARG);

        *sample_offset = me->points[idx].offset;
        *value = me->points[idx].value;
        return V3_OK;
    }

    static v3_result V3_API add_point(void*, int32_t, double, int32_t*)
    {
        // there is nothing here for input parameters, plugins are not meant to call this!
        return V3_NOT_IMPLEMENTED;
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_input_param_value_queue)
};

struct carla_v3_input_param_changes : v3_param_changes_cpp {
    const uint32_t paramCount;

    struct UpdatedParam {
        bool updated;
        float value;
    }* const updatedParams;

    carla_v3_input_param_value_queue** const queue;

    // data given to plugins
    v3_param_value_queue*** pluginExposedQueue;
    int32_t pluginExposedCount;

    carla_v3_input_param_changes(const PluginParameterData& paramData)
        : paramCount(paramData.count),
          updatedParams(new UpdatedParam[paramData.count]),
          queue(new carla_v3_input_param_value_queue*[paramData.count]),
          pluginExposedQueue(new v3_param_value_queue**[paramData.count]),
          pluginExposedCount(0)
    {
        query_interface = v3_query_interface_static<v3_param_changes_iid>;
        ref = v3_ref_static;
        unref = v3_unref_static;
        changes.get_param_count = get_param_count;
        changes.get_param_data = get_param_data;
        changes.add_param_data = add_param_data;

        CARLA_ASSERT(paramCount != 0);

        carla_zeroStructs(updatedParams, paramCount);

        for (uint32_t i=0; i<paramCount; ++i)
            queue[i] = new carla_v3_input_param_value_queue(static_cast<v3_param_id>(paramData.data[i].rindex));
    }

    ~carla_v3_input_param_changes()
    {
        for (uint32_t i=0; i<paramCount; ++i)
            delete queue[i];

        delete[] updatedParams;
        delete[] pluginExposedQueue;
        delete[] queue;
    }

    // called during start of process, gathering all parameter update requests so far
    void init()
    {
        for (uint32_t i=0; i<paramCount; ++i)
        {
            if (updatedParams[i].updated)
            {
                queue[i]->numUsed = 1;
                queue[i]->points[0].offset = 0;
                queue[i]->points[0].value = updatedParams[i].value;
            }
            else
            {
                queue[i]->numUsed = 0;
            }
        }
    }

    // called just before plugin processing, creating local queue
    void prepare()
    {
        int32_t count = 0;

        for (uint32_t i=0; i<paramCount; ++i)
        {
            if (queue[i]->numUsed)
                pluginExposedQueue[count++] = (v3_param_value_queue**)&queue[i];
        }

        pluginExposedCount = count;
    }

    // called when a parameter is set from non-rt thread
    void setParamValue(const uint32_t index, const float value) noexcept
    {
        updatedParams[index].value = value;
        updatedParams[index].updated = true;
    }

    // called as response to MIDI CC
    void setParamValueRT(const uint32_t index, const int32_t offset, const float value) noexcept
    {
        static constexpr const int8_t kQueuePointSize = sizeof(queue[0]->points)/sizeof(queue[0]->points[0]);

        if (queue[index]->numUsed < kQueuePointSize)
        {
            // still has space, add in queue
            carla_v3_input_param_value_queue::Point& point(queue[index]->points[queue[index]->numUsed++]);
            point.offset = offset;
            point.value = value;
        }
        else
        {
            // points are full, replace last one
            carla_v3_input_param_value_queue::Point& point(queue[index]->points[queue[index]->numUsed - 1]);
            point.offset = offset;
            point.value = value;
        }
    }

private:
    static int32_t V3_API get_param_count(void* const self)
    {
        carla_v3_input_param_changes* const me = *static_cast<carla_v3_input_param_changes**>(self);
        return me->pluginExposedCount;
    }

    static v3_param_value_queue** V3_API get_param_data(void* const self, const int32_t index)
    {
        carla_v3_input_param_changes* const me = *static_cast<carla_v3_input_param_changes**>(self);
        return me->pluginExposedQueue[index];
    }

    static v3_param_value_queue** V3_API add_param_data(void*, const v3_param_id*, int32_t*)
    {
        // there is nothing here for input parameters, plugins are not meant to call this!
        return nullptr;
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_input_param_changes)
};

// --------------------------------------------------------------------------------------------------------------------

struct carla_v3_output_param_value_queue : v3_param_value_queue_cpp {
    const v3_param_id paramId;
    bool used;
    int32_t offset;
    double value;

    carla_v3_output_param_value_queue(const v3_param_id pId)
        : paramId(pId),
          used(false),
          offset(0),
          value(0.0)
    {
        query_interface = v3_query_interface_static<v3_param_value_queue_iid>;
        ref = v3_ref_static;
        unref = v3_unref_static;
        queue.get_param_id = get_param_id;
        queue.get_point_count = get_point_count;
        queue.get_point = get_point;
        queue.add_point = add_point;
    }

    void init()
    {
        used = false;
        offset = 0;
        value = 0.0;
    }

private:
    static v3_param_id V3_API get_param_id(void* self)
    {
        carla_v3_output_param_value_queue* const me = *static_cast<carla_v3_output_param_value_queue**>(self);
        return me->paramId;
    }

    static int32_t V3_API get_point_count(void* self)
    {
        carla_v3_output_param_value_queue* const me = *static_cast<carla_v3_output_param_value_queue**>(self);
        return me->used ? 1 : 0;
    }

    static v3_result V3_API get_point(void* const self,
                                      const int32_t index, int32_t* const sample_offset, double* const value)
    {
        carla_v3_output_param_value_queue* const me = *static_cast<carla_v3_output_param_value_queue**>(self);
        CARLA_SAFE_ASSERT_RETURN(me->used, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_INT_RETURN(index == 0, index, V3_INVALID_ARG);

        *sample_offset = me->offset;
        *value = me->value;
        return V3_OK;
    }

    static v3_result V3_API add_point(void* const self,
                                      const int32_t sample_offset, const double value, int32_t* const index)
    {
        carla_v3_output_param_value_queue* const me = *static_cast<carla_v3_output_param_value_queue**>(self);
        CARLA_SAFE_ASSERT_INT_RETURN(sample_offset >= 0, sample_offset, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(value >= 0 && value <= 1, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(index != nullptr, V3_INVALID_ARG);

        me->offset = sample_offset;
        me->value = value;
        *index = 0;
        return V3_OK;
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_output_param_value_queue)
};

struct carla_v3_output_param_changes : v3_param_changes_cpp {
    const uint32_t numParameters;
    int32_t numParametersUsed;
    bool* const parametersUsed;
    carla_v3_output_param_value_queue** const queue;
    std::unordered_map<v3_param_id, int32_t> paramIds;

    carla_v3_output_param_changes(const PluginParameterData& paramData)
        : numParameters(paramData.count),
          numParametersUsed(0),
          parametersUsed(new bool[paramData.count]),
          queue(new carla_v3_output_param_value_queue*[paramData.count])
    {
        query_interface = v3_query_interface_static<v3_param_changes_iid>;
        ref = v3_ref_static;
        unref = v3_unref_static;
        changes.get_param_count = get_param_count;
        changes.get_param_data = get_param_data;
        changes.add_param_data = add_param_data;

        carla_zeroStructs(parametersUsed, numParameters);

        for (uint32_t i=0; i<numParameters; ++i)
        {
            const v3_param_id paramId = paramData.data[i].rindex;
            queue[i] = new carla_v3_output_param_value_queue(paramId);
            paramIds[paramId] = i;
        }
    }

    ~carla_v3_output_param_changes()
    {
        for (uint32_t i=0; i<numParameters; ++i)
            delete queue[i];
        delete[] parametersUsed;
        delete[] queue;
    }

    void prepare()
    {
        numParametersUsed = 0;
        carla_zeroStructs(parametersUsed, numParameters);
    }

private:
    static int32_t V3_API get_param_count(void*)
    {
        // there is nothing here for output parameters, plugins are not meant to call this!
        return 0;
    }

    static v3_param_value_queue** V3_API get_param_data(void*, int32_t)
    {
        // there is nothing here for output parameters, plugins are not meant to call this!
        return nullptr;
    }

    static v3_param_value_queue** V3_API add_param_data(void* const self,
                                                        const v3_param_id* const paramIdPtr,
                                                        int32_t* const index)
    {
        carla_v3_output_param_changes* const me = *static_cast<carla_v3_output_param_changes**>(self);
        CARLA_SAFE_ASSERT_RETURN(paramIdPtr != nullptr, nullptr);

        const v3_param_id paramId = *paramIdPtr;

        if (me->paramIds.find(paramId) == me->paramIds.end())
            return nullptr;

        const int32_t paramIndex = me->paramIds[paramId];
        CARLA_SAFE_ASSERT_RETURN(!me->parametersUsed[paramIndex], nullptr);

        *index = me->numParametersUsed++;
        me->parametersUsed[paramIndex] = true;
        me->queue[paramIndex]->init();

        return (v3_param_value_queue**)&me->queue[paramIndex];
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_output_param_changes)
};

// --------------------------------------------------------------------------------------------------------------------

struct carla_v3_input_event_list : v3_event_list_cpp {
    v3_event* const events;
    uint16_t numEvents;

    carla_v3_input_event_list()
        : events(new v3_event[kPluginMaxMidiEvents]),
          numEvents(0)
    {
        query_interface = v3_query_interface_static<v3_event_list_iid>;
        ref = v3_ref_static;
        unref = v3_unref_static;
        list.get_event_count = get_event_count;
        list.get_event = get_event;
        list.add_event = add_event;
    }

    ~carla_v3_input_event_list()
    {
        delete[] events;
    }

private:
    static uint32_t V3_API get_event_count(void* const self)
    {
        const carla_v3_input_event_list* const me = *static_cast<const carla_v3_input_event_list**>(self);
        return me->numEvents;
    }

    static v3_result V3_API get_event(void* const self, const int32_t index, v3_event* const event)
    {
        const carla_v3_input_event_list* const me = *static_cast<const carla_v3_input_event_list**>(self);
        CARLA_SAFE_ASSERT_RETURN(index < static_cast<int32_t>(me->numEvents), V3_INVALID_ARG);
        std::memcpy(event, &me->events[index], sizeof(v3_event));
        return V3_OK;
    }

    static v3_result V3_API add_event(void*, v3_event*)
    {
        // there is nothing here for input events, plugins are not meant to call this!
        return V3_NOT_IMPLEMENTED;
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_input_event_list)
};

// --------------------------------------------------------------------------------------------------------------------

struct carla_v3_output_event_list : v3_event_list_cpp {
    v3_event* const events;
    uint16_t numEvents;

    carla_v3_output_event_list()
        : events(new v3_event[kPluginMaxMidiEvents]),
          numEvents(0)
    {
        query_interface = v3_query_interface_static<v3_event_list_iid>;
        ref = v3_ref_static;
        unref = v3_unref_static;
        list.get_event_count = get_event_count;
        list.get_event = get_event;
        list.add_event = add_event;
    }

    ~carla_v3_output_event_list()
    {
        delete[] events;
    }

private:
    static uint32_t V3_API get_event_count(void*)
    {
        carla_debug("TODO %s", __PRETTY_FUNCTION__);
        // there is nothing here for output events, plugins are not meant to call this!
        return 0;
    }

    static v3_result V3_API get_event(void*, int32_t, v3_event*)
    {
        carla_debug("TODO %s", __PRETTY_FUNCTION__);
        // there is nothing here for output events, plugins are not meant to call this!
        return V3_NOT_IMPLEMENTED;
    }

    static v3_result V3_API add_event(void* const self, v3_event* const event)
    {
        carla_v3_output_event_list* const me = *static_cast<carla_v3_output_event_list**>(self);
        if (me->numEvents >= kPluginMaxMidiEvents)
            return V3_NOMEM;
        std::memcpy(&me->events[me->numEvents++], event, sizeof(v3_event));
        return V3_OK;
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_output_event_list)
};

// --------------------------------------------------------------------------------------------------------------------

#ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
struct HostTimer {
    v3_timer_handler** handler;
    uint64_t periodInMs;
    uint64_t lastCallTimeInMs;
};

static constexpr const HostTimer kTimerFallback   = { nullptr, 0, 0 };
static /*           */ HostTimer kTimerFallbackNC = { nullptr, 0, 0 };

#ifdef _POSIX_VERSION
struct HostPosixFileDescriptor {
    v3_event_handler** handler;
    int hostfd;
    int pluginfd;
};

static constexpr const HostPosixFileDescriptor kPosixFileDescriptorFallback   = { nullptr, -1, -1 };
static /*           */ HostPosixFileDescriptor kPosixFileDescriptorFallbackNC = { nullptr, -1, -1 };
#endif

struct carla_v3_run_loop : v3_run_loop_cpp {
    LinkedList<HostTimer> timers;
   #ifdef _POSIX_VERSION
    LinkedList<HostPosixFileDescriptor> posixfds;
   #endif

    carla_v3_run_loop()
    {
        query_interface = v3_query_interface_static<v3_run_loop_iid>;
        ref = v3_ref_static;
        unref = v3_unref_static;
        loop.register_event_handler = register_event_handler;
        loop.unregister_event_handler = unregister_event_handler;
        loop.register_timer = register_timer;
        loop.unregister_timer = unregister_timer;
    }

private:
    static v3_result V3_API register_event_handler(void* const self, v3_event_handler** const handler, const int fd)
    {
      #ifdef _POSIX_VERSION
        carla_v3_run_loop* const loop = *static_cast<carla_v3_run_loop**>(self);

       #ifdef CARLA_VST3_POSIX_EPOLL
        const int hostfd = ::epoll_create1(0);
       #else
        const int hostfd = ::kqueue();
       #endif
        CARLA_SAFE_ASSERT_RETURN(hostfd >= 0, V3_INTERNAL_ERR);

       #ifdef CARLA_VST3_POSIX_EPOLL
        struct ::epoll_event ev = {};
        ev.events = EPOLLIN|EPOLLOUT;
        ev.data.fd = fd;

        if (::epoll_ctl(hostfd, EPOLL_CTL_ADD, fd, &ev) < 0)
        {
            ::close(hostfd);
            return V3_INTERNAL_ERR;
        }
       #endif

        const HostPosixFileDescriptor posixfd = { handler, hostfd, fd };
        return loop->posixfds.append(posixfd) ? V3_OK : V3_NOMEM;
      #else
        return V3_NOT_IMPLEMENTED;
        // unused
        (void)self; (void)handler; (void)fd;
      #endif
    }

    static v3_result V3_API unregister_event_handler(void* const self, v3_event_handler** const handler)
    {
       #ifdef _POSIX_VERSION
        carla_v3_run_loop* const loop = *static_cast<carla_v3_run_loop**>(self);

        for (LinkedList<HostPosixFileDescriptor>::Itenerator it = loop->posixfds.begin2(); it.valid(); it.next())
        {
            const HostPosixFileDescriptor& posixfd(it.getValue(kPosixFileDescriptorFallback));

            if (posixfd.handler == handler)
            {
               #ifdef CARLA_VST3_POSIX_EPOLL
                ::epoll_ctl(posixfd.hostfd, EPOLL_CTL_DEL, posixfd.pluginfd, nullptr);
               #endif
                ::close(posixfd.hostfd);
                loop->posixfds.remove(it);
                return V3_OK;
            }
        }

        return V3_INVALID_ARG;
       #else
        return V3_NOT_IMPLEMENTED;
        // unused
        (void)self; (void)handler;
       #endif
    }

    static v3_result V3_API register_timer(void* const self, v3_timer_handler** const handler, const uint64_t ms)
    {
        carla_v3_run_loop* const loop = *static_cast<carla_v3_run_loop**>(self);

        const HostTimer timer = { handler, ms, 0 };
        return loop->timers.append(timer) ? V3_OK : V3_NOMEM;
    }

    static v3_result V3_API unregister_timer(void* const self, v3_timer_handler** const handler)
    {
        carla_v3_run_loop* const loop = *static_cast<carla_v3_run_loop**>(self);

        for (LinkedList<HostTimer>::Itenerator it = loop->timers.begin2(); it.valid(); it.next())
        {
            const HostTimer& timer(it.getValue(kTimerFallback));

            if (timer.handler == handler)
            {
                loop->timers.remove(it);
                return V3_OK;
            }
        }

        return V3_INVALID_ARG;
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_run_loop)
    CARLA_PREVENT_HEAP_ALLOCATION
};
#endif // V3_VIEW_PLATFORM_TYPE_NATIVE

// --------------------------------------------------------------------------------------------------------------------

struct carla_v3_component_handler : v3_component_handler_cpp {
    v3HostCallback* const callback;

    carla_v3_component_handler(v3HostCallback* const cb)
        : callback(cb)
    {
        query_interface = carla_query_interface;
        ref = v3_ref_static;
        unref = v3_unref_static;
        comp.begin_edit = begin_edit;
        comp.perform_edit = perform_edit;
        comp.end_edit = end_edit;
        comp.restart_component = restart_component;
    }

private:
    static v3_result V3_API carla_query_interface(void* const self, const v3_tuid iid, void** const iface)
    {
        if (v3_query_interface_static<v3_component_handler_iid>(self, iid, iface) == V3_OK)
            return V3_OK;

        // TODO
        if (v3_tuid_match(iid, v3_component_handler2_iid))
        {
            *iface = nullptr;
            return V3_NO_INTERFACE;
        }

        *iface = nullptr;
        carla_stdout("TODO carla_v3_component_handler::query_interface %s", tuid2str(iid));
        return V3_NO_INTERFACE;
    }

    static v3_result V3_API begin_edit(void* const self, const v3_param_id paramId)
    {
        carla_v3_component_handler* const comp = *static_cast<carla_v3_component_handler**>(self);
        return comp->callback->v3BeginEdit(paramId);
    }

    static v3_result V3_API perform_edit(void* const self, const v3_param_id paramId, const double value)
    {
        carla_v3_component_handler* const comp = *static_cast<carla_v3_component_handler**>(self);
        return comp->callback->v3PerformEdit(paramId, value);
    }

    static v3_result V3_API end_edit(void* const self, const v3_param_id paramId)
    {
        carla_v3_component_handler* const comp = *static_cast<carla_v3_component_handler**>(self);
        return comp->callback->v3EndEdit(paramId);
    }

    static v3_result V3_API restart_component(void* const self, const int32_t flags)
    {
        carla_v3_component_handler* const comp = *static_cast<carla_v3_component_handler**>(self);
        return comp->callback->v3RestartComponent(flags);
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_component_handler)
    CARLA_PREVENT_HEAP_ALLOCATION
};

// --------------------------------------------------------------------------------------------------------------------

#ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
struct carla_v3_plugin_frame : v3_plugin_frame_cpp {
    v3HostCallback* const callback;
    carla_v3_run_loop loop;
    carla_v3_run_loop* loopPtr;

    carla_v3_plugin_frame(v3HostCallback* const cb)
        : callback(cb),
          loopPtr(&loop)
    {
        query_interface = carla_query_interface;
        ref = v3_ref_static;
        unref = v3_unref_static;
        frame.resize_view = resize_view;
    }

private:
    static v3_result V3_API carla_query_interface(void* const self, const v3_tuid iid, void** const iface)
    {
        if (v3_query_interface_static<v3_plugin_frame_iid>(self, iid, iface) == V3_OK)
            return V3_OK;

        carla_v3_plugin_frame* const frame = *static_cast<carla_v3_plugin_frame**>(self);

        if (v3_tuid_match(iid, v3_run_loop_iid))
        {
            *iface = &frame->loopPtr;
            return V3_OK;
        }

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    static v3_result V3_API resize_view(void* const self,
                                        struct v3_plugin_view** const view, struct v3_view_rect* const rect)
    {
        const carla_v3_plugin_frame* const me = *static_cast<const carla_v3_plugin_frame**>(self);
        return me->callback->v3ResizeView(view, rect);
    }

    CARLA_DECLARE_NON_COPYABLE(carla_v3_plugin_frame)
    CARLA_PREVENT_HEAP_ALLOCATION
};
#endif

// --------------------------------------------------------------------------------------------------------------------

class CarlaPluginVST3 : public CarlaPlugin,
                       #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
                        private CarlaPluginUI::Callback,
                       #endif
                        private v3HostCallback
{
public:
    CarlaPluginVST3(CarlaEngine* const engine, const uint id)
        : CarlaPlugin(engine, id),
          kEngineHasIdleOnMainThread(engine->hasIdleOnMainThread()),
          fFirstActive(true),
          fAudioAndCvOutBuffers(nullptr),
          fLastKnownLatency(0),
          fRestartFlags(0),
          fLastChunk(nullptr),
          fLastTimeInfo(),
          fV3TimeContext(),
          fV3Application(),
          fV3ApplicationPtr(&fV3Application),
          fComponentHandler(this),
          fComponentHandlerPtr(&fComponentHandler),
         #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
          fPluginFrame(this),
          fPluginFramePtr(&fPluginFrame),
         #endif
          fV3ClassInfo(),
          fV3(),
          fEvents()
    {
        carla_debug("CarlaPluginVST3::CarlaPluginVST3(%p, %i)", engine, id);

        carla_zeroStruct(fV3TimeContext);
    }

    ~CarlaPluginVST3() override
    {
        carla_debug("CarlaPluginVST3::~CarlaPluginVST3()");

        runIdleCallbacksAsNeeded(false);

      #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
        fPluginFrame.loop.timers.clear();
       #ifdef _POSIX_VERSION
        fPluginFrame.loop.posixfds.clear();
       #endif

        // close UI
        if (pData->hints & PLUGIN_HAS_CUSTOM_UI)
        {
            if (! fUI.isEmbed)
                showCustomUI(false);

            if (fUI.isAttached)
            {
                fUI.isAttached = false;
                v3_cpp_obj(fV3.view)->set_frame(fV3.view, nullptr);
                v3_cpp_obj(fV3.view)->removed(fV3.view);
            }
        }

        if (fV3.view != nullptr)
        {
            v3_cpp_obj_unref(fV3.view);
            fV3.view = nullptr;
        }
      #endif

        pData->singleMutex.lock();
        pData->masterMutex.lock();

        if (pData->client != nullptr && pData->client->isActive())
            pData->client->deactivate(true);

        if (pData->active)
        {
            deactivate();
            pData->active = false;
        }

        if (fLastChunk != nullptr)
        {
            std::free(fLastChunk);
            fLastChunk = nullptr;
        }

        clearBuffers();

        fV3.exit();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Information (base)

    PluginType getType() const noexcept override
    {
        return PLUGIN_VST3;
    }

    PluginCategory getCategory() const noexcept override
    {
        return getPluginCategoryFromV3SubCategories(fV3ClassInfo.v2.sub_categories);
    }

    uint32_t getLatencyInFrames() const noexcept override
    {
        return fLastKnownLatency;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Information (count)

    /* TODO
    uint32_t getMidiInCount() const noexcept override
    {
    }

    uint32_t getMidiOutCount() const noexcept override
    {
    }

    uint32_t getParameterScalePointCount(const uint32_t paramIndex) const noexcept override
    {
    }
    */

    // ----------------------------------------------------------------------------------------------------------------
    // Information (current data)

    uint getAudioPortHints(const bool isOutput, const uint32_t portIndex) const noexcept override
    {
        uint hints = 0x0;

        if (isOutput)
        {
            const uint32_t numOutputs = static_cast<uint32_t>(fBuses.numOutputs);

            for (uint32_t b=0, i=0; b < numOutputs; ++b, i += fBuses.outputs[b].num_channels)
            {
                if (i != portIndex)
                    continue;

                if (fBuses.outputInfo[i].bus_type == V3_AUX)
                    hints |= AUDIO_PORT_IS_SIDECHAIN;
            }
        }
        else
        {
            const uint32_t numInputs = static_cast<uint32_t>(fBuses.numInputs);

            for (uint32_t b=0, i=0; b < numInputs; ++b, i += fBuses.inputs[b].num_channels)
            {
                if (i != portIndex)
                    continue;

                if (fBuses.inputInfo[i].bus_type == V3_AUX)
                    hints |= AUDIO_PORT_IS_SIDECHAIN;
            }
        }

        return hints;
    }

    std::size_t getChunkData(void** const dataPtr) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS, 0);
        CARLA_SAFE_ASSERT_RETURN(fV3.component != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(dataPtr != nullptr, 0);

        std::free(fLastChunk);

        carla_v3_bstream stream;
        carla_v3_bstream* const streamPtr = &stream;
        v3_bstream** const v3stream = (v3_bstream**)&streamPtr;

        stream.canWrite = true;

        if (v3_cpp_obj(fV3.component)->get_state(fV3.component, v3stream) == V3_OK)
        {
            *dataPtr = fLastChunk = stream.buffer;
            runIdleCallbacksAsNeeded(false);
            return stream.size;
        }

        *dataPtr = fLastChunk = nullptr;
        runIdleCallbacksAsNeeded(false);
        return 0;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Information (per-plugin data)

    uint getOptionsAvailable() const noexcept override
    {
        uint options = 0x0;

        // can't disable fixed buffers if using latency
        if (fLastKnownLatency == 0)
            options |= PLUGIN_OPTION_FIXED_BUFFERS;

        /* TODO
        if (numPrograms > 1)
            options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        */

        options |= PLUGIN_OPTION_USE_CHUNKS;

        if (hasMidiInput())
        {
            options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            options |= PLUGIN_OPTION_SEND_PITCHBEND;
            options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
        }

        return options;
    }

    float getParameterValue(const uint32_t paramIndex) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fV3.controller != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(paramIndex < pData->param.count, 0.0f);

        // FIXME use pending RT value?

        const v3_param_id paramId = pData->param.data[paramIndex].rindex;
        const double normalized = v3_cpp_obj(fV3.controller)->get_parameter_normalised(fV3.controller, paramId);

        return static_cast<float>(
            v3_cpp_obj(fV3.controller)->normalised_parameter_to_plain(fV3.controller, paramId, normalized));
    }

    /* TODO
    float getParameterScalePointValue(uint32_t paramIndex, uint32_t scalePointId) const noexcept override
    {
    }
    */

    bool getLabel(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fV3ClassInfo.v1.name, STR_MAX);
        return true;
    }

    bool getMaker(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fV3ClassInfo.v2.vendor, STR_MAX);
        return true;
    }

    bool getCopyright(char* const strBuf) const noexcept override
    {
        return getMaker(strBuf);
    }

    bool getRealName(char* const strBuf) const noexcept override
    {
        std::strncpy(strBuf, fV3ClassInfo.v1.name, STR_MAX);
        return true;
    }

    bool getParameterName(const uint32_t paramIndex, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fV3.controller != nullptr, 0.0f);
        CARLA_SAFE_ASSERT_RETURN(paramIndex < pData->param.count, false);

        v3_param_info paramInfo = {};
        CARLA_SAFE_ASSERT_RETURN(v3_cpp_obj(fV3.controller)->get_parameter_info(fV3.controller,
                                                                                static_cast<int32_t>(paramIndex),
                                                                                &paramInfo) == V3_OK, false);

        strncpy_utf8(strBuf, paramInfo.title, STR_MAX);
        return true;
    }

    bool getParameterSymbol(const uint32_t paramIndex, char* strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(paramIndex < pData->param.count, false);

        std::snprintf(strBuf, STR_MAX, "%d", pData->param.data[paramIndex].rindex);
        return true;
    }

    bool getParameterText(const uint32_t paramIndex, char* const strBuf) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fV3.controller != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(paramIndex < pData->param.count, false);

        const v3_param_id paramId = pData->param.data[paramIndex].rindex;
        const double normalized = v3_cpp_obj(fV3.controller)->get_parameter_normalised(fV3.controller, paramId);

        v3_str_128 paramText;
        CARLA_SAFE_ASSERT_RETURN(v3_cpp_obj(fV3.controller)->get_parameter_string_for_value(fV3.controller,
                                                                                            paramId,
                                                                                            normalized,
                                                                                            paramText) == V3_OK, false);

        if (paramText[0] != '\0')
            strncpy_utf8(strBuf, paramText, STR_MAX);
        else
            std::snprintf(strBuf, STR_MAX, "%.12g",
                          v3_cpp_obj(fV3.controller)->normalised_parameter_to_plain(fV3.controller, paramId, normalized));

        return true;
    }

    bool getParameterUnit(const uint32_t paramIndex, char* const strBuf) const noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fV3.controller != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(paramIndex < pData->param.count, false);

        v3_param_info paramInfo = {};
        CARLA_SAFE_ASSERT_RETURN(v3_cpp_obj(fV3.controller)->get_parameter_info(fV3.controller,
                                                                                static_cast<int32_t>(paramIndex),
                                                                                &paramInfo) == V3_OK, false);

        strncpy_utf8(strBuf, paramInfo.units, STR_MAX);
        return true;
    }

    /* TODO
    bool getParameterGroupName(const uint32_t paramIndex, char* const strBuf) const noexcept override
    {
    }

    bool getParameterScalePointLabel(const uint32_t paramIndex,
                                     const uint32_t scalePointId, char* const strBuf) const noexcept override
    {
    }
    */

    // ----------------------------------------------------------------------------------------------------------------
    // Set data (state)

    /* TODO
    void prepareForSave(const bool temporary) override
    {
        // component to edit controller state or vice-versa here
    }
    */

    // ----------------------------------------------------------------------------------------------------------------
    // Set data (internal stuff)

    /* TODO
    void setName(const char* newName) override
    {
    }
    */

    // ----------------------------------------------------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t paramIndex, const float value,
                           const bool sendGui, const bool sendOsc, const bool sendCallback) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fV3.controller != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(paramIndex < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(fEvents.paramInputs != nullptr,);

        const v3_param_id paramId = pData->param.data[paramIndex].rindex;
        const float fixedValue = pData->param.getFixedValue(paramIndex, value);
        const double normalized = v3_cpp_obj(fV3.controller)->plain_parameter_to_normalised(fV3.controller,
                                                                                            paramId,
                                                                                            fixedValue);

        // report value to component (next process call)
        fEvents.paramInputs->setParamValue(paramIndex, static_cast<float>(normalized));

        // report value to edit controller
        v3_cpp_obj(fV3.controller)->set_parameter_normalised(fV3.controller, paramId, normalized);

        CarlaPlugin::setParameterValue(paramIndex, fixedValue, sendGui, sendOsc, sendCallback);
    }

    void setParameterValueRT(const uint32_t paramIndex, const float value, const uint32_t frameOffset,
                             const bool sendCallbackLater) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fV3.controller != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(paramIndex < pData->param.count,);
        CARLA_SAFE_ASSERT_RETURN(fEvents.paramInputs != nullptr,);

        const v3_param_id paramId = pData->param.data[paramIndex].rindex;
        const float fixedValue = pData->param.getFixedValue(paramIndex, value);
        const double normalized = v3_cpp_obj(fV3.controller)->plain_parameter_to_normalised(fV3.controller,
                                                                                            paramId,
                                                                                            fixedValue);

        // report value to component (next process call)
        fEvents.paramInputs->setParamValueRT(paramIndex, frameOffset, static_cast<float>(normalized));

        CarlaPlugin::setParameterValueRT(paramIndex, fixedValue, frameOffset, sendCallbackLater);
    }

    void setChunkData(const void* data, std::size_t dataSize) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->options & PLUGIN_OPTION_USE_CHUNKS,);
        CARLA_SAFE_ASSERT_RETURN(fV3.component != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fV3.controller != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(dataSize > 0,);

        carla_v3_bstream stream;
        carla_v3_bstream* const streamPtr = &stream;
        v3_bstream** const v3stream = (v3_bstream**)&streamPtr;

        stream.buffer = const_cast<void*>(data);
        stream.size = dataSize;
        stream.canRead = true;

        if (v3_cpp_obj(fV3.component)->set_state(fV3.component, v3stream) == V3_OK)
        {
            v3_cpp_obj(fV3.controller)->set_state(fV3.controller, v3stream);
            pData->updateParameterValues(this, true, true, false);
        }

        runIdleCallbacksAsNeeded(false);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set ui stuff

   #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
    void setCustomUITitle(const char* const title) noexcept override
    {
        if (fUI.window != nullptr)
        {
            try {
                fUI.window->setTitle(title);
            } CARLA_SAFE_EXCEPTION("set custom ui title");
        }

        CarlaPlugin::setCustomUITitle(title);
    }

    void showCustomUI(const bool yesNo) override
    {
        if (fUI.isVisible == yesNo)
            return;

        CARLA_SAFE_ASSERT_RETURN(fV3.view != nullptr,);

        if (yesNo)
        {
            String uiTitle;

            if (pData->uiTitle.isNotEmpty())
            {
                uiTitle = pData->uiTitle;
            }
            else
            {
                uiTitle  = pData->name;
                uiTitle += " (GUI)";
            }

            if (fUI.window == nullptr)
            {
                const EngineOptions& opts(pData->engine->getOptions());

                const bool isStandalone = opts.pluginsAreStandalone;
                const bool isResizable = v3_cpp_obj(fV3.view)->can_resize(fV3.view) == V3_TRUE;

               #if defined(CARLA_OS_MAC)
                fUI.window = CarlaPluginUI::newCocoa(this, opts.frontendWinId, isStandalone, isResizable);
               #elif defined(CARLA_OS_WIN)
                fUI.window = CarlaPluginUI::newWindows(this, opts.frontendWinId, isStandalone, isResizable);
               #elif defined(HAVE_X11)
                fUI.window = CarlaPluginUI::newX11(this, opts.frontendWinId, isStandalone, isResizable, false);
               #else
                pData->engine->callback(true, true,
                                        ENGINE_CALLBACK_UI_STATE_CHANGED,
                                        pData->id,
                                        -1,
                                        0, 0, 0.0f,
                                        "Unsupported UI type");
                return;
               #endif

                fUI.window->setTitle(uiTitle.buffer());

               #ifndef CARLA_OS_MAC
                if (d_isNotZero(opts.uiScale))
                {
                    // TODO inform plugin of what UI scale we use
                }
               #endif

                v3_cpp_obj(fV3.view)->set_frame(fV3.view, (v3_plugin_frame**)&fPluginFramePtr);

                if (v3_cpp_obj(fV3.view)->attached(fV3.view, fUI.window->getPtr(),
                                                   V3_VIEW_PLATFORM_TYPE_NATIVE) == V3_OK)
                {
                    v3_view_rect rect = {};
                    if (v3_cpp_obj(fV3.view)->get_size(fV3.view, &rect) == V3_OK)
                    {
                        const int32_t width = rect.right - rect.left;
                        const int32_t height = rect.bottom - rect.top;
                        carla_stdout("view attached ok, size %i %i", width, height);

                        CARLA_SAFE_ASSERT_INT2(width > 1 && height > 1, width, height);

                        if (width > 1 && height > 1)
                        {
                            fUI.isResizingFromInit = true;
                            fUI.width = width;
                            fUI.height = height;
                            fUI.window->setSize(static_cast<uint>(width), static_cast<uint>(height), true, true);
                        }
                    }
                    else
                    {
                        carla_stdout("view attached ok, size failed");
                    }

                    if (isResizable)
                    {
                        carla_zeroStruct(rect);
                        if (v3_cpp_obj(fV3.view)->check_size_constraint(fV3.view, &rect) == V3_OK)
                        {
                            const int32_t width = rect.right - rect.left;
                            const int32_t height = rect.bottom - rect.top;
                            carla_stdout("size constraint ok %i %i", width, height);

                            CARLA_SAFE_ASSERT_INT2(width > 1 && height > 1, width, height);

                            if (width > 1 && height > 1)
                                fUI.window->setMinimumSize(static_cast<uint>(width), static_cast<uint>(height));
                            else if (fUI.width > 1 && fUI.height > 1)
                                fUI.window->setMinimumSize(fUI.width, fUI.height);
                        }
                        else
                        {
                            carla_stdout("view attached ok, size constraint failed");
                        }
                    }
                }
                else
                {
                    v3_cpp_obj(fV3.view)->set_frame(fV3.view, nullptr);

                    delete fUI.window;
                    fUI.window = nullptr;

                    carla_stderr2("Plugin refused to open its own UI");
                    return pData->engine->callback(true, true,
                                                   ENGINE_CALLBACK_UI_STATE_CHANGED,
                                                   pData->id,
                                                   -1,
                                                   0, 0, 0.0f,
                                                   "Plugin refused to open its own UI");
                }
            }

            fUI.window->show();
            fUI.isVisible = true;
        }
        else
        {
            fUI.isVisible = false;

            if (fUI.window != nullptr)
                fUI.window->hide();

            if (fUI.isEmbed)
            {
                fUI.isAttached = false;
                fUI.isEmbed = false;
                v3_cpp_obj(fV3.view)->set_frame(fV3.view, nullptr);
                v3_cpp_obj(fV3.view)->removed(fV3.view);
            }
        }

        runIdleCallbacksAsNeeded(true);
    }

    void* embedCustomUI(void* const ptr) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.window == nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(fV3.view != nullptr, nullptr);

        v3_cpp_obj(fV3.view)->set_frame(fV3.view, (v3_plugin_frame**)&fPluginFramePtr);

       #ifndef CARLA_OS_MAC
        const EngineOptions& opts(pData->engine->getOptions());

        if (d_isNotZero(opts.uiScale))
        {
            // TODO
        }
       #endif

        if (v3_cpp_obj(fV3.view)->attached(fV3.view, ptr, V3_VIEW_PLATFORM_TYPE_NATIVE) == V3_OK)
        {
            fUI.isAttached = true;
            fUI.isEmbed = true;
            fUI.isVisible = true;
            v3_view_rect rect = {};

            if (v3_cpp_obj(fV3.view)->get_size(fV3.view, &rect) == V3_OK)
            {
                const int32_t width = rect.right - rect.left;
                const int32_t height = rect.bottom - rect.top;
                carla_stdout("view attached ok, size %i %i", width, height);

                CARLA_SAFE_ASSERT_INT2(width > 1 && height > 1, width, height);

                if (width > 1 && height > 1)
                {
                    fUI.isResizingFromInit = true;
                    fUI.width = width;
                    fUI.height = height;
                    pData->engine->callback(true, true,
                                            ENGINE_CALLBACK_EMBED_UI_RESIZED,
                                            pData->id, width, height,
                                            0, 0.0f, nullptr);
                }
            }
            else
            {
                carla_stdout("view attached ok, size failed");
            }
        }
        else
        {
            fUI.isVisible = false;
            v3_cpp_obj(fV3.view)->set_frame(fV3.view, nullptr);

            carla_stderr2("Plugin refused to open its own UI");
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_UI_STATE_CHANGED,
                                    pData->id,
                                    -1,
                                    0, 0, 0.0f,
                                    "Plugin refused to open its own UI");
        }

        return nullptr;
    }
   #endif // V3_VIEW_PLATFORM_TYPE_NATIVE

    void runIdleCallbacksAsNeeded(const bool isIdleCallback)
    {
        int32_t flags = fRestartFlags;

        if (isIdleCallback)
        {
        }

        fRestartFlags = flags;

      #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
       #ifdef _POSIX_VERSION
        LinkedList<HostPosixFileDescriptor>& posixfds(fPluginFrame.loop.posixfds);

        if (posixfds.isNotEmpty())
        {
            for (LinkedList<HostPosixFileDescriptor>::Itenerator it = posixfds.begin2(); it.valid(); it.next())
            {
                HostPosixFileDescriptor& posixfd(it.getValue(kPosixFileDescriptorFallbackNC));

               #ifdef CARLA_VST3_POSIX_EPOLL
                struct ::epoll_event event;
               #else
                const int16_t filter = EVFILT_WRITE;
                struct ::kevent kev = {}, event;
                struct ::timespec timeout = {};
                EV_SET(&kev, posixfd.pluginfd, filter, EV_ADD|EV_ENABLE, 0, 0, nullptr);
               #endif

                for (int i=0; i<50; ++i)
                {
                   #ifdef CARLA_VST3_POSIX_EPOLL
                    switch (::epoll_wait(posixfd.hostfd, &event, 1, 0))
                   #else
                    switch (::kevent(posixfd.hostfd, &kev, 1, &event, 1, &timeout))
                   #endif
                    {
                    case 1:
                        v3_cpp_obj(posixfd.handler)->on_fd_is_set(posixfd.handler, posixfd.pluginfd);
                        break;
                    case -1:
                        // fall through
                    case 0:
                        i = 50;
                        break;
                    default:
                        carla_safe_exception("posix fd received abnormal value", __FILE__, __LINE__);
                        i = 50;
                        break;
                    }
                }
            }
        }
       #endif // _POSIX_VERSION

        LinkedList<HostTimer>& timers(fPluginFrame.loop.timers);

        if (timers.isNotEmpty())
        {
            for (LinkedList<HostTimer>::Itenerator it = timers.begin2(); it.valid(); it.next())
            {
                HostTimer& timer(it.getValue(kTimerFallbackNC));
                const uint32_t currentTimeInMs = water::Time::getMillisecondCounter();

                if (currentTimeInMs > timer.lastCallTimeInMs + timer.periodInMs)
                {
                    timer.lastCallTimeInMs = currentTimeInMs;
                    v3_cpp_obj(timer.handler)->on_timer(timer.handler);
                }
            }
        }
      #endif // V3_VIEW_PLATFORM_TYPE_NATIVE
    }

    void idle() override
    {
        if (kEngineHasIdleOnMainThread)
            runIdleCallbacksAsNeeded(true);

        CarlaPlugin::idle();
    }

    void uiIdle() override
    {
        if (!kEngineHasIdleOnMainThread)
            runIdleCallbacksAsNeeded(true);

       #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
        if (fUI.window != nullptr)
            fUI.window->idle();

        if (fUI.isResizingFromHost)
        {
            fUI.isResizingFromHost = false;

//             if (!fUI.isResizingFromPlugin && !fUI.isResizingFromInit)
            {
                carla_stdout("Host resize stopped");
//                 v3_view_rect rect = { 0, 0, static_cast<int32_t>(fUI.width), static_cast<int32_t>(fUI.height) };
//                 v3_cpp_obj(fV3.view)->on_size(fV3.view, &rect);
            }
        }

        if (fUI.isResizingFromPlugin)
        {
            fUI.isResizingFromPlugin = false;
            carla_stdout("Plugin resize stopped");
        }
       #endif

        CarlaPlugin::uiIdle();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Plugin state

    void reload() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fV3.component != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fV3.controller != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fV3.processor != nullptr,);
        carla_debug("CarlaPluginVST3::reload() - start");

        // Safely disable plugin for reload
        const ScopedDisabler sd(this);

        if (pData->active)
            deactivate();

        clearBuffers();

        const int32_t numAudioInputBuses = v3_cpp_obj(fV3.component)->get_bus_count(fV3.component, V3_AUDIO, V3_INPUT);
        const int32_t numAudioOutputBuses = v3_cpp_obj(fV3.component)->get_bus_count(fV3.component, V3_AUDIO, V3_OUTPUT);
        const int32_t numEventInputBuses = v3_cpp_obj(fV3.component)->get_bus_count(fV3.component, V3_EVENT, V3_INPUT);
        const int32_t numEventOutputBuses = v3_cpp_obj(fV3.component)->get_bus_count(fV3.component, V3_EVENT, V3_OUTPUT);
        const int32_t numParameters = v3_cpp_obj(fV3.controller)->get_parameter_count(fV3.controller);

        CARLA_SAFE_ASSERT(numAudioInputBuses >= 0);
        CARLA_SAFE_ASSERT(numAudioOutputBuses >= 0);
        CARLA_SAFE_ASSERT(numEventInputBuses >= 0);
        CARLA_SAFE_ASSERT(numEventOutputBuses >= 0);
        CARLA_SAFE_ASSERT(numParameters >= 0);

        uint32_t aIns, aOuts, cvIns, cvOuts;
        aIns = aOuts = cvIns = cvOuts = 0;

        bool needsCtrlIn, needsCtrlOut;
        needsCtrlIn = needsCtrlOut = false;

        fBuses.createNew(numAudioInputBuses, numAudioOutputBuses);

        for (int32_t b=0; b<numAudioInputBuses; ++b)
        {
            carla_zeroStruct(fBuses.inputs[b]);
            carla_zeroStruct(fBuses.inputInfo[b]);
            fBuses.inputInfo[b].offset = aIns + cvIns;

            v3_bus_info busInfo = {};
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(fV3.component)->get_bus_info(fV3.component,
                                                                            V3_AUDIO, V3_INPUT, b, &busInfo) == V3_OK);

            const int32_t numChannels = busInfo.channel_count;
            CARLA_SAFE_ASSERT_BREAK(numChannels >= 0);

            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(fV3.component)->activate_bus(fV3.component,
                                                                            V3_AUDIO, V3_INPUT, b, true) == V3_OK);

            fBuses.inputs[b].num_channels = numChannels;
            fBuses.inputInfo[b].bus_type = busInfo.bus_type;
            fBuses.inputInfo[b].flags = busInfo.flags;

            if (busInfo.flags & V3_IS_CONTROL_VOLTAGE)
                cvIns += static_cast<uint32_t>(numChannels);
            else
                aIns += static_cast<uint32_t>(numChannels);
        }

        for (int32_t b=0; b<numAudioOutputBuses; ++b)
        {
            carla_zeroStruct(fBuses.outputs[b]);
            carla_zeroStruct(fBuses.outputInfo[b]);
            fBuses.outputInfo[b].offset = aOuts + cvOuts;

            v3_bus_info busInfo = {};
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(fV3.component)->get_bus_info(fV3.component,
                                                                            V3_AUDIO, V3_OUTPUT, b, &busInfo) == V3_OK);

            const int32_t numChannels = busInfo.channel_count;
            CARLA_SAFE_ASSERT_BREAK(numChannels >= 0);

            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(fV3.component)->activate_bus(fV3.component,
                                                                            V3_AUDIO, V3_OUTPUT, b, true) == V3_OK);

            fBuses.outputs[b].num_channels = numChannels;
            fBuses.outputInfo[b].bus_type = busInfo.bus_type;
            fBuses.outputInfo[b].flags = busInfo.flags;

            if (busInfo.flags & V3_IS_CONTROL_VOLTAGE)
                cvOuts += static_cast<uint32_t>(numChannels);
            else
                aOuts += static_cast<uint32_t>(numChannels);
        }

        if (aIns > 0)
        {
            pData->audioIn.createNew(aIns);
        }

        if (aOuts > 0)
        {
            pData->audioOut.createNew(aOuts);
            needsCtrlIn = true;
        }

        if (cvIns > 0)
            pData->cvIn.createNew(cvIns);

        if (cvOuts > 0)
            pData->cvOut.createNew(cvOuts);

        if (numEventInputBuses > 0)
            needsCtrlIn = true;

        if (numEventOutputBuses > 0)
            needsCtrlOut = true;

        if (numParameters > 0)
        {
            pData->param.createNew(numParameters, false);
            needsCtrlIn = true;
        }

        if (aOuts + cvOuts > 0)
        {
            fAudioAndCvOutBuffers = new float*[aOuts + cvOuts];

            for (uint32_t i=0; i < aOuts + cvOuts; ++i)
                fAudioAndCvOutBuffers[i] = nullptr;
        }

        const EngineProcessMode processMode = pData->engine->getProccessMode();
        const uint portNameSize = pData->engine->getMaxPortNameSize();
        String portName;

        // Audio Ins
        for (uint32_t j=0; j < aIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (aIns > 1)
            {
                portName += "input_";
                portName += String(j+1);
            }
            else
                portName += "input";

            portName.truncate(portNameSize);

            pData->audioIn.ports[j].port = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio,
                                                                                         portName, true, j);
            pData->audioIn.ports[j].rindex = j;
        }

        // Audio Outs
        for (uint32_t j=0; j < aOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (aOuts > 1)
            {
                portName += "output_";
                portName += String(j+1);
            }
            else
                portName += "output";

            portName.truncate(portNameSize);

            pData->audioOut.ports[j].port = (CarlaEngineAudioPort*)pData->client->addPort(kEnginePortTypeAudio,
                                                                                          portName, false, j);
            pData->audioOut.ports[j].rindex = j;
        }

        // CV Ins
        for (uint32_t j=0; j < cvIns; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (cvIns > 1)
            {
                portName += "cv_input_";
                portName += String(j+1);
            }
            else
                portName += "cv_input";

            portName.truncate(portNameSize);

            pData->cvIn.ports[j].port = (CarlaEngineCVPort*)pData->client->addPort(kEnginePortTypeCV,
                                                                                   portName, true, j);
            pData->cvIn.ports[j].rindex = j;
        }

        // CV Outs
        for (uint32_t j=0; j < cvOuts; ++j)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            if (cvOuts > 1)
            {
                portName += "cv_output_";
                portName += String(j+1);
            }
            else
                portName += "cv_output";

            portName.truncate(portNameSize);

            pData->cvOut.ports[j].port = (CarlaEngineCVPort*)pData->client->addPort(kEnginePortTypeCV,
                                                                                    portName, false, j);
            pData->cvOut.ports[j].rindex = j;
        }

        for (int32_t j=0; j < numParameters; ++j)
        {
            v3_param_info paramInfo = {};
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(fV3.controller)->get_parameter_info(fV3.controller,
                                                                                   j, &paramInfo) == V3_OK);

            char strBuf[200];
            strncpy_utf8(strBuf, paramInfo.title, 128);

            const v3_param_id paramId = paramInfo.param_id;
            pData->param.data[j].index  = static_cast<uint32_t>(j);
            pData->param.data[j].rindex = paramId;

            if (paramInfo.flags & (V3_PARAM_IS_BYPASS|V3_PARAM_IS_HIDDEN|V3_PARAM_PROGRAM_CHANGE))
                continue;

            double min, max, def, step, stepSmall, stepLarge;

            min = v3_cpp_obj(fV3.controller)->normalised_parameter_to_plain(fV3.controller, paramId, 0.0);
            max = v3_cpp_obj(fV3.controller)->normalised_parameter_to_plain(fV3.controller, paramId, 1.0);
            def = v3_cpp_obj(fV3.controller)->normalised_parameter_to_plain(fV3.controller, paramId,
                                                                            paramInfo.default_normalised_value);

            if (min >= max)
                max = min + 0.1;

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            if (paramInfo.flags & V3_PARAM_READ_ONLY)
                pData->param.data[j].type = PARAMETER_OUTPUT;
            else
                pData->param.data[j].type = PARAMETER_INPUT;

            if (paramInfo.step_count == 1)
            {
                step = max - min;
                stepSmall = step;
                stepLarge = step;
                pData->param.data[j].hints |= PARAMETER_IS_BOOLEAN;
            }
            /*
            else if (paramInfo.step_count != 0 && (paramInfo.flags & V3_PARAM_IS_LIST) != 0x0)
            {
                step = 1.0;
                stepSmall = 1.0;
                stepLarge = std::min(max - min, 10.0);
                pData->param.data[j].hints |= PARAMETER_IS_INTEGER;
            }
            */
            else
            {
                float range = max - min;
                step = range/100.0;
                stepSmall = range/1000.0;
                stepLarge = range/10.0;
            }

            pData->param.data[j].hints |= PARAMETER_IS_ENABLED;
            pData->param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;

            if (paramInfo.flags & V3_PARAM_CAN_AUTOMATE)
            {
                pData->param.data[j].hints |= PARAMETER_IS_AUTOMATABLE;

                if ((paramInfo.flags & V3_PARAM_IS_LIST) == 0x0)
                    pData->param.data[j].hints |= PARAMETER_CAN_BE_CV_CONTROLLED;
            }

            pData->param.ranges[j].min = min;
            pData->param.ranges[j].max = max;
            pData->param.ranges[j].def = def;
            pData->param.ranges[j].step = step;
            pData->param.ranges[j].stepSmall = stepSmall;
            pData->param.ranges[j].stepLarge = stepLarge;
        }

        if (numParameters > 0)
        {
            fEvents.paramInputs = new carla_v3_input_param_changes(pData->param);
            fEvents.paramOutputs = new carla_v3_output_param_changes(pData->param);
        }

        if (needsCtrlIn)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-in";
            portName.truncate(portNameSize);

            pData->event.portIn = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent,
                                                                                portName, true, 0);
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            pData->event.cvSourcePorts = pData->client->createCVSourcePorts();
#endif
            fEvents.eventInputs = new carla_v3_input_event_list;
        }

        if (needsCtrlOut)
        {
            portName.clear();

            if (processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT)
            {
                portName  = pData->name;
                portName += ":";
            }

            portName += "events-out";
            portName.truncate(portNameSize);

            pData->event.portOut = (CarlaEngineEventPort*)pData->client->addPort(kEnginePortTypeEvent,
                                                                                 portName, false, 0);
            fEvents.eventOutputs = new carla_v3_output_event_list;
        }

        // plugin hints
        const PluginCategory v3category = getPluginCategoryFromV3SubCategories(fV3ClassInfo.v2.sub_categories);

        pData->hints = 0x0;

        if (v3category == PLUGIN_CATEGORY_SYNTH)
            pData->hints |= PLUGIN_IS_SYNTH;

       #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
        if (fV3.view != nullptr &&
            v3_cpp_obj(fV3.view)->is_platform_type_supported(fV3.view, V3_VIEW_PLATFORM_TYPE_NATIVE) == V3_TRUE)
        {
            pData->hints |= PLUGIN_HAS_CUSTOM_UI;
            pData->hints |= PLUGIN_HAS_CUSTOM_EMBED_UI;
            pData->hints |= PLUGIN_NEEDS_UI_MAIN_THREAD;
            if (v3_cpp_obj(fV3.view)->can_resize(fV3.view) == V3_TRUE)
                pData->hints |= PLUGIN_HAS_CUSTOM_RESIZABLE_UI;
        }
       #endif

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            pData->hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            pData->hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts % 2 == 0)
            pData->hints |= PLUGIN_CAN_BALANCE;

        // extra plugin hints
        pData->extraHints = 0x0;

        if (numEventInputBuses > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_IN;

        if (numEventOutputBuses > 0)
            pData->extraHints |= PLUGIN_EXTRA_HINT_HAS_MIDI_OUT;

        // check initial latency
        if ((fLastKnownLatency = v3_cpp_obj(fV3.processor)->get_latency_samples(fV3.processor)) != 0)
        {
            pData->client->setLatency(fLastKnownLatency);
#ifndef BUILD_BRIDGE
            pData->latency.recreateBuffers(std::max(aIns+cvIns, aOuts+cvOuts), fLastKnownLatency);
#endif
        }

        // initial audio setup
        v3_process_setup setup = {
            pData->engine->isOffline() ? V3_OFFLINE : V3_REALTIME,
            V3_SAMPLE_32,
            static_cast<int32_t>(pData->engine->getBufferSize()),
            pData->engine->getSampleRate()
        };
        v3_cpp_obj(fV3.processor)->setup_processing(fV3.processor, &setup);

        // activate all buses
        for (int32_t j=0; j<numAudioInputBuses; ++j)
        {
            v3_bus_info busInfo = {};
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(fV3.component)->get_bus_info(fV3.component,
                                                                            V3_AUDIO, V3_INPUT, j, &busInfo) == V3_OK);

            if ((busInfo.flags & V3_DEFAULT_ACTIVE) == 0x0) {
                CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(fV3.component)->activate_bus(fV3.component,
                                                                                V3_AUDIO, V3_INPUT, j, true) == V3_OK);
            }
        }

        for (int32_t j=0; j<numAudioOutputBuses; ++j)
        {
            v3_bus_info busInfo = {};
            CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(fV3.component)->get_bus_info(fV3.component,
                                                                            V3_AUDIO, V3_OUTPUT, j, &busInfo) == V3_OK);

            if ((busInfo.flags & V3_DEFAULT_ACTIVE) == 0x0) {
                CARLA_SAFE_ASSERT_BREAK(v3_cpp_obj(fV3.component)->activate_bus(fV3.component,
                                                                                V3_AUDIO, V3_OUTPUT, j, true) == V3_OK);
            }
        }

        if (fEvents.paramInputs != nullptr && fV3.midiMapping != nullptr)
            fMidiControllerAssignments.init(fV3.midiMapping, numEventInputBuses);

        bufferSizeChanged(pData->engine->getBufferSize());
        reloadPrograms(true);

        if (pData->active)
            activate();
        else
            runIdleCallbacksAsNeeded(false);

        carla_debug("CarlaPluginVST3::reload() - end");
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Plugin processing

    void activate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fV3.component != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fV3.processor != nullptr,);

        try {
            v3_cpp_obj(fV3.component)->set_active(fV3.component, true);
        } CARLA_SAFE_EXCEPTION("set_active on");

        try {
            v3_cpp_obj(fV3.processor)->set_processing(fV3.processor, true);
        } CARLA_SAFE_EXCEPTION("set_processing on");

        fFirstActive = true;
        runIdleCallbacksAsNeeded(false);
    }

    void deactivate() noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fV3.component != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fV3.processor != nullptr,);

        try {
            v3_cpp_obj(fV3.processor)->set_processing(fV3.processor, false);
        } CARLA_SAFE_EXCEPTION("set_processing off");

        try {
            v3_cpp_obj(fV3.component)->set_active(fV3.component, false);
        } CARLA_SAFE_EXCEPTION("set_active off");

        runIdleCallbacksAsNeeded(false);
    }

    void process(const float* const* const audioIn, float** const audioOut,
                 const float* const* const cvIn, float** const cvOut, const uint32_t frames) override
    {
        // ------------------------------------------------------------------------------------------------------------
        // Check if active

        if (! pData->active)
        {
            // disable any output sound
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
                carla_zeroFloats(audioOut[i], frames);
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
                carla_zeroFloats(cvOut[i], frames);
            return;
        }

        fEvents.init();

        // ------------------------------------------------------------------------------------------------------------
        // Check if needs reset

        if (pData->needsReset)
        {
            if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
            {
                for (uint8_t c=0; c<MAX_MIDI_CHANNELS; ++c)
                {
                    v3_param_id paramId;
                    if (fMidiControllerAssignments.get(0, c, MIDI_CONTROL_ALL_NOTES_OFF, paramId))
                    {
                        // TODO create mapping between paramId -> index
                        uint32_t index = UINT32_MAX;
                        for (uint32_t i=0; i < pData->param.count; ++i)
                        {
                            if (static_cast<v3_param_id>(pData->param.data[i].rindex) == paramId)
                            {
                                index = i;
                                break;
                            }
                        }

                        if (index == UINT32_MAX)
                            break;

                        fEvents.paramInputs->setParamValueRT(index, 0, 0.f);
                    }
                }
            }
            else if (pData->ctrlChannel >= 0 && pData->ctrlChannel < MAX_MIDI_CHANNELS && fEvents.eventInputs != nullptr)
            {
                fEvents.eventInputs->numEvents = MAX_MIDI_NOTE;

                for (uint8_t i=0; i < MAX_MIDI_NOTE; ++i)
                {
                    v3_event& event(fEvents.eventInputs->events[i]);
                    carla_zeroStruct(event);
                    event.type = V3_EVENT_NOTE_OFF;
                    event.note_off.channel = (pData->ctrlChannel & MIDI_CHANNEL_BIT);
                    event.note_off.pitch = i;
                }
            }

            pData->needsReset = false;
        }

        // ------------------------------------------------------------------------------------------------------------
        // Set TimeInfo

        const EngineTimeInfo timeInfo(pData->engine->getTimeInfo());

        fV3TimeContext.state = V3_PROCESS_CTX_PROJECT_TIME_VALID | V3_PROCESS_CTX_CONT_TIME_VALID;
        fV3TimeContext.sample_rate = pData->engine->getSampleRate();
        fV3TimeContext.project_time_in_samples = fV3TimeContext.continuous_time_in_samples
                                               = static_cast<int64_t>(timeInfo.frame);

        if (fFirstActive || ! fLastTimeInfo.compareIgnoringRollingFrames(timeInfo, frames))
            fLastTimeInfo = timeInfo;

        if (timeInfo.playing)
            fV3TimeContext.state |= V3_PROCESS_CTX_PLAYING;

        if (timeInfo.usecs != 0)
        {
            fV3TimeContext.system_time_ns = static_cast<int64_t>(timeInfo.usecs / 1000);
            fV3TimeContext.state |= V3_PROCESS_CTX_SYSTEM_TIME_VALID;
        }

        if (timeInfo.bbt.valid)
        {
            CARLA_SAFE_ASSERT_INT(timeInfo.bbt.bar > 0, timeInfo.bbt.bar);
            CARLA_SAFE_ASSERT_INT(timeInfo.bbt.beat > 0, timeInfo.bbt.beat);

            const double ppqBar  = static_cast<double>(timeInfo.bbt.beatsPerBar) * (timeInfo.bbt.bar - 1);
            // const double ppqBeat = static_cast<double>(timeInfo.bbt.beat - 1);
            // const double ppqTick = timeInfo.bbt.tick / timeInfo.bbt.ticksPerBeat;

            // PPQ Pos
            fV3TimeContext.project_time_quarters = static_cast<double>(timeInfo.frame) / (fV3TimeContext.sample_rate * 60 / timeInfo.bbt.beatsPerMinute);
            // fTimeInfo.project_time_quarters = ppqBar + ppqBeat + ppqTick;
            fV3TimeContext.state |= V3_PROCESS_CTX_PROJECT_TIME_VALID;

            // Tempo
            fV3TimeContext.bpm = timeInfo.bbt.beatsPerMinute;
            fV3TimeContext.state |= V3_PROCESS_CTX_TEMPO_VALID;

            // Bars
            fV3TimeContext.bar_position_quarters = ppqBar;
            fV3TimeContext.state |= V3_PROCESS_CTX_BAR_POSITION_VALID;

            // Time Signature
            fV3TimeContext.time_sig_numerator = static_cast<int32_t>(timeInfo.bbt.beatsPerBar + 0.5f);
            fV3TimeContext.time_sig_denom = static_cast<int32_t>(timeInfo.bbt.beatType + 0.5f);
            fV3TimeContext.state |= V3_PROCESS_CTX_TIME_SIG_VALID;
        }
        else
        {
            // Tempo
            fV3TimeContext.bpm = 120.0;
            fV3TimeContext.state |= V3_PROCESS_CTX_TEMPO_VALID;

            // Time Signature
            fV3TimeContext.time_sig_numerator = 4;
            fV3TimeContext.time_sig_denom = 4;
            fV3TimeContext.state |= V3_PROCESS_CTX_TIME_SIG_VALID;

            // Missing info
            fV3TimeContext.project_time_quarters = 0.0;
            fV3TimeContext.bar_position_quarters = 0.0;
        }

        // ------------------------------------------------------------------------------------------------------------
        // Event Input and Processing

        if (pData->event.portIn != nullptr && fEvents.eventInputs != nullptr)
        {
            // --------------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (pData->extNotes.mutex.tryLock())
            {
                ExternalMidiNote note = { 0, 0, 0 };
                uint16_t numEvents = fEvents.eventInputs->numEvents;

                for (; numEvents < kPluginMaxMidiEvents && ! pData->extNotes.data.isEmpty();)
                {
                    note = pData->extNotes.data.getFirst(note, true);

                    CARLA_SAFE_ASSERT_CONTINUE(note.channel >= 0 && note.channel < MAX_MIDI_CHANNELS);

                    v3_event& event(fEvents.eventInputs->events[numEvents++]);
                    carla_zeroStruct(event);

                    if (note.velo > 0)
                    {
                        event.type = V3_EVENT_NOTE_ON;
                        event.note_on.channel = (note.channel & MIDI_CHANNEL_BIT);
                        event.note_on.pitch = note.note;
                        event.note_on.velocity = static_cast<float>(note.velo) / 127.f;
                    }
                    else
                    {
                        event.type = V3_EVENT_NOTE_OFF;
                        event.note_off.channel = (note.channel & MIDI_CHANNEL_BIT);
                        event.note_off.pitch = note.note;
                    }
                }

                pData->extNotes.mutex.unlock();

                fEvents.eventInputs->numEvents = numEvents;

            } // End of MIDI Input (External)

            // --------------------------------------------------------------------------------------------------------
            // Event Input (System)

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            bool allNotesOffSent  = false;
#endif
            bool isSampleAccurate = (pData->options & PLUGIN_OPTION_FIXED_BUFFERS) == 0;

            uint32_t startTime = 0;
            uint32_t timeOffset = 0;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
            if (cvIn != nullptr && pData->event.cvSourcePorts != nullptr)
                pData->event.cvSourcePorts->initPortBuffers(cvIn, frames, isSampleAccurate, pData->event.portIn);
#endif

            for (uint32_t i=0, numEvents = pData->event.portIn->getEventCount(); i < numEvents; ++i)
            {
                EngineEvent& event(pData->event.portIn->getEvent(i));

                uint32_t eventTime = event.time;
                CARLA_SAFE_ASSERT_UINT2_CONTINUE(eventTime < frames, eventTime, frames);

                if (eventTime < timeOffset)
                {
                    carla_stderr2("Timing error, eventTime:%u < timeOffset:%u for '%s'",
                                  eventTime, timeOffset, pData->name);
                    eventTime = timeOffset;
                }

                if (isSampleAccurate && eventTime > timeOffset)
                {
                    if (processSingle(audioIn, audioOut, cvIn, cvOut, eventTime - timeOffset, timeOffset))
                    {
                        startTime  = 0;
                        timeOffset = eventTime;

                        // TODO
                    }
                    else
                    {
                        startTime += timeOffset;
                    }
                }

                switch (event.type)
                {
                case kEngineEventTypeNull:
                    break;

                case kEngineEventTypeControl: {
                    EngineControlEvent& ctrlEvent(event.ctrl);

                    switch (ctrlEvent.type)
                    {
                    case kEngineControlEventTypeNull:
                        break;

                    case kEngineControlEventTypeParameter: {
                        float value;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        // non-midi
                        if (event.channel == kEngineEventNonMidiChannel)
                        {
                            const uint32_t k = ctrlEvent.param;
                            CARLA_SAFE_ASSERT_CONTINUE(k < pData->param.count);

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                            continue;
                        }

                        // Control backend stuff
                        if (event.channel == pData->ctrlChannel)
                        {
                            if (MIDI_IS_CONTROL_BREATH_CONTROLLER(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_DRYWET) != 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue;
                                setDryWetRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_CHANNEL_VOLUME(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_VOLUME) != 0)
                            {
                                ctrlEvent.handled = true;
                                value = ctrlEvent.normalizedValue*127.0f/100.0f;
                                setVolumeRT(value, true);
                            }
                            else if (MIDI_IS_CONTROL_BALANCE(ctrlEvent.param) && (pData->hints & PLUGIN_CAN_BALANCE) != 0)
                            {
                                float left, right;
                                value = ctrlEvent.normalizedValue/0.5f - 1.0f;

                                if (value < 0.0f)
                                {
                                    left  = -1.0f;
                                    right = (value*2.0f)+1.0f;
                                }
                                else if (value > 0.0f)
                                {
                                    left  = (value*2.0f)-1.0f;
                                    right = 1.0f;
                                }
                                else
                                {
                                    left  = -1.0f;
                                    right = 1.0f;
                                }

                                ctrlEvent.handled = true;
                                setBalanceLeftRT(left, true);
                                setBalanceRightRT(right, true);
                            }
                        }
#endif
                        // Control plugin parameters
                        uint32_t k;
                        for (k=0; k < pData->param.count; ++k)
                        {
                            if (pData->param.data[k].midiChannel != event.channel)
                                continue;
                            if (pData->param.data[k].mappedControlIndex != ctrlEvent.param)
                                continue;
                            if (pData->param.data[k].type != PARAMETER_INPUT)
                                continue;
                            if ((pData->param.data[k].hints & PARAMETER_IS_AUTOMATABLE) == 0)
                                continue;

                            ctrlEvent.handled = true;
                            value = pData->param.getFinalUnnormalizedValue(k, ctrlEvent.normalizedValue);
                            setParameterValueRT(k, value, event.time, true);
                        }

                        if ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) != 0 && ctrlEvent.param < MAX_MIDI_VALUE)
                        {
                            v3_param_id paramId;
                            if (fMidiControllerAssignments.get(0, event.channel, ctrlEvent.param, paramId))
                            {
                                // TODO create mapping between paramId -> index
                                uint32_t index = UINT32_MAX;
                                for (uint32_t i=0; i < pData->param.count; ++i)
                                {
                                    if (static_cast<v3_param_id>(pData->param.data[i].rindex) == paramId)
                                    {
                                        index = i;
                                        break;
                                    }
                                }

                                if (index == UINT32_MAX)
                                    break;

                                fEvents.paramInputs->setParamValueRT(index, event.time, ctrlEvent.normalizedValue);
                            }
                        }

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                        if (! ctrlEvent.handled)
                            checkForMidiLearn(event);
#endif
                        break;
                    } // case kEngineControlEventTypeParameter

                    case kEngineControlEventTypeMidiBank:
                        break;

                    case kEngineControlEventTypeMidiProgram:
                        if (event.channel == pData->ctrlChannel && (pData->options & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) != 0)
                        {
                            if (ctrlEvent.param < pData->prog.count)
                            {
                                setProgramRT(ctrlEvent.param, true);
                                break;
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllSoundOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
                            v3_param_id paramId;
                            if (fMidiControllerAssignments.get(0, event.channel, MIDI_CONTROL_ALL_SOUND_OFF, paramId))
                            {
                                // TODO create mapping between paramId -> index
                                uint32_t index = UINT32_MAX;
                                for (uint32_t i=0; i < pData->param.count; ++i)
                                {
                                    if (static_cast<v3_param_id>(pData->param.data[i].rindex) == paramId)
                                    {
                                        index = i;
                                        break;
                                    }
                                }

                                if (index == UINT32_MAX)
                                    break;

                                fEvents.paramInputs->setParamValueRT(index, event.time, 0.f);
                            }
                        }
                        break;

                    case kEngineControlEventTypeAllNotesOff:
                        if (pData->options & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
                        {
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
                            if (event.channel == pData->ctrlChannel && ! allNotesOffSent)
                            {
                                allNotesOffSent = true;
                                postponeRtAllNotesOff();
                            }
#endif

                            v3_param_id paramId;
                            if (fMidiControllerAssignments.get(0, event.channel, MIDI_CONTROL_ALL_NOTES_OFF, paramId))
                            {
                                // TODO create mapping between paramId -> index
                                uint32_t index = UINT32_MAX;
                                for (uint32_t i=0; i < pData->param.count; ++i)
                                {
                                    if (static_cast<v3_param_id>(pData->param.data[i].rindex) == paramId)
                                    {
                                        index = i;
                                        break;
                                    }
                                }

                                if (index == UINT32_MAX)
                                    break;

                                fEvents.paramInputs->setParamValueRT(index, event.time, 0.f);
                            }
                        }
                        break;
                    } // switch (ctrlEvent.type)
                    break;
                } // case kEngineEventTypeControl

                case kEngineEventTypeMidi: {
                    const EngineMidiEvent& midiEvent(event.midi);

                    if (midiEvent.size > 3)
                        continue;
#ifdef CARLA_PROPER_CPP11_SUPPORT
                    static_assert(3 <= EngineMidiEvent::kDataSize, "Incorrect data");
#endif

                    uint8_t status = uint8_t(MIDI_GET_STATUS_FROM_DATA(midiEvent.data));

                    if ((status == MIDI_STATUS_NOTE_OFF || status == MIDI_STATUS_NOTE_ON) && (pData->options & PLUGIN_OPTION_SKIP_SENDING_NOTES))
                        continue;
                    if (status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH && (pData->options & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH) == 0)
                        continue;
                    if (status == MIDI_STATUS_CONTROL_CHANGE && ((pData->options & PLUGIN_OPTION_SEND_CONTROL_CHANGES) == 0 || fEvents.paramInputs == nullptr || fV3.midiMapping == nullptr))
                        continue;
                    if (status == MIDI_STATUS_CHANNEL_PRESSURE && ((pData->options & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE) == 0 || fEvents.paramInputs == nullptr || fV3.midiMapping == nullptr))
                        continue;
                    if (status == MIDI_STATUS_PITCH_WHEEL_CONTROL && ((pData->options & PLUGIN_OPTION_SEND_PITCHBEND) == 0 || fEvents.paramInputs == nullptr || fV3.midiMapping == nullptr))
                        continue;

                    // Fix bad note-off
                    if (status == MIDI_STATUS_NOTE_ON && midiEvent.data[2] == 0)
                        status = MIDI_STATUS_NOTE_OFF;

                    switch (status)
                    {
                    case MIDI_STATUS_NOTE_OFF:
                        if (fEvents.eventInputs->numEvents < kPluginMaxMidiEvents)
                        {
                            const uint8_t note = midiEvent.data[1];

                            v3_event& v3event(fEvents.eventInputs->events[fEvents.eventInputs->numEvents++]);
                            carla_zeroStruct(v3event);

                            v3event.type = V3_EVENT_NOTE_OFF;
                            v3event.note_off.channel = event.channel & MIDI_CHANNEL_BIT;
                            v3event.note_off.pitch   = note;

                            pData->postponeNoteOffRtEvent(true, event.channel, note);
                        }
                        break;

                    case MIDI_STATUS_NOTE_ON:
                        if (fEvents.eventInputs->numEvents < kPluginMaxMidiEvents)
                        {
                            const uint8_t note = midiEvent.data[1];
                            const uint8_t velo = midiEvent.data[2];

                            v3_event& v3event(fEvents.eventInputs->events[fEvents.eventInputs->numEvents++]);
                            carla_zeroStruct(v3event);

                            v3event.type = V3_EVENT_NOTE_ON;
                            v3event.note_on.channel  = event.channel & MIDI_CHANNEL_BIT;
                            v3event.note_on.pitch    = note;
                            v3event.note_on.velocity = static_cast<float>(velo) / 127.f;

                            pData->postponeNoteOnRtEvent(true, event.channel, note, velo);
                        }
                        break;

                    case MIDI_STATUS_POLYPHONIC_AFTERTOUCH:
                        if (fEvents.eventInputs->numEvents < kPluginMaxMidiEvents)
                        {
                            const uint8_t note     = midiEvent.data[1];
                            const uint8_t pressure = midiEvent.data[2];

                            v3_event& v3event(fEvents.eventInputs->events[fEvents.eventInputs->numEvents++]);
                            carla_zeroStruct(v3event);

                            v3event.type = V3_EVENT_POLY_PRESSURE;
                            v3event.poly_pressure.channel  = event.channel;
                            v3event.poly_pressure.pitch    = note;
                            v3event.poly_pressure.pressure = static_cast<float>(pressure) / 127.f;
                        }
                        break;

                    case MIDI_STATUS_CONTROL_CHANGE:
                        {
                            const uint8_t control = midiEvent.data[1];
                            const uint8_t value   = midiEvent.data[2];

                            v3_param_id paramId;
                            if (fMidiControllerAssignments.get(midiEvent.port, event.channel, control, paramId))
                            {
                                // TODO create mapping between paramId -> index
                                uint32_t index = UINT32_MAX;
                                for (uint32_t i=0; i < pData->param.count; ++i)
                                {
                                    if (static_cast<v3_param_id>(pData->param.data[i].rindex) == paramId)
                                    {
                                        index = i;
                                        break;
                                    }
                                }

                                if (index == UINT32_MAX)
                                    break;

                                fEvents.paramInputs->setParamValueRT(index,
                                                                     event.time,
                                                                     static_cast<float>(value) / 127.f);
                            }
                        }
                        break;

                    case MIDI_STATUS_CHANNEL_PRESSURE:
                        {
                            const uint8_t pressure = midiEvent.data[1];

                            v3_param_id paramId;
                            if (fMidiControllerAssignments.get(midiEvent.port, event.channel, 128, paramId))
                            {
                                // TODO create mapping between paramId -> index
                                uint32_t index = UINT32_MAX;
                                for (uint32_t i=0; i < pData->param.count; ++i)
                                {
                                    if (static_cast<v3_param_id>(pData->param.data[i].rindex) == paramId)
                                    {
                                        index = i;
                                        break;
                                    }
                                }

                                if (index == UINT32_MAX)
                                    break;

                                fEvents.paramInputs->setParamValueRT(index,
                                                                     event.time,
                                                                     static_cast<float>(pressure) / 127.f);
                            }
                        }
                        break;

                    case MIDI_STATUS_PITCH_WHEEL_CONTROL:
                        {
                            const uint16_t pitchbend = (midiEvent.data[2] << 7) | midiEvent.data[1];

                            v3_param_id paramId;
                            if (fMidiControllerAssignments.get(midiEvent.port, event.channel, 129, paramId))
                            {
                                // TODO create mapping between paramId -> index
                                uint32_t index = UINT32_MAX;
                                for (uint32_t i=0; i < pData->param.count; ++i)
                                {
                                    if (static_cast<v3_param_id>(pData->param.data[i].rindex) == paramId)
                                    {
                                        index = i;
                                        break;
                                    }
                                }

                                if (index == UINT32_MAX)
                                    break;

                                fEvents.paramInputs->setParamValueRT(index,
                                                                     event.time,
                                                                     static_cast<float>(pitchbend) / 16384.f);
                            }
                        }
                        break;

                    } // switch (status)

                } break;
                } // switch (event.type)
            }

            pData->postRtEvents.trySplice();

            if (frames > timeOffset)
                processSingle(audioIn, audioOut, cvIn, cvOut, frames - timeOffset, timeOffset);

        } // End of Event Input and Processing

        // ------------------------------------------------------------------------------------------------------------
        // Plugin processing (no events)

        else
        {
            processSingle(audioIn, audioOut, cvIn, cvOut, frames, 0);

        } // End of Plugin processing (no events)

        fFirstActive = false;

        // ------------------------------------------------------------------------------------------------------------
    }

    bool processSingle(const float* const* const inBuffer, float** const outBuffer,
                       const float* const* const cvIn, float** const cvOut,
                       const uint32_t frames, const uint32_t timeOffset)
    {
        CARLA_SAFE_ASSERT_RETURN(frames > 0, false);

        if (pData->audioIn.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(inBuffer != nullptr, false);
        }
        if (pData->audioOut.count > 0)
        {
            CARLA_SAFE_ASSERT_RETURN(outBuffer != nullptr, false);
            CARLA_SAFE_ASSERT_RETURN(fAudioAndCvOutBuffers != nullptr, false);
        }

        // ------------------------------------------------------------------------------------------------------------
        // Try lock, silence otherwise

#ifndef STOAT_TEST_BUILD
        if (pData->engine->isOffline())
        {
            pData->singleMutex.lock();
        }
        else
#endif
        if (! pData->singleMutex.tryLock())
        {
            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                for (uint32_t k=0; k < frames; ++k)
                    outBuffer[i][k+timeOffset] = 0.0f;
            }
            for (uint32_t i=0; i < pData->cvOut.count; ++i)
            {
                for (uint32_t k=0; k < frames; ++k)
                    cvOut[i][k+timeOffset] = 0.0f;
            }

            return false;
        }

        // ------------------------------------------------------------------------------------------------------------
        // Set audio buffers

        float* bufferAudioIn[96]; // std::max(1u, pData->audioIn.count + pData->cvIn.count)
        float* bufferAudioOut[96]; // std::max(1u, pData->audioOut.count + pData->cvOut.count)

        for (uint32_t i=0; i < pData->audioIn.count; ++i)
            bufferAudioIn[i] = const_cast<float*>(inBuffer[i]+timeOffset);

        for (uint32_t i=0, j=pData->audioIn.count; i < pData->cvIn.count; ++i, ++j)
            bufferAudioIn[j] = const_cast<float*>(cvIn[i]+timeOffset);

        for (uint32_t i=0; i < pData->audioOut.count + pData->cvOut.count; ++i)
        {
            bufferAudioOut[i] = fAudioAndCvOutBuffers[i]+timeOffset;
            carla_zeroFloats(bufferAudioOut[i], frames);
        }

        // ------------------------------------------------------------------------------------------------------------
        // Set MIDI events

        // TODO

        // ------------------------------------------------------------------------------------------------------------
        // Run plugin

        fEvents.prepare();

        for (int32_t b = 0, j = 0; b < fBuses.numInputs; ++b)
        {
            fBuses.inputs[b].channel_buffers_32 = const_cast<float**>(bufferAudioIn + j);
            j += fBuses.inputs[b].num_channels;
        }

        for (int32_t b = 0, j = 0; b < fBuses.numOutputs; ++b)
        {
            fBuses.outputs[b].channel_buffers_32 = bufferAudioOut + j;
            j += fBuses.outputs[b].num_channels;
        }

        v3_process_data processData = {
            pData->engine->isOffline() ? V3_OFFLINE : V3_REALTIME,
            V3_SAMPLE_32,
            static_cast<int32_t>(frames),
            fBuses.numInputs,
            fBuses.numOutputs,
            fBuses.inputs,
            fBuses.outputs,
            fEvents.paramInputs != nullptr ? (v3_param_changes**)&fEvents.paramInputs : nullptr,
            fEvents.paramOutputs != nullptr ? (v3_param_changes**)&fEvents.paramOutputs : nullptr,
            fEvents.eventInputs != nullptr ? (v3_event_list**)&fEvents.eventInputs : nullptr,
            fEvents.eventOutputs != nullptr ? (v3_event_list**)&fEvents.eventOutputs : nullptr,
            &fV3TimeContext
        };

        try {
            v3_cpp_obj(fV3.processor)->process(fV3.processor, &processData);
        } CARLA_SAFE_EXCEPTION("process");

        // ------------------------------------------------------------------------------------------------------------
        // Handle MIDI output

        int32_t minPortOutOffset = 0;

        if (fEvents.eventOutputs != nullptr)
        {
            uint8_t midiData[3], midiSize;

            for (uint32_t i=0; i < fEvents.eventOutputs->numEvents; ++i)
            {
                v3_event& v3event(fEvents.eventOutputs->events[i]);

                if (v3event.bus_index != 0)
                    continue;

                switch (v3event.type)
                {
                case V3_EVENT_NOTE_OFF:
                    midiData[0] = MIDI_STATUS_NOTE_OFF | (v3event.note_off.channel & MIDI_CHANNEL_BIT);
                    midiData[1] = v3event.note_off.pitch;
                    midiData[2] = carla_fixedValue<uint8_t>(0,
                                                            MAX_MIDI_VALUE - 1,
                                                            v3event.note_off.velocity * MAX_MIDI_VALUE);
                    midiSize = 3;
                    break;
                case V3_EVENT_NOTE_ON:
                    midiData[0] = MIDI_STATUS_NOTE_ON | (v3event.note_on.channel & MIDI_CHANNEL_BIT);
                    midiData[1] = v3event.note_on.pitch;
                    midiData[2] = carla_fixedValue<uint8_t>(0,
                                                            MAX_MIDI_VALUE - 1,
                                                            v3event.note_on.velocity * MAX_MIDI_VALUE);
                    midiSize = 3;
                    break;
                case V3_EVENT_POLY_PRESSURE:
                    midiData[0] = MIDI_STATUS_POLYPHONIC_AFTERTOUCH | (v3event.poly_pressure.channel & MIDI_CHANNEL_BIT);
                    midiData[1] = v3event.poly_pressure.pitch;
                    midiData[2] = carla_fixedValue<uint8_t>(0,
                                                            MAX_MIDI_VALUE - 1,
                                                            v3event.poly_pressure.pressure * MAX_MIDI_VALUE);
                    midiSize = 3;
                    break;
                case V3_EVENT_LEGACY_MIDI_CC_OUT:
                    midiData[0] = MIDI_STATUS_CONTROL_CHANGE | (v3event.midi_cc_out.channel & MIDI_CHANNEL_BIT);
                    midiData[1] = v3event.midi_cc_out.cc_number;
                    midiData[2] = v3event.midi_cc_out.value;
                    midiSize = 3;
                    break;
                default:
                    continue;
                }

                if (! pData->event.portOut->writeMidiEvent(static_cast<uint32_t>(v3event.sample_offset) + timeOffset,
                                                           midiSize,
                                                           midiData))
                    break;

                minPortOutOffset = v3event.sample_offset;
            }
        }

        // ------------------------------------------------------------------------------------------------------------
        // Handle parameter outputs

        if (fEvents.paramOutputs != nullptr && fEvents.paramOutputs->numParametersUsed != 0)
        {
            uint8_t channel;
            uint16_t param;

            for (uint32_t i=0; i < pData->param.count; ++i)
            {
                if (fEvents.paramOutputs->parametersUsed[i])
                {
                    carla_v3_output_param_value_queue* const queue = fEvents.paramOutputs->queue[i];
                    const v3_param_id paramId = pData->param.data[i].rindex;

                    const float value = v3_cpp_obj(fV3.controller)->normalised_parameter_to_plain(fV3.controller,
                                                                                                  paramId,
                                                                                                  queue->value);

                    pData->postponeParameterChangeRtEvent(true, static_cast<int32_t>(i), value);

                    if (pData->param.data[i].type == PARAMETER_OUTPUT && pData->param.data[i].mappedControlIndex > 0)
                    {
                        channel = pData->param.data[i].midiChannel;
                        param = static_cast<uint16_t>(pData->param.data[i].mappedControlIndex);

                        pData->event.portOut->writeControlEvent(std::max(minPortOutOffset, queue->offset) + timeOffset,
                                                                channel,
                                                                kEngineControlEventTypeParameter,
                                                                param,
                                                                -1,
                                                                queue->value);
                    }
                }
            }
        }

        pData->postRtEvents.trySplice();

        fEvents.init();

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
        // ------------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        {
            const bool doDryWet  = (pData->hints & PLUGIN_CAN_DRYWET) != 0
                                    && carla_isNotEqual(pData->postProc.dryWet, 1.0f);
            const bool doBalance = (pData->hints & PLUGIN_CAN_BALANCE) != 0
                                    && ! (carla_isEqual(pData->postProc.balanceLeft, -1.0f)
                                          && carla_isEqual(pData->postProc.balanceRight, 1.0f));
            const bool isMono    = (pData->audioIn.count == 1);

            bool isPair;
            float bufValue;
            float* const oldBufLeft = pData->postProc.extraBuffer;

            for (uint32_t i=0; i < pData->audioOut.count; ++i)
            {
                // Dry/Wet
                if (doDryWet)
                {
                    const uint32_t c = isMono ? 0 : i;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        bufValue = inBuffer[c][k+timeOffset];
                        fAudioAndCvOutBuffers[i][k] = (fAudioAndCvOutBuffers[i][k] * pData->postProc.dryWet)
                                                    + (bufValue * (1.0f - pData->postProc.dryWet));
                    }
                }

                // Balance
                if (doBalance)
                {
                    isPair = (i % 2 == 0);

                    if (isPair)
                    {
                        CARLA_ASSERT(i+1 < pData->audioOut.count);
                        carla_copyFloats(oldBufLeft, fAudioAndCvOutBuffers[i], frames);
                    }

                    float balRangeL = (pData->postProc.balanceLeft  + 1.0f)/2.0f;
                    float balRangeR = (pData->postProc.balanceRight + 1.0f)/2.0f;

                    for (uint32_t k=0; k < frames; ++k)
                    {
                        if (isPair)
                        {
                            // left
                            fAudioAndCvOutBuffers[i][k]  = oldBufLeft[k]                 * (1.0f - balRangeL);
                            fAudioAndCvOutBuffers[i][k] += fAudioAndCvOutBuffers[i+1][k] * (1.0f - balRangeR);
                        }
                        else
                        {
                            // right
                            fAudioAndCvOutBuffers[i][k]  = fAudioAndCvOutBuffers[i][k] * balRangeR;
                            fAudioAndCvOutBuffers[i][k] += oldBufLeft[k]               * balRangeL;
                        }
                    }
                }

                // Volume (and buffer copy)
                {
                    for (uint32_t k=0; k < frames; ++k)
                        outBuffer[i][k+timeOffset] = fAudioAndCvOutBuffers[i][k] * pData->postProc.volume;
                }
            }

            for (uint32_t i=0, j=pData->audioOut.count; i < pData->cvOut.count; ++i, ++j)
                carla_copyFloats(cvOut[i] + timeOffset, fAudioAndCvOutBuffers[j] + timeOffset, frames);

        } // End of Post-processing
#else // BUILD_BRIDGE_ALTERNATIVE_ARCH
        for (uint32_t i=0; i < pData->audioOut.count; ++i)
            carla_copyFloats(outBuffer[i] + timeOffset, fAudioAndCvOutBuffers[i] + timeOffset, frames);

        for (uint32_t i=0, j=pData->audioOut.count; i < pData->cvOut.count; ++i, ++j)
            carla_copyFloats(cvOut[i] + timeOffset, fAudioAndCvOutBuffers[j] + timeOffset, frames);
#endif

        // ------------------------------------------------------------------------------------------------------------

        pData->singleMutex.unlock();
        return true;
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        CARLA_ASSERT_INT(newBufferSize > 0, newBufferSize);
        carla_debug("CarlaPluginVST3::bufferSizeChanged(%i)", newBufferSize);

        if (pData->active)
            deactivate();

        for (uint32_t i=0; i < pData->audioOut.count + pData->cvOut.count; ++i)
        {
            if (fAudioAndCvOutBuffers[i] != nullptr)
                delete[] fAudioAndCvOutBuffers[i];
            fAudioAndCvOutBuffers[i] = new float[newBufferSize];
        }

        v3_process_setup setup = {
            pData->engine->isOffline() ? V3_OFFLINE : V3_REALTIME,
            V3_SAMPLE_32,
            static_cast<int32_t>(newBufferSize),
            pData->engine->getSampleRate()
        };
        v3_cpp_obj(fV3.processor)->setup_processing(fV3.processor, &setup);

        if (pData->active)
            activate();

        CarlaPlugin::bufferSizeChanged(newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        CARLA_ASSERT_INT(newSampleRate > 0.0, newSampleRate);
        carla_debug("CarlaPluginVST3::sampleRateChanged(%g)", newSampleRate);

        if (pData->active)
            deactivate();

        v3_process_setup setup = {
            pData->engine->isOffline() ? V3_OFFLINE : V3_REALTIME,
            V3_SAMPLE_32,
            static_cast<int32_t>(pData->engine->getBufferSize()),
            newSampleRate
        };
        v3_cpp_obj(fV3.processor)->setup_processing(fV3.processor, &setup);

        if (pData->active)
            activate();
    }

    void offlineModeChanged(const bool isOffline) override
    {
        carla_debug("CarlaPluginVST3::offlineModeChanged(%d)", isOffline);

        if (pData->active)
            deactivate();

        v3_process_setup setup = {
            isOffline ? V3_OFFLINE : V3_REALTIME,
            V3_SAMPLE_32,
            static_cast<int32_t>(pData->engine->getBufferSize()),
            pData->engine->getSampleRate()
        };
        v3_cpp_obj(fV3.processor)->setup_processing(fV3.processor, &setup);

        if (pData->active)
            activate();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Plugin buffers

    void clearBuffers() noexcept override
    {
        carla_debug("CarlaPluginVST3::clearBuffers() - start");

        if (fAudioAndCvOutBuffers != nullptr)
        {
            for (uint32_t i=0; i < pData->audioOut.count + pData->cvOut.count; ++i)
            {
                if (fAudioAndCvOutBuffers[i] != nullptr)
                {
                    delete[] fAudioAndCvOutBuffers[i];
                    fAudioAndCvOutBuffers[i] = nullptr;
                }
            }

            delete[] fAudioAndCvOutBuffers;
            fAudioAndCvOutBuffers = nullptr;
        }

        CarlaPlugin::clearBuffers();

        carla_debug("CarlaPluginVST3::clearBuffers() - end");
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Post-poned UI Stuff

    void uiParameterChange(const uint32_t index, const float value) noexcept override
    {
        CARLA_SAFE_ASSERT_RETURN(fV3.controller != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(index < pData->param.count,);

        const v3_param_id paramId = pData->param.data[index].rindex;
        const double normalized = v3_cpp_obj(fV3.controller)->plain_parameter_to_normalised(fV3.controller,
                                                                                            paramId, value);
        v3_cpp_obj(fV3.controller)->set_parameter_normalised(fV3.controller, paramId, normalized);
    }

    // ----------------------------------------------------------------------------------------------------------------

    bool hasMidiInput() const noexcept
    {
        return pData->extraHints & PLUGIN_EXTRA_HINT_HAS_MIDI_IN ||
               std::strstr(fV3ClassInfo.v2.sub_categories, "Instrument") != nullptr ||
               v3_cpp_obj(fV3.component)->get_bus_count(fV3.component, V3_EVENT, V3_INPUT) > 0;
    }

    // ----------------------------------------------------------------------------------------------------------------

    const void* getNativeDescriptor() const noexcept override
    {
        return fV3.component;
    }

    const void* getExtraStuff() const noexcept override
    {
        return fV3.controller;
    }

    // ----------------------------------------------------------------------------------------------------------------

    bool init(const CarlaPluginPtr plugin,
              const char* const filename,
              const char* name,
              const char* /*const label*/,
              const uint options)
    {
        CARLA_SAFE_ASSERT_RETURN(pData->engine != nullptr, false);

        // ------------------------------------------------------------------------------------------------------------
        // first checks

        if (pData->client != nullptr)
        {
            pData->engine->setLastError("Plugin client is already registered");
            return false;
        }

        if (filename == nullptr || filename[0] == '\0')
        {
            pData->engine->setLastError("null filename");
            return false;
        }

        V3_ENTRYFN v3_entry;
        V3_EXITFN v3_exit;
        V3_GETFN v3_get;

        // filename is full path to binary
        if (water::File(filename).existsAsFile())
        {
            if (! pData->libOpen(filename))
            {
                pData->engine->setLastError(pData->libError(filename));
                return false;
            }

            v3_entry = pData->libSymbol<V3_ENTRYFN>(V3_ENTRYFNNAME);
            v3_exit = pData->libSymbol<V3_EXITFN>(V3_EXITFNNAME);
            v3_get = pData->libSymbol<V3_GETFN>(V3_GETFNNAME);
        }
        // assume filename is a vst3 bundle
        else
        {
           #ifdef CARLA_OS_MAC
            if (! fMacBundleLoader.load(filename))
            {
                pData->engine->setLastError("Failed to load VST3 bundle executable");
                return false;
            }

            v3_entry = fMacBundleLoader.getSymbol<V3_ENTRYFN>(CFSTR(V3_ENTRYFNNAME));
            v3_exit = fMacBundleLoader.getSymbol<V3_EXITFN>(CFSTR(V3_EXITFNNAME));
            v3_get = fMacBundleLoader.getSymbol<V3_GETFN>(CFSTR(V3_GETFNNAME));
           #else
            water::String binaryfilename = filename;

            if (!binaryfilename.endsWithChar(CARLA_OS_SEP))
                binaryfilename += CARLA_OS_SEP_STR;

            binaryfilename += "Contents" CARLA_OS_SEP_STR V3_CONTENT_DIR CARLA_OS_SEP_STR;
            binaryfilename += water::File(filename).getFileNameWithoutExtension();
           #ifdef CARLA_OS_WIN
            binaryfilename += ".vst3";
           #else
            binaryfilename += ".so";
           #endif

            if (! water::File(binaryfilename.toRawUTF8()).existsAsFile())
            {
                pData->engine->setLastError("Failed to find a suitable VST3 bundle binary");
                return false;
            }

            if (! pData->libOpen(binaryfilename.toRawUTF8()))
            {
                pData->engine->setLastError(pData->libError(binaryfilename.toRawUTF8()));
                return false;
            }

            v3_entry = pData->libSymbol<V3_ENTRYFN>(V3_ENTRYFNNAME);
            v3_exit = pData->libSymbol<V3_EXITFN>(V3_EXITFNNAME);
            v3_get = pData->libSymbol<V3_GETFN>(V3_GETFNNAME);
           #endif
        }

        // ------------------------------------------------------------------------------------------------------------
        // ensure entry and exit points are available

        if (v3_entry == nullptr || v3_exit == nullptr || v3_get == nullptr)
        {
            pData->engine->setLastError("Not a VST3 plugin");
            return false;
        }

        // ------------------------------------------------------------------------------------------------------------
        // call entry point

       #if defined(CARLA_OS_MAC)
        v3_entry(pData->lib == nullptr ? fMacBundleLoader.getRef() : nullptr);
       #elif defined(CARLA_OS_WIN)
        v3_entry();
       #else
        v3_entry(pData->lib);
       #endif

        // ------------------------------------------------------------------------------------------------------------
        // fetch initial factory

        v3_plugin_factory** const factory = v3_get();

        if (factory == nullptr)
        {
            pData->engine->setLastError("VST3 factory failed to create a valid instance");
            return false;
        }

        // ------------------------------------------------------------------------------------------------------------
        // initialize and find requested plugin

        fV3.exitfn = v3_exit;
        fV3.factory1 = factory;

        v3_funknown** const hostContext = (v3_funknown**)&fV3ApplicationPtr;

        if (! fV3.queryFactories(hostContext))
        {
            pData->engine->setLastError("VST3 plugin failed to properly create factories");
            return false;
        }

        if (! fV3.findPlugin(fV3ClassInfo))
        {
            pData->engine->setLastError("Failed to find the requested plugin in the VST3 bundle");
            return false;
        }

        if (! fV3.initializePlugin(fV3ClassInfo.v1.class_id,
                                   hostContext,
                                   (v3_component_handler**)&fComponentHandlerPtr))
        {
            pData->engine->setLastError("VST3 plugin failed to initialize");
            return false;
        }

        // ------------------------------------------------------------------------------------------------------------
        // do some basic safety checks

        if (v3_cpp_obj(fV3.processor)->can_process_sample_size(fV3.processor, V3_SAMPLE_32) != V3_OK)
        {
            pData->engine->setLastError("VST3 plugin does not support 32bit audio, cannot continue");
            return false;
        }

        // ------------------------------------------------------------------------------------------------------------
        // get info

        if (name != nullptr && name[0] != '\0')
        {
            pData->name = pData->engine->getUniquePluginName(name);
        }
        else
        {
            if (fV3ClassInfo.v1.name[0] != '\0')
                pData->name = pData->engine->getUniquePluginName(fV3ClassInfo.v1.name);
            else if (const char* const shortname = std::strrchr(filename, CARLA_OS_SEP))
                pData->name = pData->engine->getUniquePluginName(shortname+1);
            else
                pData->name = pData->engine->getUniquePluginName("unknown");
        }

        pData->filename = carla_strdup(filename);

        // ------------------------------------------------------------------------------------------------------------
        // register client

        pData->client = pData->engine->addClient(plugin);

        if (pData->client == nullptr || ! pData->client->isOk())
        {
            pData->engine->setLastError("Failed to register plugin client");
            return false;
        }

        // ------------------------------------------------------------------------------------------------------------
        // set default options

        pData->options = 0x0;

        if (fLastKnownLatency != 0 /*|| hasMidiOutput()*/ || isPluginOptionEnabled(options, PLUGIN_OPTION_FIXED_BUFFERS))
            pData->options |= PLUGIN_OPTION_FIXED_BUFFERS;

        if (isPluginOptionEnabled(options, PLUGIN_OPTION_USE_CHUNKS))
            pData->options |= PLUGIN_OPTION_USE_CHUNKS;

        if (hasMidiInput())
        {
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CONTROL_CHANGES))
                pData->options |= PLUGIN_OPTION_SEND_CONTROL_CHANGES;
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_CHANNEL_PRESSURE))
                pData->options |= PLUGIN_OPTION_SEND_CHANNEL_PRESSURE;
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH))
                pData->options |= PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH;
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_PITCHBEND))
                pData->options |= PLUGIN_OPTION_SEND_PITCHBEND;
            if (isPluginOptionEnabled(options, PLUGIN_OPTION_SEND_ALL_SOUND_OFF))
                pData->options |= PLUGIN_OPTION_SEND_ALL_SOUND_OFF;
            if (isPluginOptionInverseEnabled(options, PLUGIN_OPTION_SKIP_SENDING_NOTES))
                pData->options |= PLUGIN_OPTION_SKIP_SENDING_NOTES;
        }

        /*
        if (numPrograms > 1 && isPluginOptionEnabled(options, PLUGIN_OPTION_MAP_PROGRAM_CHANGES))
                pData->options |= PLUGIN_OPTION_MAP_PROGRAM_CHANGES;
        */

        // ------------------------------------------------------------------------------------------------------------

        return true;
    }

protected:
    v3_result v3BeginEdit(const v3_param_id paramId) override
    {
        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            if (static_cast<v3_param_id>(pData->param.data[i].rindex) == paramId)
            {
                pData->engine->touchPluginParameter(pData->id, i, true);
                return V3_OK;
            }
        }
        return V3_INVALID_ARG;
    }

    v3_result v3PerformEdit(const v3_param_id paramId, const double value) override
    {
        CARLA_SAFE_ASSERT_RETURN(fEvents.paramInputs != nullptr, V3_INTERNAL_ERR);

        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            if (static_cast<v3_param_id>(pData->param.data[i].rindex) == paramId)
            {
                // report value to component (next process call)
                fEvents.paramInputs->setParamValue(i, static_cast<float>(value));

                const double plain = v3_cpp_obj(fV3.controller)->normalised_parameter_to_plain(fV3.controller,
                                                                                               paramId,
                                                                                               value);
                const float fixedValue = pData->param.getFixedValue(i, plain);
                CarlaPlugin::setParameterValue(i, fixedValue, false, true, true);
                return V3_OK;
            }
        }

        return V3_INVALID_ARG;
    }

    v3_result v3EndEdit(const v3_param_id paramId) override
    {
        for (uint32_t i=0; i < pData->param.count; ++i)
        {
            if (static_cast<v3_param_id>(pData->param.data[i].rindex) == paramId)
            {
                pData->engine->touchPluginParameter(pData->id, i, false);
                return V3_OK;
            }
        }
        return V3_INVALID_ARG;
    }

    v3_result v3RestartComponent(const int32_t flags) override
    {
        fRestartFlags |= flags;
        return V3_OK;
    }

   #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
    v3_result v3ResizeView(struct v3_plugin_view** const view, struct v3_view_rect* const rect) override
    {
        CARLA_SAFE_ASSERT_RETURN(fV3.view != nullptr, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_RETURN(fV3.view == view, V3_INVALID_ARG);

        const int32_t width = rect->right - rect->left;
        const int32_t height = rect->bottom - rect->top;
        CARLA_SAFE_ASSERT_INT_RETURN(width > 0, width, V3_INVALID_ARG);
        CARLA_SAFE_ASSERT_INT_RETURN(height > 0, height, V3_INVALID_ARG);

        carla_stdout("v3ResizeView %d %d", width, height);

        fUI.isResizingFromPlugin = true;
        fUI.width = width;
        fUI.height = height;

        if (fUI.isEmbed)
        {
            pData->engine->callback(true, true,
                                    ENGINE_CALLBACK_EMBED_UI_RESIZED,
                                    pData->id,
                                    width, height,
                                    0, 0.0f, nullptr);
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr, V3_NOT_INITIALIZED);
            fUI.window->setSize(static_cast<uint>(width), static_cast<uint>(height), true, false);
        }

        return V3_OK;
    }

    void handlePluginUIClosed() override
    {
        // CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_debug("CarlaPluginVST3::handlePluginUIClosed()");

        fUI.isResizingFromHost = fUI.isResizingFromInit = false;
        fUI.isResizingFromPlugin = false;

        showCustomUI(false);
        pData->engine->callback(true, true,
                                ENGINE_CALLBACK_UI_STATE_CHANGED,
                                pData->id,
                                0,
                                0, 0, 0.0f, nullptr);
    }

    void handlePluginUIResized(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fV3.view != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fUI.window != nullptr,);
        carla_stdout("CarlaPluginVST3::handlePluginUIResized(%u, %u | vs %u %u) %s %s %s",
                     width, height,
                     fUI.width, fUI.height,
                     bool2str(fUI.isResizingFromPlugin),
                     bool2str(fUI.isResizingFromInit),
                     bool2str(fUI.isResizingFromHost));

        if (fUI.isResizingFromInit)
        {
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.width == width, fUI.width, width,);
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.height == height, fUI.height, height,);
            fUI.isResizingFromInit = false;
            return;
        }

        if (fUI.isResizingFromPlugin)
        {
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.width == width, fUI.width, width,);
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.height == height, fUI.height, height,);
            fUI.isResizingFromPlugin = false;
            return;
        }

        if (fUI.isResizingFromHost)
        {
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.width == width, fUI.width, width,);
            CARLA_SAFE_ASSERT_UINT2_RETURN(fUI.height == height, fUI.height, height,);
            fUI.isResizingFromHost = false;
            return;
        }

        if (fUI.width != width || fUI.height != height)
        {
            v3_view_rect rect = { 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) };
            if (v3_cpp_obj(fV3.view)->check_size_constraint(fV3.view, &rect) == V3_OK)
            {
                const uint width2 = rect.right - rect.left;
                const uint height2 = rect.bottom - rect.top;

                if (width2 != width || height2 != height)
                {
                    fUI.isResizingFromHost = true;
                    fUI.width = width2;
                    fUI.height = height2;
                    fUI.window->setSize(width2, height2, true, false);
                }
                else
                {
                    v3_cpp_obj(fV3.view)->on_size(fV3.view, &rect);
                }
            }
        }
    }
   #endif // V3_VIEW_PLATFORM_TYPE_NATIVE

private:
   #ifdef CARLA_OS_MAC
    BundleLoader fMacBundleLoader;
   #endif
    const bool kEngineHasIdleOnMainThread;
    bool fFirstActive; // first process() call after activate()
    float** fAudioAndCvOutBuffers;
    uint32_t fLastKnownLatency;
    int32_t fRestartFlags;
    void* fLastChunk;
    EngineTimeInfo fLastTimeInfo;
    v3_process_context fV3TimeContext;

    carla_v3_host_application fV3Application;
    carla_v3_host_application* const fV3ApplicationPtr;

    carla_v3_component_handler fComponentHandler;
    carla_v3_component_handler* const fComponentHandlerPtr;

   #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
    carla_v3_plugin_frame fPluginFrame;
    carla_v3_plugin_frame* const fPluginFramePtr;
   #endif

    // v3_class_info_2 is ABI compatible with v3_class_info
    union ClassInfo {
        v3_class_info v1;
        v3_class_info_2 v2;
    } fV3ClassInfo;

    struct PluginPointers {
        V3_EXITFN exitfn;
        v3_plugin_factory** factory1;
        v3_plugin_factory_2** factory2;
        v3_plugin_factory_3** factory3;
        v3_component** component;
        v3_edit_controller** controller;
        v3_audio_processor** processor;
        v3_connection_point** connComponent;
        v3_connection_point** connController;
        v3_midi_mapping** midiMapping;
       #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
        v3_plugin_view** view;
       #endif
        bool shouldTerminateComponent;
        bool shouldTerminateController;

        PluginPointers()
            : exitfn(nullptr),
              factory1(nullptr),
              factory2(nullptr),
              factory3(nullptr),
              component(nullptr),
              controller(nullptr),
              processor(nullptr),
              connComponent(nullptr),
              connController(nullptr),
              midiMapping(nullptr),
             #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
              view(nullptr),
             #endif
              shouldTerminateComponent(false),
              shouldTerminateController(false) {}

        ~PluginPointers()
        {
            // must have been cleaned up by now
            CARLA_SAFE_ASSERT(exitfn == nullptr);
        }

        // must have exitfn and factory1 set
        bool queryFactories(v3_funknown** const hostContext)
        {
            // query 2nd factory
            if (v3_cpp_obj_query_interface(factory1, v3_plugin_factory_2_iid, &factory2) == V3_OK)
            {
                CARLA_SAFE_ASSERT_RETURN(factory2 != nullptr, exit());
            }
            else
            {
                CARLA_SAFE_ASSERT(factory2 == nullptr);
                factory2 = nullptr;
            }

            // query 3rd factory
            if (factory2 != nullptr && v3_cpp_obj_query_interface(factory2, v3_plugin_factory_3_iid, &factory3) == V3_OK)
            {
                CARLA_SAFE_ASSERT_RETURN(factory3 != nullptr, exit());
            }
            else
            {
                CARLA_SAFE_ASSERT(factory3 == nullptr);
                factory3 = nullptr;
            }

            // set host context (application) if 3rd factory provided
            if (factory3 != nullptr)
                v3_cpp_obj(factory3)->set_host_context(factory3, hostContext);

            return true;
        }

        // must have all possible factories and exitfn set
        bool findPlugin(ClassInfo& classInfo)
        {
            // get factory info
            v3_factory_info factoryInfo = {};
            CARLA_SAFE_ASSERT_RETURN(v3_cpp_obj(factory1)->get_factory_info(factory1, &factoryInfo) == V3_OK, exit());

            // get num classes
            const int32_t numClasses = v3_cpp_obj(factory1)->num_classes(factory1);
            CARLA_SAFE_ASSERT_RETURN(numClasses > 0, exit());

            // go through all relevant classes
            for (int32_t i=0; i<numClasses; ++i)
            {
                carla_zeroStruct(classInfo);

                if (factory2 != nullptr)
                    v3_cpp_obj(factory2)->get_class_info_2(factory2, i, &classInfo.v2);
                else
                    v3_cpp_obj(factory1)->get_class_info(factory1, i, &classInfo.v1);

                // safety check
                CARLA_SAFE_ASSERT_CONTINUE(classInfo.v1.cardinality == 0x7FFFFFFF);

                // only check for audio plugins
                if (std::strcmp(classInfo.v1.category, "Audio Module Class") != 0)
                    continue;

                // FIXME multi-plugin bundle
                break;
            }

            return true;
        }

        bool initializePlugin(const v3_tuid uid,
                              v3_funknown** const hostContext,
                              v3_component_handler** const handler)
        {
            // create instance
            void* instance = nullptr;
            CARLA_SAFE_ASSERT_RETURN(v3_cpp_obj(factory1)->create_instance(factory1, uid, v3_component_iid,
                                                                           &instance) == V3_OK,
                                     exit());
            CARLA_SAFE_ASSERT_RETURN(instance != nullptr, exit());

            component = static_cast<v3_component**>(instance);

            // initialize instance
            CARLA_SAFE_ASSERT_RETURN(v3_cpp_obj_initialize(component, hostContext) == V3_OK, exit());
            shouldTerminateComponent = true;

            // create edit controller
            if (v3_cpp_obj_query_interface(component, v3_edit_controller_iid, &controller) != V3_OK)
                controller = nullptr;

            // if we cannot cast from component, try to create edit controller from factory
            if (controller == nullptr)
            {
                v3_tuid cuid = {};
                if (v3_cpp_obj(component)->get_controller_class_id(component, cuid) == V3_OK)
                {
                    instance = nullptr;
                    if (v3_cpp_obj(factory1)->create_instance(factory1, cuid,
                                                              v3_edit_controller_iid, &instance) == V3_OK)
                        controller = static_cast<v3_edit_controller**>(instance);
                }

                CARLA_SAFE_ASSERT_RETURN(controller != nullptr, exit());

                // component is separate from controller, needs its dedicated initialize and terminate
                CARLA_SAFE_ASSERT_RETURN(v3_cpp_obj_initialize(controller, hostContext) == V3_OK, exit());
                shouldTerminateController = true;
            }

            v3_cpp_obj(controller)->set_component_handler(controller, handler);

            // create processor
            CARLA_SAFE_ASSERT_RETURN(v3_cpp_obj_query_interface(component, v3_audio_processor_iid,
                                                                &processor) == V3_OK, exit());
            CARLA_SAFE_ASSERT_RETURN(processor != nullptr, exit());

            // connect component to controller
            if (v3_cpp_obj_query_interface(component, v3_connection_point_iid, &connComponent) != V3_OK)
                connComponent = nullptr;

            if (v3_cpp_obj_query_interface(controller, v3_connection_point_iid, &connController) != V3_OK)
                connController = nullptr;

            if (connComponent != nullptr && connController != nullptr)
            {
                v3_cpp_obj(connComponent)->connect(connComponent, connController);
                v3_cpp_obj(connController)->connect(connController, connComponent);
            }

            // get midi mapping interface
            if (v3_cpp_obj_query_interface(component, v3_midi_mapping_iid, &midiMapping) != V3_OK)
            {
                midiMapping = nullptr;
                if (v3_cpp_obj_query_interface(controller, v3_midi_mapping_iid, &midiMapping) != V3_OK)
                    midiMapping = nullptr;
            }

           #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
            // create view
            view = v3_cpp_obj(controller)->create_view(controller, "editor");
           #endif

            return true;
        }

        bool exit()
        {
           #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
            // must be deleted by now
            CARLA_SAFE_ASSERT(view == nullptr);
           #endif

            if (midiMapping != nullptr)
            {
                v3_cpp_obj_unref(midiMapping);
                midiMapping = nullptr;
            }

            if (connComponent != nullptr && connController != nullptr)
            {
                v3_cpp_obj(connComponent)->disconnect(connComponent, connController);
                v3_cpp_obj(connController)->disconnect(connController, connComponent);
            }

            if (connComponent != nullptr)
            {
                v3_cpp_obj_unref(connComponent);
                connComponent = nullptr;
            }

            if (connController != nullptr)
            {
                v3_cpp_obj_unref(connController);
                connController = nullptr;
            }

            if (processor != nullptr)
            {
                v3_cpp_obj_unref(processor);
                processor = nullptr;
            }

            if (controller != nullptr)
            {
                if (shouldTerminateController)
                {
                    v3_cpp_obj_terminate(controller);
                    shouldTerminateController = false;
                }

                v3_cpp_obj_unref(controller);
                controller = nullptr;
            }

            if (component != nullptr)
            {
                if (shouldTerminateComponent)
                {
                    v3_cpp_obj_terminate(component);
                    shouldTerminateComponent = false;
                }

                v3_cpp_obj_unref(component);
                component = nullptr;
            }

            if (factory3 != nullptr)
            {
                v3_cpp_obj_unref(factory3);
                factory3 = nullptr;
            }

            if (factory2 != nullptr)
            {
                v3_cpp_obj_unref(factory2);
                factory2 = nullptr;
            }

            if (factory1 != nullptr)
            {
                v3_cpp_obj_unref(factory1);
                factory1 = nullptr;
            }

            if (exitfn != nullptr)
            {
                exitfn();
                exitfn = nullptr;
            }

            // return false so it can be used as error/fail condition
            return false;
        }

        CARLA_DECLARE_NON_COPYABLE(PluginPointers)
    } fV3;

    struct Buses {
        int32_t numInputs;
        int32_t numOutputs;
        v3_audio_bus_buffers* inputs;
        v3_audio_bus_buffers* outputs;
        v3_bus_mini_info* inputInfo;
        v3_bus_mini_info* outputInfo;

        Buses()
            : numInputs(0),
              numOutputs(0),
              inputs(nullptr),
              outputs(nullptr),
              inputInfo(nullptr),
              outputInfo(nullptr) {}

        ~Buses()
        {
            delete[] inputs;
            delete[] outputs;
            delete[] inputInfo;
            delete[] outputInfo;
        }

        void createNew(const int32_t numAudioInputBuses, const int32_t numAudioOutputBuses)
        {
            delete[] inputs;
            delete[] outputs;
            delete[] inputInfo;
            delete[] outputInfo;

            numInputs = numAudioInputBuses;
            numOutputs = numAudioOutputBuses;

            if (numAudioInputBuses > 0)
            {
                inputs = new v3_audio_bus_buffers[numAudioInputBuses];
                inputInfo = new v3_bus_mini_info[numAudioInputBuses];
            }
            else
            {
                inputs =  nullptr;
                inputInfo = nullptr;
            }

            if (numAudioOutputBuses > 0)
            {
                outputs = new v3_audio_bus_buffers[numAudioOutputBuses];
                outputInfo = new v3_bus_mini_info[numAudioOutputBuses];
            }
            else
            {
                outputs = nullptr;
                outputInfo = nullptr;
            }
        }

        CARLA_DECLARE_NON_COPYABLE(Buses)
    } fBuses;

    struct Events {
        carla_v3_input_param_changes* paramInputs;
        carla_v3_output_param_changes* paramOutputs;
        carla_v3_input_event_list* eventInputs;
        carla_v3_output_event_list* eventOutputs;

        Events() noexcept
          : paramInputs(nullptr),
            paramOutputs(nullptr),
            eventInputs(nullptr),
            eventOutputs(nullptr) {}

        ~Events()
        {
            delete paramInputs;
            delete paramOutputs;
            delete eventInputs;
            delete eventOutputs;
        }

        void init()
        {
            if (paramInputs != nullptr)
                paramInputs->init();

            if (eventInputs != nullptr)
                eventInputs->numEvents = 0;

            if (eventOutputs != nullptr)
                eventOutputs->numEvents = 0;
        }

        void prepare()
        {
            if (paramInputs != nullptr)
                paramInputs->prepare();

            if (paramOutputs != nullptr)
                paramOutputs->prepare();
        }

        CARLA_DECLARE_NON_COPYABLE(Events)
    } fEvents;

    struct MidiControllerAssignments {
        v3_param_id* mappings;
        bool* used;

        MidiControllerAssignments() noexcept
          : mappings(nullptr),
            used(nullptr) {}

        ~MidiControllerAssignments() noexcept
        {
            clear();
        }

        void init(v3_midi_mapping** const midiMapping, const uint32_t numPorts)
        {
            clear();

            if (numPorts == 0)
                return;

            const uint32_t dataPoints = numPorts * MAX_MIDI_CHANNELS * 130;

            mappings = new v3_param_id[dataPoints];
            used = new bool[dataPoints];

            carla_zeroStructs(mappings, dataPoints);
            carla_zeroStructs(used, dataPoints);

            v3_param_id paramId;
            for (uint32_t p=0; p<numPorts; ++p)
            {
                for (uint8_t ch=0; ch<MAX_MIDI_CHANNELS; ++ch)
                {
                    for (uint16_t cc=0; cc<130; ++cc)
                    {
                        if (v3_cpp_obj(midiMapping)->get_midi_controller_assignment(midiMapping, p,
                                                                                    ch, cc, &paramId) == V3_OK)
                        {
                            const uint32_t index = p * (130 + MAX_MIDI_CHANNELS) + cc * MAX_MIDI_CHANNELS + ch;
                            mappings[index] = paramId;
                            used[index] = true;
                        }
                    }
                }
            }
        }

        void clear() noexcept
        {
            delete[] mappings;
            delete[] used;
            mappings = nullptr;
            used = nullptr;
        }

        bool get(const uint8_t port, const uint8_t channel, const uint8_t cc, v3_param_id& paramId) noexcept
        {
            CARLA_SAFE_ASSERT_UINT_RETURN(channel < MAX_MIDI_CHANNELS, channel, false);
            CARLA_SAFE_ASSERT_UINT_RETURN(cc < 130, cc, false);

            if (used == nullptr)
                return false;

            const uint32_t index = port * (130 + MAX_MIDI_CHANNELS) + cc * MAX_MIDI_CHANNELS + channel;
            if (used[index])
            {
                paramId = mappings[index];
                return true;
            }

            return false;
        }
    } fMidiControllerAssignments;

   #ifdef V3_VIEW_PLATFORM_TYPE_NATIVE
    struct UI {
        bool isAttached;
        bool isEmbed;
        bool isResizingFromHost;
        bool isResizingFromInit;
        bool isResizingFromPlugin;
        bool isVisible;
        uint32_t width, height;
        CarlaPluginUI* window;

        UI() noexcept
            : isAttached(false),
              isEmbed(false),
              isResizingFromHost(false),
              isResizingFromInit(false),
              isResizingFromPlugin(false),
              isVisible(false),
              width(0),
              height(0),
              window(nullptr) {}

        ~UI()
        {
            CARLA_ASSERT(isEmbed || ! isVisible);

            if (window != nullptr)
            {
                delete window;
                window = nullptr;
            }
        }

        CARLA_DECLARE_NON_COPYABLE(UI)
    } fUI;
   #endif

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPluginVST3)
};

// --------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newVST3(const Initializer& init)
{
    carla_debug("CarlaPlugin::newVST3({%p, \"%s\", \"%s\", \"%s\"})",
                init.engine, init.filename, init.name, init.label);

    std::shared_ptr<CarlaPluginVST3> plugin(new CarlaPluginVST3(init.engine, init.id));

    if (! plugin->init(plugin, init.filename, init.name, init.label, init.options))
        return nullptr;

    return plugin;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
