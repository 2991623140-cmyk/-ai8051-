#include "board.h"
#include "audio.h"
#include "input.h"
#include "oled.h"
#include "usb_debug.h"
#include "song.h"

static volatile unsigned int tick_ms;
static volatile unsigned char scan_pending;
static unsigned int led_elapsed;

void Timer0_ISR(void) interrupt 1
{
    ++tick_ms;
    scan_pending = 1;
    if (++led_elapsed >= 500u) {
        led_elapsed = 0;
        STATUS_LED = !STATUS_LED;
        P20_LED = !P20_LED;
    }
}

static void Timer_Init(void)
{
    unsigned int reload = (unsigned int)(65536UL - PIANO_FOSC / 1000UL);
    TMOD = (TMOD & 0xf0) | 0x00;
    T0x12 = 1;
    TM0PS = 0;
    TH0 = (unsigned char)(reload >> 8);
    TL0 = (unsigned char)reload;
    ET0 = 1;
    TR0 = 1;
}

void main(void)
{
    unsigned int ui_last = 0;
    signed char step;
    unsigned char test_stage = 0;
    WTST = 0;
    EAXSFR();       /* Required before accessing AI8051U extended SFRs. */
    CKCON = 0;
    P_SW2 |= 0x80; /* Enable extended SFR access. */
    P3M1 &= ~0x80; P3M0 |= 0x80; STATUS_LED = 1;
    P20_LED = 1;
    P2M1 &= ~0x01;
    P2M0 |= 0x01;
    led_elapsed = 0;
    Input_Init();
    Timer_Init();
    EA = 1;
    OLED_Init();
    OLED_ShowStatus(0, 200, 0, OLED_HIDE_UNDERRUNS);
    Audio_Init();
    Song_Init();
    UsbDebug_Init();
    while (1) {
        Audio_Service();
        UsbDebug_Task(tick_ms);
        Song_Task(tick_ms);
        if (scan_pending) {
            scan_pending = 0;
            Input_Scan1ms();
            if (test_stage == 0 && tick_ms >= 100u) { Audio_NoteOn(5); test_stage = 1; }
            if (test_stage == 1 && tick_ms >= 450u) { Audio_NoteOff(5); test_stage = 2; }
            if (test_stage == 2 && tick_ms >= 600u) { Audio_NoteOn(7); test_stage = 3; }
            if (test_stage == 3 && tick_ms >= 950u) { Audio_NoteOff(7); test_stage = 4; }
        }
        step = Input_GetTransposeStep();
        if (step) Audio_SetTranspose((signed char)(Audio_GetTranspose() + step));
        if (Input_GetTransposeButton()) Song_Next();
        if (Input_GetTimbreButton()) {
            unsigned char next = Audio_GetTimbre() + 1u;
            Audio_SetTimbre(next > 10u ? 1u : next);
        }
        step = Input_GetVolumeStep();
        if (step) Audio_SetVolume((signed long)Audio_GetVolume() + (signed long)step * 5L);
        if (Input_GetVolumeButton()) {
            Audio_NoteOff(5);
            Audio_NoteOff(7);
            test_stage = 4;
            Song_Toggle(tick_ms);
        }
        if ((unsigned int)(tick_ms - ui_last) >= 100u) {
            ui_last = tick_ms;
            OLED_ShowStatus(Audio_GetTranspose(), Audio_GetVolume(), Input_GetKeys(), OLED_HIDE_UNDERRUNS);
            OLED_ShowSong(Song_GetSelected(), Song_GetState(), Song_IsReady());
        }
    }
}
