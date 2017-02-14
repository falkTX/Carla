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
    double bpm, beats, phase;
} hylia_time_info_t;

hylia_t* hylia_create(double bpm, uint32_t buffer_size, uint32_t sample_rate);
void hylia_enable(hylia_t* link, bool on, double bpm);
void hylia_process(hylia_t* link, uint32_t frames, hylia_time_info_t* info);
void hylia_set_tempo(hylia_t* link, double tempo);
void hylia_cleanup(hylia_t* link);

#ifdef __cplusplus
}
#endif

#endif // MOD_LINK_H_INCLUDED
