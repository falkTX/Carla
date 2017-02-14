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
    HyliaTransport(double bpm, double bufferSize, double sampleRate)
        : link(bpm),
          engine(link),
          outputLatency(0),
          sampleTime(0)
    {
        outputLatency = std::chrono::microseconds(llround(1.0e6 * bufferSize / sampleRate));
    }

    void setEnabled(bool enabled, double bpm)
    {
        link.enable(enabled);

        if (enabled)
        {
            sampleTime = 0;
            engine.setTempo(bpm);
        }
    }

    void setTempo(double tempo)
    {
        engine.setTempo(tempo);
    }

    void process(uint32_t frames, LinkTimeInfo* info)
    {
        const auto hostTime = hostTimeFilter.sampleTimeToHostTime(sampleTime);
        const auto bufferBeginAtOutput = hostTime + outputLatency;

        engine.timelineCallback(bufferBeginAtOutput, info);

        sampleTime += frames;
    }

private:
    ableton::Link link;
    ableton::linkaudio::AudioEngine engine;

    ableton::link::HostTimeFilter<ableton::platforms::stl::Clock> hostTimeFilter;
    std::chrono::microseconds outputLatency;
    uint32_t sampleTime;
};

hylia_t* hylia_create(double bpm, uint32_t buffer_size, uint32_t sample_rate)
{
    HyliaTransport* t;

    try {
        t = new HyliaTransport(bpm, buffer_size, sample_rate);
    } catch (...) {
        return nullptr;
    }

    return (hylia_t*)t;
}

void hylia_enable(hylia_t* link, bool on, double bpm)
{
    ((HyliaTransport*)link)->setEnabled(on, bpm);
}

void hylia_set_tempo(hylia_t* link, double bpm)
{
    ((HyliaTransport*)link)->setTempo(bpm);
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
