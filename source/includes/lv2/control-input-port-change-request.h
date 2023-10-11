/*
  LV2 ControlInputPort change request extension
  Copyright 2020 Filipe Coelho <falktx@falktx.com>

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
   @file control-input-port-change-request.h
   C header for the LV2 ControlInputPort change request extension <http://kx.studio/ns/lv2ext/control-input-port-change-request>.
*/

#ifndef LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_H
#define LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_H

#include "lv2.h"

#define LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_URI    "http://kx.studio/ns/lv2ext/control-input-port-change-request"
#define LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_PREFIX LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_URI "#"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

/** A status code for LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_URI functions. */
typedef enum {
	LV2_CONTROL_INPUT_PORT_CHANGE_SUCCESS           = 0,  /**< Completed successfully. */
	LV2_CONTROL_INPUT_PORT_CHANGE_ERR_UNKNOWN       = 1,  /**< Unknown error. */
	LV2_CONTROL_INPUT_PORT_CHANGE_ERR_INVALID_INDEX = 2   /**< Failed due to invalid port index. */
} LV2_ControlInputPort_Change_Status;

/**
 *  Opaque handle for LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_URI feature.
 */
typedef void* LV2_ControlInputPort_Change_Request_Handle;

/**
 * On instantiation, host must supply LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_URI feature.
 * LV2_Feature::data must be pointer to LV2_ControlInputPort_Change_Request.
*/
typedef struct _LV2_ControlInputPort_Change_Request {
    /**
     *  Opaque host data.
     */
    LV2_ControlInputPort_Change_Request_Handle handle;

    /**
     * request_change()
     *
     * Ask the host to change a plugin's control input port value.
     * Parameter handle MUST be the 'handle' member of this struct.
     * Parameter index is port index to change.
     * Parameter value is the requested value to change the control port input to.
     *
     * Returns status of the request.
     * The host may decline this request, if e.g. it is currently automating this port.
     *
     * The plugin MUST call this function during run().
     */
    LV2_ControlInputPort_Change_Status (*request_change)(LV2_ControlInputPort_Change_Request_Handle handle,
                                                         uint32_t index,
                                                         float value);

} LV2_ControlInputPort_Change_Request;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_H */
