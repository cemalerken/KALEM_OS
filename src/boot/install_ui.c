#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../include/install_ui.h"
#include "../include/gui.h"
#include "../include/font.h"

// Renk tanımları
#define COLOR_PRIMARY          0x3477EB  // Mavi
#define COLOR_ACCENT           0x00C853  // Yeşil
#define COLOR_BACKGROUND       0xF5F5F5  // Açık gri
#define COLOR_SURFACE          0xFFFFFF  // Beyaz
#define COLOR_ERROR            0xF44336  // Kırmızı
#define COLOR_TEXT_PRIMARY     0x212121  // Koyu gri
#define COLOR_TEXT_SECONDARY   0x757575  // Orta gri
#define COLOR_GRADIENT_START   0x2196F3  // Parlak mavi
#define COLOR_GRADIENT_END     0x673AB7  // Mor

// Animasyon değerleri
#define ANIMATION_FRAMES       12
#define ANIMATION_SPEED        50  // ms

// Global değişkenler
install_state_t install_state;
install_ui_t ui;
static uint8_t animation_frame = 0;
static gui_timer_t* animation_timer = NULL;

// İleri bildirimler
static void install_ui_draw_welcome_screen();
static void install_ui_handle_welcome_events(gui_window_t* window, gui_event_t event);
static void on_welcome_next();
static void on_welcome_live();

// Animasyon çerçevelerini güncellemek için zamanlayıcı işleyicisi
static void on_animation_timer(gui_timer_t* timer, void* user_data) {
    animation_frame = (animation_frame + 1) % ANIMATION_FRAMES;
    gui_window_invalidate(ui.main_window);
}

// Kurulum arayüzünü başlat
void install_ui_init() {
    // Durum bilgisini sıfırla
    memset(&install_state, 0, sizeof(install_state_t));
    install_state.current_stage = INSTALL_STAGE_WELCOME;
    
    // UI bileşenlerini oluştur
    ui.main_window = gui_create_window("KALEM OS Kurulum", 0, 0, 800, 600, GUI_WINDOW_STYLE_FULLSCREEN);
    ui.main_layout = gui_layout_create(GUI_LAYOUT_VERTICAL);
    gui_window_set_layout(ui.main_window, ui.main_layout);
    
    // Logo
    ui.logo_image = gui_image_create("/usr/share/kalem/images/logo_large.png");
    gui_layout_add_widget(ui.main_layout, ui.logo_image);
    
    // Başlık
    ui.title_label = gui_label_create("KALEM OS Kurulum");
    gui_label_set_font_size(ui.title_label, 28);
    gui_label_set_color(ui.title_label, COLOR_PRIMARY);
    gui_layout_add_widget(ui.main_layout, ui.title_label);
    
    // Alt başlık
    ui.subtitle_label = gui_label_create("Türkiye'nin İşletim Sistemi");
    gui_label_set_font_size(ui.subtitle_label, 18);
    gui_label_set_color(ui.subtitle_label, COLOR_TEXT_SECONDARY);
    gui_layout_add_widget(ui.main_layout, ui.subtitle_label);
    
    // Durum etiketi
    ui.status_label = gui_label_create("");
    gui_label_set_font_size(ui.status_label, 14);
    gui_layout_add_widget(ui.main_layout, ui.status_label);
    
    // İlerleme çubuğu
    ui.progress_bar = gui_progress_bar_create();
    gui_progress_bar_set_color(ui.progress_bar, COLOR_PRIMARY);
    gui_layout_add_widget(ui.main_layout, ui.progress_bar);
    
    // Genel butonlar (aşama geçişlerinde kullanılır)
    ui.next_button = gui_button_create("İleri");
    ui.back_button = gui_button_create("Geri");
    ui.cancel_button = gui_button_create("İptal");
    
    // Animasyon zamanlayıcısı
    animation_timer = gui_timer_create(ANIMATION_SPEED);
    gui_timer_set_callback(animation_timer, on_animation_timer, NULL);
    gui_timer_start(animation_timer);
    
    // Aşamaya özel UI bileşenlerini başlat
    install_ui_draw_welcome_screen();
}

// Kurulum arayüzünü göster
void install_ui_show() {
    if (!ui.main_window) {
        install_ui_init();
    }
    
    gui_window_show(ui.main_window);
}

// Kurulum arayüzünü kapat
void install_ui_close() {
    if (ui.main_window) {
        gui_window_hide(ui.main_window);
        gui_window_destroy(ui.main_window);
        ui.main_window = NULL;
    }
    
    if (animation_timer) {
        gui_timer_stop(animation_timer);
        gui_timer_destroy(animation_timer);
        animation_timer = NULL;
    }
}

// İlerleme çubuğunu güncelle
void install_ui_update_progress(uint8_t progress, const char* operation) {
    install_state.install_progress = progress;
    if (operation) {
        strncpy(install_state.current_operation, operation, sizeof(install_state.current_operation) - 1);
        install_state.current_operation[sizeof(install_state.current_operation) - 1] = '\0';
    }
    
    if (ui.progress_bar) {
        gui_progress_bar_set_value(ui.progress_bar, progress);
    }
    
    if (ui.status_label && operation) {
        gui_label_set_text(ui.status_label, operation);
    }
    
    gui_window_invalidate(ui.main_window);
}

// Kurulum tamamlandı
void install_ui_complete() {
    install_state.current_stage = INSTALL_STAGE_COMPLETE;
    install_ui_update_progress(100, "Kurulum tamamlandı!");
}

// Kurulum hatası
void install_ui_error(const char* error_message) {
    if (ui.status_label && error_message) {
        gui_label_set_text(ui.status_label, error_message);
        gui_label_set_color(ui.status_label, COLOR_ERROR);
    }
}

// Hoş geldiniz ekranını oluştur
static void install_ui_draw_welcome_screen() {
    // Ana düzeni temizle
    gui_layout_clear(ui.main_layout);
    
    // Logo ekle
    gui_layout_add_widget(ui.main_layout, ui.logo_image);
    
    // Başlık
    gui_layout_add_widget(ui.main_layout, ui.title_label);
    gui_label_set_text(ui.title_label, "KALEM OS'a Hoş Geldiniz");
    
    // Alt başlık
    gui_layout_add_widget(ui.main_layout, ui.subtitle_label);
    gui_label_set_text(ui.subtitle_label, "Türkiye'nin İşletim Sistemi Kurulumu");
    
    // Bilgilendirme
    gui_label_t* info_label = gui_label_create("KALEM OS kurulumuna hoş geldiniz. Bu sihirbaz size kurulum sürecinde rehberlik edecektir.");
    gui_label_set_font_size(info_label, 14);
    gui_layout_add_widget(ui.main_layout, info_label);
    
    // Buton düzeni
    gui_layout_t* button_layout = gui_layout_create(GUI_LAYOUT_HORIZONTAL);
    gui_layout_set_spacing(button_layout, 20);
    gui_layout_add_widget(ui.main_layout, button_layout);
    
    // Kurulum butonu
    ui.welcome.install_button = gui_button_create("Kuruluma Başla");
    gui_button_set_color(ui.welcome.install_button, COLOR_PRIMARY);
    gui_button_set_text_color(ui.welcome.install_button, COLOR_SURFACE);
    gui_button_set_font_size(ui.welcome.install_button, 16);
    gui_button_set_callback(ui.welcome.install_button, on_welcome_next);
    gui_layout_add_widget(button_layout, ui.welcome.install_button);
    
    // Canlı mod butonu
    ui.welcome.live_button = gui_button_create("Canlı Mod");
    gui_button_set_color(ui.welcome.live_button, COLOR_ACCENT);
    gui_button_set_text_color(ui.welcome.live_button, COLOR_SURFACE);
    gui_button_set_font_size(ui.welcome.live_button, 16);
    gui_button_set_callback(ui.welcome.live_button, on_welcome_live);
    gui_layout_add_widget(button_layout, ui.welcome.live_button);
    
    // Version etiketi
    gui_label_t* version_label = gui_label_create("KALEM OS 1.0");
    gui_label_set_font_size(version_label, 12);
    gui_label_set_color(version_label, COLOR_TEXT_SECONDARY);
    gui_layout_add_widget(ui.main_layout, version_label);
    
    // Pencereyi yeniden çiz
    gui_window_invalidate(ui.main_window);
    
    // Olay işleyicisini ayarla
    gui_window_set_event_handler(ui.main_window, install_ui_handle_welcome_events);
}

// Hoş geldiniz ekranı için olay işleyicisi
static void install_ui_handle_welcome_events(gui_window_t* window, gui_event_t event) {
    // Olayları işle
    switch (event.type) {
        case GUI_EVENT_PAINT:
            {
                // Arka plan gradient çiz
                gui_rect_t client_rect;
                gui_window_get_client_rect(window, &client_rect);
                gui_draw_gradient_rect(window, 0, 0, client_rect.width, client_rect.height,
                                    COLOR_GRADIENT_START, COLOR_GRADIENT_END);
                
                // Logo ve animasyon efekti
                // Not: Gerçek uygulamada daha karmaşık animasyonlar olacaktır
                int logo_x = client_rect.width / 2 - 100;
                int logo_y = 50 + (animation_frame % 3);
                gui_draw_image(window, ui.logo_image, logo_x, logo_y);
            }
            break;
            
        case GUI_EVENT_BUTTON_CLICK:
            if (event.button.id == gui_button_get_id(ui.welcome.install_button)) {
                on_welcome_next();
            } else if (event.button.id == gui_button_get_id(ui.welcome.live_button)) {
                on_welcome_live();
            }
            break;
            
        case GUI_EVENT_KEY_DOWN:
            if (event.key.code == GUI_KEY_ESCAPE) {
                // ESC tuşu - çıkış
                install_ui_close();
            }
            break;
            
        default:
            break;
    }
}

// İleri butonuna tıklandığında
static void on_welcome_next() {
    // Kurulum moduna geç
    install_state.live_mode = 0;
    
    // Bir sonraki aşamaya geç (Sorumluluk beyanı)
    install_state.current_stage = INSTALL_STAGE_DISCLAIMER;
    
    // Bu dosyada sadece hoş geldiniz ekranını gösteriyoruz
    // Gerçek uygulamada burada disclaimer_screen fonksiyonu çağrılır
    gui_label_set_text(ui.status_label, "Sorumluluk beyanı ekranına geçilecek...");
}

// Canlı mod butonuna tıklandığında
static void on_welcome_live() {
    // Canlı moda geç
    install_state.live_mode = 1;
    
    // Pencereyi kapat
    install_ui_close();
    
    // Live moda başla
    gui_label_set_text(ui.status_label, "Canlı mod başlatılıyor...");
} 