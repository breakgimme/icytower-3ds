#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <3ds.h>

#include "sfx.h"

Music *audio_stream_bg_beat;
Music *audio_stream_bg_menu;

Sound *sample_aight;
Sound *sample_amazing;
Sound *sample_cheer;
Sound *sample_extreme;
Sound *sample_fantastic;
Sound *sample_gameover;
Sound *sample_good;
Sound *sample_great;
Sound *sample_hurryup;
Sound *sample_menu_change;
Sound *sample_menu_choose;
Sound *sample_ring;
Sound *sample_splat;
Sound *sample_splendid;
Sound *sample_step;
Sound *sample_super;
Sound *sample_sweet;
Sound *sample_tryagain;
Sound *sample_unbelievable;
Sound *sample_wow;

Sound *sample_harold_edge;
Sound *sample_harold_falling;
Sound *sample_harold_jump_hi;
Sound *sample_harold_jump_lo;
Sound *sample_harold_jump_mid;
Sound *sample_harold_wazup;
Sound *sample_harold_yo;

Music *audio_stream_disco_dave_bg_dave;
Sound *sample_disco_dave_ahey;
Sound *sample_disco_dave_cmonyo;
Sound *sample_disco_dave_diggin;
Sound *sample_disco_dave_goinon;
Sound *sample_disco_dave_ho;
Sound *sample_disco_dave_stayinalive;
Sound *sample_disco_dave_watchit;

#pragma pack(push, 1)
typedef struct {
    uint16_t channels;
    uint32_t rate;
    int16_t *data;
    uint32_t frames;
} WavData;
#pragma pack(pop)

static bool load_wav(const char *path, WavData *out) {
    FILE *f = fopen(path, "rb");
    uint8_t hdr[12];
    uint16_t channels = 0, bits = 0;
    uint32_t rate = 0;
    int16_t *data = NULL;
    uint32_t frames = 0;
    bool have_fmt = false;

    if (!f)
        return false;
    if (fread(hdr, 1, 12, f) != 12 || memcmp(hdr, "RIFF", 4) != 0 ||
        memcmp(hdr + 8, "WAVE", 4) != 0) {
        fclose(f);
        return false;
    }
    while (true) {
        uint8_t chunk[8];
        uint32_t size;
        long next;
        if (fread(chunk, 1, 8, f) != 8)
            break;
        size = (uint32_t)chunk[4] | ((uint32_t)chunk[5] << 8) |
               ((uint32_t)chunk[6] << 16) | ((uint32_t)chunk[7] << 24);
        next = ftell(f) + (long)size;
        if (memcmp(chunk, "fmt ", 4) == 0) {
            uint8_t fmt[16];
            uint16_t audio_fmt;
            if (size < 16 || fread(fmt, 1, 16, f) != 16)
                break;
            audio_fmt = (uint16_t)fmt[0] | ((uint16_t)fmt[1] << 8);
            channels = (uint16_t)fmt[2] | ((uint16_t)fmt[3] << 8);
            rate = (uint32_t)fmt[4] | ((uint32_t)fmt[5] << 8) |
                   ((uint32_t)fmt[6] << 16) | ((uint32_t)fmt[7] << 24);
            bits = (uint16_t)fmt[14] | ((uint16_t)fmt[15] << 8);
            if (audio_fmt != 1 || bits != 16 ||
                (channels != 1 && channels != 2)) {
                break;
            }
            have_fmt = true;
        } else if (memcmp(chunk, "data", 4) == 0) {
            if (!have_fmt || size == 0)
                break;
            data = linearAlloc(size);
            if (!data)
                break;
            if (fread(data, 1, size, f) != size) {
                linearFree(data);
                data = NULL;
                break;
            }
            DSP_FlushDataCache(data, size);
            frames = size / (sizeof(int16_t) * channels);
            out->channels = channels;
            out->rate = rate;
            out->data = data;
            out->frames = frames;
            fclose(f);
            return true;
        }
        if (fseek(f, next, SEEK_SET) != 0)
            break;
    }
    fclose(f);
    return false;
}

static Sound *load_sound(const char *path) {
    WavData w;
    Sound *s;
    if (!load_wav(path, &w))
        return NULL;
    s = malloc(sizeof(*s));
    if (!s) {
        linearFree(w.data);
        return NULL;
    }
    s->data = w.data;
    s->frames = w.frames;
    s->channels = w.channels;
    s->rate = w.rate;
    return s;
}

static Music *load_music(const char *path) {
    WavData w;
    Music *m;
    if (!load_wav(path, &w))
        return NULL;
    m = malloc(sizeof(*m));
    if (!m) {
        linearFree(w.data);
        return NULL;
    }
    m->data = w.data;
    m->frames = w.frames;
    m->channels = w.channels;
    m->rate = w.rate;
    return m;
}

static void free_sound(Sound *s) {
    if (s) {
        linearFree(s->data);
        free(s);
    }
}

static void free_music(Music *m) {
    if (m) {
        linearFree(m->data);
        free(m);
    }
}

#define MUSIC_CHANNEL 0
#define VOICE_COUNT 8
#define VOICE_FIRST 1

static ndspWaveBuf voice_buf[VOICE_COUNT];
static int voice_next;
static ndspWaveBuf music_buf;
static float music_gain = 1.0f;

bool sfx_init(void) {
    int i;
    if (R_FAILED(ndspInit()))
        return false;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    for (i = 0; i < 16; ++i)
        ndspChnReset(i);
    memset(voice_buf, 0, sizeof(voice_buf));
    memset(&music_buf, 0, sizeof(music_buf));
    voice_next = 0;
    music_gain = 1.0f;
    return true;
}

void sfx_shutdown(void) {
    int i;
    for (i = 0; i < 16; ++i) {
        ndspChnSetPaused(i, true);
        ndspChnWaveBufClear(i);
    }
    ndspExit();
}

static void setup_channel(int ch, int channels, uint32_t rate,
                          float gain) {
    float mix[12];
    int i;
    ndspChnReset(ch);
    ndspChnSetInterp(ch, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(ch, (float)rate);
    ndspChnSetFormat(ch, channels == 2 ? NDSP_FORMAT_STEREO_PCM16
                                      : NDSP_FORMAT_MONO_PCM16);
    for (i = 0; i < 12; ++i)
        mix[i] = 0.0f;
    mix[0] = gain;
    mix[1] = gain;
    ndspChnSetMix(ch, mix);
}

void play_sample(Sound *s, float gain) {
    ndspWaveBuf *wb;
    int ch;
    if (!s || gain <= 0.0f)
        return;
    wb = &voice_buf[voice_next];
    ch = VOICE_FIRST + voice_next;
    voice_next = (voice_next + 1) % VOICE_COUNT;
    ndspChnWaveBufClear(ch);
    setup_channel(ch, s->channels, s->rate, gain);
    memset(wb, 0, sizeof(*wb));
    wb->data_vaddr = s->data;
    wb->nsamples = s->frames;

    wb->looping = false;
    DSP_FlushDataCache(s->data, s->frames * sizeof(int16_t) * s->channels);
    ndspChnWaveBufAdd(ch, wb);
    ndspChnSetPaused(ch, false);
}

void play_music(Music *m) {
    if (!m)
        return;
    ndspChnWaveBufClear(MUSIC_CHANNEL);
    setup_channel(MUSIC_CHANNEL, m->channels, m->rate, music_gain);
    memset(&music_buf, 0, sizeof(music_buf));
    music_buf.data_vaddr = m->data;
    music_buf.nsamples = m->frames;

    music_buf.looping = true;
    DSP_FlushDataCache(m->data, m->frames * sizeof(int16_t) * m->channels);
    ndspChnWaveBufAdd(MUSIC_CHANNEL, &music_buf);
    ndspChnSetPaused(MUSIC_CHANNEL, false);
}

void stop_music(Music *m) {
    (void)m;
    ndspChnSetPaused(MUSIC_CHANNEL, true);
    ndspChnWaveBufClear(MUSIC_CHANNEL);
}

void set_music_gain(float gain) {
    float mix[12];
    int i;
    if (gain < 0.0f)
        gain = 0.0f;
    if (gain > 1.0f)
        gain = 1.0f;
    music_gain = gain;
    for (i = 0; i < 12; ++i)
        mix[i] = 0.0f;
    mix[0] = gain;
    mix[1] = gain;
    ndspChnSetMix(MUSIC_CHANNEL, mix);
}

#define ASSET_ROOT "romfs:/"

#define LOAD_AUDIO_STREAM(name)     (audio_stream_##name = load_music(ASSET_ROOT "sfx/" #name ".wav"))

#define LOAD_SAMPLE(name)     (sample_##name = load_sound(ASSET_ROOT "sfx/" #name ".wav"))

#define LOAD_AUDIO_STREAM_CHARACTER(character, name)                    (audio_stream_##character##_##name = load_music(                          ASSET_ROOT "sfx/" #character "/" #name ".wav"))

#define LOAD_SAMPLE_CHARACTER(character, name)     (sample_##character##_##name =                       load_sound(ASSET_ROOT "sfx/" #character "/" #name ".wav"))

#define DESTROY_AUDIO_STREAM(name)             do {                                           free_music(audio_stream_##name);           audio_stream_##name = NULL;            } while (0)

#define DESTROY_SAMPLE(name)             do {                                     free_sound(sample_##name);           sample_##name = NULL;            } while (0)

#define DESTROY_AUDIO_STREAM_CHARACTER(character, name)          do {                                                             free_music(audio_stream_##character##_##name);               audio_stream_##character##_##name = NULL;                } while (0)

#define DESTROY_SAMPLE_CHARACTER(character, name)        do {                                                     free_sound(sample_##character##_##name);             sample_##character##_##name = NULL;              } while (0)

bool sfx_load_audio_streams_and_samples(void) {
    if (!LOAD_AUDIO_STREAM(bg_beat))
        goto destroy;
    if (!LOAD_AUDIO_STREAM(bg_menu))
        goto destroy;
    if (!LOAD_SAMPLE(aight))
        goto destroy;
    if (!LOAD_SAMPLE(amazing))
        goto destroy;
    if (!LOAD_SAMPLE(cheer))
        goto destroy;
    if (!LOAD_SAMPLE(extreme))
        goto destroy;
    if (!LOAD_SAMPLE(fantastic))
        goto destroy;
    if (!LOAD_SAMPLE(gameover))
        goto destroy;
    if (!LOAD_SAMPLE(good))
        goto destroy;
    if (!LOAD_SAMPLE(great))
        goto destroy;
    if (!LOAD_SAMPLE(hurryup))
        goto destroy;
    if (!LOAD_SAMPLE(menu_change))
        goto destroy;
    if (!LOAD_SAMPLE(menu_choose))
        goto destroy;
    if (!LOAD_SAMPLE(ring))
        goto destroy;
    if (!LOAD_SAMPLE(splat))
        goto destroy;
    if (!LOAD_SAMPLE(splendid))
        goto destroy;
    if (!LOAD_SAMPLE(step))
        goto destroy;
    if (!LOAD_SAMPLE(super))
        goto destroy;
    if (!LOAD_SAMPLE(sweet))
        goto destroy;
    if (!LOAD_SAMPLE(tryagain))
        goto destroy;
    if (!LOAD_SAMPLE(unbelievable))
        goto destroy;
    if (!LOAD_SAMPLE(wow))
        goto destroy;

    if (!LOAD_SAMPLE_CHARACTER(harold, edge))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(harold, falling))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(harold, jump_hi))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(harold, jump_lo))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(harold, jump_mid))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(harold, wazup))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(harold, yo))
        goto destroy;

    if (!LOAD_AUDIO_STREAM_CHARACTER(disco_dave, bg_dave))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(disco_dave, ahey))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(disco_dave, cmonyo))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(disco_dave, diggin))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(disco_dave, goinon))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(disco_dave, ho))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(disco_dave, stayinalive))
        goto destroy;
    if (!LOAD_SAMPLE_CHARACTER(disco_dave, watchit))
        goto destroy;

    return true;
destroy:
    sfx_destroy_audio_streams_and_samples();
    return false;
}

void sfx_destroy_audio_streams_and_samples(void) {
    DESTROY_AUDIO_STREAM(bg_beat);
    DESTROY_AUDIO_STREAM(bg_menu);
    DESTROY_SAMPLE(aight);
    DESTROY_SAMPLE(amazing);
    DESTROY_SAMPLE(cheer);
    DESTROY_SAMPLE(extreme);
    DESTROY_SAMPLE(fantastic);
    DESTROY_SAMPLE(gameover);
    DESTROY_SAMPLE(good);
    DESTROY_SAMPLE(great);
    DESTROY_SAMPLE(hurryup);
    DESTROY_SAMPLE(menu_change);
    DESTROY_SAMPLE(menu_choose);
    DESTROY_SAMPLE(ring);
    DESTROY_SAMPLE(splat);
    DESTROY_SAMPLE(splendid);
    DESTROY_SAMPLE(step);
    DESTROY_SAMPLE(super);
    DESTROY_SAMPLE(sweet);
    DESTROY_SAMPLE(tryagain);
    DESTROY_SAMPLE(unbelievable);
    DESTROY_SAMPLE(wow);

    DESTROY_SAMPLE_CHARACTER(harold, edge);
    DESTROY_SAMPLE_CHARACTER(harold, falling);
    DESTROY_SAMPLE_CHARACTER(harold, jump_hi);
    DESTROY_SAMPLE_CHARACTER(harold, jump_lo);
    DESTROY_SAMPLE_CHARACTER(harold, jump_mid);
    DESTROY_SAMPLE_CHARACTER(harold, wazup);
    DESTROY_SAMPLE_CHARACTER(harold, yo);

    DESTROY_AUDIO_STREAM_CHARACTER(disco_dave, bg_dave);
    DESTROY_SAMPLE_CHARACTER(disco_dave, ahey);
    DESTROY_SAMPLE_CHARACTER(disco_dave, cmonyo);
    DESTROY_SAMPLE_CHARACTER(disco_dave, diggin);
    DESTROY_SAMPLE_CHARACTER(disco_dave, goinon);
    DESTROY_SAMPLE_CHARACTER(disco_dave, ho);
    DESTROY_SAMPLE_CHARACTER(disco_dave, stayinalive);
    DESTROY_SAMPLE_CHARACTER(disco_dave, watchit);
}
