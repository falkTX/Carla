
#ifndef __APP_DETAILS_H__
#define __APP_DETAILS_H__

#include "../../../../source/includes/CarlaDefines.h"

#define SFX_APP_VERSION CARLA_VERSION_STRING
#define SFX_APP_BANNER  "Carla self-contained executable " SFX_APP_VERSION ", based on UnZipSFX."

#ifndef SFX_APP_MININAME
# define SFX_APP_MININAME "Carla"
#endif

#ifdef WIN32
# define SFX_AUTORUN_CMD  "\\" SFX_APP_MININAME ".exe"
#else
# define SFX_AUTORUN_CMD  "./" SFX_APP_MININAME
#endif

void  sfx_app_set_args(int argc, char** argv);
int   sfx_app_autorun_now();
char* sfx_get_tmp_path(int withAppName);

#endif // __APP_DETAILS_H__
