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

#ifndef MIDI_DATA
#define MIDI_DATA 

#include <stdbool.h>

//#include "azimuth/util/music.h"
#define AZ_MUSIC_NUM_TRACKS 5

typedef struct {
   bool voices[AZ_MUSIC_NUM_TRACKS];
   int num_voices;
} midi_part_data;

typedef struct {
   midi_part_data **parts;
   int num_parts;
   int tempo;
} midi_music_voices_data;

midi_music_voices_data *midi_read_voices_data(char *file_name);
void midi_free_music_voices_data(midi_music_voices_data *voices_data);
void midi_print_music_voices_data(midi_music_voices_data *voices_data);

#endif //MIDI_DATA
