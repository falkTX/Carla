/*
 * Carla Tests
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "RtLinkedList.hpp"

#include "CarlaString.hpp"
#include "CarlaMutex.hpp"

const unsigned short MIN_RT_EVENTS = 5;
const unsigned short MAX_RT_EVENTS = 10;

struct MyData {
    char str[234];
    int id;
};

struct PostRtEvents {
    CarlaMutex mutex;
    RtLinkedList<MyData>::Pool dataPool;
    RtLinkedList<MyData> data;
    RtLinkedList<MyData> dataPendingRT;

    PostRtEvents() noexcept
        : dataPool(MIN_RT_EVENTS, MAX_RT_EVENTS),
          data(dataPool),
          dataPendingRT(dataPool) {}

    ~PostRtEvents() noexcept
    {
        clear();
    }

    void appendRT(const MyData& event) noexcept
    {
        dataPendingRT.append(event);
    }

    void clear() noexcept
    {
        mutex.lock();
        data.clear();
        dataPendingRT.clear();
        mutex.unlock();
    }

    void trySplice() noexcept
    {
        if (mutex.tryLock())
        {
            dataPendingRT.moveTo(data, true);
            mutex.unlock();
        }
    }

} postRtEvents;

void run5Tests();
void run5Tests()
{
    ushort k = 0;
    MyData allMyData[MAX_RT_EVENTS];

    // Make a safe copy of events while clearing them
    postRtEvents.mutex.lock();

    while (! postRtEvents.data.isEmpty())
    {
        static MyData fallback = { { '\0' }, 0 };
        const MyData& my(postRtEvents.data.getFirst(fallback, true));
        allMyData[k++] = my;
    }

    postRtEvents.mutex.unlock();

    carla_stdout("Post-Rt Event Count: %i", k);
    assert(k == 5);

    // data should be empty now
    assert(postRtEvents.data.count() == 0);
    assert(postRtEvents.data.isEmpty());
    assert(postRtEvents.dataPendingRT.count() == 0);
    assert(postRtEvents.dataPendingRT.isEmpty());

    // Handle events now
    for (ushort i=0; i < k; ++i)
    {
        const MyData& my(allMyData[i]);

        carla_stdout("Got data: %i %s", my.id, my.str);
    }
}

int main()
{
    MyData m1; m1.id = 1; std::strcpy(m1.str, "1");
    MyData m2; m2.id = 2; std::strcpy(m2.str, "2");
    MyData m3; m3.id = 3; std::strcpy(m3.str, "3");
    MyData m4; m4.id = 4; std::strcpy(m4.str, "4");
    MyData m5; m5.id = 5; std::strcpy(m5.str, "5");

    // start
    assert(postRtEvents.data.count() == 0);
    assert(postRtEvents.data.isEmpty());
    assert(postRtEvents.dataPendingRT.count() == 0);
    assert(postRtEvents.dataPendingRT.isEmpty());

    // single append
    postRtEvents.appendRT(m1);
    assert(postRtEvents.data.count() == 0);
    assert(postRtEvents.dataPendingRT.count() == 1);
    postRtEvents.trySplice();
    assert(postRtEvents.data.count() == 1);
    assert(postRtEvents.dataPendingRT.count() == 0);

    // +3 appends
    postRtEvents.appendRT(m2);
    postRtEvents.appendRT(m4);
    postRtEvents.appendRT(m3);
    assert(postRtEvents.data.count() == 1);
    assert(postRtEvents.dataPendingRT.count() == 3);
    postRtEvents.trySplice();
    assert(postRtEvents.data.count() == 4);
    assert(postRtEvents.dataPendingRT.count() == 0);

    for (RtLinkedList<MyData>::Itenerator it = postRtEvents.data.begin2(); it.valid(); it.next())
    {
        const MyData& my(it.getValue());

        carla_stdout("FOR DATA!!!: %i %s", my.id, my.str);

        if (my.id == 2)
        {
            // +1 append
            postRtEvents.dataPendingRT.insertAt(m5, it);
            assert(postRtEvents.data.count() == 4);
            assert(postRtEvents.dataPendingRT.count() == 1);
        }
    }

    for (const MyData& my : postRtEvents.data)
        carla_stdout("FOR DATA!!! NEW AUTO Itenerator!!!: %i %s", my.id, my.str);

    postRtEvents.trySplice();
    assert(postRtEvents.data.count() == 5);
    assert(postRtEvents.dataPendingRT.count() == 0);

    run5Tests();

    // reset
    postRtEvents.clear();
    assert(postRtEvents.data.count() == 0);
    assert(postRtEvents.data.isEmpty());
    assert(postRtEvents.dataPendingRT.count() == 0);
    assert(postRtEvents.dataPendingRT.isEmpty());

    // test non-rt
    const unsigned int CARLA_EVENT_DATA_ATOM    = 0x01;
    const unsigned int CARLA_EVENT_DATA_MIDI_LL = 0x04;

    LinkedList<uint32_t> evIns, evOuts;
    evIns.append(CARLA_EVENT_DATA_ATOM);
    evOuts.append(CARLA_EVENT_DATA_ATOM);
    evOuts.append(CARLA_EVENT_DATA_MIDI_LL);

    if (evIns.count() > 0)
    {
        for (size_t j=0, count=evIns.count(); j < count; ++j)
        {
            const uint32_t& type(evIns.getAt(j, 0));

            if (type == CARLA_EVENT_DATA_ATOM)
                pass();
            else if (type == CARLA_EVENT_DATA_MIDI_LL)
                pass();
        }
    }

    evIns.clear();
    evOuts.clear();

    return 0;
}
