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

// disable SIMD for macos-old builds
#include "CarlaDefines.h"
#if defined(CARLA_OS_WASM)
# define MINIMP3_NO_SIMD
#elif defined(CARLA_OS_MAC) && !defined(CARLA_PROPER_CPP11_SUPPORT)
# define MINIMP3_NO_SIMD
#endif

#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif

/* internal abstraction */

typedef struct {
	mp3dec_ex_t dec_ex;
} minimp3_audio_decoder;

static void err_to_string(int err_code, char *buf) {
	switch (err_code)
	{
	case MP3D_E_PARAM:
		strcpy (buf, "Parameter error");
		break;
	case MP3D_E_MEMORY:
		strcpy (buf, "Memory error");
		break;
	case MP3D_E_IOERROR:
		strcpy (buf, "IO error");
		break;
	case MP3D_E_USER:
		strcpy (buf, "User error");
		break;
	case MP3D_E_DECODE:
		strcpy (buf, "Decode error");
		break;
	default:
		strcpy (buf, "Unknown error");
		break;
	}
}

static int ad_info_minimp3(void *sf, struct adinfo *nfo) {
	minimp3_audio_decoder *priv = (minimp3_audio_decoder*) sf;
	if (!priv) return -1;
	if (nfo) {
		nfo->channels = priv->dec_ex.info.channels;
		nfo->frames = priv->dec_ex.samples / nfo->channels;
		nfo->sample_rate = priv->dec_ex.info.hz;
		nfo->length = nfo->sample_rate ? (nfo->frames * 1000) / nfo->sample_rate : 0;
		nfo->bit_depth = 16;
		nfo->bit_rate = priv->dec_ex.info.bitrate_kbps * 1000;
		nfo->meta_data = NULL;
		nfo->can_seek = 0;
	}
	return 0;
}

static void *ad_open_minimp3(const char *filename, struct adinfo *nfo) {
	minimp3_audio_decoder *priv = (minimp3_audio_decoder*)calloc (1, sizeof(minimp3_audio_decoder));
	int res = mp3dec_ex_open(&priv->dec_ex, filename, MP3D_SEEK_TO_SAMPLE);
	if (res) {
		dbg(0, "unable to open file '%s'.", filename);
		char err_str[600];
		err_to_string (res, err_str);
		puts (err_str);
		dbg (0, "error=%i", res);
		free (priv);
		return NULL;
	}
	ad_info_minimp3 (priv, nfo);
	return (void*) priv;
}

static int ad_close_minimp3(void *sf) {
	minimp3_audio_decoder *priv = (minimp3_audio_decoder*) sf;
	if (!priv) return -1;
	if (!sf) {
		dbg (0, "fatal: bad file close.\n");
		return -1;
	}
	mp3dec_ex_close (&priv->dec_ex);
	free (priv);
	return 0;
}

static int64_t ad_seek_minimp3(void *sf, int64_t pos)
{
	minimp3_audio_decoder *priv = (minimp3_audio_decoder*) sf;
	if (!priv) return -1;
	return mp3dec_ex_seek (&priv->dec_ex, pos);
}

static ssize_t ad_read_minimp3(void *sf, float* d, size_t len)
{
	minimp3_audio_decoder *priv = (minimp3_audio_decoder*) sf;
	if (!priv) return -1;
	return mp3dec_ex_read (&priv->dec_ex, d, len);
}

static int ad_get_bitrate_minimp3(void *sf) {
	minimp3_audio_decoder *priv = (minimp3_audio_decoder*) sf;
	if (!priv) return -1;
	return priv->dec_ex.info.bitrate_kbps * 1000;
}

static int ad_eval_minimp3(const char *f)
{
	char *ext = strrchr(f, '.');
	if (strstr (f, "://")) return 0;
	if (!ext) return 5;
	if (!strcasecmp(ext, ".mp3")) return 100;
	return 0;
}

static const ad_plugin ad_minimp3 = {
#if 1
	&ad_eval_minimp3,
	&ad_open_minimp3,
	&ad_close_minimp3,
	&ad_info_minimp3,
	&ad_seek_minimp3,
	&ad_read_minimp3,
	&ad_get_bitrate_minimp3
#else
	&ad_eval_null,
	&ad_open_null,
	&ad_close_null,
	&ad_info_null,
	&ad_seek_null,
	&ad_read_null,
	&ad_bitrate_null
#endif
};

/* dlopen handler */
const ad_plugin * adp_get_minimp3() {
	return &ad_minimp3;
}
