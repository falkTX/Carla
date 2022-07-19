/*
 * Common Carla code
 * Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "carla_shared.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <QtGui/QFontMetrics>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------

#ifdef CARLA_OS_UNIX
# include <signal.h>
#endif

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "carla_host.hpp"

#include "CarlaUtils.h"
#include "CarlaMathUtils.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Global Carla object

CarlaObject::CarlaObject() noexcept
    : host(nullptr),
      gui(nullptr),
      nogui(false),
      term(false) {}

CarlaObject gCarla;

//---------------------------------------------------------------------------------------------------------------------
// Get Icon from user theme, using our own as backup (Oxygen)

QIcon getIcon(const QString icon, const int size)
{
    return QIcon::fromTheme(icon, QIcon(QString(":/%1x%1/%2.png").arg(size).arg(icon)));
}

//---------------------------------------------------------------------------------------------------------------------
// Handle some basic command-line arguments shared between all carla variants

QString handleInitialCommandLineArguments(const int argc, char* argv[])
{
    static const QStringList listArgsNoGUI   = { "-n", "--n", "-no-gui", "--no-gui", "-nogui", "--nogui" };
    static const QStringList listArgsHelp    = { "-h", "--h", "-help", "--help" };
    static const QStringList listArgsVersion = { "-v", "--v", "-version", "--version" };

    QString initName(argv[0]); // = os.path.basename(file) if (file is not None and os.path.dirname(file) in PATH) else sys.argv[0]
    // libPrefix = None

    for (int i=1; i<argc; ++i)
    {
        const QString arg(argv[i]);

        if (arg.startsWith("--with-appname="))
        {
            // initName = os.path.basename(arg.replace("--with-appname=", ""));
        }
        else if (arg.startsWith("--with-libprefix=") || arg == "--gdb")
        {
            pass();
        }
        else if (listArgsNoGUI.contains(arg))
        {
            gCarla.nogui = true;
        }
        else if (listArgsHelp.contains(arg))
        {
            carla_stdout("Usage: %s [OPTION]... [FILE|URL]", initName);
            carla_stdout("");
            carla_stdout(" where FILE can be a Carla project or preset file to be loaded, or URL if using Carla-Control");
            carla_stdout("");
            carla_stdout(" and OPTION can be one or more of the following:");
            carla_stdout("");
            carla_stdout("    --gdb    \t Run Carla inside gdb.");
            carla_stdout(" -n,--no-gui \t Run Carla headless, don't show UI.");
            carla_stdout("");
            carla_stdout(" -h,--help   \t Print this help text and exit.");
            carla_stdout(" -v,--version\t Print version information and exit.");
            carla_stdout("");

            std::exit(0);
        }
        else if (listArgsVersion.contains(arg))
        {
            /*
            QString pathBinaries, pathResources = getPaths();
            */

            carla_stdout("Using Carla version %s", CARLA_VERSION_STRING);
            /*
            carla_stdout("  Qt version:     %s", QT_VERSION_STR);
            carla_stdout("  Binary dir:     %s", pathBinaries.toUtf8());
            carla_stdout("  Resources dir:  %s", pathResources.toUtf8());
            */

            std::exit(0);
        }
    }

    return initName;
}

//---------------------------------------------------------------------------------------------------------------------
// Get initial project file (as passed in the command-line parameters)

QString getInitialProjectFile(bool)
{
    // TODO
    return "";
}

//---------------------------------------------------------------------------------------------------------------------
// Get paths (binaries, resources)

bool getPaths(QString& pathBinaries, QString& pathResources)
{
    const QString libFolder(carla_get_library_folder());

    QDir dir(libFolder);

    // FIXME need to completely rework this in C++ mode, so check if all cases are valid

    if (libFolder.endsWith("bin"))
    {
        CARLA_SAFE_ASSERT_RETURN(dir.cd("resources"), false);
        pathBinaries  = libFolder;
        pathResources = dir.absolutePath();
        return true;
    }
    else if (libFolder.endsWith("carla"))
    {
        for (int i=2; --i>=0;)
        {
            CARLA_SAFE_ASSERT_INT_RETURN(dir.cdUp(), i, false);
            CARLA_SAFE_ASSERT_INT_RETURN(dir.cd("share"), i, false);

            if (dir.exists())
            {
                CARLA_SAFE_ASSERT_INT_RETURN(dir.cd("carla"), i, false);
                CARLA_SAFE_ASSERT_INT_RETURN(dir.cd("resources"), i, false);
                pathBinaries  = libFolder;
                pathResources = dir.absolutePath();
                return true;
            }
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
// Signal handler

static void signalHandler(const int sig)
{
    switch (sig)
    {
    case SIGINT:
    case SIGTERM:
        gCarla.term = true;
        if (gCarla.host != nullptr)
            emit gCarla.host->SignalTerminate();
        break;
    case SIGUSR1:
        if (gCarla.host != nullptr)
            emit gCarla.host->SignalSave();
        break;
    }
}

#ifdef CARLA_OS_WIN
static BOOL WINAPI winSignalHandler(DWORD dwCtrlType) noexcept
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        signalHandler(SIGINT);
        return TRUE;
    }
    return FALSE;
}
#endif

void setUpSignals()
{
#if defined(CARLA_OS_UNIX)
    struct sigaction sig;
    carla_zeroStruct(sig);
    sig.sa_handler = signalHandler;
    sig.sa_flags   = SA_RESTART;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGTERM, &sig, nullptr);
    sigaction(SIGINT, &sig, nullptr);
    sigaction(SIGUSR1, &sig, nullptr);
#elif defined(CARLA_OS_WIN)
    SetConsoleCtrlHandler(winSignalHandler, TRUE);
#endif
}

//---------------------------------------------------------------------------------------------------------------------
// QLineEdit and QPushButton combo

QString getAndSetPath(QWidget* const parent, QLineEdit* const lineEdit)
{
    const QCarlaString newPath = QFileDialog::getExistingDirectory(parent, parent->tr("Set Path"), lineEdit->text(), QFileDialog::ShowDirsOnly);
    if (newPath.isNotEmpty())
        lineEdit->setText(newPath);
    return newPath;
}

//---------------------------------------------------------------------------------------------------------------------
// fill up a qlists from a C arrays

void fillQStringListFromStringArray(QStringList& list, const char* const* const stringArray)
{
    int count = 0;

    // count number of strings first
    for (; stringArray[count] != nullptr; ++count) {}

    // allocate list
    list.reserve(count);

    // fill in strings
    for (count = 0; stringArray[count] != nullptr; ++count)
        list.append(stringArray[count]);
}

void fillQDoubleListFromDoubleArray(QList<double>& list, const double* const doubleArray)
{
    int count = 0;

    // count number of strings first
    for (; carla_isNotZero(doubleArray[count]); ++count) {}

    // allocate list
    list.reserve(count);

    // fill in strings
    for (count = 0; carla_isNotZero(doubleArray[count]); ++count)
        list.append(doubleArray[count]);
}

void fillQUIntListFromUIntArray(QList<uint>& list, const uint* const uintArray)
{
    int count = 0;

    // count number of strings first
    for (; uintArray[count] != 0; ++count) {}

    // allocate list
    list.reserve(count);

    // fill in strings
    for (count = 0; uintArray[count] != 0; ++count)
        list.append(uintArray[count]);
}

//---------------------------------------------------------------------------------------------------------------------
// Backwards-compatible horizontalAdvance/width call, depending on Qt version

int fontMetricsHorizontalAdvance(const QFontMetrics& fm, const QString& s)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    return fm.horizontalAdvance(s);
#else
    return fm.width(s);
#endif
}

//---------------------------------------------------------------------------------------------------------------------
// Check if a string array contains a string

bool stringArrayContainsString(const char* const* const stringArray, const char* const string) noexcept
{
    for (uint i=0; stringArray[i] != nullptr; ++i)
    {
        if (std::strcmp(stringArray[i], string) == 0)
            return true;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
// Get index of a QList<double> value

int getIndexOfQDoubleListValue(const QList<double>& list, const double value)
{
    if (list.size() > 0)
    {
        for (QList<double>::const_iterator n = list.cbegin(), e = list.cend(); n != e; ++n)
            if (carla_isEqual(*n, value))
                return int(n - list.cbegin());
    }

    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
// Check if two QList<double> instances match

bool isQDoubleListEqual(const QList<double>& list1, const QList<double>& list2)
{
    if (list1.size() != list2.size())
        return false;
    if (list1.isEmpty())
        return true;

    for (QList<double>::const_iterator l1n = list1.cbegin(), l2n = list2.cbegin(), l1e = list1.cend(); l1n != l1e; ++l1n, ++l2n)
        if (carla_isNotEqual(*l1n, *l2n))
            return false;

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
// Custom QMessageBox which resizes itself to fit text

void QMessageBoxWithBetterWidth::showEvent(QShowEvent* const event)
{
    const QFontMetrics metrics(fontMetrics());
    const QStringList lines(text().trimmed().split("\n") + informativeText().trimmed().split("\n"));

    if (lines.size() > 0)
    {
        int width = 0;

        for (const QString& line : lines)
            width = std::max(fontMetricsHorizontalAdvance(metrics, line), width);

        if (QGridLayout* const layout_ = dynamic_cast<QGridLayout*>(layout()))
            layout_->setColumnMinimumWidth(2, width + 12);
    }

    QMessageBox::showEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
// Custom MessageBox

int CustomMessageBox(QWidget* const parent,
                     const QMessageBox::Icon icon,
                     const QString title,
                     const QString text,
                     const QString extraText,
                     const QMessageBox::StandardButtons buttons,
                     const QMessageBox::StandardButton defButton)
{
    QMessageBoxWithBetterWidth msgBox(parent);
    msgBox.setIcon(icon);
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setInformativeText(extraText);
    msgBox.setStandardButtons(buttons);
    msgBox.setDefaultButton(defButton);
    return msgBox.exec();
}

//---------------------------------------------------------------------------------------------------------------------
