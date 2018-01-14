
#ifndef __APP_DETAILS_H__
#define __APP_DETAILS_H__

#define REAL_BUILD
#include "../../../../source/includes/CarlaDefines.h"

#define SFX_APP_MININAME_TITLE "CarlaControl"
#define SFX_APP_MININAME_LCASE "carlacontrol"

#define SFX_APP_BANNER  SFX_APP_MININAME_TITLE " self-contained executable " CARLA_VERSION_STRING ", based on UnZipSFX."

#ifdef WIN32
# define SFX_AUTORUN_CMD  "\\" SFX_APP_MININAME_TITLE ".exe"
#else
# define SFX_AUTORUN_CMD  "/" SFX_APP_MININAME_LCASE
#endif

void  sfx_app_set_args(int argc, char** argv);
int   sfx_app_autorun_now();
char* sfx_get_tmp_path(int withAppName);

#endif // __APP_DETAILS_H__
