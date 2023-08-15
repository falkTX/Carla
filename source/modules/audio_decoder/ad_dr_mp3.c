/**
   Copyright (C) 2011-2013 Robin Gareus <robin@gareus.org>
   Copyright (C) 2014-2023 Filipe Coelho <falktx@falktx.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser Public License as published by
   the Free Software Foundation; either version 2.1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ad_plugin.h"

#define DR_MP3_FLOAT_OUTPUT
#define DR_MP3_IMPLEMENTATION
#define DRMP3_DATA_CHUNK_SIZE DRMP3_MIN_DATA_CHUNK_SIZE*40
#include "dr_mp3.h"

/* internal abstraction */

#define DR_MP3_MAX_SEEK_POINTS 500

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif

typedef struct {
	drmp3 mp3;
	drmp3_seek_point seekPoints[DR_MP3_MAX_SEEK_POINTS];
} drmp3_audio_decoder;

static int ad_info_dr_mp3(void *sf, struct adinfo *nfo) {
	drmp3_audio_decoder *priv = (drmp3_audio_decoder*) sf;
	if (!priv) return -1;
	if (nfo) {
		nfo->channels = priv->mp3.channels;
		nfo->frames = drmp3_get_pcm_frame_count(&priv->mp3);
		nfo->sample_rate = priv->mp3.sampleRate;
		nfo->length = nfo->sample_rate ? (nfo->frames * 1000) / nfo->sample_rate : 0;
		nfo->bit_depth = 16;
		nfo->bit_rate = priv->mp3.frameInfo.bitrate_kbps * 1000;
		nfo->meta_data = NULL;
		nfo->can_seek = 1;
	}
	return 0;
}

static void *ad_open_dr_mp3(const char *filename, struct adinfo *nfo) {
	drmp3_audio_decoder *priv = (drmp3_audio_decoder*)calloc (1, sizeof(drmp3_audio_decoder));
	drmp3_bool32 res = drmp3_init_file(&priv->mp3, filename, NULL);
	if (res == DRMP3_FALSE) {
		dbg(0, "unable to open file '%s'.", filename);
		free (priv);
		return NULL;
	}
	drmp3_uint32 seekPointCount = DR_MP3_MAX_SEEK_POINTS;
	drmp3_calculate_seek_points (&priv->mp3, &seekPointCount, priv->seekPoints);
	drmp3_bind_seek_table (&priv->mp3, seekPointCount, priv->seekPoints);
	ad_info_dr_mp3 (priv, nfo);
	return (void*) priv;
}

static int ad_close_dr_mp3(void *sf) {
	drmp3_audio_decoder *priv = (drmp3_audio_decoder*) sf;
	if (!priv) return -1;
	if (!sf) {
		dbg (0, "fatal: bad file close.\n");
		return -1;
	}
	drmp3_uninit (&priv->mp3);
	free (priv);
	return 0;
}

static int64_t ad_seek_dr_mp3(void *sf, int64_t pos)
{
	drmp3_audio_decoder *priv = (drmp3_audio_decoder*) sf;
	if (!priv) return -1;
	return drmp3_seek_to_pcm_frame (&priv->mp3, pos);
}

static ssize_t ad_read_dr_mp3(void *sf, float* d, size_t len)
{
	drmp3_audio_decoder *priv = (drmp3_audio_decoder*) sf;
	if (!priv) return -1;
	return drmp3_read_pcm_frames_f32 (&priv->mp3, len / priv->mp3.channels, d) * priv->mp3.channels;
}

static int ad_get_bitrate_dr_mp3(void *sf) {
	drmp3_audio_decoder *priv = (drmp3_audio_decoder*) sf;
	if (!priv) return -1;
	return priv->mp3.frameInfo.bitrate_kbps * 1000;
}

static int ad_eval_dr_mp3(const char *f)
{
	char *ext = strrchr(f, '.');
	if (strstr (f, "://")) return 0;
	if (!ext) return 5;
	if (!strcasecmp(ext, ".mp3")) return 100;
	return 0;
}

static const ad_plugin ad_dr_mp3 = {
#if 1
	&ad_eval_dr_mp3,
	&ad_open_dr_mp3,
	&ad_close_dr_mp3,
	&ad_info_dr_mp3,
	&ad_seek_dr_mp3,
	&ad_read_dr_mp3,
	&ad_get_bitrate_dr_mp3
#else
	&ad_eval_null,
	&ad_open_null,
	&ad_close_null,
	&ad_info_null,
	&ad_seek_null,
	&ad_read_null,
	&ad_bit_rate_null
#endif
};

/* dlopen handler */
const ad_plugin * adp_get_dr_mp3() {
	return &ad_dr_mp3;
}
