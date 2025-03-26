#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "gui.h"
#include <stdint.h>

// Ayarlar kategorileri
typedef enum {
    SETTINGS_CAT_SYSTEM,
    SETTINGS_CAT_DISPLAY,
    SETTINGS_CAT_SOUND,
    SETTINGS_CAT_NETWORK,
    SETTINGS_CAT_HARDWARE,
    SETTINGS_CAT_DATETIME,
    SETTINGS_CAT_LANGUAGE,
    SETTINGS_CAT_COUNT
} settings_category_t;

// Ekran çözünürlük yapısı
typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t bpp;         // Piksel başına bit
    char name[32];       // Çözünürlük adı (örn. "1024x768")
} display_resolution_t;

// Ekran kartı bilgisi
typedef struct {
    char name[64];
    uint32_t vram_size;
    uint32_t resolution_count;
    display_resolution_t* resolutions;
    uint32_t current_resolution;
} display_adapter_t;

// Ses kartı bilgisi
typedef struct {
    char name[64];
    uint8_t channels;
    uint8_t volume;
    uint8_t muted;
} sound_device_t;

// Ağ adaptörü bilgisi
typedef struct {
    char name[64];
    char mac[18];
    uint8_t type;       // 0: Ethernet, 1: WiFi
    uint8_t connected;
} network_adapter_t;

// Tarih/saat bilgisi
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t timezone;   // UTC'den saat farkı
} datetime_t;

// Sistem ayarları
typedef struct {
    char hostname[32];
    char username[32];
    uint8_t theme;
    uint8_t autostart_gui;
    uint8_t screen_saver_timeout; // Dakika cinsinden
    uint8_t power_save_mode;      // 0: Kapalı, 1: Düşük, 2: Orta, 3: Yüksek
} system_settings_t;

// Ayarlar penceresi yapısı
typedef struct {
    gui_window_t* window;
    settings_category_t current_category;
    
    // Donanım bilgileri
    display_adapter_t display;
    sound_device_t sound;
    network_adapter_t* network_adapters;
    uint32_t network_adapter_count;
    
    // Sistem ayarları
    system_settings_t system;
    
    // Tarih/saat
    datetime_t datetime;
    
    // Dil ayarları
    uint8_t language;  // 0: Türkçe, 1: İngilizce
} settings_window_t;

// Ayarlar uygulamasını başlat
void settings_init();

// Ayarlar penceresini göster
void settings_show();

// Ayarlar penceresi çizim fonksiyonu
void settings_paint(gui_window_t* window);

// Kategori değiştir
void settings_change_category(settings_window_t* settings, settings_category_t category);

// Donanım algılama işlemleri
void settings_detect_hardware();
void settings_detect_display();
void settings_detect_sound();
void settings_detect_network();

// Çözünürlük değiştir
int settings_change_resolution(uint32_t width, uint32_t height, uint8_t bpp);

// Ses ayarları
void settings_set_volume(uint8_t volume);
void settings_toggle_mute();

// Saat ayarları
void settings_set_datetime(datetime_t* datetime);
void settings_get_datetime(datetime_t* datetime);

// Sürücü yönetimi
int settings_install_driver(const char* device_name);
int settings_update_drivers();
int settings_check_driver_updates();

// Kategori çizim fonksiyonları
void settings_draw_system(settings_window_t* settings, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void settings_draw_display(settings_window_t* settings, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void settings_draw_sound(settings_window_t* settings, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void settings_draw_network(settings_window_t* settings, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void settings_draw_hardware(settings_window_t* settings, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void settings_draw_datetime(settings_window_t* settings, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void settings_draw_language(settings_window_t* settings, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

// Ayarlar uygulamasının ana fonksiyonu
void app_settings();

#endif /* __SETTINGS_H__ */ 