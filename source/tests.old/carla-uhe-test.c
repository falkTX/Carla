
#include "CarlaHost.h"

#include <dlfcn.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

// --------------------------------------------------------------------------------------------------------

static volatile bool term = false;

static void signalHandler(int sig)
{
    switch (sig)
    {
    case SIGINT:
    case SIGTERM:
        term = true;
        break;
    }
}

// --------------------------------------------------------------------------------------------------------

typedef bool (*carla_engine_init_func)(const char* driverName, const char* clientName);
typedef void (*carla_engine_idle_func)(void);
typedef bool (*carla_is_engine_running_func)(void);
typedef bool (*carla_engine_close_func)(void);
typedef bool (*carla_add_plugin_func)(BinaryType btype, PluginType ptype,
                                      const char* filename, const char* name, const char* label, int64_t uniqueId,
                                      const void* extraPtr, uint options);
typedef const char* (*carla_get_last_error_func)(void);

int main(void)
{
    void* lib = dlopen("/home/falktx/FOSS/GIT-mine/falkTX/Carla/bin/libcarla_standalone2.so", RTLD_NOW|RTLD_GLOBAL);

    if (!lib)
    {
        printf("Failed to load carla lib\n");
        return 1;
    }

    const carla_engine_init_func       carla_engine_init       = dlsym(lib, "carla_engine_init");
    const carla_engine_idle_func       carla_engine_idle       = dlsym(lib, "carla_engine_idle");
    const carla_is_engine_running_func carla_is_engine_running = dlsym(lib, "carla_is_engine_running");
    const carla_engine_close_func      carla_engine_close      = dlsym(lib, "carla_engine_close");
    const carla_add_plugin_func        carla_add_plugin        = dlsym(lib, "carla_add_plugin");
    const carla_get_last_error_func    carla_get_last_error    = dlsym(lib, "carla_get_last_error");

    if (! carla_engine_init("JACK", "Carla-uhe-test"))
    {
        printf("Engine failed to initialize, possible reasons:\n%s\n", carla_get_last_error());
        return 1;
    }

    if (! carla_add_plugin(BINARY_NATIVE, PLUGIN_VST2, "/home/falktx/.vst/u-he/ACE.64.so", "", "", 0, NULL, 0))
    {
        printf("Failed to load plugin, possible reasons:\n%s\n", carla_get_last_error());
        carla_engine_close();
        return 1;
    }

    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

    while (carla_is_engine_running() && ! term)
    {
        carla_engine_idle();
        sleep(1);
    }

    carla_engine_close();

    return 0;
}

// --------------------------------------------------------------------------------------------------------
