#include "board.h"
#include "audio.h"
#include "input.h"
#include "song.h"
#include "song_data.h"

static unsigned char selected, state, notes[8];
static unsigned int position, started, remaining;

static unsigned int Count(void)
{
    return SongData_Count(selected);
}

static unsigned int EventDelay(unsigned int index)
{
    return SongData_Delay(selected, index);
}

static unsigned char EventNote(unsigned int index)
{
    return SongData_Note(selected, index);
}

static unsigned char EventVelocity(unsigned int index)
{
    return SongData_Velocity(selected, index);
}

static void Release(unsigned char restore)
{
    unsigned char i;
    for (i = 0; i < 8; ++i) {
        Audio_NoteOff(i);
        if (restore && (Input_GetKeys() & (1u << i))) Audio_NoteOn(i);
    }
}

void Song_Init(void)
{
    unsigned char i;
    selected = state = 0;
    position = started = remaining = 0;
    for (i = 0; i < 8; ++i) notes[i] = 255;
}

void Song_Next(void)
{
    unsigned char next = (selected + 1u) % SONG_COUNT;
    Song_Select(next);
}

void Song_Select(unsigned char index)
{
    if (index >= SONG_COUNT) return;
    if (state) Release(1);
    state = 0;
    selected = index;
    position = 0;
}

void Song_Toggle(unsigned int now)
{
    unsigned char i;
    unsigned int elapsed;
    if (!Song_IsReady()) return;
    if (state == 1) {
        elapsed = (unsigned int)(now - started);
        remaining = elapsed < remaining ? remaining - elapsed : 0;
        state = 2;
        Release(1);
    } else {
        Release(0);
        if (!state) {
            position = 0;
            for (i = 0; i < 8; ++i) notes[i] = 255;
            remaining = 0; /* Start at the first note, skipping MIDI leading silence. */
        } else {
            for (i = 0; i < 8; ++i) if (notes[i] != 255) Audio_MidiOn(i, notes[i]);
        }
        state = 1;
        started = now;
    }
}

void Song_Task(unsigned int now)
{
    unsigned char i, budget = 8, note, velocity;
    if (state != 1) return;
    while (budget-- && (unsigned int)(now - started) >= remaining) {
        started += remaining;
        note = EventNote(position);
        velocity = EventVelocity(position);
        if (note != 255) {
            /* Importer assigns slots by channel and note, avoiding cross-track note-offs. */
            if (velocity >= 1u && velocity <= 8u) {
                i = velocity - 1u;
                notes[i] = note;
                Audio_MidiOn(i, note);
            } else if (velocity >= 128u && velocity < 136u) {
                i = velocity - 128u;
                notes[i] = 255;
                Audio_NoteOff(i);
            }
        }
        if (++position >= Count()) { state = 0; Release(1); return; }
        remaining = EventDelay(position);
    }
}

unsigned char Song_GetSelected(void) { return selected; }
unsigned char Song_GetState(void) { return state; }
unsigned char Song_IsReady(void) { return Count() != 0; }
unsigned int Song_GetPosition(void) { return position; }
