#include <libcd.h>
#include <libspu.h>

#include "audio.h"

uint32_t sound_spu_addr;

void audio_init()
{
    SpuInit();
    SpuSetCommonCDVolume(0x3fff, 0x3fff);
    SpuSetCommonMasterVolume(0x3fff, 0x3fff);
    SpuSetCommonCDMix(SPU_ON);
    sound_spu_addr = SPU_RAM_START;
}

static int play_track[2] = { 0, 0 };

void audio_play_cdda_music(int32_t track, bool repeat)
{
    play_track[0] = track;
    CdPlay(repeat ? 2 : 1, play_track, 0);    
}

void audio_stop_cdda_music()
{
    CdPlay(0, play_track, 0);    
}

static void audio_transfer_vag_to_spu(uint32_t spu_addr, uint8_t* vag_start, uint32_t vag_size)
{
    uint32_t l_top = SpuSetTransferStartAddr(spu_addr);
	uint32_t l_size = SpuWrite(vag_start + VAG_HEADER_SIZE, vag_size);
	SpuIsTransferCompleted (SPU_TRANSFER_WAIT);
}

void audio_add_sound(uint8_t sound_id, uint8_t* sound_data)
{
    VAG* vag = (VAG*) sound_data;
    uint32_t sound_raw_size = SWAP_ENDIAN32(vag->data_size);
    
    audio_transfer_vag_to_spu(sound_spu_addr, sound_data, sound_raw_size);

    SpuVoiceAttr voice_attr;
    voice_attr.mask = (
        SPU_VOICE_VOLL |
        SPU_VOICE_VOLR |
        SPU_VOICE_PITCH |
        SPU_VOICE_WDSA |
        SPU_VOICE_ADSR_AMODE |
        SPU_VOICE_ADSR_SMODE |
        SPU_VOICE_ADSR_RMODE |
        SPU_VOICE_ADSR_AR |
        SPU_VOICE_ADSR_DR |
        SPU_VOICE_ADSR_SR |
        SPU_VOICE_ADSR_RR |
        SPU_VOICE_ADSR_SL
    );

    voice_attr.volume.left = 0x3fff;
    voice_attr.volume.right = 0x3fff;
    voice_attr.pitch = 4096;
    voice_attr.a_mode = SPU_VOICE_LINEARIncN;
    voice_attr.s_mode = SPU_VOICE_LINEARIncN;
    voice_attr.r_mode = SPU_VOICE_LINEARDecN;
    voice_attr.ar = 0x0;
    voice_attr.dr = 0x0;
    voice_attr.sr = 0x0;
    voice_attr.rr = 0x0;
    voice_attr.sl = 0xf;

    voice_attr.voice = 1 << sound_id;
    voice_attr.addr = sound_spu_addr;
    SpuSetVoiceAttr (&voice_attr);
    sound_spu_addr += sound_raw_size;
    SpuSetKey(SpuOff, 1 << sound_id);
}

void audio_play_sound(uint8_t sound_id)
{
	SpuSetKey(SpuOn, 1 << sound_id);
}

void audio_stop_sound(uint8_t sound_id)
{
	SpuSetKey(SpuOff, 1 << sound_id);
}