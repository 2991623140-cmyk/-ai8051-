#ifndef PIANO_SONG_H
#define PIANO_SONG_H

void Song_Init(void);
void Song_Next(void);
void Song_Toggle(unsigned int now);
void Song_Task(unsigned int now);
unsigned char Song_GetSelected(void);
unsigned char Song_GetState(void);
unsigned char Song_IsReady(void);
unsigned int Song_GetPosition(void);
void Song_Select(unsigned char index);

#endif
