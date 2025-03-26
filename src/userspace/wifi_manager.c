#include "../include/wifi.h"
#include "../include/gui.h"
#include "../include/font.h"
#include "../include/vga.h"
#include <string.h>

// Pencere referansı
static gui_window_t* wifi_window = NULL;

// Mevcut WiFi ağları
static wifi_network_t networks[10];
static int network_count = 0;
static int selected_network = -1;

// WiFi penceresi çizim fonksiyonu
static void wifi_manager_paint(gui_window_t* window);

// WiFi donanımını başlat
void wifi_init() {
    // Gerçek bir sistemde WiFi donanımı burada başlatılır
    // Bu demo için varsayılan ağlar ekliyoruz
    strcpy(networks[0].ssid, "KALEM-WiFi");
    networks[0].signal_strength = 90;
    networks[0].security_type = WIFI_SECURITY_WPA2;
    networks[0].connected = 0;
    
    strcpy(networks[1].ssid, "Misafir");
    networks[1].signal_strength = 60;
    networks[1].security_type = WIFI_SECURITY_NONE;
    networks[1].connected = 0;
    
    strcpy(networks[2].ssid, "TurkTelekom_ABCD");
    networks[2].signal_strength = 75;
    networks[2].security_type = WIFI_SECURITY_WPA;
    networks[2].connected = 0;
    
    strcpy(networks[3].ssid, "Superonline-5G");
    networks[3].signal_strength = 85;
    networks[3].security_type = WIFI_SECURITY_WPA2;
    networks[3].connected = 0;
    
    strcpy(networks[4].ssid, "CafeWiFi");
    networks[4].signal_strength = 45;
    networks[4].security_type = WIFI_SECURITY_WPA;
    networks[4].connected = 0;
    
    network_count = 5;
}

// Mevcut ağları tara
int wifi_scan_networks(wifi_network_t* out_networks, int max_count) {
    if (max_count > network_count) {
        max_count = network_count;
    }
    
    if (out_networks) {
        memcpy(out_networks, networks, max_count * sizeof(wifi_network_t));
    }
    
    return max_count;
}

// Ağa bağlan
int wifi_connect(const char* ssid, const char* password) {
    for (int i = 0; i < network_count; i++) {
        if (strcmp(networks[i].ssid, ssid) == 0) {
            // Önce tüm ağları bağlantısız yap
            for (int j = 0; j < network_count; j++) {
                networks[j].connected = 0;
            }
            
            // Bu ağa bağlı olarak işaretle
            networks[i].connected = 1;
            
            // Pencereyi güncelle
            if (wifi_window) {
                gui_desktop_draw();
            }
            
            return WIFI_STATUS_OK;
        }
    }
    
    return WIFI_STATUS_ERROR;
}

// Bağlantıyı kes
int wifi_disconnect() {
    for (int i = 0; i < network_count; i++) {
        networks[i].connected = 0;
    }
    
    // Pencereyi güncelle
    if (wifi_window) {
        gui_desktop_draw();
    }
    
    return WIFI_STATUS_OK;
}

// WiFi durumunu öğren
int wifi_get_status() {
    for (int i = 0; i < network_count; i++) {
        if (networks[i].connected) {
            return WIFI_STATUS_OK;
        }
    }
    
    return WIFI_STATUS_ERROR;
}

// Bağlı olduğumuz SSID adını öğren
const char* wifi_get_current_ssid() {
    for (int i = 0; i < network_count; i++) {
        if (networks[i].connected) {
            return networks[i].ssid;
        }
    }
    
    return NULL;
}

// Sinyal gücünü öğren
uint8_t wifi_get_signal_strength() {
    for (int i = 0; i < network_count; i++) {
        if (networks[i].connected) {
            return networks[i].signal_strength;
        }
    }
    
    return 0;
}

// WiFi yöneticisi penceresini göster
void wifi_show_manager() {
    // Pencere zaten açıksa, odakla
    if (wifi_window) {
        gui_window_bring_to_front(wifi_window);
        return;
    }
    
    // Pencereyi oluştur
    wifi_window = gui_window_create("WiFi Ag Yoneticisi", 60, 60, 350, 300, GUI_WINDOW_STYLE_NORMAL);
    if (!wifi_window) return;
    
    // Pencere kapatıldığında referansı temizleyecek kapanış fonksiyonu
    wifi_window->on_close = [](gui_window_t* window) {
        wifi_window = NULL;
    };
    
    // Çizim fonksiyonunu ayarla
    wifi_window->on_paint = wifi_manager_paint;
    
    // Pencereyi göster
    gui_window_show(wifi_window);
}

// WiFi simge durumunu çiz (sinyal çubuklarını göster)
static void wifi_draw_signal_strength(uint32_t x, uint32_t y, uint8_t strength) {
    // Arkaplan
    vga_fill_rect(x, y, 16, 16, GUI_COLOR_WINDOW_BG);
    
    // Sinyal gücüne göre çubuklar
    uint8_t color = GUI_COLOR_GREEN;
    if (strength < 40) {
        color = GUI_COLOR_RED;
    } else if (strength < 70) {
        color = GUI_COLOR_YELLOW;
    }
    
    // Çubuklar (4 seviye)
    if (strength > 10) {
        vga_fill_rect(x + 2, y + 12, 2, 4, color);
    }
    if (strength > 30) {
        vga_fill_rect(x + 6, y + 8, 2, 8, color);
    }
    if (strength > 60) {
        vga_fill_rect(x + 10, y + 4, 2, 12, color);
    }
    if (strength > 85) {
        vga_fill_rect(x + 14, y, 2, 16, color);
    }
}

// Güvenlik türü simgesini çiz
static void wifi_draw_security_type(uint32_t x, uint32_t y, uint8_t security_type) {
    switch (security_type) {
        case WIFI_SECURITY_NONE:
            // Açık ağ simgesi
            font_draw_string(x, y, "o", GUI_COLOR_GREEN, 0xFF);
            break;
        case WIFI_SECURITY_WEP:
        case WIFI_SECURITY_WPA:
        case WIFI_SECURITY_WPA2:
            // Kilitli ağ simgesi
            font_draw_string(x, y, "x", GUI_COLOR_RED, 0xFF);
            break;
    }
}

// WiFi yöneticisi çizim fonksiyonu
static void wifi_manager_paint(gui_window_t* window) {
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    
    // Pencere arkaplanı
    vga_fill_rect(x, y, window->client.width, window->client.height, GUI_COLOR_WINDOW_BG);
    
    // Başlık ve durumlar
    font_draw_string(x + 10, y + 10, "WiFi Ag Yoneticisi", GUI_COLOR_BLACK, 0xFF);
    
    if (wifi_get_status() == WIFI_STATUS_OK) {
        char status_text[64];
        sprintf(status_text, "Durum: Bagli (%s)", wifi_get_current_ssid());
        font_draw_string(x + 10, y + 30, status_text, GUI_COLOR_DARK_BLUE, 0xFF);
    } else {
        font_draw_string(x + 10, y + 30, "Durum: Bagli degil", GUI_COLOR_RED, 0xFF);
    }
    
    // Tablo başlıkları
    font_draw_string(x + 10, y + 60, "Ag Adi", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 200, y + 60, "Sinyal", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 250, y + 60, "Guvenlik", GUI_COLOR_BLACK, 0xFF);
    
    // Ayırıcı çizgi
    vga_draw_hline(x + 10, y + 75, window->client.width - 20, GUI_COLOR_DARK_GRAY);
    
    // Ağ listesi
    for (int i = 0; i < network_count; i++) {
        uint32_t row_y = y + 85 + i * 25;
        uint8_t text_color = networks[i].connected ? GUI_COLOR_DARK_BLUE : GUI_COLOR_BLACK;
        
        // Seçili öğeyi vurgula
        if (i == selected_network) {
            vga_fill_rect(x + 5, row_y - 2, window->client.width - 10, 20, GUI_COLOR_LIGHT_BLUE);
            text_color = GUI_COLOR_WHITE;
        }
        
        // Ağ adı
        font_draw_string(x + 10, row_y, networks[i].ssid, text_color, 0xFF);
        
        // Sinyal gücü
        wifi_draw_signal_strength(x + 200, row_y, networks[i].signal_strength);
        
        // Güvenlik türü
        wifi_draw_security_type(x + 250, row_y, networks[i].security_type);
        
        // Bağlantı durumu
        if (networks[i].connected) {
            font_draw_string(x + 300, row_y, "✓", GUI_COLOR_GREEN, 0xFF);
        }
    }
    
    // Butonlar
    gui_button_t button_connect = {
        .id = 1,
        .label = "Baglan",
        .x = window->client.width - 180,
        .y = window->client.height - 40,
        .width = 80,
        .height = 30,
        .state = 0
    };
    
    gui_button_t button_disconnect = {
        .id = 2,
        .label = "Baglantıyı Kes",
        .x = window->client.width - 90,
        .y = window->client.height - 40,
        .width = 80,
        .height = 30,
        .state = 0
    };
    
    gui_button_draw(window, &button_connect);
    gui_button_draw(window, &button_disconnect);
} 