#include "../include/android/android.h"
#include "../include/gui.h"
#include "../include/kalem_app.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Android ayarlar penceresi
static gui_window_t* android_settings_window = NULL;

// Buton sabitleri
#define BUTTON_ENABLE_ANDROID         1
#define BUTTON_DISABLE_ANDROID        2
#define BUTTON_RESTART_ANDROID        3
#define BUTTON_MEMORY_LIMIT_256       4
#define BUTTON_MEMORY_LIMIT_512       5
#define BUTTON_MEMORY_LIMIT_1024      6
#define BUTTON_HW_ACCEL_ENABLE        7
#define BUTTON_HW_ACCEL_DISABLE       8
#define BUTTON_NETWORK_ENABLE         9
#define BUTTON_NETWORK_DISABLE        10
#define BUTTON_AUDIO_ENABLE           11
#define BUTTON_AUDIO_DISABLE          12
#define BUTTON_APPLY                  13
#define BUTTON_CANCEL                 14
#define BUTTON_OPEN_APP_MANAGER       15
#define BUTTON_VERSION_SELECT         16

// Form bileşenleri
typedef struct {
    gui_button_t* enable_android;
    gui_button_t* disable_android;
    gui_button_t* restart_android;
    gui_button_t* memory_256mb;
    gui_button_t* memory_512mb;
    gui_button_t* memory_1024mb;
    gui_button_t* hw_accel_enable;
    gui_button_t* hw_accel_disable;
    gui_button_t* network_enable;
    gui_button_t* network_disable;
    gui_button_t* audio_enable;
    gui_button_t* audio_disable;
    gui_button_t* apply_button;
    gui_button_t* cancel_button;
    gui_button_t* open_app_manager;
    gui_dropdown_t* version_select;
    
    // Durum göstergeleri
    gui_label_t* status_label;
    gui_progress_bar_t* memory_bar;
    gui_progress_bar_t* cpu_bar;
    
    // Seçili ayarlar (geçici depolama)
    uint8_t android_enabled;
    uint32_t memory_limit_mb;
    uint8_t hw_accel_enabled;
    uint8_t network_enabled;
    uint8_t audio_enabled;
    android_version_t android_version;
} android_settings_form_t;

// Form yapısı
static android_settings_form_t form = {0};

// İleri bildirimler
static void update_settings_display();
static void update_system_stats();
static void handle_button_click(gui_button_t* button);
static void apply_settings();

// Android ayarlar penceresini oluştur
gui_window_t* android_settings_create() {
    // Pencere zaten varsa, mevcut pencereyi göster
    if (android_settings_window) {
        gui_window_show(android_settings_window);
        update_settings_display();
        return android_settings_window;
    }
    
    // Yeni pencere oluştur
    android_settings_window = gui_window_create(
        "Android Ayarları",
        100, 50,    // x, y konumu
        600, 500,   // genişlik, yükseklik
        GUI_WINDOW_STYLE_NORMAL | GUI_WINDOW_STYLE_TITLEBAR | GUI_WINDOW_STYLE_CLOSE
    );
    
    if (!android_settings_window) {
        return NULL;
    }
    
    // Form yapısını sıfırla
    memset(&form, 0, sizeof(form));
    
    // Mevcut Android yapılandırmasını al
    android_system_t* system = android_get_system();
    if (system && system->initialized) {
        form.android_enabled = 1;
        form.memory_limit_mb = system->config.memory_limit_mb;
        form.hw_accel_enabled = system->config.enable_hw_acceleration;
        form.network_enabled = system->config.enable_network;
        form.audio_enabled = system->config.enable_audio;
        form.android_version = system->config.android_version;
    } else {
        // Android başlatılmamış, varsayılan değerler kullan
        form.android_enabled = 0;
        form.memory_limit_mb = 512;
        form.hw_accel_enabled = 1;
        form.network_enabled = 1;
        form.audio_enabled = 1;
        form.android_version = ANDROID_VERSION_10;
    }
    
    // Durum başlığı
    gui_label_create(android_settings_window, 20, 20, 560, 30, "Android Durum");
    
    // Durum etiketi oluştur
    form.status_label = gui_label_create(android_settings_window, 20, 50, 560, 30, "Durum: Hazırlanıyor...");
    
    // Bellek kullanım çubuğu
    gui_label_create(android_settings_window, 20, 90, 100, 20, "Bellek Kullanımı:");
    form.memory_bar = gui_progress_bar_create(android_settings_window, 130, 90, 450, 20, 0, 100, 0);
    
    // CPU kullanım çubuğu
    gui_label_create(android_settings_window, 20, 120, 100, 20, "CPU Kullanımı:");
    form.cpu_bar = gui_progress_bar_create(android_settings_window, 130, 120, 450, 20, 0, 100, 0);
    
    // Yatay ayırıcı
    gui_separator_create(android_settings_window, 20, 150, 560, 2, GUI_SEPARATOR_HORIZONTAL);
    
    // Android Etkinleştirme bölümü
    gui_label_create(android_settings_window, 20, 170, 560, 30, "Android Etkinleştirme");
    
    // Android etkinleştirme butonları
    form.enable_android = gui_button_create(android_settings_window, 20, 210, 150, 30, "Etkinleştir", BUTTON_ENABLE_ANDROID);
    form.disable_android = gui_button_create(android_settings_window, 180, 210, 150, 30, "Devre Dışı Bırak", BUTTON_DISABLE_ANDROID);
    form.restart_android = gui_button_create(android_settings_window, 340, 210, 150, 30, "Yeniden Başlat", BUTTON_RESTART_ANDROID);
    gui_button_set_callback(form.enable_android, handle_button_click);
    gui_button_set_callback(form.disable_android, handle_button_click);
    gui_button_set_callback(form.restart_android, handle_button_click);
    
    // Yatay ayırıcı
    gui_separator_create(android_settings_window, 20, 250, 560, 2, GUI_SEPARATOR_HORIZONTAL);
    
    // Versiyon seçimi
    gui_label_create(android_settings_window, 20, 260, 150, 30, "Android Versiyonu:");
    form.version_select = gui_dropdown_create(android_settings_window, 180, 260, 200, 30, BUTTON_VERSION_SELECT);
    gui_dropdown_add_item(form.version_select, "Android 6.0 (Marshmallow)", ANDROID_VERSION_6);
    gui_dropdown_add_item(form.version_select, "Android 7.0 (Nougat)", ANDROID_VERSION_7);
    gui_dropdown_add_item(form.version_select, "Android 8.0 (Oreo)", ANDROID_VERSION_8);
    gui_dropdown_add_item(form.version_select, "Android 9.0 (Pie)", ANDROID_VERSION_9);
    gui_dropdown_add_item(form.version_select, "Android 10", ANDROID_VERSION_10);
    gui_dropdown_add_item(form.version_select, "Android 11", ANDROID_VERSION_11);
    gui_dropdown_set_selected(form.version_select, form.android_version);
    
    // Bellek limiti bölümü
    gui_label_create(android_settings_window, 20, 300, 560, 30, "Bellek Limiti");
    
    // Bellek limiti butonları
    form.memory_256mb = gui_button_create(android_settings_window, 20, 330, 150, 30, "256 MB", BUTTON_MEMORY_LIMIT_256);
    form.memory_512mb = gui_button_create(android_settings_window, 180, 330, 150, 30, "512 MB", BUTTON_MEMORY_LIMIT_512);
    form.memory_1024mb = gui_button_create(android_settings_window, 340, 330, 150, 30, "1024 MB", BUTTON_MEMORY_LIMIT_1024);
    gui_button_set_callback(form.memory_256mb, handle_button_click);
    gui_button_set_callback(form.memory_512mb, handle_button_click);
    gui_button_set_callback(form.memory_1024mb, handle_button_click);
    
    // Donanım hızlandırma bölümü
    gui_label_create(android_settings_window, 20, 370, 280, 30, "Donanım Hızlandırma");
    
    // Donanım hızlandırma butonları
    form.hw_accel_enable = gui_button_create(android_settings_window, 20, 400, 150, 30, "Etkinleştir", BUTTON_HW_ACCEL_ENABLE);
    form.hw_accel_disable = gui_button_create(android_settings_window, 180, 400, 150, 30, "Devre Dışı", BUTTON_HW_ACCEL_DISABLE);
    gui_button_set_callback(form.hw_accel_enable, handle_button_click);
    gui_button_set_callback(form.hw_accel_disable, handle_button_click);
    
    // Network bölümü
    gui_label_create(android_settings_window, 340, 370, 240, 30, "Ağ Erişimi");
    
    // Network butonları
    form.network_enable = gui_button_create(android_settings_window, 340, 400, 110, 30, "Etkinleştir", BUTTON_NETWORK_ENABLE);
    form.network_disable = gui_button_create(android_settings_window, 460, 400, 110, 30, "Devre Dışı", BUTTON_NETWORK_DISABLE);
    gui_button_set_callback(form.network_enable, handle_button_click);
    gui_button_set_callback(form.network_disable, handle_button_click);
    
    // Ses bölümü
    gui_label_create(android_settings_window, 20, 440, 240, 30, "Ses Desteği");
    
    // Ses butonları
    form.audio_enable = gui_button_create(android_settings_window, 20, 470, 150, 30, "Etkinleştir", BUTTON_AUDIO_ENABLE);
    form.audio_disable = gui_button_create(android_settings_window, 180, 470, 150, 30, "Devre Dışı", BUTTON_AUDIO_DISABLE);
    gui_button_set_callback(form.audio_enable, handle_button_click);
    gui_button_set_callback(form.audio_disable, handle_button_click);
    
    // Uygulama yöneticisi butonu
    form.open_app_manager = gui_button_create(android_settings_window, 340, 470, 240, 30, "Uygulama Yöneticisini Aç", BUTTON_OPEN_APP_MANAGER);
    gui_button_set_callback(form.open_app_manager, handle_button_click);
    
    // Uygula ve İptal butonları
    form.apply_button = gui_button_create(android_settings_window, 340, 510, 110, 30, "Uygula", BUTTON_APPLY);
    form.cancel_button = gui_button_create(android_settings_window, 460, 510, 110, 30, "İptal", BUTTON_CANCEL);
    gui_button_set_callback(form.apply_button, handle_button_click);
    gui_button_set_callback(form.cancel_button, handle_button_click);
    
    // Görünümü güncelle
    update_settings_display();
    
    // Periyodik güncelleme için zamanlayıcı başlat
    // Gerçek bir uygulamada zamanlayıcı işlevselliği burada olacaktır
    
    return android_settings_window;
}

// Android ayarlar penceresini kapat
void android_settings_close() {
    if (android_settings_window) {
        gui_window_close(android_settings_window);
        android_settings_window = NULL;
    }
}

// Android ayarlar görünümünü güncelle
static void update_settings_display() {
    if (!android_settings_window) {
        return;
    }
    
    // Android etkinleştirme butonlarının durumunu güncelle
    gui_button_set_enabled(form.enable_android, !form.android_enabled);
    gui_button_set_enabled(form.disable_android, form.android_enabled);
    gui_button_set_enabled(form.restart_android, form.android_enabled);
    
    // Bellek limiti butonlarının durumunu güncelle
    gui_button_set_state(form.memory_256mb, form.memory_limit_mb == 256 ? GUI_BUTTON_PRESSED : GUI_BUTTON_NORMAL);
    gui_button_set_state(form.memory_512mb, form.memory_limit_mb == 512 ? GUI_BUTTON_PRESSED : GUI_BUTTON_NORMAL);
    gui_button_set_state(form.memory_1024mb, form.memory_limit_mb == 1024 ? GUI_BUTTON_PRESSED : GUI_BUTTON_NORMAL);
    
    // Donanım hızlandırma butonlarının durumunu güncelle
    gui_button_set_state(form.hw_accel_enable, form.hw_accel_enabled ? GUI_BUTTON_PRESSED : GUI_BUTTON_NORMAL);
    gui_button_set_state(form.hw_accel_disable, !form.hw_accel_enabled ? GUI_BUTTON_PRESSED : GUI_BUTTON_NORMAL);
    
    // Ağ butonlarının durumunu güncelle
    gui_button_set_state(form.network_enable, form.network_enabled ? GUI_BUTTON_PRESSED : GUI_BUTTON_NORMAL);
    gui_button_set_state(form.network_disable, !form.network_enabled ? GUI_BUTTON_PRESSED : GUI_BUTTON_NORMAL);
    
    // Ses butonlarının durumunu güncelle
    gui_button_set_state(form.audio_enable, form.audio_enabled ? GUI_BUTTON_PRESSED : GUI_BUTTON_NORMAL);
    gui_button_set_state(form.audio_disable, !form.audio_enabled ? GUI_BUTTON_PRESSED : GUI_BUTTON_NORMAL);
    
    // Versiyon seçimini güncelle
    gui_dropdown_set_selected(form.version_select, form.android_version);
    
    // Sistem istatistiklerini güncelle
    update_system_stats();
}

// Sistem istatistiklerini güncelle
static void update_system_stats() {
    if (!android_settings_window) {
        return;
    }
    
    // Android sistem durumunu al
    android_system_t* system = android_get_system();
    android_system_info_t info = {0};
    
    // Sistem başlatılmış mı kontrol et
    if (system && system->initialized) {
        // Sistem bilgisi al
        info = android_get_system_info();
        
        // Durum etiketini güncelle
        char status_text[256];
        snprintf(status_text, sizeof(status_text), 
                "Durum: %s | Sürüm: %s | Çalışan Uygulamalar: %u", 
                info.state == ANDROID_SYSTEM_STATE_RUNNING ? "Çalışıyor" : "Durduruldu",
                info.version_string,
                info.running_apps_count);
        gui_label_set_text(form.status_label, status_text);
        
        // İlerleme çubuklarını güncelle
        float memory_percent = (float)info.memory_usage_mb / (float)info.memory_limit_mb * 100.0f;
        gui_progress_bar_set_value(form.memory_bar, (int)memory_percent);
        gui_progress_bar_set_text(form.memory_bar, "%d MB / %d MB (%.1f%%)", 
                                info.memory_usage_mb, info.memory_limit_mb, memory_percent);
        
        gui_progress_bar_set_value(form.cpu_bar, (int)info.cpu_usage_percent);
        gui_progress_bar_set_text(form.cpu_bar, "%.1f%% / %.1f%%", 
                                info.cpu_usage_percent, info.cpu_limit_percent);
    } else {
        // Android başlatılmamış
        gui_label_set_text(form.status_label, "Durum: Android etkin değil");
        gui_progress_bar_set_value(form.memory_bar, 0);
        gui_progress_bar_set_text(form.memory_bar, "0 MB / 0 MB (0%)");
        gui_progress_bar_set_value(form.cpu_bar, 0);
        gui_progress_bar_set_text(form.cpu_bar, "0% / 0%");
    }
}

// Buton tıklama olayını işle
static void handle_button_click(gui_button_t* button) {
    uint32_t button_id = gui_button_get_id(button);
    
    switch (button_id) {
        case BUTTON_ENABLE_ANDROID:
            form.android_enabled = 1;
            break;
            
        case BUTTON_DISABLE_ANDROID:
            form.android_enabled = 0;
            break;
            
        case BUTTON_RESTART_ANDROID:
            // Android sistemini yeniden başlat
            if (android_get_system() && android_get_system()->initialized) {
                android_restart_all();
            }
            break;
            
        case BUTTON_MEMORY_LIMIT_256:
            form.memory_limit_mb = 256;
            break;
            
        case BUTTON_MEMORY_LIMIT_512:
            form.memory_limit_mb = 512;
            break;
            
        case BUTTON_MEMORY_LIMIT_1024:
            form.memory_limit_mb = 1024;
            break;
            
        case BUTTON_HW_ACCEL_ENABLE:
            form.hw_accel_enabled = 1;
            break;
            
        case BUTTON_HW_ACCEL_DISABLE:
            form.hw_accel_enabled = 0;
            break;
            
        case BUTTON_NETWORK_ENABLE:
            form.network_enabled = 1;
            break;
            
        case BUTTON_NETWORK_DISABLE:
            form.network_enabled = 0;
            break;
            
        case BUTTON_AUDIO_ENABLE:
            form.audio_enabled = 1;
            break;
            
        case BUTTON_AUDIO_DISABLE:
            form.audio_enabled = 0;
            break;
            
        case BUTTON_OPEN_APP_MANAGER:
            // Android uygulama yöneticisini aç
            // Gerçek bir uygulamada, burada uygulama yöneticisi açılır
            gui_message_box(android_settings_window, "Bilgi", "Uygulama Yöneticisi açılıyor...", GUI_MESSAGE_INFO);
            break;
            
        case BUTTON_APPLY:
            // Ayarları uygula
            apply_settings();
            break;
            
        case BUTTON_CANCEL:
            // Pencereyi kapat
            android_settings_close();
            break;
    }
    
    // Görünümü güncelle
    update_settings_display();
}

// Android versiyon değişikliğini işle
static void handle_version_change(gui_dropdown_t* dropdown, int selected_index, void* user_data) {
    // Seçilen Android versiyonunu güncelle
    form.android_version = (android_version_t)selected_index;
}

// Ayarları uygula
static void apply_settings() {
    android_system_t* system = android_get_system();
    
    // Android başlatılmış mı veya kapatılmış mı kontrol et
    if (form.android_enabled) {
        if (!system || !system->initialized) {
            // Android etkin değilse başlat
            android_config_t config = {0};
            config.art_enabled = 1;
            config.container_enabled = 1;
            config.binder_enabled = 1;
            config.graphics_bridge_enabled = 1;
            config.memory_limit_mb = form.memory_limit_mb;
            config.cpu_limit_percent = 50;
            config.enable_hw_acceleration = form.hw_accel_enabled;
            config.enable_audio = form.audio_enabled;
            config.enable_network = form.network_enabled;
            config.android_version = form.android_version;
            
            if (android_system_initialize(&config) != ANDROID_SUCCESS) {
                gui_message_box(android_settings_window, "Hata", 
                                "Android sistemi başlatılamadı!", GUI_MESSAGE_ERROR);
                return;
            }
            
            gui_message_box(android_settings_window, "Bilgi", 
                           "Android sistemi başlatıldı.", GUI_MESSAGE_INFO);
        } else {
            // Android zaten etkin, yapılandırmayı güncelle
            android_config_t config = {0};
            config.memory_limit_mb = form.memory_limit_mb;
            config.cpu_limit_percent = 50;
            config.enable_hw_acceleration = form.hw_accel_enabled;
            config.enable_audio = form.audio_enabled;
            config.enable_network = form.network_enabled;
            
            if (android_update_config(&config) != ANDROID_SUCCESS) {
                gui_message_box(android_settings_window, "Hata", 
                                "Android yapılandırması güncellenemedi!", GUI_MESSAGE_ERROR);
                return;
            }
            
            gui_message_box(android_settings_window, "Bilgi", 
                           "Android yapılandırması güncellendi.", GUI_MESSAGE_INFO);
        }
    } else {
        // Android'i kapat
        if (system && system->initialized) {
            if (android_system_cleanup() != ANDROID_SUCCESS) {
                gui_message_box(android_settings_window, "Hata", 
                                "Android sistemi kapatılamadı!", GUI_MESSAGE_ERROR);
                return;
            }
            
            gui_message_box(android_settings_window, "Bilgi", 
                           "Android sistemi kapatıldı.", GUI_MESSAGE_INFO);
        }
    }
    
    // Görünümü güncelle
    update_settings_display();
}

// Android ayarlar penceresini göster (dışarıdan çağrılabilir)
void android_settings_show() {
    gui_window_t* window = android_settings_create();
    if (window) {
        gui_window_show(window);
    }
}

// Ana uygulama noktası (bağımsız çalıştırma için)
int android_settings_main(int argc, char* argv[]) {
    // GUI sistemi başlatılmış olmalı
    
    // Android ayarlar penceresini göster
    android_settings_show();
    
    return 0;
} 