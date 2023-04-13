#include "CarlaFrontend.h"
#include <QApplication>
#include <cstdio>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    if (const PluginListDialogResults* ret = carla_frontend_createAndExecPluginListDialog(nullptr))
    {
        printf("got ret label %s\n", ret->label);
        return 0;
    }

    return 1;
}
