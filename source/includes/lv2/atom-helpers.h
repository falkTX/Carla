// lv2_atom_helpers.h
//
/****************************************************************************
   Copyright (C) 2005-2013, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

/*  Helper functions for LV2 atom:Sequence event buffer.
 *
 *  tentatively adapted from:
 *
 *  - lv2_evbuf.h,c - An abstract/opaque LV2 event buffer implementation.
 *
 *  - event-helpers.h - Helper functions for the LV2 Event extension.
 *    <http://lv2plug.in/ns/ext/event>
 *
 *    Copyright 2008-2012 David Robillard <http://drobilla.net>
 */

#ifndef LV2_ATOM_HELPERS_H
#define LV2_ATOM_HELPERS_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "atom-util.h"

#ifdef __cplusplus
extern "C" {
#endif

// An abstract/opaque LV2 atom:Sequence buffer.
//
typedef
struct _LV2_Atom_Buffer
{
	uint32_t capacity;
	uint32_t chunk_type;
	uint32_t sequence_type;
	uint32_t _alignment_padding;
	LV2_Atom_Sequence atoms;

} LV2_Atom_Buffer;


// Clear and initialize an existing LV2 atom:Sequenece buffer.
//
static inline
void lv2_atom_buffer_reset ( LV2_Atom_Buffer *buf, bool input )
{
	if (input) {
		buf->atoms.atom.size = sizeof(LV2_Atom_Sequence_Body);
		buf->atoms.atom.type = buf->sequence_type;
	} else {
		buf->atoms.atom.size = buf->capacity;
		buf->atoms.atom.type = buf->chunk_type;
	}
}


// Allocate a new, empty LV2 atom:Sequence buffer.
//
static inline
LV2_Atom_Buffer *lv2_atom_buffer_new (
	uint32_t capacity, uint32_t chunk_type, uint32_t sequence_type, bool input )
{
	LV2_Atom_Buffer *buf = (LV2_Atom_Buffer *)
		malloc(sizeof(LV2_Atom_Buffer) + sizeof(LV2_Atom_Sequence) + capacity);

	buf->capacity = capacity;
	buf->chunk_type = chunk_type;
	buf->sequence_type = sequence_type;

	lv2_atom_buffer_reset(buf, input);

	return buf;
}


// Free an LV2 atom:Sequenece buffer allocated with lv2_atome_buffer_new.
//
static inline
void lv2_atom_buffer_free ( LV2_Atom_Buffer *buf )
{
	free(buf);
}


// Return the total padded size of events stored in a LV2 atom:Sequence buffer.
//
static inline
uint32_t lv2_atom_buffer_get_size ( LV2_Atom_Buffer *buf )
{
	if (buf->atoms.atom.type == buf->sequence_type)
		return buf->atoms.atom.size - uint32_t(sizeof(LV2_Atom_Sequence_Body));
	else
		return 0;
}


// Return the actual LV2 atom:Sequence implementation.
//
static inline
LV2_Atom_Sequence *lv2_atom_buffer_get_sequence ( LV2_Atom_Buffer *buf )
{
	return &buf->atoms;
}


// An iterator over an atom:Sequence buffer.
//
typedef
struct _LV2_Atom_Buffer_Iterator
{
	LV2_Atom_Buffer *buf;
	uint32_t offset;

} LV2_Atom_Buffer_Iterator;


// Reset an iterator to point to the start of an LV2 atom:Sequence buffer.
//
static inline
bool lv2_atom_buffer_begin (
	LV2_Atom_Buffer_Iterator *iter, LV2_Atom_Buffer *buf )
{
	iter->buf = buf;
	iter->offset = 0;

	return (buf->atoms.atom.size > 0);
}


// Reset an iterator to point to the end of an LV2 atom:Sequence buffer.
//
static inline
bool lv2_atom_buffer_end (
	LV2_Atom_Buffer_Iterator *iter, LV2_Atom_Buffer *buf )
{
	iter->buf = buf;
	iter->offset = lv2_atom_pad_size(lv2_atom_buffer_get_size(buf));

	return (iter->offset < buf->capacity - sizeof(LV2_Atom_Event));
}


// Check if a LV2 atom:Sequenece buffer iterator is valid.
//
static inline
bool lv2_atom_buffer_is_valid ( LV2_Atom_Buffer_Iterator *iter )
{
	return iter->offset < lv2_atom_buffer_get_size(iter->buf);
}


// Advance a LV2 atom:Sequenece buffer iterator forward one event.
//
static inline
bool lv2_atom_buffer_increment ( LV2_Atom_Buffer_Iterator *iter )
{
	if (!lv2_atom_buffer_is_valid(iter))
		return false;

	LV2_Atom_Buffer *buf = iter->buf;
	LV2_Atom_Sequence *atoms = &buf->atoms;
	uint32_t size = ((LV2_Atom_Event *) ((char *)
		LV2_ATOM_CONTENTS(LV2_Atom_Sequence, atoms) + iter->offset))->body.size;
	iter->offset += lv2_atom_pad_size(uint32_t(sizeof(LV2_Atom_Event)) + size);

	return true;
}


// Get the event currently pointed at a LV2 atom:Sequence buffer iterator.
//
static inline
LV2_Atom_Event *lv2_atom_buffer_get (
	LV2_Atom_Buffer_Iterator *iter, uint8_t **data )
{
	if (!lv2_atom_buffer_is_valid(iter))
		return NULL;

	LV2_Atom_Buffer *buf = iter->buf;
	LV2_Atom_Sequence *atoms = &buf->atoms;
	LV2_Atom_Event *ev = (LV2_Atom_Event *) ((char *)
		LV2_ATOM_CONTENTS(LV2_Atom_Sequence, atoms) + iter->offset);

	*data = (uint8_t *) LV2_ATOM_BODY(&ev->body);

	return ev;
}


// Write an event at a LV2 atom:Sequence buffer iterator.

#if defined(__GNUC__) && __GNUC__ > 7
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

static inline
bool lv2_atom_buffer_write (
	LV2_Atom_Buffer_Iterator *iter,
	uint32_t frames,
	uint32_t /*subframes*/,
	uint32_t type,
	uint32_t size,
	const uint8_t *data )
{
	LV2_Atom_Buffer *buf = iter->buf;
	LV2_Atom_Sequence *atoms = &buf->atoms;
	if  (buf->capacity - sizeof(LV2_Atom) - atoms->atom.size
		< sizeof(LV2_Atom_Event) + size)
		return false;

	LV2_Atom_Event *ev = (LV2_Atom_Event*) ((char *)
		LV2_ATOM_CONTENTS(LV2_Atom_Sequence, atoms) + iter->offset);

	ev->time.frames = frames;
	ev->body.type = type;
	ev->body.size = size;

	memcpy(LV2_ATOM_BODY(&ev->body), data, size);

	size = lv2_atom_pad_size(uint32_t(sizeof(LV2_Atom_Event)) + size);
	atoms->atom.size += size;
	iter->offset += size;

	return true;
}

#if defined(__GNUC__) && __GNUC__ > 7
# pragma GCC diagnostic pop
#endif

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif	// LV2_ATOM_HELPERS_H

// end of lv2_atom_helpers.h
