/*=============================================================================
| Written by Rayerdyne (2019) to make midi file from custom music format used |
| in Azimuth, using parts of Azimuth's functionnalities.                      | 
|                                                                             |
| Do wathever you want with this part of the code, its only initial goal was  |
| to make music arrangement easier. The code is given without any warranty.   |
|                                                                             |
| /!\ See Azimuth's sources files for their licenses.                         |
|                                                                             |
=============================================================================*/

#include <stdbool.h>
#include <stdio.h>
#include "azimuth/util/music.h"

#include "midi/data.h"

#ifndef MIDI_H
#define MIDI_H

#define MIDI_VELOCITY (unsigned char) 0x40
#define MIDI_QUARTER_DIVISIONS 240

typedef enum {
   MIDI_NOTE_OFF,
   MIDI_NOTE_ON,
} midi_event_type_t;

bool midi_write_music(az_music_t *music, midi_music_voices_data *data, FILE *file);

#endif //MIDI_H
