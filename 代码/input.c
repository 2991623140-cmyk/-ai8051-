#include "input.h"
#include "audio.h"
#include "song.h"

static unsigned char key_stable, key_count[8];
static unsigned char enc_old[2], button_count[2], button_down[2], button_edge[2];
static signed char enc_acc[2], enc_step[2];
static unsigned char timbre_count, timbre_down, timbre_edge;

void Input_Init(void)
{
    unsigned char i;
    P0M0 = 0; P0M1 = 0; P0 = 0xff; P0PU = 0xff;
    P1M0 &= 0xc0; P1M1 &= 0xc0; P1 |= 0x3f; P1PU |= 0x3f;
    TIMBRE_BUTTON = 1;
    P2M0 &= ~0x02; P2M1 |= 0x02;
    P2PU |= 0x02;
    P2PD &= ~0x02;
    P2IE |= 0x02;
    P2NCS &= ~0x02;
    P2BP |= 0x02;
    timbre_count = timbre_down = timbre_edge = 0;
    key_stable = 0;
    for (i = 0; i < 8; ++i) key_count[i] = 0;
    enc_old[0] = P1 & 3;
    enc_old[1] = (P1 >> 3) & 3;
    for (i = 0; i < 2; ++i) {
        enc_acc[i] = enc_step[i] = 0;
        button_count[i] = button_down[i] = button_edge[i] = 0;
    }
}

static void ScanEncoder(unsigned char which, unsigned char ab, unsigned char pressed)
{
    static const signed char code direction[16] = {
         0,-1, 1, 0, 1, 0, 0,-1,
        -1, 0, 0, 1, 0, 1,-1, 0
    };
    signed char acc;
    acc = enc_acc[which] + direction[(enc_old[which] << 2) | ab];
    enc_old[which] = ab;
    if (acc >= 4) { acc -= 4; if (enc_step[which] < 100) ++enc_step[which]; }
    if (acc <= -4) { acc += 4; if (enc_step[which] > -100) --enc_step[which]; }
    enc_acc[which] = acc;
    if (pressed) {
        if (button_count[which] < 5) ++button_count[which];
        if (button_count[which] == 5 && !button_down[which]) {
            button_down[which] = 1;
            button_edge[which] = 1;
        }
    } else {
        button_count[which] = 0;
        button_down[which] = 0;
    }
}

void Input_Scan1ms(void)
{
    unsigned char i, pins = P0, bitmask, sensor_mask, timbre_pressed;
    for (i = 0; i < 8; ++i) {
        bitmask = 1u << i;
        /* The sensor strip is mounted in reverse: P0.7 is logical key 0. */
        sensor_mask = 0x80u >> i;
        if (((pins & sensor_mask) == 0) != ((key_stable & bitmask) != 0)) {
            if (++key_count[i] >= 5) {
                key_count[i] = 0;
                if (pins & sensor_mask) {
                    key_stable &= ~bitmask;
                    if (Song_GetState() != 1) Audio_NoteOff(i);
                } else {
                    key_stable |= bitmask;
                    if (Song_GetState() != 1) Audio_NoteOn(i);
                }
            }
        } else key_count[i] = 0;
    }
    pins = P1;
    ScanEncoder(0, pins & 3, (pins & 4) == 0);
    ScanEncoder(1, (pins >> 3) & 3, (pins & 0x20) == 0);
    /* Debounce press and release; one event per stable falling edge. */
    timbre_pressed = (P2 & 0x02u) == 0u ? 1u : 0u;
    if (timbre_pressed != timbre_down) {
        if (++timbre_count >= 10u) {
            timbre_count = 0;
            timbre_down = timbre_pressed;
            if (timbre_down) timbre_edge = 1;
        }
    } else timbre_count = 0;
}

signed char Input_GetTransposeStep(void) { signed char s = enc_step[0]; enc_step[0] = 0; return s; }
signed char Input_GetVolumeStep(void) { signed char s = enc_step[1]; enc_step[1] = 0; return s; }
unsigned char Input_GetTransposeButton(void) { unsigned char e = button_edge[0]; button_edge[0] = 0; return e; }
unsigned char Input_GetVolumeButton(void) { unsigned char e = button_edge[1]; button_edge[1] = 0; return e; }
unsigned char Input_GetKeys(void) { return key_stable; }
unsigned char Input_GetTimbreButton(void) { unsigned char e = timbre_edge; timbre_edge = 0; return e; }
