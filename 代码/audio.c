#include "audio.h"
#include "sine_table.h"
#include "piano_table.h"
#include "midi_pitch.h"
#include "timbre_tables.h"
#include "more_timbres.h"

#define AUDIO_FRAMES 256u
#define AUDIO_BYTES  (AUDIO_FRAMES * 4u)

typedef struct {
    unsigned int phase;
    unsigned int increment;
    unsigned int envelope;
    unsigned char held;
    unsigned char attack;
    unsigned char midi_note;
    unsigned char timbre;
    unsigned char decay_shift;
    unsigned char release_shift;
    unsigned int attack_step;
} Voice;

static Voice voices[8];
static unsigned int key_level[8] = {80, 80, 80, 80, 42, 35, 34, 31};
static const unsigned int code base_increment[8] = {
    1070, 1201, 1348, 1429, 1604, 1800, 2020, 2140
};
static unsigned char xdata pcm[2][AUDIO_BYTES];
static volatile unsigned char playing;
static volatile unsigned char ready[2];
static volatile unsigned char fill_request;
static volatile signed char transpose;
static volatile unsigned int volume = 200;
static unsigned char timbre = 1;
volatile unsigned int audio_underruns;
volatile unsigned int audio_dma_errors;
volatile unsigned int audio_dma_blocks;
volatile unsigned int audio_output_peak;

static void Dma_Start(unsigned char bank)
{
    unsigned int address = (unsigned int)&pcm[bank][0];
    DMA_I2ST_TXAH = (unsigned char)(address >> 8);
    DMA_I2ST_TXAL = (unsigned char)address;
    DMA_I2ST_AMTH = (AUDIO_BYTES - 1u) >> 8;
    DMA_I2ST_AMT = (unsigned char)(AUDIO_BYTES - 1u);
    DMA_I2ST_CR = 0xc0;
}

/* Vector 63 is redirected to C interrupt 13 by isr.asm. */
void I2ST_DMA_ISR(void) interrupt 13
{
    unsigned char completed, next;
    unsigned char status = DMA_I2ST_STA;
    DMA_I2ST_STA = 0;
    if (status & 4) ++audio_dma_errors;
    if (!(status & 1)) return;
    ++audio_dma_blocks;
    completed = playing;
    next = completed ^ 1u;
    if (ready[next]) {
        playing = next;
        ready[next] = 0;
        fill_request = completed;
    } else {
        ++audio_underruns;
        fill_request = next;
    }
    Dma_Start(playing);
}

static void Render(unsigned char bank)
{
    unsigned int frame, pos = 0;
    unsigned char voice_index;
    unsigned int gain, worst_peak, safe_gain, peak = 0, magnitude;
    unsigned int compensated_gain[8];
    unsigned long envelope_sum = 0;
    unsigned char index, fraction;
    signed int next_wave;
    signed int mixed, sample, wave;
    Voice *v;
    gain = (unsigned int)(((unsigned long)volume * 512UL + 300UL) / 600UL);
    /* Reserve headroom for each held voice's full attack and each release tail. */
    for (voice_index = 0; voice_index < 8; ++voice_index) {
        v = &voices[voice_index];
        envelope_sum += ((unsigned long)(v->held && (v->attack || !v->timbre || v->timbre == 6u || v->timbre == 7u)
                            ? 32767u : v->envelope) * key_level[voice_index] + 99UL) / 100UL;
    }
    worst_peak = (unsigned int)((127UL * envelope_sum) >> 15) + 8u;
    safe_gain = 28000u / worst_peak;
    if (gain > safe_gain) gain = safe_gain;
    for (voice_index = 0; voice_index < 8; ++voice_index) {
        compensated_gain[voice_index] = (unsigned int)(((unsigned long)gain * key_level[voice_index]) / 100UL);
    }
    for (frame = 0; frame < AUDIO_FRAMES; ++frame) {
        mixed = 0;
        for (voice_index = 0; voice_index < 8; ++voice_index) {
            v = &voices[voice_index];
            if (v->envelope == 0 && !v->held) continue;
            if (v->held) {
                if (v->attack || !v->timbre || v->timbre == 6u || v->timbre == 7u) {
                    if (v->envelope < 32767u - v->attack_step) v->envelope += v->attack_step;
                    else { v->envelope = 32767u; v->attack = 0; }
                } else if (v->envelope) {
                    v->envelope -= (v->envelope >> v->decay_shift) + 1u;
                }
            } else {
                if (v->envelope) v->envelope -= (v->envelope >> v->release_shift) + 1u;
            }
            v->phase += v->increment;
            if (v->timbre) {
                index = (unsigned char)(v->phase >> 8);
                fraction = (unsigned char)v->phase;
                if (v->timbre == 2u) {
                    wave = bell_256[index];
                    next_wave = bell_256[(unsigned char)(index + 1u)];
                } else if (v->timbre == 3u) {
                    wave = epiano_256[index];
                    next_wave = epiano_256[(unsigned char)(index + 1u)];
                } else if (v->timbre == 4u) {
                    wave = wood_256[index]; next_wave = wood_256[(unsigned char)(index + 1u)];
                } else if (v->timbre == 5u) {
                    wave = musicbox_256[index]; next_wave = musicbox_256[(unsigned char)(index + 1u)];
                } else if (v->timbre == 6u) {
                    wave = organ_256[index]; next_wave = organ_256[(unsigned char)(index + 1u)];
                } else if (v->timbre == 7u) {
                    wave = strings_256[index]; next_wave = strings_256[(unsigned char)(index + 1u)];
                } else if (v->timbre == 8u) {
                    wave = bass_256[index]; next_wave = bass_256[(unsigned char)(index + 1u)];
                } else if (v->timbre == 9u) {
                    wave = pluck_256[index]; next_wave = pluck_256[(unsigned char)(index + 1u)];
                } else if (v->timbre == 10u) {
                    wave = drum_256[index]; next_wave = drum_256[(unsigned char)(index + 1u)];
                } else {
                    wave = piano_256[index];
                    next_wave = piano_256[(unsigned char)(index + 1u)];
                }
            } else {
                index = (unsigned char)(v->phase >> 10);
                fraction = (unsigned char)(v->phase >> 2);
                wave = sine_64[index];
                next_wave = sine_64[(index + 1u) & 63u];
            }
            wave += (signed int)(((signed long)(next_wave - wave) * fraction) >> 8);
            mixed += (signed int)(((signed long)wave * v->envelope) >> 15)
                     * (signed int)compensated_gain[voice_index];
        }
        /* The envelope bound keeps the amplified mix inside signed 16-bit range. */
        sample = mixed;
        magnitude = (unsigned int)(sample < 0 ? -sample : sample);
        if (magnitude > peak) peak = magnitude;
        pcm[bank][pos++] = (unsigned char)(sample >> 8);
        pcm[bank][pos++] = (unsigned char)sample;
        pcm[bank][pos++] = (unsigned char)(sample >> 8);
        pcm[bank][pos++] = (unsigned char)sample;
    }
    audio_output_peak = peak;
}

void Audio_Init(void)
{
    unsigned char i;
    for (i = 0; i < 8; ++i) {
        voices[i].phase = 0;
        voices[i].increment = base_increment[i];
        voices[i].envelope = 0;
        voices[i].held = 0;
        voices[i].attack = 0;
        voices[i].midi_note = 255;
        voices[i].timbre = timbre;
    }
    playing = 0;
    ready[0] = ready[1] = 0;
    fill_request = 0xff;
    audio_underruns = audio_dma_errors = 0;
    audio_dma_blocks = audio_output_peak = 0;
    Render(0);
    Render(1);
    ready[1] = 1;
    P_SW3 &= 0x3f; /* P3.2 BCLK, P3.4 DIN, P3.5 LRCK. */
    /* Explicitly drive the three transmit pins instead of relying on GPIO bypass. */
    P3M1 &= ~0x34;
    P3M0 |= 0x34;
    P3BP |= 0x34;
    I2S_CLKDIV = 1;
    I2SMD = 0xff;
    I2SMCKDIV = 1;
    /* I2S derives from HSCLK, whose reset divisor is two, not SYSCLK directly. */
    HSCLKDIV = 1;
    I2SCR = 0x02; /* TX DMA, no per-word TX interrupt. */
    I2SPRH = 0;  /* MCLK disabled, ODD=0. */
    I2SPRL = 39; /* 40 MHz / (32 * (2*39)) = 16025.64 Hz. */
    I2SCFGH = 2; /* Master transmit. */
    I2SCFGL = 0; /* Philips I2S, 16-bit data, 16-bit channel. */
    DMA_I2S_ITVH = 0;
    DMA_I2S_ITVL = 0;
    DMA_I2ST_CFG = 0x8b; /* interrupt, high IRQ and bus priority */
    DMA_I2ST_STA = 0;
    I2SCFGH |= 4;
    Dma_Start(0);
}

void Audio_Service(void)
{
    unsigned char bank = fill_request;
    if (bank < 2) {
        fill_request = 0xff;
        Render(bank);
        ready[bank] = 1;
    }
}

static void SetVoiceTimbre(unsigned char key)
{
    static const unsigned char code decay[11] = {14,14,14,13,11,14,14,14,12,12,10};
    static const unsigned char code release[11] = {8,8,13,8,10,13,8,11,8,10,10};
    voices[key].timbre = timbre;
    voices[key].decay_shift = decay[timbre];
    voices[key].release_shift = release[timbre];
    voices[key].attack_step = timbre == 7u ? 16u : timbre == 6u ? 128u : 256u;
    if (timbre == 8u) voices[key].increment >>= 1;
    if (timbre == 10u) voices[key].increment = 450u;
}

void Audio_NoteOn(unsigned char key)
{
    if (key >= 8) return;
    voices[key].midi_note = 255;
    voices[key].timbre = timbre;
    {
        signed int pitch = 60 + (signed int)transpose;
        static const unsigned char code intervals[8] = {0,2,4,5,7,9,11,12};
        voices[key].increment = midi_increment[pitch + intervals[key]];
    }
    SetVoiceTimbre(key);
    voices[key].phase = 0;
    voices[key].envelope = 0;
    voices[key].held = 1;
    voices[key].attack = 1;
}

void Audio_MidiOn(unsigned char slot, unsigned char note)
{
    signed int pitch = (signed int)note + transpose;
    if (slot >= 8 || pitch < 0 || pitch > 127) return;
    voices[slot].midi_note = note;
    voices[slot].timbre = timbre;
    voices[slot].increment = midi_increment[pitch];
    SetVoiceTimbre(slot);
    voices[slot].phase = voices[slot].envelope = 0;
    voices[slot].held = voices[slot].attack = 1;
}

void Audio_NoteOff(unsigned char key)
{
    if (key < 8) voices[key].held = 0;
}

void Audio_SetTranspose(signed char semitone)
{
    unsigned char i;
    signed char step;
    unsigned long increment;
    if (semitone < -24) semitone = -24;
    if (semitone > 24) semitone = 24;
    transpose = semitone;
    for (i = 0; i < 8; ++i) {
        if (voices[i].midi_note != 255) {
            signed int pitch = (signed int)voices[i].midi_note + semitone;
            if (pitch < 0) pitch = 0;
            if (pitch > 127) pitch = 127;
            voices[i].increment = midi_increment[pitch];
            if (voices[i].timbre == 8u) voices[i].increment >>= 1;
            if (voices[i].timbre == 10u) voices[i].increment = 450u;
            continue;
        }
        increment = base_increment[i];
        if (semitone > 0) {
            for (step = 0; step < semitone; ++step) increment = (increment * 4339UL + 2048UL) >> 12;
        } else {
            for (step = 0; step > semitone; --step) increment = (increment * 3866UL + 2048UL) >> 12;
        }
        voices[i].increment = (unsigned int)increment;
        if (voices[i].timbre == 8u) voices[i].increment >>= 1;
        if (voices[i].timbre == 10u) voices[i].increment = 450u;
    }
}

signed char Audio_GetTranspose(void) { return transpose; }
void Audio_SetVolume(signed long percent)
{
    if (percent < 0) percent = 0;
    if (percent > 32767L) percent = 32767L;
    volume = (unsigned int)percent;
}
unsigned int Audio_GetVolume(void) { return volume; }
void Audio_SetTimbre(unsigned char mode) { if (mode < 11) timbre = mode; }
unsigned char Audio_GetTimbre(void) { return timbre; }
void Audio_SetKeyLevel(unsigned char key, unsigned int percent)
{
    if (key < 8 && percent <= 600) key_level[key] = percent;
}
unsigned int Audio_GetKeyLevel(unsigned char key) { return key < 8 ? key_level[key] : 0; }
