/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_UI_QT_HPP_INCLUDED
#define DISTRHO_UI_QT_HPP_INCLUDED

#include "DistrhoUI.hpp"

#include <QtCore/Qt>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QWidget>
#else
# include <QtGui/QWidget>
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Qt UI

class QtUI : public UI,
             public QWidget
{
public:
    QtUI();
    virtual ~QtUI() override;

    // -------------------------------------------------------------------
    // UI Helpers

    void setSize(unsigned int width, unsigned int height);

protected:
    // -------------------------------------------------------------------
    // Information (Qt)

    virtual bool d_isResizable()      const noexcept { return false; }
    virtual uint d_getMinimumWidth()  const noexcept { return 100; }
    virtual uint d_getMinimumHeight() const noexcept { return 100; }

private:
    // -------------------------------------------------------------------
    // Implemented internally

    unsigned int d_getWidth()  const noexcept override { return width(); }
    unsigned int d_getHeight() const noexcept override { return height(); }

    friend class UIInternal;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_QT_HPP_INCLUDED
