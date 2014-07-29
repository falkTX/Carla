/*
 * Carla patchbay utils
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_PATCHBAY_UTILS_HPP_INCLUDED
#define CARLA_PATCHBAY_UTILS_HPP_INCLUDED

#include "LinkedList.hpp"

#define STR_MAX 0xFF

// -----------------------------------------------------------------------

struct GroupNameToId {
    uint group;
    char name[STR_MAX+1]; // globally unique

    void clear() noexcept
    {
        group   = 0;
        name[0] = '\0';
    }

    void setData(const uint g, const char n[]) noexcept
    {
        group = g;
        rename(n);
    }

    void rename(const char n[]) noexcept
    {
        std::strncpy(name, n, STR_MAX);
        name[STR_MAX] = '\0';
    }

    bool operator==(const GroupNameToId& groupNameToId) const noexcept
    {
        if (groupNameToId.group != group)
            return false;
        if (std::strcmp(groupNameToId.name, name) != 0)
            return false;
        return true;
    }

    bool operator!=(const GroupNameToId& groupNameToId) const noexcept
    {
        return !operator==(groupNameToId);
    }
};

struct PatchbayGroupList {
    uint lastId;
    LinkedList<GroupNameToId> list;

    PatchbayGroupList() noexcept
        : lastId(0),
          list() {}

    void clear() noexcept
    {
        lastId = 0;
        list.clear();
    }

    uint getGroupId(const char* const groupName) const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(groupName != nullptr && groupName[0] != '\0', 0);

        for (LinkedList<GroupNameToId>::Itenerator it = list.begin(); it.valid(); it.next())
        {
            const GroupNameToId& groupNameToId(it.getValue());

            if (std::strcmp(groupNameToId.name, groupName) == 0)
                return groupNameToId.group;
        }

        return 0;
    }

    const char* getGroupName(const uint groupId) const noexcept
    {
        static const char fallback[] = { '\0' };

        for (LinkedList<GroupNameToId>::Itenerator it = list.begin(); it.valid(); it.next())
        {
            const GroupNameToId& groupNameToId(it.getValue());

            if (groupNameToId.group == groupId)
                return groupNameToId.name;
        }

        return fallback;
    }
};

// -----------------------------------------------------------------------

struct PortNameToId {
    uint group;
    uint port;
    char name[STR_MAX+1]; // locally unique
    char fullName[STR_MAX+1]; // globally unique

    void clear() noexcept
    {
        group       = 0;
        port        = 0;
        name[0]     = '\0';
        fullName[0] = '\0';
    }

    void setData(const uint g, const uint p, const char n[], const char fn[]) noexcept
    {
        group = g;
        port  = p;
        rename(n, fn);
    }

    void rename(const char n[], const char fn[]) noexcept
    {
        std::strncpy(name, n, STR_MAX);
        name[STR_MAX] = '\0';

        std::strncpy(fullName, fn, STR_MAX);
        fullName[STR_MAX] = '\0';
    }

    bool operator==(const PortNameToId& portNameToId) noexcept
    {
        if (portNameToId.group != group || portNameToId.port != port)
            return false;
        if (std::strcmp(portNameToId.name, name) != 0)
            return false;
        if (std::strcmp(portNameToId.fullName, fullName) != 0)
            return false;
        return true;
    }

    bool operator!=(const PortNameToId& portNameToId) noexcept
    {
        return !operator==(portNameToId);
    }
};

struct PatchbayPortList {
    uint lastId;
    LinkedList<PortNameToId> list;

    PatchbayPortList() noexcept
        : lastId(0),
          list() {}

    void clear() noexcept
    {
        lastId = 0;
        list.clear();
    }

    const char* getFullPortName(const uint groupId, const uint portId) const noexcept
    {
        static const char fallback[] = { '\0' };

        for (LinkedList<PortNameToId>::Itenerator it = list.begin(); it.valid(); it.next())
        {
            const PortNameToId& portNameToId(it.getValue());

            if (portNameToId.group == groupId && portNameToId.port == portId)
                return portNameToId.fullName;
        }

        return fallback;
    }

    const PortNameToId& getPortNameToId(const char* const fullPortName) const noexcept
    {
        static const PortNameToId fallback = { 0, 0, { '\0' }, { '\0' } };

        CARLA_SAFE_ASSERT_RETURN(fullPortName != nullptr && fullPortName[0] != '\0', fallback);

        for (LinkedList<PortNameToId>::Itenerator it = list.begin(); it.valid(); it.next())
        {
            const PortNameToId& portNameToId(it.getValue());

            if (std::strcmp(portNameToId.fullName, fullPortName) == 0)
                return portNameToId;
        }

        return fallback;
    }
};

// -----------------------------------------------------------------------

struct ConnectionToId {
    uint id;
    uint groupA, portA;
    uint groupB, portB;

    void clear() noexcept
    {
        id     = 0;
        groupA = 0;
        portA  = 0;
        groupB = 0;
        portB  = 0;
    }

    void setData(const uint i, const uint gA, const uint pA, const uint gB, const uint pB) noexcept
    {
        id     = i;
        groupA = gA;
        portA  = pA;
        groupB = gB;
        portB  = pB;
    }

    bool operator==(const ConnectionToId& connectionToId) const noexcept
    {
        if (connectionToId.id != id)
            return false;
        if (connectionToId.groupA != groupA || connectionToId.portA != portA)
            return false;
        if (connectionToId.groupB != groupB || connectionToId.portB != portB)
            return false;
        return true;
    }

    bool operator!=(const ConnectionToId& connectionToId) const noexcept
    {
        return !operator==(connectionToId);
    }
};

struct PatchbayConnectionList {
    uint lastId;
    LinkedList<ConnectionToId> list;

    PatchbayConnectionList() noexcept
        : lastId(0),
          list() {}

    void clear() noexcept
    {
        lastId = 0;
        list.clear();
    }
};

// -----------------------------------------------------------------------

#endif // CARLA_PATCHBAY_UTILS_HPP_INCLUDED
