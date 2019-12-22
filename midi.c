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

#include <stdlib.h>
#include <math.h>

#include "midi/midi.h"

// <copied>
#define HALF_STEPS_PER_OCTAVE 12

// Pitch number (in half steps above c0) and frequency (in Hz) of a4:
#define A4_PITCH (9 + 4 * HALF_STEPS_PER_OCTAVE)
#define A4_FREQUENCY 440.0
// </copied>

typedef enum { NO_NOTE_BEGUN, MIDDLE_NOTES, 
               LAST_NOTE_ON, LAST_NOTE_OFF,} midi_track_state;

static void write_beginning(int tempo, unsigned long *plage_size_pos, FILE *file);
static void write_ending(unsigned long plage_size_pos, FILE *file);
static void write_note(midi_event_type_t type, az_music_note_t note, int channel, FILE *file);
static void write_note_event(midi_event_type_t type, int channel, unsigned char height, FILE *file);
static void write_part(az_music_part_t *part, midi_part_data *part_data, int tempo, FILE *file);
static void write_vlq(unsigned long i, FILE *file);
static void write_empty_event(int channel, FILE *file);
static int note_from_frequency(double f);

static void place  (int order[AZ_MUSIC_NUM_TRACKS], unsigned long remaining_time[AZ_MUSIC_NUM_TRACKS], int index);
static unsigned long divisions_from_note(az_music_note_t note, int tempo);
static void skip_uninteresting(az_music_track_t *track, int *i);
static bool notes_remaining(midi_track_state tracks_state[AZ_MUSIC_NUM_TRACKS]);
static bool part_no_notes(midi_part_data *part_data);

/*static void afficher(int order[AZ_MUSIC_NUM_TRACKS], unsigned long remaining_time[AZ_MUSIC_NUM_TRACKS]) {
   int i;
   printf("order: ");
   for (i = 0; i < AZ_MUSIC_NUM_TRACKS; i++)
      printf("%d, ", order[i]);
   printf("\nremaining_time: ");
   for (i = 0; i < AZ_MUSIC_NUM_TRACKS; i++)
      printf("%lu, ", remaining_time[i]);
   printf("\n\n");
}*/


bool midi_write_music(az_music_t *music, midi_music_voices_data *data, FILE *file) {
   printf("============\n");
   midi_print_music_voices_data(data);
   printf("============\n");
   
   unsigned long plage_size_pos = 0;
   write_beginning(data->tempo, &plage_size_pos, file);
   
   int i;
   for (i = 0; i < music->num_parts; i++)
      write_part(&music->parts[i], data->parts[i], data->tempo, file);
   
   write_ending(plage_size_pos, file);
   return true;
}


#define FIRST order[0]

static void write_part(az_music_part_t *part, midi_part_data *part_data, int tempo, FILE *file) {
   if (part_no_notes(part_data)) {
      printf("--- empty part ---\n");
      write_empty_event(0, file);
      write_vlq(4*MIDI_QUARTER_DIVISIONS, file);
      return;
      }
   else
      printf("\n------- part -------\n");
   
   int i, j;
   int pos[AZ_MUSIC_NUM_TRACKS] = {0};
   int prev[AZ_MUSIC_NUM_TRACKS] = {0};
   int order[AZ_MUSIC_NUM_TRACKS];
   for (i = 0; i < AZ_MUSIC_NUM_TRACKS; i++)
      order[i] = i;
   unsigned long remaining_time[AZ_MUSIC_NUM_TRACKS] = {0};
   midi_track_state tracks_state[AZ_MUSIC_NUM_TRACKS] = {
         NO_NOTE_BEGUN, NO_NOTE_BEGUN, NO_NOTE_BEGUN, NO_NOTE_BEGUN, NO_NOTE_BEGUN};
   
   for (i = 0; i < AZ_MUSIC_NUM_TRACKS; i++) {
      if (!part_data->voices[i]) {
         for (j = 0; j < AZ_MUSIC_NUM_TRACKS-1; j++)
            order[j] = order[j+1];                		// shift
         order[AZ_MUSIC_NUM_TRACKS-1] = -i-1;
         printf("track %d contains no notes\n", i);
         continue;
         }
      //printf("track i: %d, ptr: [%p]\n", i, (void*) &part->tracks[i]);
      skip_uninteresting(&part->tracks[i], &pos[i]);
      remaining_time[i] =  divisions_from_note (part->tracks[i].notes[pos[i]], tempo);
      place(order, remaining_time, i);
      }
   
   /*printf("--init--\n");
   afficher(order, remaining_time);*/
   
/*   write_note(MIDI_NOTE_ON, part->tracks[FIRST].notes[pos[FIRST]], FIRST, file);
   unsigned long time_elapsed = remaining_time[FIRST];
   write_vlq(time_elapsed, file);
   for (i = 0; i < AZ_MUSIC_NUM_TRACKS && order[i] >= 0; i++)
      remaining_time[order[i]] -= time_elapsed;*/
   
   //int k = 0;
   while (notes_remaining (tracks_state) && FIRST >= 0)
      {
      if (tracks_state[FIRST] == NO_NOTE_BEGUN) 
         tracks_state[FIRST] = MIDDLE_NOTES;
      else {
         write_note(MIDI_NOTE_OFF, part->tracks[FIRST].notes[prev[FIRST]], FIRST, file);
         write_vlq(0, file);
         }
      
      if (tracks_state[FIRST] == LAST_NOTE_ON) {
         //printf("no more notes (last is off) on track %d\n", FIRST);
         tracks_state[FIRST] = LAST_NOTE_OFF;
         
         //then it has to be discarded in "order"
         int discarded_voice = FIRST;
         for (i = 0; i+1 < AZ_MUSIC_NUM_TRACKS && order[i+1] >= 0; i++)
            order[i] = order[i+1];
         order[i] = -discarded_voice - 1;
         continue;
         }
      
      if (part->tracks[FIRST].notes[pos[FIRST]].type == AZ_NOTE_REST)
         write_empty_event(FIRST, file);
      else
         write_note(MIDI_NOTE_ON, part->tracks[FIRST].notes[pos[FIRST]], FIRST, file);

      prev[FIRST] = pos[FIRST];
      unsigned long time_elapsed = remaining_time[FIRST];
      write_vlq(time_elapsed, file);
      for (i = 0; i < AZ_MUSIC_NUM_TRACKS && order[i] >= 0; i++)
         remaining_time[order[i]] -= time_elapsed;
         
      remaining_time[FIRST] = divisions_from_note(part->tracks[FIRST].notes[pos[FIRST]], tempo);
      place(order, remaining_time, FIRST);
      
      pos[FIRST]++;
      skip_uninteresting(&part->tracks[FIRST], &pos[FIRST]);
      if (pos[FIRST] >= part->tracks[FIRST].num_notes) {
         //printf("last beginning on track %d\n", FIRST);
         tracks_state[FIRST] = LAST_NOTE_ON;
         
         /*fprintf(stderr, "FIRST=%d\n", FIRST);
         afficher(order, remaining_time);*/
         }
      /*printf("FIRST: %d\n", FIRST);
      afficher(order, remaining_time);*/
      
      /*printf("--%d--\n", k++);
      int l;
      for (l = 0; l < AZ_MUSIC_NUM_TRACKS; l++)
         printf("pos: %d/%d   ", pos[l], part->tracks[l].num_notes);
      printf("\n");*/
      }
      
   //fseek(file, -1, SEEK_CUR);
   //write_vlq(4*MIDI_QUARTER_DIVISIONS, file);
}

/* place the index'th item of the remaining_time tab, such that order tab always verifies
   remaining_time[order[i]] decreases when i increases*/
static void place  (int order[AZ_MUSIC_NUM_TRACKS], unsigned long remaining_time[AZ_MUSIC_NUM_TRACKS], int index)
{
   int i, j;

   for (j = 0; j < AZ_MUSIC_NUM_TRACKS-1 && order[j+1] >= 0; j++)
      order[j] = order[j+1];                		// shift

   for (i = 0; ((i+1 < AZ_MUSIC_NUM_TRACKS) ? (order[i+1] >= 0) : 1) &&
               i < AZ_MUSIC_NUM_TRACKS-1 && 
               remaining_time[index] >= remaining_time[order[i]]; i++);//              get to its place

   for (j = AZ_MUSIC_NUM_TRACKS-1; order[j] < 0; j--);
   for (/*j = AZ_MUSIC_NUM_TRACKS-1*/; j > i; j--)
      order[j] = order[j-1];
   order[i] = index;
   
   return;
}

static void skip_uninteresting(az_music_track_t *track, int *i) {
   if (*i >= track->num_notes)
      return;
   int j = *i;
   for (j = *i;   j < track->num_notes                 &&
                  track->notes[j].type != AZ_NOTE_REST && 
                  track->notes[j].type != AZ_NOTE_TONE && 
                  track->notes[j].type != AZ_NOTE_DRUM; j++)
      {
      //fprintf(stderr, "skipped n°%d, type: %d\n", j, (int) track->notes[j].type);
      /*if (j+1 >= track->num_notes)  break;
      if (track->notes[j].type == AZ_NOTE_ENVELOPE)
         printf("<%f>", track->notes[j].attributes.envelope.attack_time);*/
      }
   *i = j;
}


static bool notes_remaining(midi_track_state tracks_state[AZ_MUSIC_NUM_TRACKS])
{
   int i;
   for (i = 0; i < AZ_MUSIC_NUM_TRACKS; i++){
      if (tracks_state[i] != LAST_NOTE_OFF)
         return true;
      }
   return false;
}

static bool part_no_notes(midi_part_data *part_data) {
   int i;
   for (i = 0; i < AZ_MUSIC_NUM_TRACKS; i++)
      if (part_data->voices[i])
         return false;
         
   return true;
}

static unsigned long divisions_from_note(az_music_note_t note, int tempo) {
   double secs = -1;
   switch (note.type) {
      case AZ_NOTE_DRUM:
         secs = note.attributes.drum.duration;
         break;
      case AZ_NOTE_REST:
         secs = note.attributes.rest.duration;
         break;
      case AZ_NOTE_TONE:
         secs = note.attributes.tone.duration;
         break;
      case AZ_NOTE_ENVELOPE:
         fprintf(stdout, "envelope, %f\n", note.attributes.envelope.decay_fraction);
      default:
         fprintf(stderr, "unexpected type of note: %d\n", (int) note.type);
         break;
      }
   
   double a = (double) tempo / 60.0;
   double b = secs * a;
   //fprintf(stderr, "divisions: tempo: %d, secs: %f, res: %lu\n", tempo, secs, (unsigned long) (b * MIDI_QUARTER_DIVISIONS));
   return b * MIDI_QUARTER_DIVISIONS;
//   return (unsigned long) ((secs * ((double) tempo) / 60.0) * MIDI_QUARTER_DIVISIONS);
}

static void write_beginning (int tempo, unsigned long *plage_size_pos, FILE *file) {
   const unsigned char begin_file[14] = {  0x4d, 0x54, 0x68, 0x64, //constant
                                           0x00, 0x00, 0x00, 0x06, //size of following:
                                           0x00, 0x01,             //SMF
                                           0x00, 0x02,             //number of data plage
                                           0x00, MIDI_QUARTER_DIVISIONS},            //120 = #divisions of a quarter
                     end_plage[3] = {  0xff, 0x2f, 0x00},
                     set_signature[7] = {  0xff, 0x58, 0x04, 0x04, 0x2, 0x18, 0x08},
                     instr[5] = {  0, 34, 15, 0x77, 0x77};
   unsigned char begin_plage[8]= { 0x4d, 0x54, 0x72, 0x6b, // constant
                                   0x00, 0x00, 0x00, 0x00}, // 4 bytes : plage size
                 set_tempo[6] =  { 0xff, 0x51, 0x03,        // prefix
                                   0x00, 0x00, 0x00},       // value 
                 set_instr[2] = { 0xc0, 0x00};
// 0xb<4b: canal> then instr nb : 0-piano, 119 = 0x77-synth drums
// see https://soundprogramming.net/file-formats/general-midi-instrument-list/
   
   fwrite(begin_file, 1, 14, file);
   fwrite(begin_plage, 1, 8, file);
   unsigned long begin_data_plage = ftell(file) - 4;
   
   int i;
   unsigned long usecs_quarter = 60000000/tempo;
   for (i = 0; i < 3; i++)
      set_tempo[3+i] = (char) (usecs_quarter >> 8*(2-i)) % 256;
   
   write_vlq(0, file);
   fwrite(set_tempo, 1, 6, file);
   write_vlq(0, file);
   fwrite(set_signature, 1, 7, file);
   write_vlq(0, file);
   for (i = 0; i < AZ_MUSIC_NUM_TRACKS; i++) {
      set_instr[1] = instr[i];
      fwrite(set_instr, 1, 2, file);
      write_vlq(0, file);
      
      set_instr[0]++;
//      set_instr[4]++;
//      set_instr[7]++;
      }
   fwrite(end_plage, 1, 3, file);
   
   unsigned long begin_data_size = ftell(file) - begin_data_plage - 4;
   fseek(file, begin_data_plage, SEEK_SET);
   for (i = 0; i < 4; i++)
      fprintf(file, "%c", (char) (begin_data_size >> 8*(3-i)));
   fseek(file, 0, SEEK_END);
   
   fwrite(begin_plage, 1, 8, file);
   *plage_size_pos = ftell(file) - 4;
   write_vlq(0, file);
   
//   printf("begin_data_size: %lu\nplage_size_pos: %lu\n", begin_data_size, *plage_size_pos);
}

static void write_note(midi_event_type_t type, az_music_note_t note, int channel, FILE *file) {
   switch (note.type) {
      case AZ_NOTE_TONE:
         write_note_event(type, channel, note_from_frequency(note.attributes.tone.frequency), file);
         break;
      case AZ_NOTE_DRUM:
         write_note_event(type, channel, 57+channel, file);
         break;                              //-> any different drums.
      case AZ_NOTE_REST:
         fseek(file, -1, SEEK_CUR);
         break;
      default:    break;
      }
}

#define MIDI_NOTE_ON_PREFIX (unsigned char) 0x90
#define MIDI_NOTE_OFF_PREFIX (unsigned char) 0x80
static void write_note_event(midi_event_type_t type, int channel, unsigned char height, FILE *file) {
   unsigned char tab[3];
   switch (type) {
      case MIDI_NOTE_ON:
         tab[0] = MIDI_NOTE_ON_PREFIX + channel % 16;
         break;
      case MIDI_NOTE_OFF:
         tab[0] = MIDI_NOTE_OFF_PREFIX + channel % 16;
         break;
      default:
         fprintf(stderr, "unexpected event type\n");
         exit(EXIT_FAILURE);
         break;
      }
   tab[1] = height;
   tab[2] = MIDI_VELOCITY;
   fwrite(tab, 1, 3, file);
}

static void write_ending(unsigned long plage_size_pos, FILE *file) {
   const unsigned char end_plage[3] = {  0xff, 0x2f, 0x00};

   fwrite(end_plage, 1, 3, file);
   unsigned long size = ftell(file) - plage_size_pos - 4;
   fseek(file, plage_size_pos, SEEK_SET);
   int i;
   for (i = 0; i < 4; i++)
      fprintf(file, "%c", (char) (size >> 8*(3-i)));
      
   printf("size: %lu\n", size);
}

//variable length quantity
//note to myself: operators << and >> in C shift (next operand) bits.
//function from : https://connect.ed-diamond.com/GNU-Linux-Magazine/GLMF-196/Format-MIDI-composez-en-C
static void write_vlq(unsigned long i, FILE *file) {
   int ok;
   if (i > 0x0FFFFFFF) {
      fprintf(stderr, "delay to print is > 0x0FFFFFFF\n");
      exit(EXIT_FAILURE);
      }

   unsigned long output = i & 0x7F;
   i = i >> 7 ;
   while (i != 0) {
      output = (output << 8)  + ((i & 0x7F) | 0x80);
      i = i >> 7;
   }

   do {
      fwrite(&output, 1, 1, file);
      ok = output & 0x80;
      if (ok) output = output >> 8;
      } while (ok);
   return;    
}

static void write_empty_event(int channel, FILE *file) {
   unsigned char tab[] = {0xe0, 0x00, 0x40};
   tab[0] += channel;
   fwrite(tab, 1, 3, file);
}

#define MIDI_A4_NUMBER 69

static int note_from_frequency(double f) {
   return 12*log(f / 440.0) / log(2) + MIDI_A4_NUMBER;
}
