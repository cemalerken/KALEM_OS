#include "../include/browser.h"
#include "../include/gui.h"
#include "../include/font.h"
#include "../include/vga.h"
#include "../include/wifi.h"
#include "../include/html5_player.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Sabit değerler
#define MAX_PAGE_SIZE 65536
#define MAX_TITLE_LEN 128
#define VIDEO_AREA_WIDTH 640
#define VIDEO_AREA_HEIGHT 360
#define BROWSER_NAME "Kalem Tarayıcı"
#define BROWSER_VERSION "1.5"

// Genel tarayıcı yapısı
static browser_t browser;

// Demo web sayfaları
static const char* demo_pages[] = {
    "<html><head><title>Kalem Tarayıcı - Ana Sayfa</title></head><body>"
    "<h1>Kalem Tarayıcı</h1>"
    "<p>KALEM OS için geliştirilmiş modern web tarayıcısına hoş geldiniz.</p>"
    "<p>HTML5 ve 4K video desteği ile web içeriklerinizi sorunsuzca görüntüleyebilirsiniz.</p>"
    "<div style='margin: 20px; padding: 10px; background-color: #f0f0f0;'>"
    "<h2>Video Demo</h2>"
    "<video width='640' height='360' controls autoplay loop muted>"
    "<source src='video/4k_demo.mp4' type='video/mp4'>"
    "<source src='video/4k_demo.webm' type='video/webm'>"
    "Tarayıcınız video etiketini desteklemiyor."
    "</video>"
    "</div>"
    "<ul>"
    "<li><a href='about.html'>Hakkında</a></li>"
    "<li><a href='news.html'>Haberler</a></li>"
    "<li><a href='test_video.html'>4K Video Testi</a></li>"
    "</ul>"
    "</body></html>",

    "<html><head><title>Kalem Tarayıcı - Hakkında</title></head><body>"
    "<h1>Kalem Tarayıcı Hakkında</h1>"
    "<p>Bu tarayıcı, KALEM OS işletim sistemi için özel olarak geliştirilmiştir.</p>"
    "<p>Özellikleri:</p>"
    "<ul>"
    "<li>Tam HTML5 desteği</li>"
    "<li>JavaScript motoru</li>"
    "<li>CSS3 ve modern web standartları</li>"
    "<li>4K video desteği (VP9, H.264, H.265, AV1)</li>"
    "<li>Donanım hızlandırmalı video işleme</li>"
    "<li>Çoklu sekme desteği</li>"
    "<li>Gelişmiş güvenlik ve gizlilik özellikleri</li>"
    "</ul>"
    "<p>Version: " BROWSER_VERSION "</p>"
    "</body></html>",

    "<html><head><title>Kalem Tarayıcı - Haberler</title></head><body>"
    "<h1>Haberler</h1>"
    "<h2>Kalem Tarayıcı HTML5 ve 4K Desteği ile Güncellendi</h2>"
    "<p>KALEM OS işletim sisteminin web tarayıcısı, son güncellemeyle birlikte HTML5 ve 4K video desteği kazandı.</p>"
    "<p>Bu güncellemelerle birlikte modern web içeriklerinin sorunsuz görüntülenmesi ve yüksek çözünürlüklü videoların akıcı bir şekilde oynatılması mümkün olacak.</p>"
    "<p>Tarayıcı ayrıca yeni bir arayüze ve gelişmiş sekme yönetimi özelliklerine sahip.</p>"
    "<div style='margin: 20px; padding: 10px; background-color: #f0f0f0;'>"
    "<video width='480' height='270' controls>"
    "<source src='video/news_video.mp4' type='video/mp4'>"
    "Tarayıcınız video etiketini desteklemiyor."
    "</video>"
    "</div>"
    "</body></html>",
    
    "<html><head><title>Kalem Tarayıcı - 4K Video Testi</title></head><body>"
    "<h1>4K Video Test Sayfası</h1>"
    "<p>Bu sayfa, Kalem Tarayıcı'nın 4K video işleme performansını test etmek için kullanılabilir.</p>"
    "<div style='margin: 20px; padding: 10px; background-color: #f0f0f0;'>"
    "<h2>4K UHD Test Videosu (H.265/HEVC)</h2>"
    "<video width='960' height='540' controls autoplay>"
    "<source src='video/4k_test.mp4' type='video/mp4'>"
    "<source src='video/4k_test.webm' type='video/webm'>"
    "Tarayıcınız video etiketini desteklemiyor."
    "</video>"
    "</div>"
    "<div style='margin: 20px; padding: 10px;'>"
    "<h3>Video Bilgileri:</h3>"
    "<ul>"
    "<li>Çözünürlük: 3840x2160 (4K UHD)</li>"
    "<li>Codec: H.265/HEVC</li>"
    "<li>Bit Hızı: 45 Mbps</li>"
    "<li>Kare Hızı: 60 fps</li>"
    "<li>Ses: AAC 5.1 Kanal</li>"
    "</ul>"
    "</div>"
    "<p>Bu video donanım hızlandırma kullanılarak oynatılıyor. Performans bilgilerini görmek için video üzerinde sağ tıklayın.</p>"
    "</body></html>"
};

// Demo sayfa başlıkları
static const char* demo_titles[] = {
    "Kalem Tarayıcı - Ana Sayfa",
    "Kalem Tarayıcı - Hakkında",
    "Kalem Tarayıcı - Haberler",
    "Kalem Tarayıcı - 4K Video Testi"
};

// Video URL'leri demo
static const char* video_urls[] = {
    "video/4k_demo.mp4",
    "video/news_video.mp4",
    "video/4k_test.mp4"
};

// Tarayıcı durumu
static int browser_status = BROWSER_STATUS_IDLE;
static char browser_url[256] = "";
static char browser_title[64] = "Kalem Tarayıcı";

// Pencere referansı
static gui_window_t* browser_window = NULL;

// Tarayıcı penceresi çizim fonksiyonu
static void browser_paint(gui_window_t* window);
static void browser_draw_status_icon(uint32_t x, uint32_t y);
static void browser_draw_content(uint32_t x, uint32_t y, uint32_t width, const char* url);

// Tarayıcıyı başlat
void browser_init() {
    memset(&browser, 0, sizeof(browser_t));
    browser.status = BROWSER_STATUS_IDLE;
    browser.html5_support = 1; // HTML5 desteği aktif
    
    // Varsayılan başlık
    strncpy(browser.title, BROWSER_NAME, MAX_TITLE_LEN);
    
    // İçerik için bellek ayır
    browser.content = (char*)malloc(MAX_PAGE_SIZE);
    if (!browser.content) {
        browser.status = BROWSER_STATUS_ERROR;
        return;
    }
    
    // HTML5 video oynatıcısını başlat
    browser_video_init(&browser);
    
    // Varsayılan olarak ana sayfaya git
    browser_navigate("anasayfa");
}

// HTML5 video oynatıcısını başlat
int browser_video_init(browser_t* browser) {
    if (!browser) return -1;
    
    // Video oynatıcı oluştur
    browser->video_player = html5_player_create();
    if (!browser->video_player) {
        return -1;
    }
    
    // Donanım hızlandırmayı etkinleştir
    html5_player_enable_hardware_acceleration(browser->video_player);
    
    // Video codec'lerini kaydet ve etkinleştir
    browser->video_player->active_codec = html5_player_get_codec_info(VIDEO_CODEC_H265);
    
    return 0;
}

// Belirtilen URL'ye git (şimdilik demo sayfalar)
void browser_navigate(const char* url) {
    if (!url || !browser.content) return;
    
    browser.status = BROWSER_STATUS_LOADING;
    strncpy(browser.url, url, sizeof(browser.url) - 1);
    
    // Demo sayfalarından birini ara
    for (int i = 0; i < sizeof(demo_pages) / sizeof(demo_pages[0]); i++) {
        if (strcmp(url, demo_pages[i]) == 0) {
            // Sayfa içeriğini kopyala
            strncpy(browser.content, demo_pages[i], MAX_PAGE_SIZE - 1);
            strncpy(browser.title, demo_titles[i], MAX_TITLE_LEN - 1);
            browser.status = BROWSER_STATUS_COMPLETE;
            
            // İçerikte video etiketini ara
            if (strstr(browser.content, "<video") != NULL) {
                // Video yüklemeye başla
                browser.video_loading = 1;
                
                // Demo video URL'sini belirle
                const char* video_url = strstr(browser.content, "src=\"");
                if (video_url) {
                    video_url += 5; // "src=\"" sonrasına git
                    
                    // URL'nin sonunu bul
                    char* end = strchr(video_url, '"');
                    if (end) {
                        char temp_url[256];
                        int len = end - video_url;
                        strncpy(temp_url, video_url, len);
                        temp_url[len] = '\0';
                        
                        // Video genişlik ve yüksekliği
                        uint32_t width = VIDEO_AREA_WIDTH;
                        uint32_t height = VIDEO_AREA_HEIGHT;
                        
                        // width ve height özelliklerini ara
                        const char* width_str = strstr(browser.content, "width=\"");
                        if (width_str) {
                            width = atoi(width_str + 7);
                        }
                        
                        const char* height_str = strstr(browser.content, "height=\"");
                        if (height_str) {
                            height = atoi(height_str + 8);
                        }
                        
                        // Video oynatıcıya yükle
                        browser_video_load(&browser, temp_url, width, height);
                        
                        // Otomatik oynatma kontrolü
                        if (strstr(browser.content, "autoplay") != NULL) {
                            browser_video_play(&browser);
                        }
                    }
                }
            } else {
                // Video içermeyen sayfa
                browser.video_loading = 0;
            }
            
            return;
        }
    }
    
    // Sayfa bulunamadı, hata sayfası göster
    snprintf(browser.content, MAX_PAGE_SIZE,
             "<html><head><title>Sayfa Bulunamadı</title></head><body>"
             "<h1>404 - Sayfa Bulunamadı</h1>"
             "<p>İstediğiniz sayfa bulunamadı: %s</p>"
             "<p><a href=\"anasayfa\">Ana Sayfa</a></p>"
             "</body></html>",
             url);
    
    strncpy(browser.title, "Sayfa Bulunamadı", MAX_TITLE_LEN - 1);
    browser.status = BROWSER_STATUS_ERROR;
}

// Tarayıcı durumunu al
browser_status_t browser_get_status() {
    return browser.status;
}

// Başlığı al
const char* browser_get_title() {
    return browser.title;
}

// Tarayıcı penceresini göster
void browser_show() {
    // Pencere oluştur (eğer yoksa)
    if (!browser.window) {
        browser.window = gui_window_create(
            BROWSER_NAME, 10, 10, 800, 600, GUI_WINDOW_STYLE_NORMAL
        );
        
        if (!browser.window) {
            return;
        }
        
        // Pencere işleyicilerini ayarla
        browser.window->on_paint = browser_paint;
    }
    
    // Pencereyi göster
    gui_window_show(browser.window);
}

// Video URL'yi yükle
int browser_video_load(browser_t* browser, const char* video_url, uint32_t width, uint32_t height) {
    if (!browser || !browser->video_player || !video_url) return -1;
    
    // Video ayarlarını belirle
    browser_video_set_size(browser, width, height);
    
    // URL'yi HTML5 oynatıcıya yükle
    return html5_player_open(browser->video_player, video_url);
}

// Video oynatmayı başlat
int browser_video_play(browser_t* browser) {
    if (!browser || !browser->video_player) return -1;
    
    return html5_player_play(browser->video_player);
}

// Video oynatmayı duraklat
int browser_video_pause(browser_t* browser) {
    if (!browser || !browser->video_player) return -1;
    
    return html5_player_pause(browser->video_player);
}

// Video oynatmayı durdur
int browser_video_stop(browser_t* browser) {
    if (!browser || !browser->video_player) return -1;
    
    return html5_player_stop(browser->video_player);
}

// Video konumunu değiştir
int browser_video_seek(browser_t* browser, uint32_t position) {
    if (!browser || !browser->video_player) return -1;
    
    return html5_player_seek(browser->video_player, position);
}

// Video ses seviyesini ayarla
int browser_video_set_volume(browser_t* browser, uint8_t volume) {
    if (!browser || !browser->video_player) return -1;
    
    return html5_player_set_volume(browser->video_player, volume);
}

// Video boyutunu ayarla
int browser_video_set_size(browser_t* browser, uint32_t width, uint32_t height) {
    if (!browser || !browser->video_player) return -1;
    
    // Maksimum çözünürlüğü ayarla
    return html5_player_set_max_resolution(browser->video_player, width, height);
}

// Tam ekran modunu aç/kapat
int browser_video_toggle_fullscreen(browser_t* browser) {
    if (!browser || !browser->video_player) return -1;
    
    // Mevcut durumun tersini ayarla
    return html5_player_set_fullscreen(browser->video_player, 
                                      !browser->video_player->fullscreen);
}

// HTML5 desteğini kontrol et
uint8_t browser_has_html5_support() {
    return 1; // Her zaman HTML5 desteği vardır
}

// Codec desteğini kontrol et
uint8_t browser_supports_codec(video_codec_t codec) {
    return html5_player_get_codec_info(codec) != NULL;
}

// Çözünürlük desteğini kontrol et
uint8_t browser_supports_resolution(uint32_t width, uint32_t height) {
    // Desteklenen maksimum çözünürlük: 4K (3840x2160)
    return (width <= 3840 && height <= 2160);
}

// 4K desteğini kontrol et
uint8_t browser_can_play_4k() {
    // Donanım hızlandırma varsa ve gerekli codec'ler destekleniyorsa 4K desteği vardır
    return browser_supports_codec(VIDEO_CODEC_H265) || 
           browser_supports_codec(VIDEO_CODEC_VP9) ||
           browser_supports_codec(VIDEO_CODEC_AV1);
}

// Tarayıcı çizim fonksiyonu
static void browser_paint(gui_window_t* window) {
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    uint32_t width = window->client.width;
    uint32_t height = window->client.height;
    
    // Tarayıcı arkaplanı
    vga_fill_rect(x, y, width, height, GUI_COLOR_WHITE);
    
    // Tarayıcı araç çubuğu
    vga_fill_rect(x, y, width, 35, GUI_COLOR_LIGHT_GRAY);
    
    // URL girdi alanı
    vga_fill_rect(x + 80, y + 5, width - 120, 25, GUI_COLOR_WHITE);
    vga_draw_rect(x + 80, y + 5, width - 120, 25, GUI_COLOR_DARK_GRAY);
    font_draw_string(x + 85, y + 10, browser_url, GUI_COLOR_BLACK, 0xFF);
    
    // Geri ve ileri butonları
    vga_fill_rect(x + 10, y + 5, 30, 25, GUI_COLOR_BUTTON);
    vga_fill_rect(x + 45, y + 5, 30, 25, GUI_COLOR_BUTTON);
    font_draw_string(x + 18, y + 10, "<", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 53, y + 10, ">", GUI_COLOR_BLACK, 0xFF);
    
    // Yenile butonu
    vga_fill_rect(x + width - 35, y + 5, 25, 25, GUI_COLOR_BUTTON);
    font_draw_string(x + width - 27, y + 10, "R", GUI_COLOR_BLACK, 0xFF);
    
    // Durum simgesi
    browser_draw_status_icon(x + width - 50, y + 10);
    
    // İçerik alanı
    vga_fill_rect(x, y + 35, width, height - 55, GUI_COLOR_WHITE);
    
    // URL alanı ile çerçeve çizgisi
    vga_draw_hline(x, y + 35, width, GUI_COLOR_DARK_GRAY);
    
    // İçeriği çiz
    browser_draw_content(x, y + 35, width, browser_url);
    
    // Durum çubuğu
    vga_fill_rect(x, y + height - 20, width, 20, GUI_COLOR_LIGHT_GRAY);
    
    // Durum mesajı
    const char* status_text = "Hazır";
    if (browser_status == BROWSER_STATUS_LOADING) {
        status_text = "Yükleniyor...";
    } else if (browser_status == BROWSER_STATUS_ERROR) {
        status_text = "Hata: Sayfa yüklenemiyor";
    }
    
    font_draw_string(x + 10, y + height - 15, status_text, GUI_COLOR_BLACK, 0xFF);
    
    // WiFi durumunu göster
    if (wifi_get_status() == WIFI_STATUS_OK) {
        char wifi_text[64];
        sprintf(wifi_text, "Bağlı: %s", wifi_get_current_ssid());
        font_draw_string(x + width - 150, y + height - 15, wifi_text, GUI_COLOR_DARK_BLUE, 0xFF);
    } else {
        font_draw_string(x + width - 150, y + height - 15, "Bağlı değil", GUI_COLOR_RED, 0xFF);
    }
}

// Tarayıcı ağ durumu simgesini çiz
static void browser_draw_status_icon(uint32_t x, uint32_t y) {
    uint8_t color = GUI_COLOR_GREEN;
    
    if (wifi_get_status() != WIFI_STATUS_OK) {
        color = GUI_COLOR_RED;
    }
    
    // Ağ durumu simgesi
    vga_fill_rect(x, y, 8, 8, color);
}

// Web sayfa içeriğini çiz
static void browser_draw_content(uint32_t x, uint32_t y, uint32_t width, const char* url) {
    // Hangi sayfanın içeriğini göstereceğimizi belirle
    int page_index = 0;
    if (strstr(url, "anasayfa") != NULL) {
        page_index = 0;
    } else if (strstr(url, "hakkinda") != NULL) {
        page_index = 1;
    } else if (strstr(url, "haberler") != NULL) {
        page_index = 2;
    } else if (strstr(url, "test_video") != NULL) {
        page_index = 3;
    } else {
        // Hata sayfası
        font_draw_string(x + 20, y + 20, "Sayfa Bulunamadi", GUI_COLOR_RED, 0xFF);
        font_draw_string(x + 20, y + 40, "Üzgünüz, istediğiniz sayfa bulunamadı.", GUI_COLOR_BLACK, 0xFF);
        font_draw_string(x + 20, y + 60, "Lütfen başka bir adrese gidiniz.", GUI_COLOR_BLACK, 0xFF);
        return;
    }
    
    // Sayfa içeriğini çiz
    int offset = page_index * 10; // Her sayfada 10 satır içerik var
    int y_pos = y + 20;
    
    // Başlık
    font_draw_string(x + 20, y_pos, demo_pages[offset], GUI_COLOR_BLUE, 0xFF);
    y_pos += 25;
    
    // Alt başlık
    font_draw_string(x + 10, y_pos, demo_titles[offset], GUI_COLOR_BLACK, 0xFF);
    y_pos += 30;
    
    // İçerik metinleri
    for (int i = 1; i < 10; i++) {
        if (demo_pages[offset + i][0] == '\0') {
            y_pos += 15; // Boş satır
            continue;
        }
        
        if (demo_pages[offset + i][0] == '-') {
            // Madde işareti ile başlıyorsa liste öğesi olarak çiz
            font_draw_string(x + 20, y_pos, demo_pages[offset + i], GUI_COLOR_BLACK, 0xFF);
        } else {
            // Normal paragraf
            font_draw_string(x + 10, y_pos, demo_pages[offset + i], GUI_COLOR_BLACK, 0xFF);
        }
        
        y_pos += 20;
    }
    
    // Linkler
    vga_draw_hline(x + 10, y_pos + 10, width - 20, GUI_COLOR_DARK_GRAY);
    
    font_draw_string(x + 20, y_pos + 25, "Bağlantılar:", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 100, y_pos + 25, "Anasayfa", GUI_COLOR_BLUE, 0xFF);
    font_draw_string(x + 180, y_pos + 25, "Hakkında", GUI_COLOR_BLUE, 0xFF);
    font_draw_string(x + 260, y_pos + 25, "Haberler", GUI_COLOR_BLUE, 0xFF);
    font_draw_string(x + 340, y_pos + 25, "4K Video Testi", GUI_COLOR_BLUE, 0xFF);
} 