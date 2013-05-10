
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
# include <windows.h>
#endif

#include "appDetails.h"

#define CMD_BUF_LEN 1024

static int    sfx_app_argc = 0;
static char** sfx_app_argv = NULL;
static char   sfx_tmp_path[512] = { 0 };

void sfx_app_set_args(int argc, char** argv)
{
    sfx_app_argc = argc;
    sfx_app_argv = argv;
}

int sfx_app_autorun_now()
{
    int i, cmdBufLen = 0;
    char cmdBuf[CMD_BUF_LEN];

#ifdef WIN32
    strcpy(cmdBuf, sfx_get_tmp_path(1));
    strcat(cmdBuf, SFX_AUTORUN_CMD);
#else
    strcpy(cmdBuf, "cd ");
    strcat(cmdBuf, sfx_get_tmp_path(1));
    strcat(cmdBuf, "; ");
    strcat(cmdBuf, SFX_AUTORUN_CMD);
#endif

    cmdBufLen = strlen(cmdBuf);

    for (i=0; i < sfx_app_argc; i++)
    {
        if (! sfx_app_argv[i])
            continue;

        cmdBufLen += strlen(sfx_app_argv[i]) + 1;
        if (cmdBufLen >= CMD_BUF_LEN-1)
            break;

        strcat(cmdBuf, " ");
        strcat(cmdBuf, sfx_app_argv[i]);
    }

    return system(cmdBuf);
}

char* sfx_get_tmp_path(int withAppName)
{
#ifdef WIN32
    {
        GetTempPathA(512 - strlen(SFX_APP_MININAME), sfx_tmp_path);

        if (withAppName == 1)
            strcat(sfx_tmp_path, SFX_APP_MININAME);
    }
#else
    {
        char* tmp = getenv("TMP");

        if (tmp)
            strcpy(sfx_tmp_path, tmp);
        else
            strcpy(sfx_tmp_path, "/tmp");

        if (withAppName == 1)
            strcat(sfx_tmp_path, "/" SFX_APP_MININAME);
    }
#endif

    return sfx_tmp_path;
}
