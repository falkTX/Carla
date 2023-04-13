#include "CarlaFrontend.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    carla_frontend_createAndExecPluginListDialog(nullptr);
}
