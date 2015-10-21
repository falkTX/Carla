#pragma once
#include <stdint.h>
#include <functional>
#include "../globals.h"

//Expected upper bound of synths given that max polyphony is hit
#define EXPECTED_USAGE 3

struct LegatoParams;
class NotePool
{
    public:
        typedef uint8_t note_t;
        //Currently this wastes a ton of bits due ot the legatoMirror flag
        struct NoteDescriptor {
            //acceptable overlap after 2 minutes
            //run time at 48kHz 8 samples per buffer
            //19 bit minimum
            uint32_t age;
            uint8_t note;
            uint8_t sendto;
            //max of 16 kit elms and 3 kit items per
            uint8_t size;
            uint8_t status;
            bool    legatoMirror;
            bool operator==(NoteDescriptor);
        };

        //To be pedantic this wastes 2 or 6 bytes per descriptor
        //depending on 32bit/64bit alignment rules
        struct SynthDescriptor {
            SynthNote *note;
            uint8_t type;
            uint8_t kit;
        };


        //Pool of notes
        NoteDescriptor   ndesc[POLYPHONY];
        SynthDescriptor  sdesc[POLYPHONY*EXPECTED_USAGE];
        bool             needs_cleaning;


        //Iterators
        struct activeNotesIter {
            SynthDescriptor *begin() {return _b;};
            SynthDescriptor *end() {return _e;};
            SynthDescriptor *_b;
            SynthDescriptor *_e;
        };

        struct activeDescIter {
            activeDescIter(NotePool &_np):np(_np)
            {
                int off=0;
                for(int i=0; i<POLYPHONY; ++i, ++off)
                    if(np.ndesc[i].status == 0)
                        break;
                _end = np.ndesc+off;
            }
            NoteDescriptor *begin() {return np.ndesc;};
            NoteDescriptor *end() { return _end; };
            NoteDescriptor *_end;
            NotePool &np;
        };

        struct constActiveDescIter {
            constActiveDescIter(const NotePool &_np):np(_np)
            {
                int off=0;
                for(int i=0; i<POLYPHONY; ++i, ++off)
                    if(np.ndesc[i].status == 0)
                        break;
                _end = np.ndesc+off;
            }
            const NoteDescriptor *begin() const {return np.ndesc;};
            const NoteDescriptor *end() const { return _end; };
            const NoteDescriptor *_end;
            const NotePool &np;
        };

        activeNotesIter activeNotes(NoteDescriptor &n);

        activeDescIter activeDesc(void);
        constActiveDescIter activeDesc(void) const;

        NotePool(void);

        //Operations
        void insertNote(uint8_t note, uint8_t sendto, SynthDescriptor desc, bool legato=false);
        void insertLegatoNote(uint8_t note, uint8_t sendto, SynthDescriptor desc);

        void upgradeToLegato(void);
        void applyLegato(LegatoParams &par);

        //Note that isn't KEY_PLAYING or KEY_RELASED_AND_SUSTAINING
        bool existsRunningNote(void) const;
        int getRunningNotes(void) const;
        int enforceKeyLimit(int limit) const;

        void releasePlayingNotes(void);
        void releaseNote(note_t note);
        void release(NoteDescriptor &d);

        void killAllNotes(void);
        void killNote(note_t note);
        void kill(NoteDescriptor &d);
        void kill(SynthDescriptor &s);

        void cleanup(void);

        void dump(void);
};
