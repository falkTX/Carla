// SPDX-License-Identifier: GPL-2.0-or-later

#include "qt_boot.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>

namespace franca_qt {

int run_qt_app() {
    static char arg0[] = "franca";
    static char* argv[] = { arg0, nullptr };
    int argc = 1;

    // Prefer native KDE Breeze style (qqc2-desktop-style) unless overridden by user.
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Franca"));
    app.setApplicationDisplayName(QStringLiteral("Franca"));
    app.setOrganizationName(QStringLiteral("Franca"));
    app.setDesktopFileName(QStringLiteral("franca"));
    app.setWindowIcon(QIcon(QStringLiteral(":/franca.png")));

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/qt/qml/org/franca/app/qml/Main.qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}

} // namespace franca_qt
