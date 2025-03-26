#include "../include/android/android.h"
#include "../include/android/app_manager.h"
#include "../include/android_settings.h"
#include "../include/android_app_manager.h"
#include "../include/gui.h"
#include "../include/kalem_app.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Android başlatıcı penceresi
static gui_window_t* launcher_window = NULL;

// Sabit tanımlamalar
#define MAX_APP_ICONS 24
#define ICON_WIDTH  80
#define ICON_HEIGHT 100
#define ICON_SPACING 20
#define BUTTON_SETTINGS 100
#define BUTTON_APP_MANAGER 101

// İkon yapısı
typedef struct {
    gui_button_t* icon_button;
    char package_name[256];
    char app_name[256];
    uint8_t is_system_app;
} app_icon_t;

// Kullanıcı arayüzü bileşenleri
typedef struct {
    gui_label_t* status_label;
    gui_button_t* settings_button;
    gui_button_t* app_manager_button;
    
    // Uygulama simgeleri
    app_icon_t icons[MAX_APP_ICONS];
    uint32_t icon_count;
} launcher_form_t;

// Form yapısı
static launcher_form_t form = {0};

// İleri bildirimler
static void update_app_icons();
static void handle_button_click(gui_button_t* button);
static void handle_icon_click(gui_button_t* button);

// Android başlatıcı penceresini oluştur
gui_window_t* android_launcher_create() {
    // Pencere zaten varsa, mevcut pencereyi göster
    if (launcher_window) {
        gui_window_show(launcher_window);
        update_app_icons();
        return launcher_window;
    }
    
    // Yeni pencere oluştur
    launcher_window = gui_window_create(
        "Android Başlatıcı",
        50, 50,    // x, y konumu
        800, 600,   // genişlik, yükseklik
        GUI_WINDOW_STYLE_NORMAL | GUI_WINDOW_STYLE_TITLEBAR | GUI_WINDOW_STYLE_CLOSE
    );
    
    if (!launcher_window) {
        return NULL;
    }
    
    // Form yapısını sıfırla
    memset(&form, 0, sizeof(form));
    
    // Durum etiketi oluştur
    form.status_label = gui_label_create(launcher_window, 20, 20, 760, 30, "Durum: Hazırlanıyor...");
    
    // Alt kısımdaki butonlar
    form.settings_button = gui_button_create(launcher_window, 20, 550, 150, 30, "Android Ayarları", BUTTON_SETTINGS);
    gui_button_set_callback(form.settings_button, handle_button_click);
    
    form.app_manager_button = gui_button_create(launcher_window, 190, 550, 150, 30, "Uygulama Yöneticisi", BUTTON_APP_MANAGER);
    gui_button_set_callback(form.app_manager_button, handle_button_click);
    
    // İkon listesini güncelle
    update_app_icons();
    
    return launcher_window;
}

// Android başlatıcı penceresini kapat
void android_launcher_close() {
    if (launcher_window) {
        gui_window_close(launcher_window);
        launcher_window = NULL;
    }
}

// Uygulama ikonlarını güncelle
static void update_app_icons() {
    if (!launcher_window) {
        return;
    }
    
    // Android sistemi başlatılmış mı kontrol et
    android_system_t* system = android_get_system();
    if (!system || !system->initialized) {
        gui_label_set_text(form.status_label, "Durum: Android sistemi etkin değil");
        return;
    }
    
    // Mevcut ikonları temizle
    for (uint32_t i = 0; i < form.icon_count; i++) {
        if (form.icons[i].icon_button) {
            gui_button_destroy(form.icons[i].icon_button);
            form.icons[i].icon_button = NULL;
        }
    }
    form.icon_count = 0;
    
    // Uygulamaları al
    android_app_t apps[MAX_APP_ICONS];
    uint32_t app_count = 0;
    android_result_t result = android_app_manager_get_apps(apps, MAX_APP_ICONS, &app_count);
    
    if (result != ANDROID_SUCCESS) {
        gui_label_set_text(form.status_label, "Durum: Uygulama listesi alınamadı");
        return;
    }
    
    // Durumu güncelle
    char status_text[256];
    snprintf(status_text, sizeof(status_text), "Durum: %u uygulama bulundu", app_count);
    gui_label_set_text(form.status_label, status_text);
    
    // İkon konumlarını hesapla
    int area_width = 760;
    int area_height = 460;  // Alt panel için yer bırak
    int start_x = 20;
    int start_y = 60;
    
    int icons_per_row = (area_width + ICON_SPACING) / (ICON_WIDTH + ICON_SPACING);
    if (icons_per_row < 1) icons_per_row = 1;
    
    // İkonları oluştur
    for (uint32_t i = 0; i < app_count && i < MAX_APP_ICONS; i++) {
        int row = i / icons_per_row;
        int col = i % icons_per_row;
        
        int x = start_x + col * (ICON_WIDTH + ICON_SPACING);
        int y = start_y + row * (ICON_HEIGHT + ICON_SPACING);
        
        // İkon butonu oluştur
        char button_text[256];
        const char* app_name = apps[i].app_name[0] ? apps[i].app_name : apps[i].package_name;
        snprintf(button_text, sizeof(button_text), "%s", app_name);
        
        form.icons[i].icon_button = gui_button_create(launcher_window, x, y, ICON_WIDTH, ICON_HEIGHT, button_text, i);
        gui_button_set_callback(form.icons[i].icon_button, handle_icon_click);
        
        // Simge bilgilerini kaydet
        strncpy(form.icons[i].package_name, apps[i].package_name, sizeof(form.icons[i].package_name) - 1);
        strncpy(form.icons[i].app_name, app_name, sizeof(form.icons[i].app_name) - 1);
        form.icons[i].is_system_app = apps[i].is_system_app;
        
        form.icon_count++;
    }
}

// Buton tıklama olayını işle
static void handle_button_click(gui_button_t* button) {
    uint32_t button_id = gui_button_get_id(button);
    
    switch (button_id) {
        case BUTTON_SETTINGS:
            // Android ayarlarını aç
            android_settings_show();
            break;
            
        case BUTTON_APP_MANAGER:
            // Uygulama yöneticisini aç
            android_app_manager_show();
            break;
    }
}

// İkon tıklama olayını işle
static void handle_icon_click(gui_button_t* button) {
    uint32_t icon_index = gui_button_get_id(button);
    
    if (icon_index < form.icon_count) {
        // Uygulamayı başlat
        android_result_t result = android_app_manager_launch_app(form.icons[icon_index].package_name);
        
        if (result != ANDROID_SUCCESS) {
            char error_message[256];
            snprintf(error_message, sizeof(error_message), 
                    "Uygulama başlatılamadı: %s", form.icons[icon_index].package_name);
            gui_message_box(launcher_window, "Hata", error_message, GUI_MESSAGE_ERROR);
        } else {
            update_app_icons();  // Başlayan uygulamanın durumunu güncelle
        }
    }
}

// Android başlatıcı penceresini göster (dışarıdan çağrılabilir)
void android_launcher_show() {
    gui_window_t* window = android_launcher_create();
    if (window) {
        gui_window_show(window);
    }
}

// Ana uygulama noktası (bağımsız çalıştırma için)
int android_launcher_main(int argc, char* argv[]) {
    // GUI sistemi başlatılmış olmalı
    
    // Android sistemini başlat
    android_system_t* system = android_get_system();
    if (!system || !system->initialized) {
        // Android sistemi başlatılmamış, başlat
        android_config_t config = {0};
        config.art_enabled = 1;
        config.container_enabled = 1;
        config.binder_enabled = 1;
        config.graphics_bridge_enabled = 1;
        config.memory_limit_mb = 512;
        config.cpu_limit_percent = 50;
        config.enable_hw_acceleration = 1;
        config.enable_audio = 1;
        config.enable_network = 1;
        config.android_version = ANDROID_VERSION_10;
        
        android_result_t result = android_system_initialize(&config);
        if (result != ANDROID_SUCCESS) {
            gui_message_box(NULL, "Hata", 
                           "Android sistemi başlatılamadı! Uygulama sonlandırılıyor.", 
                           GUI_MESSAGE_ERROR);
            return 1;
        }
    }
    
    // Android başlatıcı penceresini göster
    android_launcher_show();
    
    return 0;
} 