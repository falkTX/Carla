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

#ifndef MOD_LINK_H_INCLUDED
#define MOD_LINK_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

typedef struct _hylia_t hylia_t;

typedef struct _hylia_time_info_t {
    double beatsPerBar, beatsPerMinute, beat, phase;
} hylia_time_info_t;

hylia_t* hylia_create(void);
void hylia_enable(hylia_t* link, bool on);
void hylia_process(hylia_t* link, uint32_t frames, hylia_time_info_t* info);
void hylia_set_beats_per_bar(hylia_t* link, double beatsPerBar);
void hylia_set_beats_per_minute(hylia_t* link, double beatsPerMinute);
void hylia_set_output_latency(hylia_t* link, uint32_t latency);
void hylia_cleanup(hylia_t* link);

#ifdef __cplusplus
}
#endif

#endif // MOD_LINK_H_INCLUDED
