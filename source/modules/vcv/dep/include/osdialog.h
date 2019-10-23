#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stddef.h>


#ifndef OSDIALOG_MALLOC
	#define OSDIALOG_MALLOC malloc
#endif

#ifndef OSDIALOG_FREE
	#define OSDIALOG_FREE free
#endif


char *osdialog_strndup(const char *s, size_t n);


typedef enum {
	OSDIALOG_INFO,
	OSDIALOG_WARNING,
	OSDIALOG_ERROR,
} osdialog_message_level;

typedef enum {
	OSDIALOG_OK,
	OSDIALOG_OK_CANCEL,
	OSDIALOG_YES_NO,
} osdialog_message_buttons;

/** Launches a message box.

Returns 1 if the "OK" or "Yes" button was pressed.
*/
int osdialog_message(osdialog_message_level level, osdialog_message_buttons buttons, const char *message);

/** Launches an input prompt with an "OK" and "Cancel" button.

`text` is the default string to fill the input box.

Returns the entered text, or NULL if the dialog was cancelled.
If the returned result is not NULL, caller must free() it.

TODO: Implement on Windows and GTK2.
*/
char *osdialog_prompt(osdialog_message_level level, const char *message, const char *text);

typedef enum {
	OSDIALOG_OPEN,
	OSDIALOG_OPEN_DIR,
	OSDIALOG_SAVE,
} osdialog_file_action;

/** Linked list of patterns. */
typedef struct osdialog_filter_patterns {
	char *pattern;
	struct osdialog_filter_patterns *next;
} osdialog_filter_patterns;

/** Linked list of file filters. */
typedef struct osdialog_filters {
	char *name;
	osdialog_filter_patterns *patterns;
	struct osdialog_filters *next;
} osdialog_filters;

/** Launches a file dialog and returns the selected path or NULL if nothing was selected.

`path` is the default folder the file dialog will attempt to open in, or NULL for the OS's default.
`filename` is the default text that will appear in the filename input, or NULL for the OS's default. Relevant to save dialog only.
`filters` is a list of patterns to filter the file selection, or NULL.

Returns the selected file, or NULL if the dialog was cancelled.
If the return result is not NULL, caller must free() it.
*/
char *osdialog_file(osdialog_file_action action, const char *path, const char *filename, osdialog_filters *filters);

/** Parses a filter string.
Example: "Source:c,cpp,m;Header:h,hpp"
Caller must eventually free with osdialog_filters_free().
*/
osdialog_filters *osdialog_filters_parse(const char *str);
void osdialog_filters_free(osdialog_filters *filters);


typedef struct {
	uint8_t r, g, b, a;
} osdialog_color;

/** Launches an RGBA color picker dialog and sets `color` to the selected color.
Returns 1 if "OK" was pressed.

`color` should be set to the initial color before calling. It is only overwritten if the user selects "OK".
`opacity` enables the opacity slider by setting to 1. Not supported on Windows.

TODO Implement on Mac.
*/
int osdialog_color_picker(osdialog_color *color, int opacity);


#ifdef __cplusplus
}
#endif
