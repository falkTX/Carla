// SPDX-License-Identifier: GPL-2.0-or-later

#include "qt_boot.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QIcon>
#include <QPointer>
#include <QProgressDialog>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QThread>
#include <QUrl>

namespace {

QQmlApplicationEngine* g_engine = nullptr;
QPointer<QProgressDialog> g_progress_dialog;
QString g_base_message;

bool on_gui_thread() {
    auto* app = QCoreApplication::instance();
    return app != nullptr && QThread::currentThread() == app->thread();
}

void set_ui_busy(bool busy) {
    if (g_engine != nullptr && g_engine->rootContext() != nullptr) {
        g_engine->rootContext()->setContextProperty(QStringLiteral("francaUiBusy"), busy);
    }
}

void close_progress_dialog() {
    if (g_progress_dialog) {
        g_progress_dialog->hide();
        g_progress_dialog->deleteLater();
        g_progress_dialog = nullptr;
    }
    g_base_message.clear();
    set_ui_busy(false);
}

} // namespace

extern "C" {

void franca_qt_set_cancelable_action(bool started,
                                     const char* message,
                                     void (*cancel_fn)(void*),
                                     void* cancel_ctx) {
    if (!on_gui_thread()) {
        return;
    }

    close_progress_dialog();

    if (!started) {
        return;
    }

    set_ui_busy(true);
    g_base_message = (message != nullptr && message[0] != '\0')
        ? QString::fromUtf8(message)
        : QStringLiteral("Working");

    auto* dlg = new QProgressDialog(
        g_base_message + QStringLiteral("…"),
        QObject::tr("Cancel"),
        0,
        0,
        nullptr);
    dlg->setWindowTitle(QStringLiteral("Franca"));
    dlg->setWindowModality(Qt::ApplicationModal);
    dlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    dlg->setAutoClose(false);
    dlg->setAutoReset(false);
    dlg->setMinimumDuration(0);
    dlg->setMinimumWidth(340);

    QObject::connect(dlg, &QProgressDialog::canceled, dlg, [cancel_fn, cancel_ctx]() {
        if (cancel_fn != nullptr) {
            cancel_fn(cancel_ctx);
        }
    });

    g_progress_dialog = dlg;
    dlg->show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
}

void franca_qt_update_cancelable_progress(int plugins_loaded) {
    if (!on_gui_thread() || !g_progress_dialog) {
        return;
    }

    if (plugins_loaded > 0) {
        const QString suffix = (plugins_loaded == 1)
            ? QStringLiteral(" (1 plugin loaded)")
            : QStringLiteral(" (%1 plugins loaded)").arg(plugins_loaded);
        g_progress_dialog->setLabelText(g_base_message + QStringLiteral("…") + suffix);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
}

void franca_qt_process_events() {
    if (!on_gui_thread() || !g_progress_dialog) {
        return;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
}

} // extern "C"

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
    g_engine = &engine;
    set_ui_busy(false);

    const QUrl url(QStringLiteral("qrc:/qt/qml/org/franca/app/qml/Main.qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);
    if (engine.rootObjects().isEmpty()) {
        g_engine = nullptr;
        return -1;
    }

    const int rc = app.exec();
    close_progress_dialog();
    g_engine = nullptr;
    return rc;
}

} // namespace franca_qt
