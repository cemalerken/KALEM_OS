#include "../include/android/android.h"
#include "../include/android/app_manager.h"
#include "../include/gui.h"
#include "../include/kalem_app.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Sabit tanımlamalar
#define MAX_APP_LIST 100
#define BUTTON_INSTALL    1
#define BUTTON_UNINSTALL  2
#define BUTTON_LAUNCH     3
#define BUTTON_STOP       4
#define BUTTON_DETAILS    5
#define BUTTON_REFRESH    6
#define BUTTON_CLOSE      7
#define BUTTON_CLEAR_DATA 8
#define BUTTON_BACKUP     9
#define BUTTON_RESTORE    10
#define BUTTON_PERMISSIONS 11

// Android uygulama yöneticisi penceresi
static gui_window_t* app_manager_window = NULL;

// Kullanıcı arayüzü bileşenleri
typedef struct {
    gui_listview_t* app_list;
    gui_label_t* status_label;
    gui_button_t* install_button;
    gui_button_t* uninstall_button;
    gui_button_t* launch_button;
    gui_button_t* stop_button;
    gui_button_t* details_button;
    gui_button_t* refresh_button;
    gui_button_t* close_button;
    gui_button_t* clear_data_button;
    gui_button_t* backup_button;
    gui_button_t* restore_button;
    gui_button_t* permissions_button;
    
    // Uygulama listesi arabelleği
    android_app_t apps[MAX_APP_LIST];
    uint32_t app_count;
    
    // Seçili uygulama indeksi
    int selected_app_index;
} app_manager_form_t;

// Form yapısı
static app_manager_form_t form = {0};

// İleri bildirimler
static void update_app_list();
static void handle_button_click(gui_button_t* button);
static void handle_app_selection(gui_listview_t* listview, int selected_index);
static void update_button_states();
static void install_app();
static void uninstall_app();
static void launch_app();
static void stop_app();
static void show_app_details();
static void clear_app_data();
static void backup_app();
static void restore_app();
static void manage_app_permissions();

// Android uygulama yöneticisi penceresini oluştur
gui_window_t* android_app_manager_create() {
    // Pencere zaten varsa, mevcut pencereyi göster
    if (app_manager_window) {
        gui_window_show(app_manager_window);
        update_app_list();
        return app_manager_window;
    }
    
    // Yeni pencere oluştur
    app_manager_window = gui_window_create(
        "Android Uygulama Yöneticisi",
        100, 50,    // x, y konumu
        800, 600,   // genişlik, yükseklik
        GUI_WINDOW_STYLE_NORMAL | GUI_WINDOW_STYLE_TITLEBAR | GUI_WINDOW_STYLE_CLOSE
    );
    
    if (!app_manager_window) {
        return NULL;
    }
    
    // Form yapısını sıfırla
    memset(&form, 0, sizeof(form));
    form.selected_app_index = -1;
    
    // Durum etiketi oluştur
    form.status_label = gui_label_create(app_manager_window, 20, 20, 760, 30, "Durum: Hazırlanıyor...");
    
    // Uygulama listesi oluştur
    form.app_list = gui_listview_create(app_manager_window, 20, 60, 760, 400);
    gui_listview_add_column(form.app_list, "Paket Adı", 250);
    gui_listview_add_column(form.app_list, "Versiyon", 100);
    gui_listview_add_column(form.app_list, "Durum", 120);
    gui_listview_add_column(form.app_list, "Boyut", 80);
    gui_listview_add_column(form.app_list, "İzinler", 210);
    gui_listview_set_selection_callback(form.app_list, handle_app_selection);
    
    // Butonları oluştur
    int button_y = 470;
    int button_width = 140;
    int button_height = 30;
    int button_spacing = 10;
    int x_pos = 20;
    
    // İlk satır butonları
    form.install_button = gui_button_create(app_manager_window, x_pos, button_y, 
                                         button_width, button_height, "Yükle", BUTTON_INSTALL);
    gui_button_set_callback(form.install_button, handle_button_click);
    x_pos += button_width + button_spacing;
    
    form.uninstall_button = gui_button_create(app_manager_window, x_pos, button_y, 
                                         button_width, button_height, "Kaldır", BUTTON_UNINSTALL);
    gui_button_set_callback(form.uninstall_button, handle_button_click);
    x_pos += button_width + button_spacing;
    
    form.launch_button = gui_button_create(app_manager_window, x_pos, button_y, 
                                       button_width, button_height, "Başlat", BUTTON_LAUNCH);
    gui_button_set_callback(form.launch_button, handle_button_click);
    x_pos += button_width + button_spacing;
    
    form.stop_button = gui_button_create(app_manager_window, x_pos, button_y, 
                                      button_width, button_height, "Durdur", BUTTON_STOP);
    gui_button_set_callback(form.stop_button, handle_button_click);
    x_pos += button_width + button_spacing;
    
    form.details_button = gui_button_create(app_manager_window, x_pos, button_y, 
                                        button_width, button_height, "Detaylar", BUTTON_DETAILS);
    gui_button_set_callback(form.details_button, handle_button_click);
    
    // İkinci satır butonları
    button_y += button_height + button_spacing;
    x_pos = 20;
    
    form.clear_data_button = gui_button_create(app_manager_window, x_pos, button_y, 
                                          button_width, button_height, "Veriyi Temizle", BUTTON_CLEAR_DATA);
    gui_button_set_callback(form.clear_data_button, handle_button_click);
    x_pos += button_width + button_spacing;
    
    form.backup_button = gui_button_create(app_manager_window, x_pos, button_y, 
                                       button_width, button_height, "Yedekle", BUTTON_BACKUP);
    gui_button_set_callback(form.backup_button, handle_button_click);
    x_pos += button_width + button_spacing;
    
    form.restore_button = gui_button_create(app_manager_window, x_pos, button_y, 
                                        button_width, button_height, "Geri Yükle", BUTTON_RESTORE);
    gui_button_set_callback(form.restore_button, handle_button_click);
    x_pos += button_width + button_spacing;
    
    form.permissions_button = gui_button_create(app_manager_window, x_pos, button_y, 
                                          button_width, button_height, "İzinler", BUTTON_PERMISSIONS);
    gui_button_set_callback(form.permissions_button, handle_button_click);
    x_pos += button_width + button_spacing;
    
    form.refresh_button = gui_button_create(app_manager_window, x_pos, button_y, 
                                        button_width, button_height, "Yenile", BUTTON_REFRESH);
    gui_button_set_callback(form.refresh_button, handle_button_click);
    
    // Alt kısım butonları
    button_y += button_height + button_spacing;
    
    form.close_button = gui_button_create(app_manager_window, 650, button_y, 
                                      130, button_height, "Kapat", BUTTON_CLOSE);
    gui_button_set_callback(form.close_button, handle_button_click);
    
    // Uygulama listesini güncelle
    update_app_list();
    
    // Buton durumlarını güncelle
    update_button_states();
    
    return app_manager_window;
}

// Android uygulama yöneticisi penceresini kapat
void android_app_manager_close() {
    if (app_manager_window) {
        gui_window_close(app_manager_window);
        app_manager_window = NULL;
    }
}

// Uygulama listesini güncelle
static void update_app_list() {
    if (!app_manager_window) {
        return;
    }
    
    // Android sistemi başlatılmış mı kontrol et
    android_system_t* system = android_get_system();
    if (!system || !system->initialized) {
        gui_label_set_text(form.status_label, "Durum: Android sistemi etkin değil");
        return;
    }
    
    // Mevcut liste içeriğini temizle
    gui_listview_clear(form.app_list);
    
    // Uygulama listesini al
    form.app_count = 0;
    android_result_t result = android_app_manager_get_apps(form.apps, MAX_APP_LIST, &form.app_count);
    
    if (result != ANDROID_SUCCESS) {
        gui_label_set_text(form.status_label, "Durum: Uygulama listesi alınamadı");
        return;
    }
    
    // Durumu güncelle
    char status_text[256];
    snprintf(status_text, sizeof(status_text), "Durum: %u uygulama bulundu", form.app_count);
    gui_label_set_text(form.status_label, status_text);
    
    // Uygulamaları listele
    for (uint32_t i = 0; i < form.app_count; i++) {
        const android_app_t* app = &form.apps[i];
        
        // Satır içeriğini oluştur
        const char* row_data[5];
        char version_str[32];
        char size_str[32];
        char permission_count_str[64];
        
        row_data[0] = app->package_name;
        
        snprintf(version_str, sizeof(version_str), "%s", app->version);
        row_data[1] = version_str;
        
        row_data[2] = app->is_running ? "Çalışıyor" : "Durduruldu";
        
        snprintf(size_str, sizeof(size_str), "%.1f MB", (float)app->size_kb / 1024.0f);
        row_data[3] = size_str;
        
        snprintf(permission_count_str, sizeof(permission_count_str), "%u izin", app->permission_count);
        row_data[4] = permission_count_str;
        
        // Satırı ekle
        gui_listview_add_row(form.app_list, row_data, 5, (void*)(uintptr_t)i);
    }
    
    // Seçili uygulama indeksini sıfırla
    form.selected_app_index = -1;
    
    // Buton durumlarını güncelle
    update_button_states();
}

// Buton durumlarını güncelle (seçili uygulama durumuna göre)
static void update_button_states() {
    int has_selection = (form.selected_app_index >= 0 && form.selected_app_index < (int)form.app_count);
    
    // Temel butonlar, seçili uygulama varsa etkinleştir
    gui_button_set_enabled(form.uninstall_button, has_selection);
    gui_button_set_enabled(form.details_button, has_selection);
    gui_button_set_enabled(form.clear_data_button, has_selection);
    gui_button_set_enabled(form.backup_button, has_selection);
    gui_button_set_enabled(form.restore_button, has_selection);
    gui_button_set_enabled(form.permissions_button, has_selection);
    
    // Özel durumlar için butonları ayarla
    if (has_selection) {
        const android_app_t* app = &form.apps[form.selected_app_index];
        
        // Başlat/Durdur butonlarını güncelle
        gui_button_set_enabled(form.launch_button, !app->is_running);
        gui_button_set_enabled(form.stop_button, app->is_running);
        
        // Sistem uygulamaları için kaldırma işlemini devre dışı bırak
        gui_button_set_enabled(form.uninstall_button, !app->is_system_app);
    } else {
        // Hiçbir uygulama seçilmediğinde Başlat/Durdur butonlarını devre dışı bırak
        gui_button_set_enabled(form.launch_button, 0);
        gui_button_set_enabled(form.stop_button, 0);
    }
}

// Listede uygulama seçildiğinde
static void handle_app_selection(gui_listview_t* listview, int selected_index) {
    if (selected_index >= 0) {
        // Seçili satırdan indeksi al
        void* user_data = gui_listview_get_row_data(listview, selected_index);
        form.selected_app_index = (int)(uintptr_t)user_data;
    } else {
        form.selected_app_index = -1;
    }
    
    // Buton durumlarını güncelle
    update_button_states();
}

// Buton tıklama olayını işle
static void handle_button_click(gui_button_t* button) {
    uint32_t button_id = gui_button_get_id(button);
    
    switch (button_id) {
        case BUTTON_INSTALL:
            install_app();
            break;
            
        case BUTTON_UNINSTALL:
            uninstall_app();
            break;
            
        case BUTTON_LAUNCH:
            launch_app();
            break;
            
        case BUTTON_STOP:
            stop_app();
            break;
            
        case BUTTON_DETAILS:
            show_app_details();
            break;
            
        case BUTTON_REFRESH:
            update_app_list();
            break;
            
        case BUTTON_CLOSE:
            android_app_manager_close();
            break;
            
        case BUTTON_CLEAR_DATA:
            clear_app_data();
            break;
            
        case BUTTON_BACKUP:
            backup_app();
            break;
            
        case BUTTON_RESTORE:
            restore_app();
            break;
            
        case BUTTON_PERMISSIONS:
            manage_app_permissions();
            break;
    }
}

// Uygulama yükleme işlevi
static void install_app() {
    // Gerçek bir uygulamada, dosya seçici diyalog penceresi açılır
    // Burada sadece geçici bir mesaj gösteriyoruz
    gui_message_box(app_manager_window, "Bilgi", 
                   "APK dosyası seçim diyalogu açılacak...", GUI_MESSAGE_INFO);
    
    // APK seçme işlevi burada olacak...
    const char* apk_path = "/tmp/example.apk";  // Örnek değer
    
    // Yükleme işlemi
    android_result_t result = android_app_manager_install_app(apk_path);
    
    if (result == ANDROID_SUCCESS) {
        gui_message_box(app_manager_window, "Başarılı", 
                       "Uygulama başarıyla yüklendi", GUI_MESSAGE_INFO);
        update_app_list();
    } else {
        gui_message_box(app_manager_window, "Hata", 
                       "Uygulama yüklenirken bir hata oluştu", GUI_MESSAGE_ERROR);
    }
}

// Uygulama kaldırma işlevi
static void uninstall_app() {
    if (form.selected_app_index < 0 || form.selected_app_index >= (int)form.app_count) {
        return;
    }
    
    const android_app_t* app = &form.apps[form.selected_app_index];
    
    // Onaylama mesajı göster
    char message[256];
    snprintf(message, sizeof(message), 
            "\"%s\" uygulamasını kaldırmak istediğinizden emin misiniz?", app->package_name);
    
    if (gui_message_box_confirm(app_manager_window, "Uygulama Kaldırma", message) != GUI_CONFIRM_YES) {
        return;
    }
    
    // Uygulamayı kaldır
    android_result_t result = android_app_manager_uninstall_app(app->package_name);
    
    if (result == ANDROID_SUCCESS) {
        gui_message_box(app_manager_window, "Başarılı", 
                       "Uygulama başarıyla kaldırıldı", GUI_MESSAGE_INFO);
        update_app_list();
    } else {
        gui_message_box(app_manager_window, "Hata", 
                       "Uygulama kaldırılırken bir hata oluştu", GUI_MESSAGE_ERROR);
    }
}

// Uygulama başlatma işlevi
static void launch_app() {
    if (form.selected_app_index < 0 || form.selected_app_index >= (int)form.app_count) {
        return;
    }
    
    const android_app_t* app = &form.apps[form.selected_app_index];
    
    // Uygulamayı başlat
    android_result_t result = android_app_manager_launch_app(app->package_name);
    
    if (result == ANDROID_SUCCESS) {
        gui_message_box(app_manager_window, "Başarılı", 
                       "Uygulama başlatıldı", GUI_MESSAGE_INFO);
        update_app_list();
    } else {
        gui_message_box(app_manager_window, "Hata", 
                       "Uygulama başlatılırken bir hata oluştu", GUI_MESSAGE_ERROR);
    }
}

// Uygulama durdurma işlevi
static void stop_app() {
    if (form.selected_app_index < 0 || form.selected_app_index >= (int)form.app_count) {
        return;
    }
    
    const android_app_t* app = &form.apps[form.selected_app_index];
    
    // Uygulamayı durdur
    android_result_t result = android_app_manager_stop_app(app->package_name);
    
    if (result == ANDROID_SUCCESS) {
        gui_message_box(app_manager_window, "Başarılı", 
                       "Uygulama durduruldu", GUI_MESSAGE_INFO);
        update_app_list();
    } else {
        gui_message_box(app_manager_window, "Hata", 
                       "Uygulama durdurulurken bir hata oluştu", GUI_MESSAGE_ERROR);
    }
}

// Uygulama ayrıntılarını gösterme işlevi
static void show_app_details() {
    if (form.selected_app_index < 0 || form.selected_app_index >= (int)form.app_count) {
        return;
    }
    
    const android_app_t* app = &form.apps[form.selected_app_index];
    
    // Detay penceresini aç (gerçek bir uygulamada, ayrı bir pencere olacak)
    char details[1024];
    snprintf(details, sizeof(details),
            "Paket Adı: %s\n"
            "Versiyon: %s\n"
            "Durum: %s\n"
            "Boyut: %.1f MB\n"
            "İzin Sayısı: %u\n"
            "Sistem Uygulaması: %s\n"
            "Yükleme Tarihi: %s\n"
            "Son Güncelleme: %s\n"
            "Min SDK: %u\n"
            "Hedef SDK: %u\n",
            app->package_name,
            app->version,
            app->is_running ? "Çalışıyor" : "Durduruldu",
            (float)app->size_kb / 1024.0f,
            app->permission_count,
            app->is_system_app ? "Evet" : "Hayır",
            app->install_date,
            app->last_update_date,
            app->min_sdk_version,
            app->target_sdk_version);
    
    gui_message_box(app_manager_window, "Uygulama Detayları", details, GUI_MESSAGE_INFO);
}

// Uygulama verisini temizleme işlevi
static void clear_app_data() {
    if (form.selected_app_index < 0 || form.selected_app_index >= (int)form.app_count) {
        return;
    }
    
    const android_app_t* app = &form.apps[form.selected_app_index];
    
    // Onaylama mesajı göster
    char message[256];
    snprintf(message, sizeof(message), 
            "\"%s\" uygulamasının verilerini temizlemek istediğinizden emin misiniz?", app->package_name);
    
    if (gui_message_box_confirm(app_manager_window, "Veri Temizleme", message) != GUI_CONFIRM_YES) {
        return;
    }
    
    // Uygulama verisini temizle
    android_result_t result = android_app_manager_clear_app_data(app->package_name);
    
    if (result == ANDROID_SUCCESS) {
        gui_message_box(app_manager_window, "Başarılı", 
                       "Uygulama verileri temizlendi", GUI_MESSAGE_INFO);
    } else {
        gui_message_box(app_manager_window, "Hata", 
                       "Uygulama verileri temizlenirken bir hata oluştu", GUI_MESSAGE_ERROR);
    }
}

// Uygulama yedekleme işlevi
static void backup_app() {
    if (form.selected_app_index < 0 || form.selected_app_index >= (int)form.app_count) {
        return;
    }
    
    const android_app_t* app = &form.apps[form.selected_app_index];
    
    // Gerçek bir uygulamada, hedef klasör seçme diyalogu açılır
    // Burada varsayılan konum kullanıyoruz
    char backup_path[256];
    snprintf(backup_path, sizeof(backup_path), "/backup/%s.kab", app->package_name);
    
    // Uygulamayı yedekle
    android_result_t result = android_app_manager_backup_app(app->package_name, backup_path);
    
    if (result == ANDROID_SUCCESS) {
        gui_message_box(app_manager_window, "Başarılı", 
                       "Uygulama başarıyla yedeklendi", GUI_MESSAGE_INFO);
    } else {
        gui_message_box(app_manager_window, "Hata", 
                       "Uygulama yedeklenirken bir hata oluştu", GUI_MESSAGE_ERROR);
    }
}

// Uygulama geri yükleme işlevi
static void restore_app() {
    if (form.selected_app_index < 0 || form.selected_app_index >= (int)form.app_count) {
        return;
    }
    
    const android_app_t* app = &form.apps[form.selected_app_index];
    
    // Gerçek bir uygulamada, yedek dosyası seçme diyalogu açılır
    // Burada varsayılan konum kullanıyoruz
    char backup_path[256];
    snprintf(backup_path, sizeof(backup_path), "/backup/%s.kab", app->package_name);
    
    // Uygulamayı geri yükle
    android_result_t result = android_app_manager_restore_app(backup_path);
    
    if (result == ANDROID_SUCCESS) {
        gui_message_box(app_manager_window, "Başarılı", 
                       "Uygulama başarıyla geri yüklendi", GUI_MESSAGE_INFO);
        update_app_list();
    } else {
        gui_message_box(app_manager_window, "Hata", 
                       "Uygulama geri yüklenirken bir hata oluştu", GUI_MESSAGE_ERROR);
    }
}

// Uygulama izinlerini yönetme işlevi
static void manage_app_permissions() {
    if (form.selected_app_index < 0 || form.selected_app_index >= (int)form.app_count) {
        return;
    }
    
    const android_app_t* app = &form.apps[form.selected_app_index];
    
    // Gerçek bir uygulamada, izin yönetimi için ayrı bir pencere açılır
    // Burada sadece bir mesaj gösteriyoruz
    gui_message_box(app_manager_window, "İzin Yönetimi", 
                   "İzin yönetimi modülü henüz uygulanmadı", GUI_MESSAGE_INFO);
}

// Android uygulama yöneticisi penceresini göster (dışarıdan çağrılabilir)
void android_app_manager_show() {
    gui_window_t* window = android_app_manager_create();
    if (window) {
        gui_window_show(window);
    }
}

// Ana uygulama noktası (bağımsız çalıştırma için)
int android_app_manager_main(int argc, char* argv[]) {
    // GUI sistemi başlatılmış olmalı
    
    // Android uygulama yöneticisi penceresini göster
    android_app_manager_show();
    
    return 0;
} 