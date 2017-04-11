/*
 * This file is part of Hylia.
 *
 * Hylia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hylia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hylia.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "hylia.h"

#include "AudioEngine.hpp"
#include "ableton/link/HostTimeFilter.hpp"
#include <chrono>

class HyliaTransport {
public:
    HyliaTransport()
        : link(120.0),
          engine(link),
          outputLatency(0),
          sampleTime(0)
    {
    }

    void setEnabled(const bool enabled)
    {
        if (enabled)
            sampleTime = 0;

        link.enable(enabled);
    }

    void setQuantum(const double quantum)
    {
        engine.setQuantum(quantum);
    }

    void setTempo(const double tempo)
    {
        engine.setTempo(tempo);
    }

    void setOutputLatency(const uint32_t latency) noexcept
    {
        outputLatency = latency;
    }

    void process(const uint32_t frames, LinkTimeInfo* const info)
    {
        const std::chrono::microseconds hostTime = hostTimeFilter.sampleTimeToHostTime(sampleTime)
                                                 + std::chrono::microseconds(outputLatency);
        engine.timelineCallback(hostTime, info);

        sampleTime += frames;
    }

private:
    ableton::Link link;
    ableton::link::AudioEngine engine;
    ableton::link::HostTimeFilter<ableton::link::platform::Clock> hostTimeFilter;

    uint32_t outputLatency, sampleTime;
};

hylia_t* hylia_create(void)
{
    HyliaTransport* t;

    try {
        t = new HyliaTransport();
    } catch (...) {
        return nullptr;
    }

    return (hylia_t*)t;
}

void hylia_enable(hylia_t* link, bool on)
{
    ((HyliaTransport*)link)->setEnabled(on);
}

void hylia_set_beats_per_bar(hylia_t* link, double beatsPerBar)
{
    ((HyliaTransport*)link)->setQuantum(beatsPerBar);
}

void hylia_set_beats_per_minute(hylia_t* link, double beatsPerMinute)
{
    ((HyliaTransport*)link)->setTempo(beatsPerMinute);
}

void hylia_set_output_latency(hylia_t* link, uint32_t latency)
{
    ((HyliaTransport*)link)->setOutputLatency(latency);
}

void hylia_process(hylia_t* link, uint32_t frames, hylia_time_info_t* info)
{
    ((HyliaTransport*)link)->process(frames, (LinkTimeInfo*)info);
}

void hylia_cleanup(hylia_t* link)
{
    delete (HyliaTransport*)link;
}

#include "AudioEngine.cpp"
