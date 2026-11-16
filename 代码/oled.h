#ifndef PIANO_OLED_H
#define PIANO_OLED_H

#include "board.h"
#define OLED_HIDE_UNDERRUNS 65535u

void OLED_Init(void);
void OLED_ShowSong(unsigned char selected, unsigned char state, unsigned char ready);
void OLED_ShowStatus(signed char transpose, unsigned int volume,
                     unsigned char keys, unsigned int underruns);

#endif
