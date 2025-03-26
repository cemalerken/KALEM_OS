#include "../include/media_player.h"
#include "../include/gui.h"
#include "../include/font.h"
#include "../include/vga.h"
#include <string.h>
#include <stdlib.h>

#define MAX_CODECS 32
#define MAX_MEDIA_FILES 100
#define MAX_PLAYLISTS 10

// Global değişkenler
static media_player_t* player = NULL;

// Kayıtlı codec'ler
static codec_package_t* registered_codecs[MAX_CODECS] = {0};
static uint32_t codec_count = 0;

// Demo medya dosyaları
static media_file_t media_files[MAX_MEDIA_FILES];
static uint32_t media_file_count = 0;

// Demo çalma listeleri
static playlist_t playlists[MAX_PLAYLISTS];
static uint32_t playlist_count = 0;

// İleri bildirimler
static void media_player_paint(gui_window_t* window);
static void media_player_create_demo_files();
static void media_player_create_demo_codecs();
static void media_player_update_ui();
static void media_player_draw_controls(uint32_t x, uint32_t y, uint32_t width);
static void media_player_draw_playlist(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
static void media_player_draw_video(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
static void media_player_draw_audio_visualizer(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
static media_format_t media_player_detect_format(const char* filename);
static codec_t media_player_detect_codec(media_format_t format, uint8_t is_video);

// Medya oynatıcı başlatma
void media_player_init() {
    // Zaten başlatılmışsa çık
    if (player) return;
    
    // Oynatıcı yapısı için bellek ayır
    player = (media_player_t*)malloc(sizeof(media_player_t));
    if (!player) return;
    
    // Varsayılan değerleri ayarla
    player->window = NULL;
    player->volume = 80;          // %80 ses
    player->muted = 0;            // Sessiz değil
    player->state = PLAYER_STATE_STOPPED;
    player->current_file = NULL;
    player->current_playlist = NULL;
    player->position = 0;
    player->repeat = 0;           // Tekrar kapalı
    player->shuffle = 0;          // Karıştır kapalı
    player->codec_data = NULL;
    player->fullscreen = 0;       // Tam ekran değil
    
    // Demo medya dosyaları ve codec'leri oluştur
    media_player_create_demo_files();
    media_player_create_demo_codecs();
    
    // İlk dosyayı seç (varsa)
    if (media_file_count > 0) {
        player->current_file = &media_files[0];
    }
}

// Medya oynatıcıyı göster
void media_player_show() {
    // Medya oynatıcı başlatılmamışsa başlat
    if (!player) {
        media_player_init();
    }
    
    // Zaten pencere açıksa, odaklan ve çık
    if (player->window) {
        gui_window_focus(player->window);
        return;
    }
    
    // Yeni bir pencere oluştur
    player->window = gui_window_create("KALEM OS Medya Oynatıcı", 50, 50, 800, 600, GUI_WINDOW_STYLE_NORMAL);
    if (!player->window) return;
    
    // Pencere çizim fonksiyonunu ayarla
    player->window->on_paint = media_player_paint;
    
    // Pencereyi göster
    gui_window_show(player->window);
    
    // UI güncelleme zamanlayıcısı başlat
    gui_add_timer(media_player_update_ui, 100); // Her 100ms'de bir güncelle
}

// Pencere çizim fonksiyonu
static void media_player_paint(gui_window_t* window) {
    if (!window || !player) return;
    
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    uint32_t width = window->client.width;
    uint32_t height = window->client.height;
    
    // Arkaplanı temizle
    gui_fill_rect(x, y, width, height, GUI_COLOR_WINDOW_BG);
    
    // Başlık çubuğu
    gui_fill_rect(x, y, width, 30, GUI_COLOR_BLUE);
    
    // Başlık metni
    const char* title = player->current_file ? 
                       player->current_file->title : 
                       "KALEM OS Medya Oynatıcı";
    
    gui_draw_text(title, x + 10, y + 10, GUI_COLOR_WHITE);
    
    // Medya içeriği
    if (player->current_file) {
        // Oynatıcı kontrolleri (altta)
        media_player_draw_controls(x, y + height - 60, width);
        
        // İçerik türüne göre farklı görselleştirme
        if (player->current_file->type == MEDIA_TYPE_VIDEO) {
            // Video ve çalma listesi
            media_player_draw_video(x + 10, y + 40, width - 220, height - 110);
            media_player_draw_playlist(x + width - 200, y + 40, 190, height - 110);
        } else {
            // Ses görselleştirici ve çalma listesi
            media_player_draw_audio_visualizer(x + 10, y + 40, width - 220, height - 110);
            media_player_draw_playlist(x + width - 200, y + 40, 190, height - 110);
        }
    } else {
        // Dosya açma bildirimi
        gui_draw_text("Medya dosyası açmak için tıklayın", 
                      x + width / 2 - 120, y + height / 2 - 20, GUI_COLOR_BLACK);
    }
}

// Oynatıcı kontrollerini çiz
static void media_player_draw_controls(uint32_t x, uint32_t y, uint32_t width) {
    if (!player || !player->current_file) return;
    
    const uint32_t CONTROL_HEIGHT = 40;
    const uint32_t BUTTON_SIZE = 30;
    const uint32_t BUTTON_MARGIN = 10;
    const uint32_t PROGRESS_HEIGHT = 6;
    
    // Arkaplan çiz
    gui_fill_rect(x, y, width, CONTROL_HEIGHT, 0x303030);
    
    uint32_t button_x = x + BUTTON_MARGIN;
    
    // Geri düğmesi
    gui_fill_rect(button_x, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2, 
                 BUTTON_SIZE, BUTTON_SIZE, 0x404040);
    gui_draw_text("⏮", button_x + 6, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2 + 6, 0xFFFFFF);
    
    button_x += BUTTON_SIZE + BUTTON_MARGIN;
    
    // Oynat/Duraklat düğmesi
    gui_fill_rect(button_x, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2, 
                 BUTTON_SIZE, BUTTON_SIZE, 0x404040);
    
    if (player->state == PLAYER_STATE_PLAYING) {
        gui_draw_text("⏸", button_x + 8, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2 + 6, 0xFFFFFF);
    } else {
        gui_draw_text("▶", button_x + 10, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2 + 6, 0xFFFFFF);
    }
    
    button_x += BUTTON_SIZE + BUTTON_MARGIN;
    
    // Durdur düğmesi
    gui_fill_rect(button_x, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2, 
                 BUTTON_SIZE, BUTTON_SIZE, 0x404040);
    gui_draw_text("⏹", button_x + 8, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2 + 6, 0xFFFFFF);
    
    button_x += BUTTON_SIZE + BUTTON_MARGIN;
    
    // İleri düğmesi
    gui_fill_rect(button_x, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2, 
                 BUTTON_SIZE, BUTTON_SIZE, 0x404040);
    gui_draw_text("⏭", button_x + 6, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2 + 6, 0xFFFFFF);
    
    button_x += BUTTON_SIZE + BUTTON_MARGIN;
    
    // Ses kontrolü
    gui_fill_rect(button_x, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2, 
                 BUTTON_SIZE, BUTTON_SIZE, 0x404040);
    
    if (player->muted) {
        gui_draw_text("🔇", button_x + 6, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2 + 6, 0xFFFFFF);
    } else if (player->volume < 30) {
        gui_draw_text("🔈", button_x + 6, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2 + 6, 0xFFFFFF);
    } else if (player->volume < 70) {
        gui_draw_text("🔉", button_x + 6, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2 + 6, 0xFFFFFF);
    } else {
        gui_draw_text("🔊", button_x + 6, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2 + 6, 0xFFFFFF);
    }
    
    button_x += BUTTON_SIZE + BUTTON_MARGIN;
    
    // Ses seviyesi çubuğu
    uint32_t volume_bar_width = 100;
    uint32_t volume_fill_width = (volume_bar_width * player->volume) / 100;
    
    gui_fill_rect(button_x, y + CONTROL_HEIGHT / 2 - 3, 
                 volume_bar_width, 6, 0x505050);
    
    if (!player->muted) {
        gui_fill_rect(button_x, y + CONTROL_HEIGHT / 2 - 3, 
                     volume_fill_width, 6, 0x00AAFF);
    }
    
    button_x += volume_bar_width + BUTTON_MARGIN * 2;
    
    // Tam ekran düğmesi (sadece video için)
    if (player->current_file->type == MEDIA_TYPE_VIDEO) {
        gui_fill_rect(button_x, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2, 
                     BUTTON_SIZE, BUTTON_SIZE, 0x404040);
        
        if (player->fullscreen) {
            gui_draw_text("🗗", button_x + 8, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2 + 6, 0xFFFFFF);
        } else {
            gui_draw_text("🗖", button_x + 8, y + (CONTROL_HEIGHT - BUTTON_SIZE) / 2 + 6, 0xFFFFFF);
        }
    }
    
    // İlerleme çubuğu (kontrollerden üstte)
    uint32_t progress_y = y - PROGRESS_HEIGHT;
    
    // İlerleme çubuğu arkaplan
    gui_fill_rect(x, progress_y, width, PROGRESS_HEIGHT, 0x505050);
    
    // İlerleme dolgusu
    if (player->current_file->duration > 0) {
        uint32_t progress_width = (width * player->position) / player->current_file->duration;
        gui_fill_rect(x, progress_y, progress_width, PROGRESS_HEIGHT, 0x00AAFF);
    }
    
    // Zaman bilgisi
    char time_text[16];
    char duration_text[16];
    
    format_time(player->position, time_text, sizeof(time_text));
    format_time(player->current_file->duration, duration_text, sizeof(duration_text));
    
    uint32_t duration_width = gui_get_text_width(duration_text);
    
    gui_draw_text(time_text, x + 10, progress_y - 20, 0xFFFFFF);
    gui_draw_text(duration_text, x + width - duration_width - 10, progress_y - 20, 0xFFFFFF);
}

// Video içeriğini çiz
static void media_player_draw_video(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (!player || !player->current_file || 
        player->current_file->type != MEDIA_TYPE_VIDEO) return;
    
    // Video içeriğini simüle et (sadece demo amaçlı)
    
    // Video arkaplanı (siyah)
    gui_fill_rect(x, y, width, height, 0x000000);
    
    // Merkezi bir logo veya simge
    const char* video_title = player->current_file->title;
    uint32_t title_width = gui_get_text_width(video_title);
    
    // Ekran ortasına başlık çiz
    gui_draw_text(video_title, x + (width - title_width) / 2, 
                 y + height / 2 - 10, 0xFFFFFF);
    
    // Video format ve codec bilgisini göster
    char info_text[100];
    snprintf(info_text, sizeof(info_text), "%s (%s), %ldx%ld, %s",
            media_format_to_string(player->current_file->format),
            codec_to_string(player->current_file->video_codec),
            player->current_file->width, player->current_file->height,
            codec_to_string(player->current_file->audio_codec));
    
    uint32_t info_width = gui_get_text_width(info_text);
    gui_draw_text(info_text, x + (width - info_width) / 2, 
                 y + height / 2 + 10, 0xAAAAAA);
    
    // Oynatma durumu ikonunu göster (duraklatıldığında/durdurulduğunda)
    if (player->state == PLAYER_STATE_PAUSED) {
        // Büyük duraklat simgesi
        gui_draw_text("⏸", x + width / 2 - 15, y + height / 2 - 40, 0xFFFFFF);
    } else if (player->state == PLAYER_STATE_STOPPED) {
        // Büyük durdur simgesi
        gui_draw_text("⏹", x + width / 2 - 15, y + height / 2 - 40, 0xFFFFFF);
    }
}

// Ses görselleştirici çiz
static void media_player_draw_audio_visualizer(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (!player || !player->current_file || 
        player->current_file->type != MEDIA_TYPE_AUDIO) return;
    
    // Ses görselleştiriciyi simüle et (sadece demo amaçlı)
    
    // Arkaplan
    gui_fill_rect(x, y, width, height, 0x101010);
    
    // Kapak resmi (simüle edilmiş)
    uint32_t album_art_size = height - 40;
    uint32_t album_art_x = x + (width - album_art_size) / 2;
    uint32_t album_art_y = y + 20;
    
    // Albüm kapağı arkaplanı
    gui_fill_rect(album_art_x, album_art_y, album_art_size, album_art_size, 0x303030);
    
    // Albüm adını göster
    const char* album_name = player->current_file->album;
    uint32_t album_width = gui_get_text_width(album_name);
    
    gui_draw_text(album_name, x + (width - album_width) / 2, 
                 album_art_y + album_art_size + 10, 0xCCCCCC);
    
    // Ses dalgaları (basit frekans çubukları)
    if (player->state == PLAYER_STATE_PLAYING) {
        // Ses spektrumunu simüle et
        const uint32_t num_bars = 32;
        const uint32_t bar_width = (album_art_size - (num_bars - 1) * 2) / num_bars;
        const uint32_t max_height = album_art_size - 40;
        
        for (uint32_t i = 0; i < num_bars; i++) {
            // Sahte rastgele yükseklik oluştur (pozisyona dayalı)
            uint32_t r = (i * 7 + player->position * 3) % 100;
            uint32_t height_pct = 30 + (r * 70) / 100; // %30-%100 arası
            
            if (player->state != PLAYER_STATE_PLAYING) {
                height_pct = 30; // Oynatılmıyorsa düşük çubuklar
            }
            
            uint32_t bar_height = (max_height * height_pct) / 100;
            uint32_t bar_x = album_art_x + 20 + i * (bar_width + 2);
            uint32_t bar_y = album_art_y + 20 + (max_height - bar_height);
            
            // Frekans çubuğunu çiz (mavi tonlarında)
            uint32_t color = 0x0066FF - (i * 4 << 8) + (i * 4);
            gui_fill_rect(bar_x, bar_y, bar_width, bar_height, color);
        }
    }
    
    // Oynatma durumu
    const char* status_text = "Durduruldu";
    if (player->state == PLAYER_STATE_PLAYING) {
        status_text = "Oynatılıyor";
    } else if (player->state == PLAYER_STATE_PAUSED) {
        status_text = "Duraklatıldı";
    }
    
    uint32_t status_width = gui_get_text_width(status_text);
    gui_draw_text(status_text, x + (width - status_width) / 2, 
                 y + 5, 0xFFFFFF);
    
    // Format ve codec bilgisi
    char info_text[100];
    snprintf(info_text, sizeof(info_text), "%s (%s), %d kbps, %d Hz, %d kanal",
            media_format_to_string(player->current_file->format),
            codec_to_string(player->current_file->audio_codec),
            player->current_file->bitrate / 1000,
            player->current_file->sample_rate,
            player->current_file->channels);
    
    uint32_t info_width = gui_get_text_width(info_text);
    gui_draw_text(info_text, x + (width - info_width) / 2, 
                 album_art_y + album_art_size + 30, 0xAAAAAA);
}

// Çalma listesini çiz
static void media_player_draw_playlist(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (!player || !player->current_playlist) return;
    
    // Arkaplan
    gui_fill_rect(x, y, width, height, 0x202020);
    
    // Başlık
    char title[64];
    snprintf(title, sizeof(title), "Çalma Listesi: %s", player->current_playlist->name);
    gui_draw_text(title, x + 10, y + 10, 0xFFFFFF);
    
    // Liste öğeleri
    const uint32_t item_height = 30;
    const uint32_t list_start_y = y + 40;
    
    if (player->current_playlist->file_count == 0) {
        // Boş liste mesajı
        gui_draw_text("Çalma Listesi Boş", x + (width - 120) / 2, y + height / 2, 0xAAAAAA);
        return;
    }
    
    for (uint32_t i = 0; i < player->current_playlist->file_count; i++) {
        media_file_t* file = player->current_playlist->files[i];
        
        // Öğe arkaplanı
        uint32_t bg_color = (i == player->current_playlist->current_index) ? 0x303060 : 0x303030;
        gui_fill_rect(x + 5, list_start_y + i * item_height, width - 10, item_height - 2, bg_color);
        
        // Dosya adı
        char display_text[128];
        snprintf(display_text, sizeof(display_text), "%d. %s - %s",
                i + 1, file->artist, file->title);
        
        // Güncel çalınan dosyayı vurgula
        uint32_t text_color = (i == player->current_playlist->current_index) ? 0xFFFFFF : 0xCCCCCC;
        gui_draw_text(display_text, x + 15, list_start_y + i * item_height + 7, text_color);
        
        // Süre bilgisi
        char duration[10];
        format_time(file->duration, duration, sizeof(duration));
        
        uint32_t duration_width = gui_get_text_width(duration);
        gui_draw_text(duration, x + width - duration_width - 15, 
                     list_start_y + i * item_height + 7, text_color);
    }
}

// Demo medya dosyaları oluştur
static void media_player_create_demo_files() {
    // Demo ses dosyaları
    media_file_t audio1 = {
        .filename = "Turku.mp3",
        .title = "Sarı Çiçek",
        .artist = "Neşet Ertaş",
        .album = "Türküler",
        .duration = 241,  // 4:01
        .type = MEDIA_TYPE_AUDIO,
        .format = MEDIA_FORMAT_MP3,
        .audio_codec = CODEC_MP3,
        .video_codec = CODEC_UNKNOWN,
        .width = 0,
        .height = 0,
        .bitrate = 320000,  // 320 kbps
        .sample_rate = 44100,
        .channels = 2,
        .data = NULL
    };
    media_files[media_file_count++] = audio1;
    
    media_file_t audio2 = {
        .filename = "Pop.mp3",
        .title = "Vazgeç Gönlüm",
        .artist = "Tarkan",
        .album = "Metamorfoz",
        .duration = 224,  // 3:44
        .type = MEDIA_TYPE_AUDIO,
        .format = MEDIA_FORMAT_MP3,
        .audio_codec = CODEC_MP3,
        .video_codec = CODEC_UNKNOWN,
        .width = 0,
        .height = 0,
        .bitrate = 320000,  // 320 kbps
        .sample_rate = 44100,
        .channels = 2,
        .data = NULL
    };
    media_files[media_file_count++] = audio2;
    
    media_file_t audio3 = {
        .filename = "Klasik.flac",
        .title = "Rondo Alla Turca",
        .artist = "Mozart",
        .album = "Klasik Eserler",
        .duration = 185,  // 3:05
        .type = MEDIA_TYPE_AUDIO,
        .format = MEDIA_FORMAT_FLAC,
        .audio_codec = CODEC_FLAC,
        .video_codec = CODEC_UNKNOWN,
        .width = 0,
        .height = 0,
        .bitrate = 1411000,  // 1411 kbps (CD kalitesi)
        .sample_rate = 48000,
        .channels = 2,
        .data = NULL
    };
    media_files[media_file_count++] = audio3;
    
    // Demo video dosyaları
    media_file_t video1 = {
        .filename = "Belgesel.mp4",
        .title = "Anadolu'nun Renkleri",
        .artist = "TRT Belgesel",
        .album = "Türkiye Belgeselleri",
        .duration = 1825,  // 30:25
        .type = MEDIA_TYPE_VIDEO,
        .format = MEDIA_FORMAT_MP4,
        .audio_codec = CODEC_AAC,
        .video_codec = CODEC_H264,
        .width = 1920,
        .height = 1080,
        .bitrate = 8000000,  // 8 Mbps
        .sample_rate = 48000,
        .channels = 2,
        .data = NULL
    };
    media_files[media_file_count++] = video1;
    
    media_file_t video2 = {
        .filename = "Film.mkv",
        .title = "Selvi Boylum Al Yazmalım",
        .artist = "Atıf Yılmaz",
        .album = "Türk Filmleri",
        .duration = 5400,  // 1:30:00
        .type = MEDIA_TYPE_VIDEO,
        .format = MEDIA_FORMAT_MKV,
        .audio_codec = CODEC_MP3,
        .video_codec = CODEC_H264,
        .width = 1280,
        .height = 720,
        .bitrate = 5000000,  // 5 Mbps
        .sample_rate = 44100,
        .channels = 2,
        .data = NULL
    };
    media_files[media_file_count++] = video2;
    
    media_file_t video3 = {
        .filename = "Klip.webm",
        .title = "Leylim Ley",
        .artist = "Barış Manço",
        .album = "Müzik Klipleri",
        .duration = 245,  // 4:05
        .type = MEDIA_TYPE_VIDEO,
        .format = MEDIA_FORMAT_WEBM,
        .audio_codec = CODEC_VORBIS,
        .video_codec = CODEC_VP9,
        .width = 1280,
        .height = 720,
        .bitrate = 3000000,  // 3 Mbps
        .sample_rate = 44100,
        .channels = 2,
        .data = NULL
    };
    media_files[media_file_count++] = video3;
    
    // Demo çalma listesi oluştur
    playlist_t turkce_muzikler = {
        .name = "Türkçe Müzikler",
        .files = (media_file_t**)malloc(3 * sizeof(media_file_t*)),
        .file_count = 3,
        .current_index = 0
    };
    
    turkce_muzikler.files[0] = &media_files[0]; // Sarı Çiçek
    turkce_muzikler.files[1] = &media_files[1]; // Vazgeç Gönlüm
    turkce_muzikler.files[2] = &media_files[2]; // Rondo Alla Turca
    
    playlists[playlist_count++] = turkce_muzikler;
    
    playlist_t videolar = {
        .name = "Videolar",
        .files = (media_file_t**)malloc(3 * sizeof(media_file_t*)),
        .file_count = 3,
        .current_index = 0
    };
    
    videolar.files[0] = &media_files[3]; // Belgesel
    videolar.files[1] = &media_files[4]; // Film
    videolar.files[2] = &media_files[5]; // Klip
    
    playlists[playlist_count++] = videolar;
    
    // Aktif çalma listesini ayarla
    if (playlist_count > 0) {
        player->current_playlist = &playlists[0];
        player->current_file = playlists[0].files[0];
    }
}

// Demo codecler oluştur
static void media_player_create_demo_codecs() {
    // Ses codeclerini oluştur
    codec_package_t* mp3_codec = (codec_package_t*)malloc(sizeof(codec_package_t));
    mp3_codec->name = "MP3 Codec";
    mp3_codec->type = CODEC_MP3;
    mp3_codec->init = NULL;
    mp3_codec->decode = NULL;
    mp3_codec->close = NULL;
    
    codec_package_t* aac_codec = (codec_package_t*)malloc(sizeof(codec_package_t));
    aac_codec->name = "AAC Codec";
    aac_codec->type = CODEC_AAC;
    aac_codec->init = NULL;
    aac_codec->decode = NULL;
    aac_codec->close = NULL;
    
    codec_package_t* flac_codec = (codec_package_t*)malloc(sizeof(codec_package_t));
    flac_codec->name = "FLAC Codec";
    flac_codec->type = CODEC_FLAC;
    flac_codec->init = NULL;
    flac_codec->decode = NULL;
    flac_codec->close = NULL;
    
    codec_package_t* vorbis_codec = (codec_package_t*)malloc(sizeof(codec_package_t));
    vorbis_codec->name = "Vorbis Codec";
    vorbis_codec->type = CODEC_VORBIS;
    vorbis_codec->init = NULL;
    vorbis_codec->decode = NULL;
    vorbis_codec->close = NULL;
    
    // Video codeclerini oluştur
    codec_package_t* h264_codec = (codec_package_t*)malloc(sizeof(codec_package_t));
    h264_codec->name = "H.264 Codec";
    h264_codec->type = CODEC_H264;
    h264_codec->init = NULL;
    h264_codec->decode = NULL;
    h264_codec->close = NULL;
    
    codec_package_t* vp9_codec = (codec_package_t*)malloc(sizeof(codec_package_t));
    vp9_codec->name = "VP9 Codec";
    vp9_codec->type = CODEC_VP9;
    vp9_codec->init = NULL;
    vp9_codec->decode = NULL;
    vp9_codec->close = NULL;
    
    // Codec'leri kaydet
    media_player_register_codec(mp3_codec);
    media_player_register_codec(aac_codec);
    media_player_register_codec(flac_codec);
    media_player_register_codec(vorbis_codec);
    media_player_register_codec(h264_codec);
    media_player_register_codec(vp9_codec);
}

// Dosya formatını algıla
static media_format_t media_player_detect_format(const char* filename) {
    if (!filename) return MEDIA_FORMAT_UNKNOWN;
    
    const char* ext = strrchr(filename, '.');
    if (!ext) return MEDIA_FORMAT_UNKNOWN;
    
    // Dosya uzantısını küçük harfe çevir
    char ext_lower[10];
    uint32_t i = 0;
    ext++; // '.' karakterini atla
    
    while (ext[i] && i < 9) {
        ext_lower[i] = ext[i] >= 'A' && ext[i] <= 'Z' ? ext[i] + 32 : ext[i];
        i++;
    }
    ext_lower[i] = '\0';
    
    // Formatı belirle
    if (strcmp(ext_lower, "mp3") == 0) return MEDIA_FORMAT_MP3;
    if (strcmp(ext_lower, "wav") == 0) return MEDIA_FORMAT_WAV;
    if (strcmp(ext_lower, "ogg") == 0) return MEDIA_FORMAT_OGG;
    if (strcmp(ext_lower, "flac") == 0) return MEDIA_FORMAT_FLAC;
    if (strcmp(ext_lower, "aac") == 0) return MEDIA_FORMAT_AAC;
    if (strcmp(ext_lower, "wma") == 0) return MEDIA_FORMAT_WMA;
    if (strcmp(ext_lower, "mp4") == 0) return MEDIA_FORMAT_MP4;
    if (strcmp(ext_lower, "avi") == 0) return MEDIA_FORMAT_AVI;
    if (strcmp(ext_lower, "mkv") == 0) return MEDIA_FORMAT_MKV;
    if (strcmp(ext_lower, "mov") == 0) return MEDIA_FORMAT_MOV;
    if (strcmp(ext_lower, "wmv") == 0) return MEDIA_FORMAT_WMV;
    if (strcmp(ext_lower, "flv") == 0) return MEDIA_FORMAT_FLV;
    if (strcmp(ext_lower, "webm") == 0) return MEDIA_FORMAT_WEBM;
    
    return MEDIA_FORMAT_UNKNOWN;
}

// Varsayılan codec algıla
static codec_t media_player_detect_codec(media_format_t format, uint8_t is_video) {
    if (is_video) {
        // Video codec'i algıla
        switch (format) {
            case MEDIA_FORMAT_MP4:
            case MEDIA_FORMAT_AVI:
            case MEDIA_FORMAT_MKV:
            case MEDIA_FORMAT_MOV:
                return CODEC_H264;
            case MEDIA_FORMAT_WEBM:
                return CODEC_VP9;
            case MEDIA_FORMAT_FLV:
                return CODEC_VP8;
            case MEDIA_FORMAT_WMV:
                return CODEC_WMA;
            default:
                return CODEC_UNKNOWN;
        }
    } else {
        // Ses codec'i algıla
        switch (format) {
            case MEDIA_FORMAT_MP3:
                return CODEC_MP3;
            case MEDIA_FORMAT_WAV:
                return CODEC_PCM;
            case MEDIA_FORMAT_OGG:
                return CODEC_VORBIS;
            case MEDIA_FORMAT_FLAC:
                return CODEC_FLAC;
            case MEDIA_FORMAT_AAC:
            case MEDIA_FORMAT_MP4:
                return CODEC_AAC;
            case MEDIA_FORMAT_WMA:
            case MEDIA_FORMAT_WMV:
                return CODEC_WMA;
            case MEDIA_FORMAT_WEBM:
                return CODEC_VORBIS;
            default:
                return CODEC_UNKNOWN;
        }
    }
}

// Codec paketi kaydet
void media_player_register_codec(codec_package_t* codec) {
    if (!codec || codec_count >= MAX_CODECS) return;
    registered_codecs[codec_count++] = codec;
}

// Codec paketi bul
codec_package_t* media_player_find_codec(codec_t type) {
    for (uint32_t i = 0; i < codec_count; i++) {
        if (registered_codecs[i]->type == type) {
            return registered_codecs[i];
        }
    }
    return NULL;
}

// Format adını döndür
const char* media_format_to_string(media_format_t format) {
    switch (format) {
        case MEDIA_FORMAT_MP3: return "MP3";
        case MEDIA_FORMAT_WAV: return "WAV";
        case MEDIA_FORMAT_OGG: return "OGG";
        case MEDIA_FORMAT_FLAC: return "FLAC";
        case MEDIA_FORMAT_AAC: return "AAC";
        case MEDIA_FORMAT_WMA: return "WMA";
        case MEDIA_FORMAT_MP4: return "MP4";
        case MEDIA_FORMAT_AVI: return "AVI";
        case MEDIA_FORMAT_MKV: return "MKV";
        case MEDIA_FORMAT_MOV: return "MOV";
        case MEDIA_FORMAT_WMV: return "WMV";
        case MEDIA_FORMAT_FLV: return "FLV";
        case MEDIA_FORMAT_WEBM: return "WebM";
        default: return "Bilinmeyen";
    }
}

// Codec adını döndür
const char* codec_to_string(codec_t codec) {
    switch (codec) {
        case CODEC_MP3: return "MP3";
        case CODEC_AAC: return "AAC";
        case CODEC_VORBIS: return "Vorbis";
        case CODEC_FLAC: return "FLAC";
        case CODEC_PCM: return "PCM";
        case CODEC_WMA: return "WMA";
        case CODEC_H264: return "H.264";
        case CODEC_H265: return "H.265";
        case CODEC_VP8: return "VP8";
        case CODEC_VP9: return "VP9";
        case CODEC_MPEG4: return "MPEG-4";
        case CODEC_AV1: return "AV1";
        case CODEC_THEORA: return "Theora";
        default: return "Bilinmeyen";
    }
}

// Dosya aç
int media_player_open_file(const char* filename) {
    if (!player || !filename) return -1;
    
    // Gerçek sistemde, bu noktada dosya açılır ve içeriği analiz edilir
    // Demo için varolan listemizden bir dosya bulalım
    
    // Önce tam dosya adı eşleşmesi ara
    for (uint32_t i = 0; i < media_file_count; i++) {
        if (strcmp(media_files[i].filename, filename) == 0) {
            player->current_file = &media_files[i];
            player->position = 0;
            player->state = PLAYER_STATE_STOPPED;
            return 0;
        }
    }
    
    // Eşleşme bulunamadıysa yeni bir dosya simüle et
    media_format_t format = media_player_detect_format(filename);
    if (format == MEDIA_FORMAT_UNKNOWN) return -2;
    
    // Ses mi video mu belirle (basitçe: format tipine göre)
    uint8_t is_video = (format >= MEDIA_FORMAT_MP4 && format <= MEDIA_FORMAT_WEBM);
    
    // Yeni bir medya dosyası oluştur
    if (media_file_count < MAX_MEDIA_FILES) {
        media_file_t new_file = {0};
        
        // Dosya adını kopyala
        strncpy(new_file.filename, filename, sizeof(new_file.filename) - 1);
        
        // Başlığı dosya adından al (uzantısız)
        char* dot = strrchr(new_file.filename, '.');
        if (dot) {
            uint32_t name_len = dot - new_file.filename;
            strncpy(new_file.title, new_file.filename, name_len);
            new_file.title[name_len] = '\0';
        } else {
            strcpy(new_file.title, "İsimsiz");
        }
        
        // Varsayılan değerler
        strcpy(new_file.artist, "Bilinmeyen Sanatçı");
        strcpy(new_file.album, "Bilinmeyen Albüm");
        new_file.duration = 180 + rand() % 240; // 3-7 dakika arası
        new_file.type = is_video ? MEDIA_TYPE_VIDEO : MEDIA_TYPE_AUDIO;
        new_file.format = format;
        new_file.audio_codec = media_player_detect_codec(format, 0);
        new_file.video_codec = is_video ? media_player_detect_codec(format, 1) : CODEC_UNKNOWN;
        
        if (is_video) {
            new_file.width = 1280;
            new_file.height = 720;
            new_file.bitrate = 5000000; // 5 Mbps
        } else {
            new_file.width = 0;
            new_file.height = 0;
            new_file.bitrate = 320000; // 320 kbps
        }
        
        new_file.sample_rate = 44100;
        new_file.channels = 2;
        new_file.data = NULL;
        
        // Dosyayı listeye ekle
        media_files[media_file_count++] = new_file;
        
        // Geçerli dosyayı güncelle
        player->current_file = &media_files[media_file_count - 1];
        player->position = 0;
        player->state = PLAYER_STATE_STOPPED;
        
        return 0;
    }
    
    return -3; // Daha fazla dosya eklenemez
}

// Oynat
int media_player_play() {
    if (!player || !player->current_file) return -1;
    
    player->state = PLAYER_STATE_PLAYING;
    
    // Gerçek bir oynatıcıda burada codec başlatılır ve oynatma başlar
    // Simülasyon amaçlı sadece durum değişir
    
    return 0;
}

// Duraklat
int media_player_pause() {
    if (!player || !player->current_file) return -1;
    
    if (player->state == PLAYER_STATE_PLAYING) {
        player->state = PLAYER_STATE_PAUSED;
    } else if (player->state == PLAYER_STATE_PAUSED) {
        player->state = PLAYER_STATE_PLAYING;
    }
    
    return 0;
}

// Durdur
int media_player_stop() {
    if (!player || !player->current_file) return -1;
    
    player->state = PLAYER_STATE_STOPPED;
    player->position = 0;
    
    return 0;
}

// İleri/geri sar
int media_player_seek(uint32_t position) {
    if (!player || !player->current_file) return -1;
    
    // Pozisyon dosya süresini aşamasın
    if (position > player->current_file->duration) {
        position = player->current_file->duration;
    }
    
    player->position = position;
    
    return 0;
}

// Ses seviyesi ayarla
int media_player_set_volume(uint32_t volume) {
    if (!player) return -1;
    
    // Ses seviyesini sınırla (0-100)
    if (volume > 100) volume = 100;
    
    player->volume = volume;
    
    // Ses kapalıysa ve ses seviyesi artırıldıysa sessiz modu kapat
    if (player->muted && volume > 0) {
        player->muted = 0;
    }
    
    return 0;
}

// Sessiz modu aç/kapat
int media_player_mute(uint8_t mute) {
    if (!player) return -1;
    
    player->muted = mute ? 1 : 0;
    
    return 0;
}

// Sonraki parça
int media_player_next() {
    if (!player || !player->current_playlist || 
        player->current_playlist->file_count <= 1) return -1;
    
    // Sonraki parçaya geç
    player->current_playlist->current_index++;
    
    // Listenin sonuna geldiyse başa dön
    if (player->current_playlist->current_index >= player->current_playlist->file_count) {
        player->current_playlist->current_index = 0;
    }
    
    // Yeni dosyayı ayarla
    player->current_file = player->current_playlist->files[player->current_playlist->current_index];
    player->position = 0;
    
    // Eğer çalıyorsa yeni parçayı çal
    if (player->state == PLAYER_STATE_PLAYING) {
        player->state = PLAYER_STATE_STOPPED;
        media_player_play();
    } else {
        player->state = PLAYER_STATE_STOPPED;
    }
    
    return 0;
}

// Önceki parça
int media_player_previous() {
    if (!player || !player->current_playlist || 
        player->current_playlist->file_count <= 1) return -1;
    
    // Eğer parça 3 saniyeden fazla çalıyorsa, başa sar
    if (player->position > 3) {
        player->position = 0;
        return 0;
    }
    
    // Önceki parçaya geç
    if (player->current_playlist->current_index > 0) {
        player->current_playlist->current_index--;
    } else {
        player->current_playlist->current_index = player->current_playlist->file_count - 1;
    }
    
    // Yeni dosyayı ayarla
    player->current_file = player->current_playlist->files[player->current_playlist->current_index];
    player->position = 0;
    
    // Eğer çalıyorsa yeni parçayı çal
    if (player->state == PLAYER_STATE_PLAYING) {
        player->state = PLAYER_STATE_STOPPED;
        media_player_play();
    } else {
        player->state = PLAYER_STATE_STOPPED;
    }
    
    return 0;
}

// Tam ekran modunu değiştir
void media_player_toggle_fullscreen() {
    if (!player || !player->current_file || 
        player->current_file->type != MEDIA_TYPE_VIDEO) return;
    
    player->fullscreen = !player->fullscreen;
    
    // Pencere boyutunu güncelle
    if (player->window) {
        if (player->fullscreen) {
            // Tam ekran yap
            gui_window_set_size(player->window, vga_get_width(), vga_get_height());
            gui_window_set_position(player->window, 0, 0);
        } else {
            // Normal boyuta getir
            gui_window_set_size(player->window, 640, 480);
            gui_window_set_position(player->window, 50, 50);
        }
    }
}

// Medya dosyalarının listesini güncelle
static void media_player_update_ui() {
    if (!player || !player->window) return;
    
    // Pencereyi yeniden çizilmeye zorla
    gui_window_invalidate(player->window);
    
    // Gerçek bir oynatıcıda burada diğer parçaları da güncellerdik
    // UI bileşenlerini güncellemek için simülasyon kodu:
    
    // Oynatma sırasında pozisyonu ilerlet
    if (player->state == PLAYER_STATE_PLAYING && player->current_file) {
        player->position++;
        
        // Dosya sonuna gelindiyse sonraki parçaya geç
        if (player->position >= player->current_file->duration) {
            media_player_next();
        }
    }
}

// Zaman formatını döndür (SS:DD)
static void format_time(uint32_t seconds, char* buffer, uint32_t buffer_size) {
    uint32_t mins = seconds / 60;
    uint32_t secs = seconds % 60;
    
    snprintf(buffer, buffer_size, "%02u:%02u", mins, secs);
} 