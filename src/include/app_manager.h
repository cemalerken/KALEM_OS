#ifndef __APP_MANAGER_H__
#define __APP_MANAGER_H__

#include "gui.h"
#include <stdint.h>

// Uygulama kategorileri
typedef enum {
    APP_CATEGORY_SYSTEM,      // Sistem
    APP_CATEGORY_OFFICE,      // Ofis
    APP_CATEGORY_MEDIA,       // Medya
    APP_CATEGORY_INTERNET,    // İnternet
    APP_CATEGORY_GAMES,       // Oyunlar
    APP_CATEGORY_DEVELOPMENT, // Geliştirme
    APP_CATEGORY_UTILITIES,   // Araçlar
    APP_CATEGORY_OTHER        // Diğer
} app_category_t;

// Uygulama durumu
typedef enum {
    APP_STATUS_NOT_INSTALLED, // Kurulu değil
    APP_STATUS_INSTALLING,    // Kuruluyor
    APP_STATUS_INSTALLED,     // Kurulu
    APP_STATUS_UPDATING,      // Güncelleniyor
    APP_STATUS_REMOVING       // Kaldırılıyor
} app_status_t;

// Uygulama kaynağı
typedef enum {
    APP_SOURCE_SYSTEM,        // Sistem uygulaması
    APP_SOURCE_STORE,         // Uygulama mağazası
    APP_SOURCE_FILE,          // Dosya sistemi
    APP_SOURCE_ANDROID_APK    // Android APK
} app_source_t;

// Uygulama yapısı
typedef struct {
    char name[64];             // Uygulama adı
    char package_name[128];    // Paket adı
    char version[32];          // Sürüm
    char developer[64];        // Geliştirici
    char description[256];     // Açıklama
    uint32_t icon_id;          // Simge ID
    uint32_t size_kb;          // Boyut (KB)
    app_category_t category;   // Kategori
    app_status_t status;       // Durum
    app_source_t source;       // Kaynak
    uint8_t is_android;        // Android uygulaması mı?
    uint8_t desktop_shortcut;  // Masaüstü kısayolu var mı?
    uint8_t autostart;         // Otomatik başlatma
    void (*app_function)();    // Uygulama başlatma fonksiyonu
} app_t;

// APK bilgileri
typedef struct {
    char package_name[128];    // Paket adı
    char app_name[64];         // Uygulama adı
    char version[32];          // Sürüm
    uint32_t min_sdk;          // Minimum SDK
    uint32_t target_sdk;       // Hedef SDK
    uint32_t icon_id;          // Simge ID
    uint8_t has_permission[16];// İzinler dizisi
} apk_info_t;

// Uygulama yöneticisi başlat
void app_manager_init();

// Uygulama yöneticisi penceresini göster
void app_manager_show();

// Uygulama ekle
int app_manager_add_app(const app_t* app);

// Uygulama kur
int app_manager_install_app(const char* package_name);

// Uygulama kaldır
int app_manager_uninstall_app(const char* package_name);

// Uygulama başlat
int app_manager_launch_app(const char* package_name);

// APK dosyasını kur
int app_manager_install_apk(const char* apk_path);

// APK bilgilerini çıkar
apk_info_t* app_manager_extract_apk_info(const char* apk_path);

// Masaüstü kısayolu ekle
int app_manager_add_desktop_shortcut(const char* package_name);

// Masaüstü kısayolu kaldır
int app_manager_remove_desktop_shortcut(const char* package_name);

// Android uygulamasını çalıştır
int app_manager_run_android_app(const char* package_name);

// Kategori adını al
const char* app_manager_get_category_name(app_category_t category);

// Durum adını al
const char* app_manager_get_status_name(app_status_t status);

// Kaynak adını al
const char* app_manager_get_source_name(app_source_t source);

// Uygulama arayüzü
void app_store();

#endif /* __APP_MANAGER_H__ */ 