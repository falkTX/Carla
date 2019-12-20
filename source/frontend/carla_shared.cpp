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

#include <QtGui/QFontMetrics>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "CarlaMathUtils.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Global Carla object

CarlaObject::CarlaObject() noexcept
    : gui(nullptr),
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

void getInitialProjectFile(void*, bool)
{
    // TODO
}

//---------------------------------------------------------------------------------------------------------------------
// Get paths (binaries, resources)

void getPaths()
{
    // TODO
}

//---------------------------------------------------------------------------------------------------------------------
// Signal handler

/*
void signalHandler(const int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        gCarla.term = true;
        if (gCarla.gui != nullptr)
            gCarla.gui.SIGTERM.emit();
    }
    else if (sig == SIGUSR1)
    {
        if (gCarla.gui != nullptr)
            gCarla.gui.SIGUSR1.emit();
    }
}
*/

void setUpSignals()
{
    /*
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGUSR1, signalHandler);
    */
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
    uint count = 0;

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
    uint count = 0;

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
    uint count = 0;

    // count number of strings first
    for (; uintArray[count] != 0; ++count) {}

    // allocate list
    list.reserve(count);

    // fill in strings
    for (count = 0; uintArray[count] != 0; ++count)
        list.append(uintArray[count]);
}

//---------------------------------------------------------------------------------------------------------------------
// Backwards-compatible horizontalAdvance/width call, depending on qt version

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
// Safer QSettings class, which does not throw if type mismatches

bool QSafeSettings::valueBool(const QString key, const bool defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (var.isNull())
        return defaultValue;

    CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::Bool), defaultValue);

    return var.isValid() ? var.toBool() : defaultValue;
}

Qt::CheckState QSafeSettings::valueCheckState(const QString key, const Qt::CheckState defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (var.isNull())
        return defaultValue;

    CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::UInt), defaultValue);

    if (! var.isValid())
        return defaultValue;

    const uint value = var.toUInt();

    switch (value)
    {
    case Qt::Unchecked:
    case Qt::PartiallyChecked:
    case Qt::Checked:
        return static_cast<Qt::CheckState>(value);
    default:
        return defaultValue;
    }
}

uint QSafeSettings::valueUInt(const QString key, const uint defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (var.isNull())
        return defaultValue;

    CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::UInt), defaultValue);

    return var.isValid() ? var.toUInt() : defaultValue;
}

double QSafeSettings::valueDouble(const QString key, const double defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (var.isNull())
        return defaultValue;

    CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::Double), defaultValue);

    return var.isValid() ? var.toDouble() : defaultValue;
}

QString QSafeSettings::valueString(const QString key, const QString defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (var.isNull())
        return defaultValue;

    CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::String), defaultValue);

    return var.isValid() ? var.toString() : defaultValue;
}

QByteArray QSafeSettings::valueByteArray(const QString key, const QByteArray defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (var.isNull())
        return defaultValue;

    CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::ByteArray), defaultValue);

    return var.isValid() ? var.toByteArray() : defaultValue;
}

QStringList QSafeSettings::valueStringList(const QString key, const QStringList defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (var.isNull())
        return defaultValue;

    CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::StringList), defaultValue);

    return var.isValid() ? var.toStringList() : defaultValue;
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
