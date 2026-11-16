#include "board.h"
#include "audio.h"
#include "input.h"
#include "song.h"
#include "ai_usb.h"
#include "usb_debug.h"
#include <stdlib.h>
#include <string.h>

static char command[24];
static unsigned char command_length, discard_command, report_stage;
static unsigned int report_interval;
static unsigned int last_report;
static char report[64];
static unsigned int current_time;

static void Execute(void)
{
    signed long value;
    unsigned char key, i, start = 1, number_start;
    if (!command_length) return;
    command[command_length] = 0;
    if (command[0] == '?' && command_length == 1) {
        report_stage = 1;
        return;
    }
    if (command[0] == 'X' && command_length == 1) {
        Song_Select(Song_GetSelected());
        for (key = 0; key < 8; ++key) Audio_NoteOff(key);
        report_stage = 1;
        return;
    }
    if (command[0] == 'P' && command_length == 1) {
        Song_Toggle(current_time);
        report_stage = 1;
        return;
    }
    if (command[0] == 'B') {
        if (command_length < 4 || command[1] < '0' || command[1] > '7' || command[2] != '=') return;
        start = 3;
    }
    number_start = start;
    if (command[start] == '-' || command[start] == '+') ++start;
    if (command_length <= start || command_length - start > 5) return;
    value = 0;
    for (i = start; i < command_length; ++i) {
        if (command[i] < '0' || command[i] > '9') return;
        value = value * 10L + (command[i] - '0');
    }
    if (command[number_start] == '-') value = -value;
    switch (command[0]) {
    case 'V':
        if (value >= 0 && value <= 32767L) Audio_SetVolume(value);
        break;
    case 'T':
        if (value >= -24 && value <= 24) Audio_SetTranspose((signed char)value);
        break;
    case 'N':
        if (value >= 0 && value < 8) Audio_NoteOn((unsigned char)value);
        break;
    case 'F':
        if (value >= 0 && value < 8) Audio_NoteOff((unsigned char)value);
        break;
    case 'C':
        if (value >= 0 && value <= 255) {
            for (key = 0; key < 8; ++key) {
                if ((unsigned int)value & (1u << key)) Audio_NoteOn(key);
                else Audio_NoteOff(key);
            }
        }
        break;
    case 'W':
        if (value >= 0 && value < 11) Audio_SetTimbre((unsigned char)value);
        break;
    case 'B':
        if (value >= 0 && value <= 600) Audio_SetKeyLevel((unsigned char)(command[1] - '0'), (unsigned int)value);
        break;
    case 'R':
        if (value >= 0 && value <= 10) report_interval = (unsigned int)value * 1000u;
        break;
    case 'G':
        if (value >= 1 && value <= 5) Song_Select((unsigned char)(value - 1));
        break;
    default: return;
    }
    report_stage = 1;
}

void UsbDebug_Init(void)
{
    command_length = discard_command = report_stage = 0;
    last_report = 0;
    report_interval = 0;
    usb_init();
    PUSB = 0;
    PUSBH = 0;
    set_usb_ispcmd("@STCISP#");
}

void UsbDebug_Task(unsigned int now)
{
    unsigned char i, c, old_ea;
    unsigned int peak, blocks, underruns, errors;
    current_time = now;
    if (DeviceState != DEVSTATE_CONFIGURED) {
        command_length = discard_command = 0;
        return;
    }
    if (bUsbOutReady) {
        for (i = 0; i < OutNumber; ++i) {
            c = UsbOutBuffer[i];
            if (c == '\r' || c == '\n') {
                if (!discard_command) Execute();
                command_length = discard_command = 0;
            } else if (!discard_command) {
                if (command_length < sizeof(command) - 1u) command[command_length++] = c;
                else discard_command = 1;
            }
        }
        usb_OUT_done();
    }
    if (report_interval && !report_stage && (unsigned int)(now - last_report) >= report_interval) report_stage = 1;
    if (!report_stage || bUsbInBusy) return;
    if (report_stage == 1) {
        sprintf(report, "VOL=%u TR=%d KEYS=%u WAVE=%u HS=%u\r\n", Audio_GetVolume(),
                (signed int)Audio_GetTranspose(), (unsigned int)Input_GetKeys(),
                (unsigned int)Audio_GetTimbre(), (unsigned int)HSCLKDIV);
        report_stage = 2;
    } else if (report_stage == 2) {
        old_ea = EA;
        EA = 0;
        peak = audio_output_peak;
        blocks = audio_dma_blocks;
        underruns = audio_underruns;
        errors = audio_dma_errors;
        EA = old_ea;
        sprintf(report, "PEAK=%u DMA=%u UND=%u ERR=%u\r\n", peak, blocks, underruns, errors);
        report_stage = 3;
    } else if (report_stage == 3) {
        sprintf(report, "BAL=%u,%u,%u,%u,%u,%u,%u,%u\r\n",
                (unsigned int)Audio_GetKeyLevel(0), (unsigned int)Audio_GetKeyLevel(1),
                (unsigned int)Audio_GetKeyLevel(2), (unsigned int)Audio_GetKeyLevel(3),
                (unsigned int)Audio_GetKeyLevel(4), (unsigned int)Audio_GetKeyLevel(5),
                (unsigned int)Audio_GetKeyLevel(6), (unsigned int)Audio_GetKeyLevel(7));
        report_stage = 4;
    } else {
        sprintf(report, "FW=ALL5 SONG=%u STATE=%u READY=%u POS=%u BTN=%u K=%u\r\n",
                (unsigned int)Song_GetSelected() + 1u, (unsigned int)Song_GetState(),
                (unsigned int)Song_IsReady(), Song_GetPosition(), (unsigned int)(P1 & 0x24),
                (unsigned int)((P2 & 2u) != 0u));
        report_stage = 0;
        last_report = now;
    }
    USB_SendData((BYTE *)report, (int)strlen(report));
}
