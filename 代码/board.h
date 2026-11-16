#ifndef PIANO_BOARD_H
#define PIANO_BOARD_H

#include "AI8051U.H"

/* Set the chip's operating clock to 40 MHz in the downloader. */
#define PIANO_FOSC 40000000UL
#define PIANO_FS   16025UL

/* The selected I2S mapping is P3.2 BCLK, P3.4 DIN, P3.5 LRCK. */
sbit OLED_SCL = P2^4;
sbit OLED_SDA = P2^5;
sbit STATUS_LED = P3^7;
sbit P20_LED = P2^0;
sbit TIMBRE_BUTTON = P2^1;

/* P0.7..P0.0: logical keys 0..7, active-low (sensor strip reversed). */
/* P1.0/1/2: transpose A/B/button; P1.3/4/5: volume A/B/button. */

#endif
