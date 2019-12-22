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
#include <stdlib.h>

#include "azimuth/util/music.h"
#include "azimuth/state/music.h"
#include "midi/midi.h"
#include "midi/data.h"


// <copied>
az_music_t music;
static void destroy_music(void) {
  az_destroy_music(&music);
}
// </copied>

int main(int argc, char **argv)
{
   if (argc < 3) {
      fprintf(stderr, "Usage: <input file> <output file>\n");
      return EXIT_SUCCESS;
      }

   midi_music_voices_data *data = midi_read_voices_data(argv[1]);
   
  // <copied>
  // Initialize drum kit:
  int num_drums = 0;
  const az_sound_data_t *drums = NULL;
  az_get_drum_kit(&num_drums, &drums);

  // Load music:
  az_reader_t reader;
  if (!az_file_reader(argv[1], &reader)) {
    fprintf(stderr, "ERROR: could not open %s\n", argv[1]);
    return EXIT_FAILURE;
  }
  if (!az_read_music(&reader, num_drums, drums, &music)) {
    fprintf(stderr, "ERROR: failed to parse music.\n");
    return EXIT_FAILURE;
  }
  az_rclose(&reader);
  atexit(destroy_music);
  // </copied>
  
  FILE *file = fopen(argv[2], "w");
  if (!file) {
   fprintf(stderr, "ERROR: could not open %s\n", argv[2]);
   return EXIT_FAILURE;
   }
  midi_write_music(&music, data, file);
   
   fclose(file);
   midi_free_music_voices_data(data);
   printf("Done.\n");
   return EXIT_SUCCESS;
}
