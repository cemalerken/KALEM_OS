#include "../include/launcher.h"
#include "../include/gui.h"
#include "../include/font.h"
#include "../include/vga.h"
#include "../include/terminal.h"
#include "../include/notepad.h"
#include "../include/calculator.h"
#include "../include/settings.h"
#include "../include/wifi.h"
#include "../include/browser.h"
#include "../include/noteplus.h"
#include "../include/archive.h"
#include "../include/app_manager.h"
#include "../include/kalem_shell.h"
#include "../include/context_menu.h"
#include <string.h>

#define MAX_ICONS 20

// Uygulama çizim fonksiyonları
static void terminal_paint(gui_window_t* window);
static void settings_paint(gui_window_t* window);
static void notepad_paint(gui_window_t* window);
static void calculator_paint(gui_window_t* window);
static void about_paint(gui_window_t* window);

// Uygulama simgeleri
static app_icon_t icons[MAX_ICONS];
static uint32_t icon_count = 0;

// Masaüstü arkaplan rengi
static const uint8_t desktop_bg_color = GUI_COLOR_DESKTOP_BG;

// Uygulama başlatıcıları
launcher_app_t app_launchers[] = {
    { "Ayarlar", settings_app, "settings.png" },
    { "Dosya Yöneticisi", file_manager_app, "file_manager.png" },
    { "Not+", noteplus_app, "noteplus.png" },
    { "Takvim", calendar_app, "calendar.png" },
    { "Terminal", terminal_app, "terminal.png" },
    { "KALEM Shell", kalem_shell_app, "shell.png" },
    { NULL, NULL, NULL } // Son eleman
};

// Başlatma
void launcher_init() {
    // Masaüstü simgelerini ekle
    launcher_add_icon("Terminal", 50, 50, GUI_COLOR_GREEN, app_terminal);
    launcher_add_icon("Ayarlar", 150, 50, GUI_COLOR_LIGHT_BLUE, app_settings);
    launcher_add_icon("Not Defteri", 250, 50, GUI_COLOR_YELLOW, app_notepad);
    launcher_add_icon("Hesap Makinesi", 350, 50, GUI_COLOR_ORANGE, app_calculator);
    launcher_add_icon("WiFi Yöneticisi", 50, 150, GUI_COLOR_CYAN, app_wifi);
    launcher_add_icon("TRweb tarayıcı", 150, 150, GUI_COLOR_LIGHT_RED, app_browser);
    launcher_add_icon("Not++", 250, 150, GUI_COLOR_LIGHT_GREEN, app_noteplus);
    launcher_add_icon("Arşiv Yöneticisi", 350, 150, GUI_COLOR_LIGHT_MAGENTA, app_archive);
    launcher_add_icon("Uygulama Mağazası", 50, 250, GUI_COLOR_LIGHT_RED, app_store);
    launcher_add_icon("Hakkında", 150, 250, GUI_COLOR_LIGHT_GRAY, app_about);
    
    // Uygulama yöneticisini başlat
    app_manager_init();
    
    // Masaüstünü çiz
    launcher_draw_desktop();
}

// Masaüstü çiz
void launcher_draw_desktop() {
    // Tüm ekranı temizle ve arkaplanı çiz
    vga_fill_rect(0, 0, vga_get_width(), vga_get_height(), desktop_bg_color);
    
    // Simgeleri çiz
    launcher_draw_icons();
}

// Simgeleri çiz
void launcher_draw_icons() {
    for (uint32_t i = 0; i < icon_count; i++) {
        app_icon_t* icon = &icons[i];
        
        // Simge arkaplanı
        uint32_t width = 80;
        uint32_t height = 80;
        vga_fill_rect(icon->x, icon->y, width, height, icon->color);
        
        // Simge çerçevesi
        vga_draw_rect(icon->x, icon->y, width, height, GUI_COLOR_WHITE);
        
        // Simge adı
        uint32_t text_width = strlen(icon->name) * 8;
        uint32_t text_x = icon->x + (width - text_width) / 2;
        uint32_t text_y = icon->y + height - 20;
        font_draw_string(text_x, text_y, icon->name, GUI_COLOR_WHITE, 0xFF);
    }
}

// Simge ekle
void launcher_add_icon(const char* name, uint32_t x, uint32_t y, uint8_t color, void (*on_click)()) {
    if (icon_count >= MAX_ICONS) return;
    
    app_icon_t* icon = &icons[icon_count++];
    strncpy(icon->name, name, sizeof(icon->name) - 1);
    icon->x = x;
    icon->y = y;
    icon->color = color;
    icon->on_click = on_click;
}

// Terminal uygulaması
void app_terminal() {
    gui_window_t* window = gui_window_create("Terminal", 50, 50, 320, 240, GUI_WINDOW_STYLE_NORMAL);
    if (!window) return;
    
    window->on_paint = terminal_paint;
    gui_window_show(window);
}

// Terminal çizim fonksiyonu
static void terminal_paint(gui_window_t* window) {
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    
    // Terminal arkaplanı
    vga_fill_rect(x, y, window->client.width, window->client.height, GUI_COLOR_BLACK);
    
    // Örnek komut çıktısı
    font_draw_string(x + 5, y + 5, "KALEM OS Terminal v1.0", GUI_COLOR_LIGHT_GREEN, 0xFF);
    font_draw_string(x + 5, y + 25, "# ls", GUI_COLOR_WHITE, 0xFF);
    font_draw_string(x + 5, y + 40, "kernel  bootloader  userspace", GUI_COLOR_LIGHT_CYAN, 0xFF);
    font_draw_string(x + 5, y + 55, "# _", GUI_COLOR_WHITE, 0xFF);
}

// Ayarlar uygulaması
void app_settings() {
    gui_window_t* window = gui_window_create("Sistem Ayarlari", 80, 80, 350, 250, GUI_WINDOW_STYLE_NORMAL);
    if (!window) return;
    
    window->on_paint = settings_paint;
    gui_window_show(window);
}

// Ayarlar çizim fonksiyonu
static void settings_paint(gui_window_t* window) {
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    
    // Ayarlar arkaplanı
    vga_fill_rect(x, y, window->client.width, window->client.height, GUI_COLOR_WINDOW_BG);
    
    // Ayarlar kategorileri
    font_draw_string(x + 10, y + 10, "Genel Ayarlar", GUI_COLOR_BLACK, 0xFF);
    vga_draw_hline(x + 10, y + 25, window->client.width - 20, GUI_COLOR_DARK_GRAY);
    
    // Örnek ayarlar
    font_draw_string(x + 20, y + 40, "Arkaplan Rengi:", GUI_COLOR_BLACK, 0xFF);
    vga_fill_rect(x + 150, y + 40, 20, 15, GUI_COLOR_BLUE);
    
    font_draw_string(x + 20, y + 60, "Ses Seviyesi:", GUI_COLOR_BLACK, 0xFF);
    vga_draw_hline(x + 150, y + 67, 100, GUI_COLOR_DARK_GRAY);
    vga_fill_rect(x + 150, y + 65, 70, 5, GUI_COLOR_LIGHT_GREEN);
    
    font_draw_string(x + 20, y + 80, "Tema:", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 150, y + 80, "Varsayilan", GUI_COLOR_BLACK, 0xFF);
    
    // Buton
    gui_button_t button = {
        .id = 1,
        .label = "Kaydet",
        .x = window->client.width - 80,
        .y = window->client.height - 30,
        .width = 70,
        .height = 25,
        .state = 0
    };
    
    gui_button_draw(window, &button);
}

// Not Defteri uygulaması
void app_notepad() {
    gui_window_t* window = gui_window_create("Not Defteri", 100, 100, 300, 220, GUI_WINDOW_STYLE_NORMAL);
    if (!window) return;
    
    window->on_paint = notepad_paint;
    gui_window_show(window);
}

// Not Defteri çizim fonksiyonu
static void notepad_paint(gui_window_t* window) {
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    
    // Not defteri arkaplanı
    vga_fill_rect(x, y, window->client.width, window->client.height, GUI_COLOR_WHITE);
    
    // Örnek metin
    font_draw_string(x + 10, y + 10, "KALEM OS Not Defteri", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 10, y + 30, "-------------", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 10, y + 50, "Merhaba Dünya!", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 10, y + 65, "KALEM OS isletim sistemi uzerinde", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 10, y + 80, "notlar alabilirsiniz.", GUI_COLOR_BLACK, 0xFF);
}

// Hesap Makinesi uygulaması
void app_calculator() {
    gui_window_t* window = gui_window_create("Hesap Makinesi", 150, 120, 200, 250, GUI_WINDOW_STYLE_NORMAL);
    if (!window) return;
    
    window->on_paint = calculator_paint;
    gui_window_show(window);
}

// Hesap Makinesi çizim fonksiyonu
static void calculator_paint(gui_window_t* window) {
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    
    // Hesap makinesi arkaplanı
    vga_fill_rect(x, y, window->client.width, window->client.height, GUI_COLOR_LIGHT_GRAY);
    
    // Ekran
    vga_fill_rect(x + 10, y + 10, window->client.width - 20, 30, GUI_COLOR_WHITE);
    font_draw_string(x + window->client.width - 50, y + 15, "123", GUI_COLOR_BLACK, 0xFF);
    
    // Tuşlar
    const char* keys[] = {
        "7", "8", "9", "+",
        "4", "5", "6", "-",
        "1", "2", "3", "*",
        "0", ".", "=", "/"
    };
    
    for (int i = 0; i < 16; i++) {
        int row = i / 4;
        int col = i % 4;
        
        uint32_t btn_x = x + 10 + col * 45;
        uint32_t btn_y = y + 50 + row * 45;
        
        vga_fill_rect(btn_x, btn_y, 40, 40, GUI_COLOR_BUTTON);
        
        // 3D kenarlar
        vga_draw_hline(btn_x, btn_y, 40, GUI_COLOR_WHITE);
        vga_draw_vline(btn_x, btn_y, 40, GUI_COLOR_WHITE);
        vga_draw_hline(btn_x, btn_y + 39, 40, GUI_COLOR_DARK_GRAY);
        vga_draw_vline(btn_x + 39, btn_y, 40, GUI_COLOR_DARK_GRAY);
        
        // Tuş metni
        font_draw_string(btn_x + 15, btn_y + 12, keys[i], GUI_COLOR_BLACK, 0xFF);
    }
}

// Hakkında uygulaması
void app_about() {
    gui_window_t* window = gui_window_create("KALEM OS Hakkında", 200, 150, 400, 300, GUI_WINDOW_STYLE_NORMAL);
    if (!window) return;
    
    // Pencere içeriğini çiz
    window->on_paint = [](gui_window_t* window) {
        uint32_t x = window->x + window->client.x;
        uint32_t y = window->y + window->client.y;
        uint32_t width = window->client.width;
        
        // Logo
        gui_fill_rect(x + width / 2 - 40, y + 10, 80, 20, GUI_COLOR_BLUE);
        font_draw_string_center(x + width / 2, y + 20, "KALEM OS", GUI_COLOR_WHITE, 0xFF);
        
        // Başlık
        font_draw_string_center(x + width / 2, y + 40, "KALEM OS - Türkiye'nin İşletim Sistemi", GUI_COLOR_DARK_BLUE, 0xFF);
        
        // Sürüm
        font_draw_string_center(x + width / 2, y + 70, "Sürüm 1.0", GUI_COLOR_BLACK, 0xFF);
        
        // Telif hakkı
        font_draw_string_center(x + width / 2, y + 100, "© 2023 KALEM OS Geliştirme Ekibi", GUI_COLOR_BLACK, 0xFF);
        
        // Bilgi
        font_draw_string_center(x + width / 2, y + 130, "Tüm hakları saklıdır.", GUI_COLOR_BLACK, 0xFF);
        
        // Açıklama
        font_draw_string_center(x + width / 2, y + 170, "Bu işletim sistemi Türkiye'de geliştirilen", GUI_COLOR_BLACK, 0xFF);
        font_draw_string_center(x + width / 2, y + 190, "yerli ve milli bir işletim sistemidir.", GUI_COLOR_BLACK, 0xFF);
        font_draw_string_center(x + width / 2, y + 210, "ZekaMX yapay zeka sistemi entegredir.", GUI_COLOR_BLACK, 0xFF);
    };
    
    // Pencereyi göster
    gui_window_show(window);
}

// Not++ uygulaması
void app_noteplus() {
    noteplus_show_editor();
}

// Arşiv Yöneticisi uygulaması
void app_archive() {
    archive_show_manager();
}

// WiFi uygulaması
void app_wifi() {
    wifi_show_manager();
}

// Web Tarayıcı uygulaması
void app_browser() {
    browser_show_window();
}

// Uygulama Mağazası
void app_store() {
    app_manager_show();
}

// İkon tıklama işlevine sağ tık desteği ekliyorum
uint8_t launcher_handle_click(uint32_t x, uint32_t y) {
    for (uint32_t i = 0; i < icon_count; i++) {
        app_icon_t* icon = &icons[i];
        
        // Simge alanı içinde mi kontrol et
        if (x >= icon->x && x < icon->x + 80 &&
            y >= icon->y && y < icon->y + 80) {
            
            // Simgeye tıklandığında uygulamayı başlat
            if (icon->on_click) {
                icon->on_click();
            }
            
            return 1; // Simge tıklandı
        }
    }
    
    return 0; // Simge bulunamadı
}

// Belirli bir koordinattaki uygulama ikon ID'sini döndür
uint32_t launcher_get_icon_at(uint32_t x, uint32_t y) {
    // Simge alanlarını kontrol et
    for (int i = 0; i < icon_count; i++) {
        // Simge konumunu hesapla
        uint32_t icon_x = icons[i].x;
        uint32_t icon_y = icons[i].y;
        
        // Koordinatlar simge alanında mı?
        if (x >= icon_x && x < icon_x + 80 &&
            y >= icon_y && y < icon_y + 80) {
            return i + 1; // 0-tabanlı dizinden 1-tabanlı ID'ye dönüştür
        }
    }
    
    return 0; // Simge bulunamadı
}

void launcher_draw_terminal_shortcut(int x, int y, int width, int height) {
    // Terminal simgesini ve başlığını çizer
    gui_fill_rect(x, y, width, height, GUI_COLOR_BLACK);
    font_draw_string(x + 5, y + 5, "KALEM OS Terminal v1.0", GUI_COLOR_LIGHT_GREEN, 0xFF);
}

void launcher_draw_notepad_shortcut(int x, int y, int width, int height) {
    // Not defteri simgesini ve başlığını çizer
    gui_fill_rect(x, y, width, height, GUI_COLOR_WHITE);
    gui_draw_rect(x, y, width, height, GUI_COLOR_GRAY);
    
    // Not defteri içindeki sayfalar
    gui_fill_rect(x + 5, y + 10, width - 10, height - 20, GUI_COLOR_LIGHT_GRAY);
    gui_draw_rect(x + 5, y + 10, width - 10, height - 20, GUI_COLOR_GRAY);
    
    font_draw_string(x + 10, y + 10, "KALEM OS Not Defteri", GUI_COLOR_BLACK, 0xFF);
    
    // Örnek metin
    font_draw_string(x + 10, y + 60, "Merhaba Dünya!", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 10, y + 80, "KALEM OS isletim sistemi uzerinde", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 10, y + 100, "notlar alabilirsiniz.", GUI_COLOR_BLACK, 0xFF);
}

void launcher_show_about_dialog() {
    // Hakkında penceresini oluştur
    int width = 400;
    int height = 300;
    gui_window_t* window = gui_window_create("KALEM OS Hakkında", 200, 150, 400, 300, GUI_WINDOW_STYLE_NORMAL);
    
    // Pencere içeriğini çiz
    int x = 0;
    int y = 0;
    
    // Logo
    gui_fill_rect(x + width / 2 - 40, y + 10, 80, 20, GUI_COLOR_BLUE);
    font_draw_string_center(x + width / 2, y + 20, "KALEM OS", GUI_COLOR_WHITE, 0xFF);
    
    // Başlık
    font_draw_string_center(x + width / 2, y + 40, "KALEM OS - Türkiye'nin İşletim Sistemi", GUI_COLOR_DARK_BLUE, 0xFF);
    
    // Sürüm
    font_draw_string_center(x + width / 2, y + 70, "Sürüm 1.0", GUI_COLOR_BLACK, 0xFF);
    
    // Telif hakkı
    font_draw_string_center(x + width / 2, y + 100, "© 2023 KALEM OS Geliştirme Ekibi", GUI_COLOR_BLACK, 0xFF);
    
    // ... Diğer bilgiler ...
    
    // Tamam düğmesi
    gui_button_t* ok_button = gui_button_create(window, "Tamam", x + width / 2 - 30, y + height - 40, 60, 30);
    
    // Pencereyi göster
    gui_window_show(window);
} 