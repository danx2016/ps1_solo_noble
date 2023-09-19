#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <stdint.h>
#include <stdbool.h>

#define MUSIC_TRACK_TITLE   2
#define MUSIC_TRACK_PLAYING 3
#define MUSIC_TRACK_VICTORY 4

extern void audio_init();
extern void audio_play_cdda_music(int32_t track, bool repeat);
extern void audio_stop_cdda_music();

// all the values in this header must be big endian
typedef struct 
{		
    uint8_t  id[4]; // VAGp
    uint32_t version;              
    uint32_t reserved;
    uint32_t data_size;
    uint32_t sampling_frequency;
    uint8_t  reserved2[12];
    uint8_t  name[16];
} VAG;

#define SWAP_ENDIAN32(x) (((x)>>24) | (((x)>>8) & 0xFF00) | (((x)<<8) & 0x00FF0000) | ((x)<<24))

#define SPU_RAM_START 0x1010
#define VAG_HEADER_SIZE 48

#define SOUND_ID_DROP   0
#define SOUND_ID_MOVE   1
#define SOUND_ID_SELECT 2
#define SOUND_ID_START  3
#define SOUND_ID_ERROR  4

extern void audio_add_sound(uint8_t sound_id, uint8_t* vag_data);
extern void audio_play_sound(uint8_t sound_id);
extern void audio_stop_sound(uint8_t sound_id);

#endif /* _AUDIO_H_ */