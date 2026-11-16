#ifndef PIANO_AUDIO_H
#define PIANO_AUDIO_H

#include "board.h"

void Audio_Init(void);
void Audio_Service(void);
void Audio_NoteOn(unsigned char key);
void Audio_NoteOff(unsigned char key);
void Audio_MidiOn(unsigned char slot, unsigned char note);
void Audio_SetTranspose(signed char semitone);
signed char Audio_GetTranspose(void);
void Audio_SetVolume(signed long percent);
unsigned int Audio_GetVolume(void);
void Audio_SetTimbre(unsigned char mode);
unsigned char Audio_GetTimbre(void);
void Audio_SetKeyLevel(unsigned char key, unsigned int percent);
unsigned int Audio_GetKeyLevel(unsigned char key);
extern volatile unsigned int audio_underruns;
extern volatile unsigned int audio_dma_errors;
extern volatile unsigned int audio_dma_blocks;
extern volatile unsigned int audio_output_peak;

#endif
