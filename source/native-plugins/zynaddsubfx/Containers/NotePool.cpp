#include "NotePool.h"
#include "../Misc/Allocator.h"
#include "../Synth/SynthNote.h"
#include <cstring>
#include <cassert>
#include <iostream>

#define SUSTAIN_BIT 0x04
#define NOTE_MASK   0x03

enum NoteStatus {
    KEY_OFF                    = 0x00,
    KEY_PLAYING                = 0x01,
    KEY_RELEASED_AND_SUSTAINED = 0x02,
    KEY_RELEASED               = 0x03
};


NotePool::NotePool(void)
    :needs_cleaning(0)
{
    memset(ndesc, 0, sizeof(ndesc));
    memset(sdesc, 0, sizeof(sdesc));
}

bool NotePool::NoteDescriptor::playing(void) const
{
    return (status&NOTE_MASK) == KEY_PLAYING;
}

bool NotePool::NoteDescriptor::sustained(void) const
{
    return (status&NOTE_MASK) == KEY_RELEASED_AND_SUSTAINED;
}

bool NotePool::NoteDescriptor::released(void) const
{
    return (status&NOTE_MASK) == KEY_RELEASED;
}

bool NotePool::NoteDescriptor::off(void) const
{
    return (status&NOTE_MASK) == KEY_OFF;
}

void NotePool::NoteDescriptor::setStatus(uint8_t s)
{
    status &= ~NOTE_MASK;
    status |= (NOTE_MASK&s);
}

void NotePool::NoteDescriptor::doSustain(void)
{
    setStatus(KEY_RELEASED_AND_SUSTAINED);
}

bool NotePool::NoteDescriptor::canSustain(void) const
{
    return !(status & SUSTAIN_BIT);
}

void NotePool::NoteDescriptor::makeUnsustainable(void)
{
    status |= SUSTAIN_BIT;
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
        if(ndesc[desc_id].off())
            break;

    if(desc_id != 0) {
        auto &nd = ndesc[desc_id-1];
        if(nd.age == 0 && nd.note == note && nd.sendto == sendto
                && nd.playing() && nd.legatoMirror == legato && nd.canSustain())
            return desc_id-1;
    }

    //Out of free descriptors
    if(desc_id >= POLYPHONY || !ndesc[desc_id].off()) {
        return -1;
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

int NotePool::usedNoteDesc(void) const
{
    if(needs_cleaning)
        const_cast<NotePool*>(this)->cleanup();

    int cnt = 0;
    for(int i=0; i<POLYPHONY; ++i)
        cnt += (ndesc[i].size != 0);
    return cnt;
}

int NotePool::usedSynthDesc(void) const
{
    if(needs_cleaning)
        const_cast<NotePool*>(this)->cleanup();

    int cnt = 0;
    for(int i=0; i<POLYPHONY*EXPECTED_USAGE; ++i)
        cnt += (bool)sdesc[i].note;
    return cnt;
}

void NotePool::insertNote(uint8_t note, uint8_t sendto, SynthDescriptor desc, bool legato)
{
    //Get first free note descriptor
    int desc_id = getMergeableDescriptor(note, sendto, legato, ndesc);
    assert(desc_id != -1);

    ndesc[desc_id].note         = note;
    ndesc[desc_id].sendto       = sendto;
    ndesc[desc_id].size        += 1;
    ndesc[desc_id].status       = KEY_PLAYING;
    ndesc[desc_id].legatoMirror = legato;

    //Get first free synth descriptor
    int sdesc_id = 0;
    while(sdesc[sdesc_id].note && sdesc_id < POLYPHONY*EXPECTED_USAGE)
        sdesc_id++;

    assert(sdesc_id < POLYPHONY*EXPECTED_USAGE);

    sdesc[sdesc_id] = desc;
};

void NotePool::upgradeToLegato(void)
{
    for(auto &d:activeDesc())
        if(d.playing())
            for(auto &s:activeNotes(d))
                insertLegatoNote(d.note, d.sendto, s);
}

void NotePool::insertLegatoNote(uint8_t note, uint8_t sendto, SynthDescriptor desc)
{
    assert(desc.note);
    try {
        desc.note = desc.note->cloneLegato();
        insertNote(note, sendto, desc, true);
    } catch (std::bad_alloc &ba) {
        std::cerr << "failed to insert legato note: " << ba.what() << std::endl;
    }
};

//There should only be one pair of notes which are still playing
void NotePool::applyLegato(LegatoParams &par)
{
    for(auto &desc:activeDesc()) {
        desc.note = par.midinote;
        for(auto &synth:activeNotes(desc))
            try {
                synth.note->legatonote(par);
            } catch (std::bad_alloc& ba) {
                std::cerr << "failed to create legato note: " << ba.what() << std::endl;
            }
    }
}

void NotePool::makeUnsustainable(uint8_t note)
{
    for(auto &desc:activeDesc()) {
        if(desc.note == note) {
            desc.makeUnsustainable();
            if(desc.sustained())
                release(desc);
        }
    }
}

bool NotePool::full(void) const
{
    for(int i=0; i<POLYPHONY; ++i)
        if(ndesc[i].off())
            return false;
    return true;
}

bool NotePool::synthFull(int sdesc_count) const
{
    int actually_free=sizeof(sdesc)/sizeof(sdesc[0]);
    for(const auto &desc:activeDesc()) {
        actually_free -= desc.size;
    }
    return actually_free < sdesc_count;
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
        if(desc.playing() || desc.sustained())
            running[desc.note] = true;
    }

    int running_count = 0;
    for(int i=0; i<256; ++i)
        running_count += running[i];

    return running_count;
}
void NotePool::enforceKeyLimit(int limit)
{
    int notes_to_kill = getRunningNotes() - limit;
    if(notes_to_kill <= 0)
        return;

    NoteDescriptor *to_kill = NULL;
    unsigned oldest = 0;
    for(auto &nd : activeDesc()) {
        if(to_kill == NULL) {
            //There must be something to kill
            oldest  = nd.age;
            to_kill = &nd;
        } else if(to_kill->released() && nd.playing()) {
            //Prefer to kill off a running note
            oldest = nd.age;
            to_kill = &nd;
        } else if(nd.age > oldest && !(to_kill->playing() && nd.released())) {
            //Get an older note when it doesn't move from running to released
            oldest = nd.age;
            to_kill = &nd;
        }
    }

    if(to_kill) {
        auto &tk = *to_kill;
        if(tk.released() || tk.sustained())
            kill(*to_kill);
        else
            entomb(*to_kill);
    }
}

void NotePool::releasePlayingNotes(void)
{
    for(auto &d:activeDesc()) {
        if(d.playing() || d.sustained()) {
            d.setStatus(KEY_RELEASED);
            for(auto s:activeNotes(d))
                s.note->releasekey();
        }
    }
}

void NotePool::release(NoteDescriptor &d)
{
    d.setStatus(KEY_RELEASED);
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
    d.setStatus(KEY_OFF);
    for(auto &s:activeNotes(d))
        kill(s);
}

void NotePool::kill(SynthDescriptor &s)
{
    //printf("Kill synth...\n");
    s.note->memory.dealloc(s.note);
    needs_cleaning = true;
}

void NotePool::entomb(NoteDescriptor &d)
{
    d.setStatus(KEY_RELEASED);
    for(auto &s:activeNotes(d))
        s.note->entomb();
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
        if(!ndesc[i].off())
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
                ndesc[i].setStatus(KEY_OFF);
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
