#include <rtosc/ports.h>
#include <rtosc/rtosc.h>
#include <cassert>
namespace rtosc {
struct AutomationMapping
{
    //0 - linear
    //1 - log
    int   control_scale;

    //0 - simple linear (only first four control points are used)
    //1 - piecewise linear
    int   control_type;

    float *control_points;
    int    npoints;
    int    upoints;

    float gain;
    float offset;
};

struct Automation
{
    //If automation is allocated to anything or not
    bool used;

    //If automation is used or not
    bool active;

    //relative or absolute
    bool relative;

    //Cached infomation
    float       param_base_value;
    char        param_path[128];
    char        param_type;
    float       param_min;
    float       param_max;
    float       param_step;       //resolution of parameter. Useful for:
                                  //- integer valued controls
    AutomationMapping map;
};

#define RTOSC_AUTOMATION_SLOT_NAME_LEN
struct AutomationSlot
{
    //If automation slot has active automations or not
    bool  active;

    //If automation slot has active automations or not
    bool  used;

    //Non-negative if a new MIDI binding is being learned
    int  learning;

    //-1 or a valid MIDI CC + MIDI Channel
    int   midi_cc;

    //Current state supplied by MIDI value or host
    float current_state;

    //Current name
    char name[128];

    //Collection of automations
    Automation *automations;
};

class AutomationMgr
{
    public:
        AutomationMgr(int slots, int per_slot, int control_points);
        ~AutomationMgr(void);

        /**
         * Create an Automation binding
         *
         * - Assumes that each binding takes a new automation slot unless learning
         *   a macro
         * - Can trigger a MIDI learn (recommended for standalone and not
         *   recommended for plugin modes)
         */
        void createBinding(int slot, const char *path, bool start_midi_learn);

        void updateMapping(int slot, int sub);



        //Get/Set Automation Slot values 0..1
        void setSlot(int slot_id, float value);
        void setSlotSub(int slot_id, int sub, float value);
        float getSlot(int slot_id);

        void clearSlot(int slot_id);
        void clearSlotSub(int slot_id, int sub);


        void setSlotSubGain(int slot_id, int sub, float f);
        float getSlotSubGain(int slot_id, int sub);
        void setSlotSubOffset(int slot_id, int sub, float f);
        float getSlotSubOffset(int slot_id, int sub);



        void setName(int slot_id, const char *msg);
        const char * getName(int slot_id);

        bool handleMidi(int channel, int cc, int val);

        void set_ports(const struct Ports &p);

        void set_instance(void *v);

        void simpleSlope(int slot, int au, float slope, float offset);

        int free_slot(void) const;

        AutomationSlot *slots;
        int nslots;
        int per_slot;
        int active_slot;
        int learn_queue_len;
        struct AutomationMgrImpl *impl;
        const rtosc::Ports *p;
        void *instance;

        std::function<void(const char *)> backend;

        int damaged;
};
};
