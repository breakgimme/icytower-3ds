#ifndef SFX_3DS_H
#define SFX_3DS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t *data;
    uint32_t frames;
    int channels;
    uint32_t rate;
} Sound;

typedef struct {
    int16_t *data;
    uint32_t frames;
    int channels;
    uint32_t rate;
} Music;

extern Music *audio_stream_bg_beat;
extern Music *audio_stream_bg_menu;

extern Sound *sample_aight;
extern Sound *sample_amazing;
extern Sound *sample_cheer;
extern Sound *sample_extreme;
extern Sound *sample_fantastic;
extern Sound *sample_gameover;
extern Sound *sample_good;
extern Sound *sample_great;
extern Sound *sample_hurryup;
extern Sound *sample_menu_change;
extern Sound *sample_menu_choose;
extern Sound *sample_ring;
extern Sound *sample_splat;
extern Sound *sample_splendid;
extern Sound *sample_step;
extern Sound *sample_super;
extern Sound *sample_sweet;
extern Sound *sample_tryagain;
extern Sound *sample_unbelievable;
extern Sound *sample_wow;

extern Sound *sample_harold_edge;
extern Sound *sample_harold_falling;
extern Sound *sample_harold_jump_hi;
extern Sound *sample_harold_jump_lo;
extern Sound *sample_harold_jump_mid;
extern Sound *sample_harold_wazup;
extern Sound *sample_harold_yo;

extern Music *audio_stream_disco_dave_bg_dave;
extern Sound *sample_disco_dave_ahey;
extern Sound *sample_disco_dave_cmonyo;
extern Sound *sample_disco_dave_diggin;
extern Sound *sample_disco_dave_goinon;
extern Sound *sample_disco_dave_ho;
extern Sound *sample_disco_dave_stayinalive;
extern Sound *sample_disco_dave_watchit;

bool sfx_init(void);
void sfx_shutdown(void);
bool sfx_load_audio_streams_and_samples(void);
void sfx_destroy_audio_streams_and_samples(void);

void play_sample(Sound *s, float gain);
void play_music(Music *m);
void stop_music(Music *m);
void set_music_gain(float gain);

#endif
