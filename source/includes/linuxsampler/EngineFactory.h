/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2005-2007 Christian Schoenebeck                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifndef __LS_ENGINEFACTORY_H__
#define __LS_ENGINEFACTORY_H__

#include <set>
#include <vector>

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated"
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated"
#endif

#include <linuxsampler/engines/Engine.h>

namespace LinuxSampler {

class EngineFactory {
public:
    static std::vector<String> AvailableEngineTypes();
    static String AvailableEngineTypesAsString();
    static Engine* Create(String EngineType) throw (Exception);
    static void Destroy(Engine* pEngine);
    static const std::set<Engine*>& EngineInstances();

protected:
    static void Erase(Engine* pEngine);
    friend class Engine;
};

} // namespace LinuxSampler

#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

#endif // __LS_ENGINEFACTORY_H__
