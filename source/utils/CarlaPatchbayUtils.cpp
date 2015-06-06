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

#include "CarlaPatchbayUtils.hpp"

static const GroupNameToId kGroupNameToIdFallback = { 0, { '\0' } };
static const PortNameToId  kPortNameToIdFallback  = { 0, 0, { '\0' }, { '\0' } };

uint PatchbayGroupList::getGroupId(const char* const groupName) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(groupName != nullptr && groupName[0] != '\0', 0);

    for (LinkedList<GroupNameToId>::Itenerator it = list.begin2(); it.valid(); it.next())
    {
        const GroupNameToId& groupNameToId(it.getValue(kGroupNameToIdFallback));
        CARLA_SAFE_ASSERT_CONTINUE(groupNameToId.group != 0);

        if (std::strncmp(groupNameToId.name, groupName, STR_MAX) == 0)
            return groupNameToId.group;
    }

    return 0;
}

const char* PatchbayGroupList::getGroupName(const uint groupId) const noexcept
{
    static const char fallback[] = { '\0' };

    for (LinkedList<GroupNameToId>::Itenerator it = list.begin2(); it.valid(); it.next())
    {
        const GroupNameToId& groupNameToId(it.getValue(kGroupNameToIdFallback));
        CARLA_SAFE_ASSERT_CONTINUE(groupNameToId.group != 0);

        if (groupNameToId.group == groupId)
            return groupNameToId.name;
    }

    return fallback;
}

const char* PatchbayPortList::getFullPortName(const uint groupId, const uint portId) const noexcept
{
    static const char fallback[] = { '\0' };

    for (LinkedList<PortNameToId>::Itenerator it = list.begin2(); it.valid(); it.next())
    {
        const PortNameToId& portNameToId(it.getValue(kPortNameToIdFallback));
        CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group != 0);

        if (portNameToId.group == groupId && portNameToId.port == portId)
            return portNameToId.fullName;
    }

    return fallback;
}

const PortNameToId& PatchbayPortList::getPortNameToId(const char* const fullPortName) const noexcept
{
    CARLA_SAFE_ASSERT_RETURN(fullPortName != nullptr && fullPortName[0] != '\0', kPortNameToIdFallback);

    for (LinkedList<PortNameToId>::Itenerator it = list.begin2(); it.valid(); it.next())
    {
        const PortNameToId& portNameToId(it.getValue(kPortNameToIdFallback));
        CARLA_SAFE_ASSERT_CONTINUE(portNameToId.group != 0);

        if (std::strncmp(portNameToId.fullName, fullPortName, STR_MAX) == 0)
            return portNameToId;
    }

    return kPortNameToIdFallback;
}