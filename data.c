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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

#define LENGTH 5

static char get_char(char c[LENGTH], FILE *file) {
   int i;
   for (i = 0; i < LENGTH - 1; i++)
      c[i] = c[i+1];
   c[LENGTH-1] = fgetc(file);
   return c[LENGTH -1];
}

static bool is_num(char c){
   return ('0' <= c && c <= '9');
}

midi_music_voices_data *midi_read_voices_data(char *file_name){
   FILE *file = fopen(file_name, "r");
   if (!file) {
      fprintf(stderr, "could not open file %s\n", file_name);
      exit(EXIT_FAILURE);
      }
   
   midi_music_voices_data *data = (midi_music_voices_data*) malloc (sizeof(midi_music_voices_data));
   data->parts = (midi_part_data**) malloc (sizeof(midi_part_data *));
   
   char line[1024];
   char *s;
   do {
      s = fgets(line, 1024, file);
      s = strstr(line, "=tempo ");
   } while ( s == NULL && !feof(file));
   
   s = s + 7;// length of '=tempo '  
   int tempo = atoi(s);
   if (tempo == 0) {
      fprintf(stderr, "ERROR reading the tempo\n");
      exit(EXIT_FAILURE);
      }
   data->tempo = tempo;
   data->num_parts = 0;
   int i = 0;
   
   char c[LENGTH+1] = {0};
   for (i = 0; i < LENGTH; i++)
      get_char(c, file);
   do {
      get_char(c, file);
      } while (strcmp (c, "!Part") != 0 && !feof(file));

   i = 0;
   do {
      data->num_parts++;
//      fprintf(stderr, "reading: part index: %d\n", i);
      data->parts = (midi_part_data **) realloc(data->parts, data->num_parts*sizeof(midi_part_data *));
      data->parts[i] = (midi_part_data *) malloc(sizeof(midi_part_data));
      data->parts[i]->num_voices = 0;
      int j;
      for (j = 0; j < AZ_MUSIC_NUM_TRACKS; j++)
         data->parts[i]->voices[j] = false;
      do {
         get_char(c, file);
         if (c[0] == '\n' && is_num(c[1]) && c[2] == '|' && c[3] == ' ' &&
             ! data->parts[i]->voices[c[1]-'1']) {
            data->parts[i]->voices[c[1]-'1'] = true;
            data->parts[i]->num_voices++;
            if (data->parts[i]->num_voices > AZ_MUSIC_NUM_TRACKS) {
               fprintf(stderr, "number of voices is too high !");
               exit(EXIT_FAILURE);
               }
            }
      } while (strcmp (c, "!Part") != 0 && !feof(file));
      i++;
   } while (!feof(file));
   
   printf("read %d parts\n", i);
//   fclose(file);
   return data;
}


void midi_free_music_voices_data(midi_music_voices_data *data) {
   int i;
   for (i = 0; i < data->num_parts; i++)
      free(data->parts[i]);
   free(data->parts);
   free(data);
}

void midi_print_music_voices_data(midi_music_voices_data *data) {
   int i;
   printf("num_parts: %d\n", data->num_parts);
   for (i = 0; i < data->num_parts; i++)
      printf("part %d: [%d|%d|%d|%d|%d]\n", i, data->parts[i]->voices[0], data->parts[i]->voices[1],
                                               data->parts[i]->voices[2], data->parts[i]->voices[3],
                                               data->parts[i]->voices[4]);
}


