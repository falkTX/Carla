/*
 * Carla plugin host
 * Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

#pragma once

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
# pragma clang diagnostic ignored "-Wdeprecated-register"
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wclass-memaccess"
# pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <QtCore/QString>

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "CarlaDefines.h"

//---------------------------------------------------------------------------------------------------------------------
// Custom QString class with a few extra methods

class QCarlaString : public QString
{
public:
    explicit inline QCarlaString()
        : QString() {}

    explicit inline QCarlaString(const char* const str)
        : QString(fromUtf8(str)) {}

#if QT_VERSION < 0x60000
    explicit inline QCarlaString(const QChar* const str, const size_t size)
        : QString(str, size) {}
#endif

    inline QCarlaString(const QString& s)
        : QString(s) {}

    inline bool isNotEmpty() const
    {
        return !isEmpty();
    }

    inline QCarlaString& operator=(const char* const str)
    {
        return (*this = fromUtf8(str));
    }

    inline QCarlaString strip() const
    {
        return simplified().remove(' ');
    }

#if QT_VERSION < 0x60000
    inline QCarlaString sliced(const size_t pos) const
    {
        return QCarlaString(data() + pos, size() - pos);
    }
#endif
};

//---------------------------------------------------------------------------------------------------------------------
// Custom QByteArray class with a few extra methods

class QCarlaByteArray : public QByteArray
{
public:
    explicit inline QCarlaByteArray()
        : QByteArray() {}

    explicit inline QCarlaByteArray(const char* const data, const size_t size)
        : QByteArray(data, size) {}

    inline QCarlaByteArray(const QByteArray& b)
        : QByteArray(b) {}

#if QT_VERSION < 0x60000
    inline QCarlaByteArray sliced(const size_t pos) const
    {
        return QCarlaByteArray(data() + pos, size() - pos);
    }
#endif
};

//---------------------------------------------------------------------------------------------------------------------
