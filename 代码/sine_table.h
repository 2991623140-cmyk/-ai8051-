#ifndef PIANO_SINE_TABLE_H
#define PIANO_SINE_TABLE_H

/* 64-point signed sine, deliberately small for eight-voice headroom. */
static const signed char code sine_64[64] = {
      0, 12, 25, 37, 49, 60, 71, 81, 90, 98,106,112,117,122,125,126,
    127,126,125,122,117,112,106, 98, 90, 81, 71, 60, 49, 37, 25, 12,
      0,-12,-25,-37,-49,-60,-71,-81,-90,-98,-106,-112,-117,-122,-125,-126,
   -127,-126,-125,-122,-117,-112,-106,-98,-90,-81,-71,-60,-49,-37,-25,-12
};

#endif
