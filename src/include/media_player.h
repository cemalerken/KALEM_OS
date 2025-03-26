#ifndef MEDIA_PLAYER_H
#define MEDIA_PLAYER_H

#include <stdint.h>
#include "../include/gui.h"

// Medya tipi
typedef enum {
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_PLAYLIST
} media_type_t;

// Medya formatları
typedef enum {
    MEDIA_FORMAT_UNKNOWN,
    MEDIA_FORMAT_MP3,
    MEDIA_FORMAT_WAV,
    MEDIA_FORMAT_OGG,
    MEDIA_FORMAT_FLAC,
    MEDIA_FORMAT_AAC,
    MEDIA_FORMAT_WMA,
    MEDIA_FORMAT_MP4,
    MEDIA_FORMAT_AVI,
    MEDIA_FORMAT_MKV,
    MEDIA_FORMAT_MOV,
    MEDIA_FORMAT_WMV,
    MEDIA_FORMAT_FLV,
    MEDIA_FORMAT_WEBM
} media_format_t;

// Codec tipleri
typedef enum {
    CODEC_UNKNOWN,
    // Ses codec'leri
    CODEC_MP3,
    CODEC_AAC,
    CODEC_VORBIS,
    CODEC_FLAC,
    CODEC_PCM,
    CODEC_WMA,
    // Video codec'leri
    CODEC_H264,
    CODEC_H265,
    CODEC_VP8,
    CODEC_VP9,
    CODEC_MPEG4,
    CODEC_AV1,
    CODEC_THEORA
} codec_t;

// Codec paketi
typedef struct {
    const char* name;
    codec_t type;
    void* (*init)(void* data, uint32_t size);
    int (*decode)(void* context, void* output, uint32_t* size);
    void (*close)(void* context);
    void* private_data;
} codec_package_t;

// Medya dosyası
typedef struct {
    char filename[256];
    char title[128];
    char artist[128];
    char album[128];
    uint32_t duration;
    media_type_t type;
    media_format_t format;
    codec_t audio_codec;
    codec_t video_codec;
    uint32_t width;
    uint32_t height;
    uint32_t bitrate;
    uint32_t sample_rate;
    uint32_t channels;
    void* data;
} media_file_t;

// Çalma listesi
typedef struct {
    char name[128];
    media_file_t** files;
    uint32_t file_count;
    uint32_t current_index;
} playlist_t;

// Oynatıcı durumu
typedef enum {
    PLAYER_STATE_STOPPED,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED
} player_state_t;

// Medya oynatıcı
typedef struct {
    gui_window_t* window;
    media_file_t* current_file;
    playlist_t* current_playlist;
    player_state_t state;
    uint32_t position;
    uint32_t volume;
    uint8_t muted;
    uint8_t fullscreen;
    uint8_t repeat;        // Tekrarlama modu
    uint8_t shuffle;       // Karıştırma modu
    void* codec_data;      // Codec özel verisi
} media_player_t;

// Fonksiyon prototipleri
void media_player_init();
void media_player_show();
void media_player_register_codec(codec_package_t* codec);
codec_package_t* media_player_find_codec(codec_t type);
int media_player_open_file(const char* filename);
int media_player_play();
int media_player_pause();
int media_player_stop();
int media_player_seek(uint32_t position);
int media_player_set_volume(uint32_t volume);
int media_player_mute(uint8_t mute);
int media_player_next();
int media_player_previous();
void media_player_toggle_fullscreen();
const char* media_format_to_string(media_format_t format);
const char* codec_to_string(codec_t codec);

#endif // MEDIA_PLAYER_H 