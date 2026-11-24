// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

namespace franca_qt {
int run_qt_app();
}

extern "C" {
void franca_qt_set_cancelable_action(bool started,
                                     const char* message,
                                     void (*cancel_fn)(void*),
                                     void* cancel_ctx);
void franca_qt_update_cancelable_progress(int plugins_loaded);
void franca_qt_process_events();
}
