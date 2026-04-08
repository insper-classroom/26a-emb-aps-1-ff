#ifndef GENIUS_SOM_H
#define GENIUS_SOM_H

#include "pico/stdlib.h"

void genius_sound_init(void);
void genius_sound_play_tone(uint freq, uint duration_ms);
void genius_sound_play_error_wav(void);

#endif
