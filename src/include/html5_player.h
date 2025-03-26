#ifndef HTML5_PLAYER_H
#define HTML5_PLAYER_H

#include <stdint.h>
#include "../include/gui.h"
#include "../include/browser.h"

// İleriye doğru bildirimler
typedef enum video_codec_t video_codec_t;
typedef enum video_resolution_t video_resolution_t;

// HTML5 oynatıcı durumları
typedef enum {
    PLAYER_STATE_IDLE,
    PLAYER_STATE_BUFFERING,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED,
    PLAYER_STATE_STOPPED,
    PLAYER_STATE_ERROR
} player_state_t;

// Video kodek bilgileri
typedef struct {
    video_codec_t type;
    const char* name;
    uint32_t max_bitrate;          // bit/saniye
    uint8_t hardware_acceleration; // 0=yok, 1=var
    void* codec_data;             // Codec özel verisi
    uint8_t supports_4k;          // 4K desteği var mı?
    float compression_ratio;      // Sıkıştırma oranı (daha yüksek = daha iyi)
    uint8_t requires_license;     // Lisans gerektirir mi?
} video_codec_info_t;

// Video akış bilgileri
typedef struct {
    uint32_t width;              // Piksel genişliği
    uint32_t height;             // Piksel yüksekliği
    uint32_t bitrate;            // bit/saniye
    float framerate;             // fps
    video_codec_t codec;         // Kullanılan codec
    uint32_t buffer_size;        // Buffer boyutu (bayt)
    uint32_t max_resolution;     // Maksimum çözünürlük
    uint8_t has_audio;           // Ses içeriyor mu?
    uint8_t audio_channels;      // Ses kanalları (1=mono, 2=stereo, 6=5.1)
    uint32_t audio_bitrate;      // Ses bit hızı
} video_stream_info_t;

// Video ön bellek istatistikleri
typedef struct {
    uint32_t total_size;         // Toplam ön bellek alanı (bayt)
    uint32_t used_size;          // Kullanılan alan (bayt)
    uint32_t buffered_duration;  // Ön belleğe alınan süre (ms)
    float fill_rate;             // Doldurma hızı (bayt/saniye)
    float drain_rate;            // Tüketim hızı (bayt/saniye)
    uint8_t is_network_limited;  // Ağ sınırlaması var mı?
    uint32_t stutter_count;      // Takılma sayısı
} video_buffer_stats_t;

// Donanım hızlandırma bilgileri
typedef struct {
    uint8_t available;           // Donanım hızlandırma mevcut mu?
    const char* type;            // Hızlandırma türü (OpenGL, DirectX, vb.)
    uint32_t max_resolution;     // Desteklenen maksimum çözünürlük
    uint8_t dedicated_memory;    // Ayrılmış bellek (MB)
    float decoding_efficiency;   // Çözme verimliliği (0-1 arası)
    uint8_t supports_hdr;        // HDR desteği var mı?
} hw_acceleration_info_t;

// HTML5 oynatıcı yapısı
typedef struct html5_player {
    player_state_t state;              // Oynatıcı durumu
    video_stream_info_t stream_info;   // Akış bilgileri
    video_buffer_stats_t buffer_stats; // Ön bellek durumu
    uint32_t position;                 // Geçerli konum (ms)
    uint32_t duration;                 // Toplam süre (ms)
    uint8_t volume;                    // Ses seviyesi (0-100)
    uint8_t muted;                     // Sessiz (0/1)
    uint8_t fullscreen;                // Tam ekran modu (0/1)
    uint8_t hardware_acceleration;     // Donanım hızlandırma (0/1)
    video_codec_info_t* active_codec;  // Aktif codec
    gui_window_t* window;              // Pencere referansı
    void* render_context;              // Oluşturma bağlamı
    void* buffer_memory;               // Video belleği
    hw_acceleration_info_t* hw_info;   // Donanım hızlandırma bilgileri
    uint8_t auto_quality;              // Otomatik kalite ayarı (0/1)
    void* frame_buffer;                // Kare tamponu
} html5_player_t;

// İşlev prototipleri
html5_player_t* html5_player_create();
void html5_player_destroy(html5_player_t* player);

int html5_player_open(html5_player_t* player, const char* url);
int html5_player_play(html5_player_t* player);
int html5_player_pause(html5_player_t* player);
int html5_player_stop(html5_player_t* player);
int html5_player_seek(html5_player_t* player, uint32_t position);
int html5_player_set_volume(html5_player_t* player, uint8_t volume);
int html5_player_mute(html5_player_t* player, uint8_t mute);
int html5_player_set_fullscreen(html5_player_t* player, uint8_t fullscreen);

video_codec_info_t* html5_player_get_codec_info(video_codec_t codec);
video_stream_info_t* html5_player_get_stream_info(html5_player_t* player);
video_buffer_stats_t* html5_player_get_buffer_stats(html5_player_t* player);
hw_acceleration_info_t* html5_player_get_hw_info();

// Video işleme ve görüntüleme
void html5_player_draw(html5_player_t* player, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void html5_player_update(html5_player_t* player);
void html5_player_process_frame(html5_player_t* player);

// Codec yönetimi
int html5_player_register_codec(video_codec_info_t* codec);
int html5_player_select_codec(html5_player_t* player, video_codec_t codec);

// Donanım hızlandırma
int html5_player_enable_hardware_acceleration(html5_player_t* player);
int html5_player_disable_hardware_acceleration(html5_player_t* player);
int html5_player_detect_hardware_capabilities();

// Performans ayarları
int html5_player_set_buffer_size(html5_player_t* player, uint32_t size);
int html5_player_set_max_resolution(html5_player_t* player, uint32_t width, uint32_t height);
int html5_player_enable_auto_quality(html5_player_t* player, uint8_t enable);
int html5_player_set_quality_level(html5_player_t* player, uint8_t level); // 0-100 arası

// Hata raporlama
const char* html5_player_get_last_error();
int html5_player_get_performance_stats(html5_player_t* player, char* buffer, size_t size);

// Gelişmiş özellikler
int html5_player_enable_hdr(html5_player_t* player, uint8_t enable);
int html5_player_set_deinterlace(html5_player_t* player, uint8_t enable);
int html5_player_capture_frame(html5_player_t* player, const char* filename);

#endif // HTML5_PLAYER_H 