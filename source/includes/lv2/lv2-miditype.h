/****************************************************************************
    
    lv2-miditype.h - header file for using MIDI in LV2 plugins
    
    Copyright (C) 2006  Lars Luthman <lars.luthman@gmail.com>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#ifndef LV2_MIDITYPE_H
#define LV2_MIDITYPE_H


/** This data structure is used to contain the MIDI events for one run() 
    cycle. The port buffer for a LV2 port that has the datatype 
    <http://ll-plugins.nongnu.org/lv2/ext/miditype> should be a pointer 
    to an instance of this struct. 

    To store two Note On events on MIDI channel 0 in a buffer, with timestamps
    12 and 35.5, you could use something like this code (assuming that 
    midi_data is a variable of type LV2_MIDI):
    @code
    
      size_t buffer_offset = 0;
      *(double*)(midi_data->data + buffer_offset) = 12;
      buffer_offset += sizeof(double);
      *(size_t*)(midi_data->data + buffer_offset) = 3;
      buffer_offset += sizeof(size_t);
      midi_data->data[buffer_offset++] = 0x90;
      midi_data->data[buffer_offset++] = 0x48;
      midi_data->data[buffer_offset++] = 0x64;
      ++midi_data->event_count;
      
      *(double*)(midi_data->data + buffer_offset) = 35.5;
      buffer_offset += sizeof(double);
      *(size_t*)(midi_data->data + buffer_offset) = 3;
      buffer_offset += sizeof(size_t);
      midi_data->data[buffer_offset++] = 0x90;
      midi_data->data[buffer_offset++] = 0x55;
      midi_data->data[buffer_offset++] = 0x64;
      ++midi_data->event_count;
      
      midi_data->size = buffer_offset;
      
    @endcode
    
    This would be done by the host in the case of an input port, and by the
    plugin in the case of an output port. Whoever is writing events to the
    buffer must also take care not to exceed the capacity of the data buffer.
    
    To read events from a buffer, you could do something like this:
    @code
    
      size_t buffer_offset = 0;
      uint32_t i;
      for (i = 0; i < midi_data->event_count; ++i) {
        double timestamp = *(double*)(midi_data->data + buffer_offset);
        buffer_offset += sizeof(double);
        size_t size = *(size_t*)(midi_data->data + buffer_offset);
        buffer_offset += sizeof(size_t);
        do_something_with_event(timestamp, size, 
                                midi_data->data + buffer_offset);
        buffer_offset += size;
      }
        
    @endcode
*/
typedef struct {

  /** The number of MIDI events in the data buffer. 
      INPUT PORTS: It's the host's responsibility to set this field to the
      number of MIDI events contained in the data buffer before calling the
      plugin's run() function. The plugin may not change this field.
      OUTPUT PORTS: It's the plugin's responsibility to set this field to the
      number of MIDI events it has stored in the data buffer before returning
      from the run() function. Any initial value should be ignored by the
      plugin.
  */
  uint32_t  event_count;
  
  /** The size of the data buffer in bytes. It is set by the host and may not
      be changed by the plugin. The host is allowed to change this between
      run() calls.
  */
  uint32_t  capacity;
  
  /** The size of the initial part of the data buffer that actually contains
      data.
      INPUT PORTS: It's the host's responsibility to set this field to the
      number of bytes used by all MIDI events it has written to the buffer
      (including timestamps and size fields) before calling the plugin's 
      run() function. The plugin may not change this field.
      OUTPUT PORTS: It's the plugin's responsibility to set this field to
      the number of bytes used by all MIDI events it has written to the
      buffer (including timestamps and size fields) before returning from
      the run() function. Any initial value should be ignored by the plugin.
  */
  uint32_t  size;
  
  /** The data buffer that is used to store MIDI events. The events are packed
      after each other, and the format of each event is as follows:
      
      First there is a timestamp, which should have the type "double",
      i.e. have the same bit size as a double and the same bit layout as a 
      double (whatever that is on the current platform). This timestamp gives 
      the offset from the beginning of the current cycle, in frames, that 
      the MIDI event occurs on. It must be strictly smaller than the 'nframes'
      parameter to the current run() call. The MIDI events in the buffer must 
      be ordered by their timestamp, e.g. an event with a timestamp of 123.23
      must be stored after an event with a timestamp of 65.0.
      
      The second part of the event is a size field, which should have the type
      "size_t" (as defined in the standard C header stddef.h). It should 
      contain the size of the MIDI data for this event, i.e. the number of 
      bytes used to store the actual MIDI event. The bytes used by the 
      timestamp and the size field should not be counted.
      
      The third part of the event is the actual MIDI data. There are some
      requirements that must be followed:
      
      * Running status is not allowed. Every event must have its own status 
        byte.
      * Note On events with velocity 0 are not allowed. These events are 
        equivalent to Note Off in standard MIDI streams, but in order to make
        plugins and hosts easier to write, as well as more efficient, only
        proper Note Off events are allowed as Note Off. 
      * "Realtime events" (status bytes 0xF8 to 0xFF) are allowed, but may not
        occur inside other events like they are allowed to in hardware MIDI 
        streams.
      * All events must be fully contained in a single data buffer, i.e. events
        may not "wrap around" by storing the first few bytes in one buffer and
        then wait for the next run() call to store the rest of the event. If 
        there isn't enough space in the current data buffer to store an event,
        the event will either have to wait until next run() call, be ignored,
        or compensated for in some more clever way.
      * All events must be valid MIDI events. This means for example that
        only the first byte in each event (the status byte) may have the eighth
        bit set, that Note On and Note Off events are always 3 bytes long etc.
        The MIDI writer (host or plugin) is responsible for writing valid MIDI
        events to the buffer, and the MIDI reader (plugin or host) can assume
        that all events are valid.
        
      On a platform where double is 8 bytes and size_t is 4 bytes, the data 
      buffer layout for a 3-byte event followed by a 4-byte event may look 
      something like this:
      _______________________________________________________________
      | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | ...
      |TIMESTAMP 1    |SIZE 1 |DATA |TIMESTAMP 2    |SIZE 2 |DATA   | ...
      
  */
  unsigned char* data;

} LV2_MIDI;



#endif
