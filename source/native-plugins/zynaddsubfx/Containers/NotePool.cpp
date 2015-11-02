#include "NotePool.h"
//XXX eliminate dependence on Part.h
#include "../Misc/Part.h"
#include "../Misc/Allocator.h"
#include "../Synth/SynthNote.h"
#include <cstring>
#include <cassert>

NotePool::NotePool(void)
    :needs_cleaning(0)
{
    memset(ndesc, 0, sizeof(ndesc));
    memset(sdesc, 0, sizeof(ndesc));
}
NotePool::activeNotesIter NotePool::activeNotes(NoteDescriptor &n)
{
    const int off_d1 = &n-ndesc;
    int off_d2 = 0;
    assert(off_d1 <= POLYPHONY);
    for(int i=0; i<off_d1; ++i)
        off_d2 += ndesc[i].size;
    return NotePool::activeNotesIter{sdesc+off_d2,sdesc+off_d2+n.size};
}

bool NotePool::NoteDescriptor::operator==(NoteDescriptor nd)
{
    return age == nd.age && note == nd.note && sendto == nd.sendto && size == nd.size && status == nd.status;
}

//return either the first unused descriptor or the last valid descriptor which
//matches note/sendto
static int getMergeableDescriptor(uint8_t note, uint8_t sendto, bool legato,
        NotePool::NoteDescriptor *ndesc)
{
    int desc_id = 0;
    for(int i=0; i<POLYPHONY; ++i, ++desc_id)
        if(ndesc[desc_id].status == Part::KEY_OFF)
            break;

    //Out of free descriptors
    if(ndesc[desc_id].status != Part::KEY_OFF)
        return -1;

    if(desc_id != 0) {
        auto &nd = ndesc[desc_id-1];
        if(nd.age == 0 && nd.note == note && nd.sendto == sendto
                && nd.status == Part::KEY_PLAYING && nd.legatoMirror == legato)
            return desc_id-1;
    }
    return desc_id;
}
        
NotePool::activeDescIter      NotePool::activeDesc(void)
{
    cleanup();
    return activeDescIter{*this};
}

NotePool::constActiveDescIter NotePool::activeDesc(void) const
{
    const_cast<NotePool*>(this)->cleanup();
    return constActiveDescIter{*this};
}

void NotePool::insertNote(uint8_t note, uint8_t sendto, SynthDescriptor desc, bool legato)
{
    //Get first free note descriptor
    int desc_id = getMergeableDescriptor(note, sendto, legato, ndesc);
    assert(desc_id != -1);

    ndesc[desc_id].note         = note;
    ndesc[desc_id].sendto       = sendto;
    ndesc[desc_id].size        += 1;
    ndesc[desc_id].status       = Part::KEY_PLAYING;
    ndesc[desc_id].legatoMirror = legato;

    //Get first free synth descriptor
    int sdesc_id = 0;
    while(sdesc[sdesc_id].note)
        sdesc_id++;
    sdesc[sdesc_id] = desc;
};

void NotePool::upgradeToLegato(void)
{
    for(auto &d:activeDesc())
        if(d.status == Part::KEY_PLAYING)
            for(auto &s:activeNotes(d))
                insertLegatoNote(d.note, d.sendto, s);
}

void NotePool::insertLegatoNote(uint8_t note, uint8_t sendto, SynthDescriptor desc)
{
    assert(desc.note);
    desc.note = desc.note->cloneLegato();
    insertNote(note, sendto, desc, true);
};

//There should only be one pair of notes which are still playing
void NotePool::applyLegato(LegatoParams &par)
{
    for(auto &desc:activeDesc()) {
        desc.note = par.midinote;
        for(auto &synth:activeNotes(desc))
            synth.note->legatonote(par);
    }
};

bool NotePool::full(void) const
{
    for(int i=0; i<POLYPHONY; ++i)
        if(ndesc[i].status == Part::KEY_OFF)
            return false;
    return true;
}

//Note that isn't KEY_PLAYING or KEY_RELASED_AND_SUSTAINING
bool NotePool::existsRunningNote(void) const
{
    //printf("runing note # =%d\n", getRunningNotes());
    return getRunningNotes();
}

int NotePool::getRunningNotes(void) const
{
    bool running[256] = {0};
    for(auto &desc:activeDesc()) {
        //printf("note!(%d)\n", desc.note);
        if(desc.status == Part::KEY_PLAYING)
            running[desc.note] = true;
    }

    int running_count = 0;
    for(int i=0; i<256; ++i)
        running_count += running[i];

    return running_count;
}
int NotePool::enforceKeyLimit(int limit) const
{
        //{
        //int oldestnotepos = -1;
        //if(notecount > keylimit)   //find out the oldest note
        //    for(int i = 0; i < POLYPHONY; ++i) {
        //        int maxtime = 0;
        //        if(((partnote[i].status == KEY_PLAYING) || (partnote[i].status == KEY_RELEASED_AND_SUSTAINED)) && (partnote[i].time > maxtime)) {
        //            maxtime = partnote[i].time;
        //            oldestnotepos = i;
        //        }
        //    }
        //if(oldestnotepos != -1)
        //    ReleaseNotePos(oldestnotepos);
        //}
    printf("Unimplemented enforceKeyLimit()\n");
    return -1;
}

void NotePool::releasePlayingNotes(void)
{
    for(auto &d:activeDesc()) {
        if(d.status == Part::KEY_PLAYING) {
            d.status = Part::KEY_RELEASED;
            for(auto s:activeNotes(d))
                s.note->releasekey();
        }
    }
}

void NotePool::release(NoteDescriptor &d)
{
    d.status = Part::KEY_RELEASED;
    for(auto s:activeNotes(d))
        s.note->releasekey();
}

void NotePool::killAllNotes(void)
{
    for(auto &d:activeDesc())
        kill(d);
}

void NotePool::killNote(uint8_t note)
{
    for(auto &d:activeDesc()) {
        if(d.note == note)
            kill(d);
    }
}

void NotePool::kill(NoteDescriptor &d)
{
    d.status = Part::KEY_OFF;
    for(auto &s:activeNotes(d))
        kill(s);
}

void NotePool::kill(SynthDescriptor &s)
{
    //printf("Kill synth...\n");
    s.note->memory.dealloc(s.note);
    needs_cleaning = true;
}

const char *getStatus(int status_bits)
{
    switch(status_bits)
    {
        case 0:  return "OFF ";
        case 1:  return "PLAY";
        case 2:  return "SUST";
        case 3:  return "RELA";
        default: return "INVD";
    }
}

void NotePool::cleanup(void)
{
    if(!needs_cleaning)
        return;
    needs_cleaning = false;
    int new_length[POLYPHONY] = {0};
    int cur_length[POLYPHONY] = {0};
    //printf("Cleanup Start\n");
    //dump();

    //Identify the current length of all segments
    //and the lengths discarding invalid entries

    int last_valid_desc = 0;
    for(int i=0; i<POLYPHONY; ++i)
        if(ndesc[i].status != Part::KEY_OFF)
            last_valid_desc = i;

    //Find the real numbers of allocated notes
    {
        int cum_old = 0;

        for(int i=0; i<=last_valid_desc; ++i) {
            cur_length[i] = ndesc[i].size;
            for(int j=0; j<ndesc[i].size; ++j)
                new_length[i] += (bool)sdesc[cum_old++].note;
        }
    }


    //Move the note descriptors
    {
        int cum_new = 0;
        for(int i=0; i<=last_valid_desc; ++i) {
            ndesc[i].size = new_length[i];
            if(new_length[i] != 0)
                ndesc[cum_new++] = ndesc[i];
            else
                ndesc[i].status = Part::KEY_OFF;
        }
        memset(ndesc+cum_new, 0, sizeof(*ndesc)*(POLYPHONY-cum_new));
    }

    //Move the synth descriptors
    {
        int total_notes=0;
        for(int i=0; i<=last_valid_desc; ++i)
            total_notes+=cur_length[i];

        int cum_new = 0;
        for(int i=0; i<total_notes; ++i)
            if(sdesc[i].note)
                sdesc[cum_new++] = sdesc[i];
        memset(sdesc+cum_new, 0, sizeof(*sdesc)*(POLYPHONY*EXPECTED_USAGE-cum_new));
    }
    //printf("Cleanup Done\n");
    //dump();
}

void NotePool::dump(void)
{
    printf("NotePool::dump<\n");
    const char *format =
        "    Note %d:%d age(%d) note(%d) sendto(%d) status(%s) legato(%d) type(%d) kit(%d) ptr(%p)\n";
    int note_id=0;
    int descriptor_id=0;
    for(auto &d:activeDesc()) {
        descriptor_id += 1;
        for(auto &s:activeNotes(d)) {
            note_id += 1;
            printf(format,
                    note_id, descriptor_id,
                    d.age, d.note, d.sendto,
                    getStatus(d.status), d.legatoMirror, s.type, s.kit, s.note);
        }
    }
    printf(">NotePool::dump\n");
}
