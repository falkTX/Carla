/*
  LV2 KXStudio Properties Extension
  Copyright 2014 Filipe Coelho <falktx@falktx.com>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file lv2_kxstudio_properties.h
   C header for the LV2 KXStudio Properties extension <http://kxstudio.sf.net/ns/lv2ext/props>.
*/

#ifndef LV2_KXSTUDIO_PROPERTIES_H
#define LV2_KXSTUDIO_PROPERTIES_H

#define LV2_KXSTUDIO_PROPERTIES_URI    "http://kxstudio.sf.net/ns/lv2ext/props"
#define LV2_KXSTUDIO_PROPERTIES_PREFIX LV2_KXSTUDIO_PROPERTIES_URI "#"

#define LV2_KXSTUDIO_PROPERTIES__NonAutomable             LV2_KXSTUDIO_PROPERTIES_PREFIX "NonAutomable"
#define LV2_KXSTUDIO_PROPERTIES__TimePositionTicksPerBeat LV2_KXSTUDIO_PROPERTIES_PREFIX "TimePositionTicksPerBeat"
#define LV2_KXSTUDIO_PROPERTIES__TransientWindowId        LV2_KXSTUDIO_PROPERTIES_PREFIX "TransientWindowId"

#endif /* LV2_KXSTUDIO_PROPERTIES_H */
