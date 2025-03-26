#ifndef __BROWSER_H__
#define __BROWSER_H__

#include "gui.h"
#include <stdint.h>
#include <stddef.h>

// İleriye doğru bildirim
typedef struct html5_player html5_player_t;

// Tarayıcı durumu
typedef enum {
    BROWSER_STATUS_IDLE,
    BROWSER_STATUS_LOADING,
    BROWSER_STATUS_COMPLETE,
    BROWSER_STATUS_ERROR
} browser_status_t;

// HTML etiketleri
typedef enum {
    HTML_TAG_UNKNOWN,
    HTML_TAG_HTML,
    HTML_TAG_HEAD,
    HTML_TAG_BODY,
    HTML_TAG_TITLE,
    HTML_TAG_A,
    HTML_TAG_P,
    HTML_TAG_H1,
    HTML_TAG_H2,
    HTML_TAG_H3,
    HTML_TAG_BR,
    HTML_TAG_IMG,
    HTML_TAG_DIV,
    HTML_TAG_SPAN,
    HTML_TAG_UL,
    HTML_TAG_OL,
    HTML_TAG_LI,
    HTML_TAG_TABLE,
    HTML_TAG_TR,
    HTML_TAG_TD,
    HTML_TAG_TH,
    HTML_TAG_FORM,
    HTML_TAG_INPUT,
    HTML_TAG_BUTTON,
    HTML_TAG_SELECT,
    HTML_TAG_OPTION,
    // HTML5 etiketleri
    HTML_TAG_VIDEO,
    HTML_TAG_AUDIO,
    HTML_TAG_SOURCE,
    HTML_TAG_CANVAS,
    HTML_TAG_SECTION,
    HTML_TAG_NAV,
    HTML_TAG_ARTICLE,
    HTML_TAG_HEADER,
    HTML_TAG_FOOTER,
    HTML_TAG_MAIN,
    HTML_TAG_ASIDE,
    HTML_TAG_FIGURE,
    HTML_TAG_FIGCAPTION
} html_tag_t;

// Video kodek tipleri
typedef enum {
    VIDEO_CODEC_NONE,
    VIDEO_CODEC_H264,   // AVC
    VIDEO_CODEC_H265,   // HEVC
    VIDEO_CODEC_VP8,    // WebM
    VIDEO_CODEC_VP9,    // WebM
    VIDEO_CODEC_AV1     // AOMedia Video 1
} video_codec_t;

// Video çözünürlükleri
typedef enum {
    VIDEO_RES_SD,      // 480p
    VIDEO_RES_HD,      // 720p
    VIDEO_RES_FULL_HD, // 1080p
    VIDEO_RES_QHD,     // 1440p (2K)
    VIDEO_RES_UHD_4K   // 2160p (4K)
} video_resolution_t;

// HTML5 video ayarları
typedef struct {
    uint8_t autoplay;       // Otomatik oynatma
    uint8_t loop;           // Döngü modu
    uint8_t muted;          // Sessiz mod
    uint8_t controls;       // Kontroller görünür
    uint32_t width;         // Video genişliği
    uint32_t height;        // Video yüksekliği
    video_codec_t preferred_codec;  // Tercih edilen codec
    video_resolution_t max_resolution;  // Maksimum çözünürlük
} html5_video_settings_t;

// Tarayıcı yapısı
typedef struct {
    gui_window_t* window;     // Pencere işaretçisi
    char url[256];            // Mevcut URL
    char title[128];          // Sayfa başlığı
    char* content;            // Sayfa içeriği
    browser_status_t status;  // Tarayıcı durumu
    
    // HTML5 video desteği için ek alanlar
    html5_player_t* video_player;  // Video oynatıcı
    uint8_t html5_support;         // HTML5 desteği var mı?
    uint8_t video_loading;         // Video yükleniyor mu?
    
    // JavaScript motoru için ek alanlar
    void* js_engine;               // JavaScript motoru
    uint8_t js_enabled;            // JavaScript etkin mi?
    
    // CSS işleyicisi için ek alanlar
    void* css_engine;              // CSS işleyici
    
    // Sekme desteği
    uint8_t tab_count;             // Sekme sayısı
    struct browser* tabs;          // Sekmeler
    uint8_t current_tab;           // Geçerli sekme
    
    // Donanım hızlandırma
    uint8_t hw_acceleration;       // Donanım hızlandırma etkin mi?
    uint8_t webgl_support;         // WebGL desteği var mı?
} browser_t;

// Tarayıcıyı başlat
void browser_init();

// Tarayıcı penceresini göster
void browser_show();

// URL'ye git
void browser_navigate(const char* url);

// Tarayıcı durumunu al
browser_status_t browser_get_status();

// Başlığı al
const char* browser_get_title();

// HTML5 Video fonksiyonları
int browser_video_init(browser_t* browser);
int browser_video_load(browser_t* browser, const char* video_url, uint32_t width, uint32_t height);
int browser_video_play(browser_t* browser);
int browser_video_pause(browser_t* browser);
int browser_video_stop(browser_t* browser);
int browser_video_seek(browser_t* browser, uint32_t position);
int browser_video_set_volume(browser_t* browser, uint8_t volume);
int browser_video_set_size(browser_t* browser, uint32_t width, uint32_t height);
int browser_video_toggle_fullscreen(browser_t* browser);

// HTML5 desteği ve uyumluluk kontrolleri
uint8_t browser_has_html5_support();
uint8_t browser_supports_codec(video_codec_t codec);
uint8_t browser_supports_resolution(uint32_t width, uint32_t height);
uint8_t browser_can_play_4k();

// JavaScript desteği
void browser_enable_javascript();
void browser_disable_javascript();
int browser_execute_script(browser_t* browser, const char* script);

// Sekme yönetimi
int browser_add_tab(browser_t* browser);
int browser_close_tab(browser_t* browser, uint8_t tab_index);
int browser_select_tab(browser_t* browser, uint8_t tab_index);

// Tarayıcı yapılandırması
void browser_set_homepage(const char* url);
void browser_clear_cache();
void browser_clear_cookies();
void browser_enable_private_mode();

#endif // __BROWSER_H__ 