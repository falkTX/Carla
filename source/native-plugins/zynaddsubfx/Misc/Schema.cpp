#include <ostream>
#include <rtosc/ports.h>
using namespace rtosc;
/*
 * root :
 *   - 'parameters' : [parameter...]
 *   - 'actions'    : [action...]
 * parameter :
 *   - 'path'      : path-id
 *   - 'name'      : string
 *   - 'shortname' : string [OPTIONAL]
 *   - 'tooltip'   : string [OPTIONAL]
 *   - 'type'      : type
 *   - 'domain'    : range [OPTIONAL]
 * type : {'int', 'float', 'boolean'}
 * action :
 *   - 'path' : path-id
 *   - 'args' : [arg...]
 * arg :
 *   - 'type'   : type
 *   - 'domain' : range [OPTIONAL]
 */

void walk_ports2(const rtosc::Ports *base,
                 char         *name_buffer,
                 size_t        buffer_size,
                 void         *data,
                 rtosc::port_walker_t walker);


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

/*
 * parameter :
 *   - 'path'      : path-id
 *   - 'name'      : string
 *   - 'shortname' : string [OPTIONAL]
 *   - 'tooltip'   : string [OPTIONAL]
 *   - 'type'      : type
 *   - 'domain'    : range [OPTIONAL]
 */
static bool first = true;
void dump_param_cb(const rtosc::Port *p, const char *name, void *v)
{
    std::ostream &o  = *(std::ostream*)v;
    auto meta        = p->meta();
    const char *args = strchr(p->name, ':');
    auto mparameter  = meta.find("parameter");
    auto mdoc        = meta.find("documentation");
    auto msname      = meta.find("shortname");
    string doc;

    //Escape Characters
    if(mdoc != p->meta().end()) {
        while(*mdoc.value) {
            if(*mdoc.value == '\n')
                doc += "\\n";
            else if(*mdoc.value == '\"')
                doc += "\\\"";
            else
                doc += *mdoc.value;
            mdoc.value++;
        }
    }
    if(meta.find("internal") != meta.end())
        return;

    char type = 0;
    if(mparameter != p->meta().end()) {
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
    } else {
        //fprintf(stderr, "Skipping \"%s\"\n", name);
        //if(args) {
        //    fprintf(stderr, "    type = %s\n", args);
        //}
        return;
    }

    const char *min = meta["min"];
    const char *max = meta["max"];

    if(!first)
        o << ",\n";
    else
        first = false;

    o << "    {\n";
    o << "        \"path\"     : \"" << name << "\",\n";
    if(msname != meta.end())
        o << "        \"shortname\": \"" << msname.value << "\",\n";
    o << "        \"name\"     : \"" << p->name << "\",\n";
    o << "        \"tooltip\"  : \"" << doc  << "\",\n";
    o << "        \"type\"     : \"" << type  << "\"";
    if(min && max)
    o << ",\n        \"range\"    : [" << min << "," << max << "]\n";
    else
        o << "\n";
    o << "    }";
}

void dump_json(std::ostream &o, const rtosc::Ports &p)
{
    first = true;
    o << "{\n";
    o << "    \"parameter\" : [\n";
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    walk_ports2(&p, buffer, 1024, &o, dump_param_cb);
    o << "\n    ],\n";
    o << "    \"actions\" : [\n";
    //walk_ports2(formatter.p, buffer, 1024, &o, dump_action_cb);
    o << "    ]\n";
    o << "}";
}


