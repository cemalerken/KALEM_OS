#include "../include/html5_player.h"
#include "../include/gui.h"
#include "../include/vga.h"
#include "../include/font.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Sabit değerler
#define MAX_CODECS 10
#define DEFAULT_BUFFER_SIZE (20 * 1024 * 1024) // 20 MB
#define MIN_BUFFER_THRESHOLD 0.2 // %20 doluluk gerekli
#define FRAME_PROCESS_INTERVAL 16 // ~60 FPS
#define MAX_4K_BITRATE 60000000 // 60 Mbps

// Kayıtlı codecler
static video_codec_info_t registered_codecs[MAX_CODECS];
static uint8_t codec_count = 0;

// Demo amaçlı video codec bilgileri - gerçek bir uygulamada dinamik yüklenirdi
static void init_default_codecs() {
    if (codec_count > 0) return; // Zaten yüklenmiş
    
    // H.264 codec
    video_codec_info_t h264;
    h264.type = VIDEO_CODEC_H264;
    h264.name = "H.264 / AVC";
    h264.max_bitrate = 50000000; // 50 Mbps
    h264.hardware_acceleration = 1;
    h264.codec_data = NULL;
    
    // H.265 codec
    video_codec_info_t h265;
    h265.type = VIDEO_CODEC_H265;
    h265.name = "H.265 / HEVC";
    h265.max_bitrate = 80000000; // 80 Mbps
    h265.hardware_acceleration = 1;
    h265.codec_data = NULL;
    
    // VP9 codec
    video_codec_info_t vp9;
    vp9.type = VIDEO_CODEC_VP9;
    vp9.name = "VP9";
    vp9.max_bitrate = 60000000; // 60 Mbps
    vp9.hardware_acceleration = 1;
    vp9.codec_data = NULL;
    
    // AV1 codec
    video_codec_info_t av1;
    av1.type = VIDEO_CODEC_AV1;
    av1.name = "AV1";
    av1.max_bitrate = 40000000; // 40 Mbps
    av1.hardware_acceleration = 1;
    av1.codec_data = NULL;
    
    // Codecler kaydediliyor
    html5_player_register_codec(&h264);
    html5_player_register_codec(&h265);
    html5_player_register_codec(&vp9);
    html5_player_register_codec(&av1);
}

// Yeni HTML5 oynatıcı oluştur
html5_player_t* html5_player_create() {
    // Codec'leri başlat
    init_default_codecs();
    
    // Oynatıcı belleği ayır
    html5_player_t* player = (html5_player_t*)malloc(sizeof(html5_player_t));
    if (!player) return NULL;
    
    // Varsayılan değerler
    memset(player, 0, sizeof(html5_player_t));
    player->state = PLAYER_STATE_IDLE;
    player->volume = 80; // %80 ses
    
    // Ön bellek ayarla
    player->buffer_stats.total_size = DEFAULT_BUFFER_SIZE;
    
    // Donanım hızlandırma varsayılan olarak açık
    player->hardware_acceleration = 1;
    
    // H.265 codec'ini varsayılan olarak seç (4K için en uygun)
    player->active_codec = html5_player_get_codec_info(VIDEO_CODEC_H265);
    
    return player;
}

// Oynatıcıyı temizle
void html5_player_destroy(html5_player_t* player) {
    if (!player) return;
    
    // Çalışıyorsa durdur
    if (player->state == PLAYER_STATE_PLAYING || 
        player->state == PLAYER_STATE_PAUSED) {
        html5_player_stop(player);
    }
    
    // Video belleğini temizle
    if (player->buffer_memory) {
        free(player->buffer_memory);
        player->buffer_memory = NULL;
    }
    
    // Oynatıcıyı serbest bırak
    free(player);
}

// Codec kaydı
int html5_player_register_codec(video_codec_info_t* codec) {
    if (codec_count >= MAX_CODECS) return -1;
    
    registered_codecs[codec_count] = *codec;
    codec_count++;
    
    return 0;
}

// Codec bul
video_codec_info_t* html5_player_get_codec_info(video_codec_t codec) {
    for (int i = 0; i < codec_count; i++) {
        if (registered_codecs[i].type == codec) {
            return &registered_codecs[i];
        }
    }
    return NULL;
}

// Codec seç
int html5_player_select_codec(html5_player_t* player, video_codec_t codec) {
    if (!player) return -1;
    
    video_codec_info_t* codec_info = html5_player_get_codec_info(codec);
    if (!codec_info) return -1;
    
    player->active_codec = codec_info;
    player->stream_info.codec = codec;
    
    return 0;
}

// Video URL'yi aç
int html5_player_open(html5_player_t* player, const char* url) {
    if (!player || !url) return -1;
    
    player->state = PLAYER_STATE_BUFFERING;
    
    // Demo amaçlı akış bilgilerini ayarla
    // Gerçek uygulamada bunlar dosyadan veya ağdan analiz edilir
    player->stream_info.width = 3840;
    player->stream_info.height = 2160;
    player->stream_info.bitrate = 30000000; // 30 Mbps
    player->stream_info.framerate = 60.0f;
    player->stream_info.buffer_size = DEFAULT_BUFFER_SIZE;
    
    // 4K video için en uygun codec'i seç
    if (player->stream_info.width >= 3840) {
        // Yüksek verimli codec'leri tercih et: HEVC veya AV1
        html5_player_select_codec(player, VIDEO_CODEC_H265);
    }
    
    // Video belleği ayır
    if (player->buffer_memory) {
        free(player->buffer_memory);
    }
    player->buffer_memory = malloc(player->buffer_stats.total_size);
    if (!player->buffer_memory) {
        player->state = PLAYER_STATE_ERROR;
        return -1;
    }
    
    // Demo amaçlı - gerçek uygulamada ön belleğe alınır
    player->buffer_stats.used_size = player->buffer_stats.total_size * 0.3; // %30 doluluk
    player->buffer_stats.buffered_duration = 10000; // 10 saniye
    player->buffer_stats.fill_rate = player->stream_info.bitrate / 8; // Bayt/saniye
    player->buffer_stats.drain_rate = player->buffer_stats.fill_rate * 0.9; // Yeterince hızlı okuma
    
    // Toplam süre
    player->duration = 300000; // 5 dakika
    player->position = 0;
    
    // Yeterli ön belleğe alındığında oynatmaya başlanabilir
    if (player->buffer_stats.used_size > 
        player->buffer_stats.total_size * MIN_BUFFER_THRESHOLD) {
        player->state = PLAYER_STATE_PAUSED; // Oynatma çağrısı bekliyor
    }
    
    return 0;
}

// Video oynatımını başlat
int html5_player_play(html5_player_t* player) {
    if (!player) return -1;
    
    if (player->state == PLAYER_STATE_BUFFERING) {
        // Yeterli ön bellek kontrolü
        if (player->buffer_stats.used_size < 
            player->buffer_stats.total_size * MIN_BUFFER_THRESHOLD) {
            return -1; // Yetersiz ön bellek
        }
    }
    
    player->state = PLAYER_STATE_PLAYING;
    return 0;
}

// Video oynatımını duraklat
int html5_player_pause(html5_player_t* player) {
    if (!player) return -1;
    
    if (player->state == PLAYER_STATE_PLAYING) {
        player->state = PLAYER_STATE_PAUSED;
    }
    
    return 0;
}

// Video oynatımını durdur
int html5_player_stop(html5_player_t* player) {
    if (!player) return -1;
    
    player->state = PLAYER_STATE_STOPPED;
    player->position = 0;
    
    // Ön belleği temizle
    player->buffer_stats.used_size = 0;
    player->buffer_stats.buffered_duration = 0;
    
    return 0;
}

// Video konumunu değiştir
int html5_player_seek(html5_player_t* player, uint32_t position) {
    if (!player) return -1;
    
    // Geçerli pozisyon kontrolü
    if (position > player->duration) {
        position = player->duration;
    }
    
    // Pozisyonu güncelle
    player->position = position;
    
    // Ön belleği temizle ve yeniden doldur
    player->buffer_stats.used_size = player->buffer_stats.total_size * 0.1; // %10
    player->buffer_stats.buffered_duration = 3000; // 3 saniye
    
    // Yeterli ön bellek dolana kadar durakla
    if (player->state == PLAYER_STATE_PLAYING) {
        player->state = PLAYER_STATE_BUFFERING;
    }
    
    return 0;
}

// Ses seviyesini ayarla
int html5_player_set_volume(html5_player_t* player, uint8_t volume) {
    if (!player) return -1;
    
    player->volume = volume > 100 ? 100 : volume;
    
    return 0;
}

// Sessiz modunu ayarla
int html5_player_mute(html5_player_t* player, uint8_t mute) {
    if (!player) return -1;
    
    player->muted = mute ? 1 : 0;
    
    return 0;
}

// Tam ekran modunu ayarla
int html5_player_set_fullscreen(html5_player_t* player, uint8_t fullscreen) {
    if (!player) return -1;
    
    player->fullscreen = fullscreen ? 1 : 0;
    
    return 0;
}

// Akış bilgilerini al
video_stream_info_t* html5_player_get_stream_info(html5_player_t* player) {
    if (!player) return NULL;
    
    return &player->stream_info;
}

// Ön bellek durumunu al
video_buffer_stats_t* html5_player_get_buffer_stats(html5_player_t* player) {
    if (!player) return NULL;
    
    return &player->buffer_stats;
}

// Donanım hızlandırmayı etkinleştir
int html5_player_enable_hardware_acceleration(html5_player_t* player) {
    if (!player) return -1;
    
    player->hardware_acceleration = 1;
    
    return 0;
}

// Donanım hızlandırmayı devre dışı bırak
int html5_player_disable_hardware_acceleration(html5_player_t* player) {
    if (!player) return -1;
    
    player->hardware_acceleration = 0;
    
    return 0;
}

// Ön bellek boyutunu ayarla
int html5_player_set_buffer_size(html5_player_t* player, uint32_t size) {
    if (!player) return -1;
    
    // Minimum 5 MB
    if (size < 5 * 1024 * 1024) {
        size = 5 * 1024 * 1024;
    }
    
    // 4K için minimum 20 MB
    if (player->stream_info.width >= 3840 && size < 20 * 1024 * 1024) {
        size = 20 * 1024 * 1024;
    }
    
    // Mevcut belleği serbest bırak
    if (player->buffer_memory) {
        free(player->buffer_memory);
    }
    
    // Yeni bellek ayır
    player->buffer_memory = malloc(size);
    if (!player->buffer_memory) {
        player->state = PLAYER_STATE_ERROR;
        return -1;
    }
    
    player->buffer_stats.total_size = size;
    player->buffer_stats.used_size = 0;
    player->buffer_stats.buffered_duration = 0;
    
    return 0;
}

// Maksimum çözünürlüğü ayarla
int html5_player_set_max_resolution(html5_player_t* player, uint32_t width, uint32_t height) {
    if (!player) return -1;
    
    player->stream_info.max_resolution = width * height;
    
    return 0;
}

// Kare işlemesi
void html5_player_process_frame(html5_player_t* player) {
    if (!player || player->state != PLAYER_STATE_PLAYING) return;
    
    // Her kare işlenmesi için bu çağrılır
    // Gerçek uygulamada codec kodunu çalıştırır ve bir kare çözer
    
    // Ön bellek durumunu güncelle
    if (player->buffer_stats.used_size > 0) {
        // Bir kare için yaklaşık bayt sayısı: bitrate / framerate / 8
        uint32_t frame_size = (uint32_t)((float)player->stream_info.bitrate / 
                                        player->stream_info.framerate / 8.0f);
        
        // Ön bellekten tüket
        if (player->buffer_stats.used_size > frame_size) {
            player->buffer_stats.used_size -= frame_size;
        } else {
            player->buffer_stats.used_size = 0;
        }
        
        // Kare başına oynatma süresi (ms)
        uint32_t frame_duration = (uint32_t)(1000.0f / player->stream_info.framerate);
        
        // Pozisyonu güncelle
        player->position += frame_duration;
        
        // Arabelleğe alınan süreyi azalt
        if (player->buffer_stats.buffered_duration > frame_duration) {
            player->buffer_stats.buffered_duration -= frame_duration;
        } else {
            player->buffer_stats.buffered_duration = 0;
        }
        
        // Video sonuna gelindi mi kontrol et
        if (player->position >= player->duration) {
            player->position = player->duration;
            player->state = PLAYER_STATE_STOPPED;
        }
        
        // Ön bellek çok azsa duraklat
        if (player->buffer_stats.used_size < 
            player->buffer_stats.total_size * 0.05) { // %5'in altında
            player->state = PLAYER_STATE_BUFFERING;
        }
    } else {
        // Ön bellek boş, arabelleğe almaya geç
        player->state = PLAYER_STATE_BUFFERING;
    }
}

// Oynatıcıyı güncelle (zamanlayıcı tarafından çağrılır)
void html5_player_update(html5_player_t* player) {
    if (!player) return;
    
    // Arabelleğe alınırken doldurmayı simüle et
    if (player->state == PLAYER_STATE_BUFFERING) {
        uint32_t buffer_increase = (uint32_t)(player->buffer_stats.fill_rate / 10.0f); // 100ms için
        player->buffer_stats.used_size += buffer_increase;
        
        // Ön bellek süresini güncelle
        float bytes_per_ms = player->stream_info.bitrate / 8.0f / 1000.0f;
        player->buffer_stats.buffered_duration += (uint32_t)(buffer_increase / bytes_per_ms);
        
        // Ön bellek dolduğunda oynatmaya başla
        if (player->buffer_stats.used_size >= 
            player->buffer_stats.total_size * MIN_BUFFER_THRESHOLD) {
            if (player->state == PLAYER_STATE_BUFFERING && 
                player->position > 0) { // 0'dan farklıysa önceki oynatma devam ediyor
                player->state = PLAYER_STATE_PLAYING;
            }
        }
        
        // Ön bellek kapasitesini aşmadığından emin ol
        if (player->buffer_stats.used_size > player->buffer_stats.total_size) {
            player->buffer_stats.used_size = player->buffer_stats.total_size;
        }
    }
    
    // Oynuyor durumundaysa kare işle
    if (player->state == PLAYER_STATE_PLAYING) {
        html5_player_process_frame(player);
    }
}

// Oynatıcıyı çiz
void html5_player_draw(html5_player_t* player, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (!player) return;
    
    // Oynatıcı arkaplanı
    gui_fill_rect(x, y, width, height, 0x000000); // Siyah arkaplan
    
    // Video içeriği (demo amaçlı)
    if (player->state == PLAYER_STATE_PLAYING || 
        player->state == PLAYER_STATE_PAUSED) {
        
        // Video içeriğini simüle et (gerçek bir uygulamada decode edilmiş kare çizilir)
        uint32_t video_width = width - 20;
        uint32_t video_height = height - 50;
        uint32_t video_x = x + 10;
        uint32_t video_y = y + 10;
        
        // Video arkaplanı
        gui_fill_rect(video_x, video_y, video_width, video_height, 0x101010);
        
        // Video içeriğini simüle et - bir renk deseni çiz
        for (uint32_t i = 0; i < video_height; i += 16) {
            for (uint32_t j = 0; j < video_width; j += 16) {
                uint8_t color_val = ((i / 16 + j / 16 + (player->position / 100)) % 200) + 50;
                uint32_t color = (color_val << 16) | (color_val << 8) | color_val;
                gui_fill_rect(video_x + j, video_y + i, 12, 12, color);
            }
        }
        
        // Duraklatıldıysa oynat simgesi göster
        if (player->state == PLAYER_STATE_PAUSED) {
            uint32_t icon_size = 40;
            uint32_t icon_x = x + (width - icon_size) / 2;
            uint32_t icon_y = y + (height - icon_size) / 2;
            
            gui_fill_rect(icon_x, icon_y, icon_size, icon_size, 0x80808080); // Yarı saydam arkaplan
            
            // Oynat üçgeni
            char play_icon[] = "▶";
            gui_draw_text(play_icon, icon_x + 12, icon_y + 8, 0xFFFFFF);
        }
        
        // Ön bellek uyarısı
        if (player->buffer_stats.used_size < 
            player->buffer_stats.total_size * 0.1) { // %10'un altında
            
            char buffer_warning[64];
            snprintf(buffer_warning, sizeof(buffer_warning), 
                    "Ön bellek: %d%%", 
                    (int)(100.0f * player->buffer_stats.used_size / player->buffer_stats.total_size));
            
            gui_draw_text(buffer_warning, x + 20, y + 30, 0xFFFF00);
        }
        
        // Kontrol çubuğu
        uint32_t ctrl_y = y + height - 30;
        gui_fill_rect(x, ctrl_y, width, 30, 0x40404080);
        
        // İlerleme çubuğu arkaplanı
        gui_fill_rect(x + 10, ctrl_y + 10, width - 20, 10, 0x808080);
        
        // İlerleme çubuğu
        if (player->duration > 0) {
            uint32_t progress_width = (width - 20) * player->position / player->duration;
            gui_fill_rect(x + 10, ctrl_y + 10, progress_width, 10, 0x00AAFF);
        }
        
        // Video bilgisi
        char video_info[128];
        char* resolution_str;
        
        if (player->stream_info.width >= 3840) {
            resolution_str = "4K UHD";
        } else if (player->stream_info.width >= 2560) {
            resolution_str = "1440p";
        } else if (player->stream_info.width >= 1920) {
            resolution_str = "1080p";
        } else if (player->stream_info.width >= 1280) {
            resolution_str = "720p";
        } else {
            resolution_str = "SD";
        }
        
        snprintf(video_info, sizeof(video_info), 
                "%s | %s | %d Mbps | %s",
                resolution_str,
                player->active_codec ? player->active_codec->name : "Bilinmeyen Codec",
                player->stream_info.bitrate / 1000000,
                player->hardware_acceleration ? "Donanım Hızlandırma" : "Yazılım İşleme");
        
        gui_draw_text(video_info, x + 10, y + height - 45, 0xFFFFFF);
        
    } else if (player->state == PLAYER_STATE_BUFFERING) {
        // Arabelleğe alma durumu
        char buffer_text[64];
        snprintf(buffer_text, sizeof(buffer_text), 
                "Arabelleğe alınıyor... %d%%", 
                (int)(100.0f * player->buffer_stats.used_size / player->buffer_stats.total_size));
        
        gui_draw_text(buffer_text, x + (width - 180) / 2, y + height / 2, 0xFFFFFF);
        
        // İlerleme çubuğu
        uint32_t bar_width = 200;
        uint32_t bar_x = x + (width - bar_width) / 2;
        uint32_t bar_y = y + height / 2 + 20;
        
        gui_fill_rect(bar_x, bar_y, bar_width, 10, 0x808080);
        
        uint32_t fill_width = bar_width * player->buffer_stats.used_size / player->buffer_stats.total_size;
        gui_fill_rect(bar_x, bar_y, fill_width, 10, 0x00AAFF);
        
    } else {
        // Hazır değil - başlatma mesajı
        gui_draw_text("Video yükleniyor...", x + (width - 120) / 2, y + height / 2, 0xFFFFFF);
    }
} 