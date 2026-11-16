#ifndef PIANO_MIDI_PITCH_H
#define PIANO_MIDI_PITCH_H
/* Frequencies above Nyquist are capped; supplied melodies use notes 60..86. */
static const unsigned int code midi_increment[128] = {
    33u, 35u, 38u, 40u, 42u, 45u, 47u, 50u, 53u, 56u, 60u, 63u, 67u, 71u, 75u, 80u,
    84u, 89u, 95u, 100u, 106u, 112u, 119u, 126u, 134u, 142u, 150u, 159u, 168u, 179u, 189u, 200u,
    212u, 225u, 238u, 252u, 267u, 283u, 300u, 318u, 337u, 357u, 378u, 401u, 425u, 450u, 477u, 505u,
    535u, 567u, 600u, 636u, 674u, 714u, 757u, 802u, 849u, 900u, 953u, 1010u, 1070u, 1134u, 1201u, 1272u,
    1348u, 1428u, 1513u, 1603u, 1698u, 1799u, 1906u, 2020u, 2140u, 2267u, 2402u, 2545u, 2696u, 2856u, 3026u, 3206u,
    3397u, 3599u, 3813u, 4039u, 4280u, 4534u, 4804u, 5089u, 5392u, 5713u, 6052u, 6412u, 6793u, 7197u, 7625u, 8079u,
    8559u, 9068u, 9607u, 10179u, 10784u, 11425u, 12105u, 12824u, 13587u, 14395u, 15251u, 16158u, 17118u, 18136u, 19215u, 20357u,
    21568u, 22850u, 24209u, 25649u, 27174u, 28790u, 30502u, 32315u, 32767u, 32767u, 32767u, 32767u, 32767u, 32767u, 32767u, 32767u
};
#endif
