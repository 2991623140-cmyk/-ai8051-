#ifndef PIANO_INPUT_H
#define PIANO_INPUT_H

#include "board.h"

void Input_Init(void);
void Input_Scan1ms(void);
signed char Input_GetTransposeStep(void);
signed char Input_GetVolumeStep(void);
unsigned char Input_GetTransposeButton(void);
unsigned char Input_GetVolumeButton(void);
unsigned char Input_GetKeys(void);
unsigned char Input_GetTimbreButton(void);

#endif
