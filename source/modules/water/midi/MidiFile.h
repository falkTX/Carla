/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2022 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

  ==============================================================================
*/

#ifndef WATER_MIDIFILE_H_INCLUDED
#define WATER_MIDIFILE_H_INCLUDED

#include "MidiMessageSequence.h"

namespace water {

//==============================================================================
/**
    Reads standard midi format files.

    To read a midi file, create a MidiFile object and call its readFrom() method. You
    can then get the individual midi tracks from it using the getTrack() method.

    @see MidiMessageSequence
*/
class MidiFile
{
public:
    //==============================================================================
    /** Creates an empty MidiFile object.
    */
    MidiFile();

    /** Destructor. */
    ~MidiFile();

    /** Creates a copy of another MidiFile. */
    MidiFile (const MidiFile& other);

    /** Copies from another MidiFile object */
    MidiFile& operator= (const MidiFile& other);

    //==============================================================================
    /** Returns the number of tracks in the file.
        @see getTrack, addTrack
    */
    size_t getNumTracks() const noexcept;

    /** Returns a pointer to one of the tracks in the file.
        @returns a pointer to the track, or nullptr if the index is out-of-range
        @see getNumTracks, addTrack
    */
    const MidiMessageSequence* getTrack (size_t index) const noexcept;

    /** Adds a midi track to the file.
        This will make its own internal copy of the sequence that is passed-in.
        @see getNumTracks, getTrack
    */
    void addTrack (const MidiMessageSequence& trackSequence);

    /** Removes all midi tracks from the file.
        @see getNumTracks
    */
    void clear();

    /** Returns the raw time format code that will be written to a stream.

        After reading a midi file, this method will return the time-format that
        was read from the file's header. It can be changed using the setTicksPerQuarterNote()
        or setSmpteTimeFormat() methods.

        If the value returned is positive, it indicates the number of midi ticks
        per quarter-note - see setTicksPerQuarterNote().

        It it's negative, the upper byte indicates the frames-per-second (but negative), and
        the lower byte is the number of ticks per frame - see setSmpteTimeFormat().
    */
    short getTimeFormat() const noexcept;

    /** Sets the time format to use when this file is written to a stream.

        If this is called, the file will be written as bars/beats using the
        specified resolution, rather than SMPTE absolute times, as would be
        used if setSmpteTimeFormat() had been called instead.

        @param ticksPerQuarterNote  e.g. 96, 960
        @see setSmpteTimeFormat
    */
    void setTicksPerQuarterNote (int ticksPerQuarterNote) noexcept;

    /** Sets the time format to use when this file is written to a stream.

        If this is called, the file will be written using absolute times, rather
        than bars/beats as would be the case if setTicksPerBeat() had been called
        instead.

        @param framesPerSecond      must be 24, 25, 29 or 30
        @param subframeResolution   the sub-second resolution, e.g. 4 (midi time code),
                                    8, 10, 80 (SMPTE bit resolution), or 100. For millisecond
                                    timing, setSmpteTimeFormat (25, 40)
        @see setTicksPerBeat
    */
    void setSmpteTimeFormat (int framesPerSecond,
                             int subframeResolution) noexcept;

    //==============================================================================
    /** Makes a list of all the tempo-change meta-events from all tracks in the midi file.
        Useful for finding the positions of all the tempo changes in a file.
        @param tempoChangeEvents    a list to which all the events will be added
    */
    void findAllTempoEvents (MidiMessageSequence& tempoChangeEvents) const;

    /** Makes a list of all the time-signature meta-events from all tracks in the midi file.
        Useful for finding the positions of all the tempo changes in a file.
        @param timeSigEvents        a list to which all the events will be added
    */
    void findAllTimeSigEvents (MidiMessageSequence& timeSigEvents) const;

    /** Makes a list of all the time-signature meta-events from all tracks in the midi file.
        @param keySigEvents         a list to which all the events will be added
    */
    void findAllKeySigEvents (MidiMessageSequence& keySigEvents) const;

    /** Returns the latest timestamp in any of the tracks.
        (Useful for finding the length of the file).
    */
    double getLastTimestamp() const;

    //==============================================================================
    /** Reads a midi file format stream.

        After calling this, you can get the tracks that were read from the file by using the
        getNumTracks() and getTrack() methods.

        The timestamps of the midi events in the tracks will represent their positions in
        terms of midi ticks. To convert them to seconds, use the convertTimestampTicksToSeconds()
        method.

        @returns true if the stream was read successfully
    */
    bool readFrom (InputStream& sourceStream);

    /** Converts the timestamp of all the midi events from midi ticks to seconds.

        This will use the midi time format and tempo/time signature info in the
        tracks to convert all the timestamps to absolute values in seconds.
    */
    void convertTimestampTicksToSeconds();


private:
    //==============================================================================
    OwnedArray<MidiMessageSequence> tracks;
    short timeFormat;

    void readNextTrack (const uint8*, int size);
};

}

#endif // WATER_MIDIFILE_H_INCLUDED
