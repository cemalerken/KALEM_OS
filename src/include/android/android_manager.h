#ifndef ANDROID_MANAGER_H
#define ANDROID_MANAGER_H

#include <stdint.h>
#include "android_container.h"
#include "android_runtime.h"

// Android uygulama türleri
typedef enum {
    ANDROID_APP_TYPE_ACTIVITY,      // Normal uygulama aktivitesi
    ANDROID_APP_TYPE_SERVICE,       // Arka plan servisi
    ANDROID_APP_TYPE_RECEIVER,      // Yayın alıcı
    ANDROID_APP_TYPE_CONTENT_PROVIDER // İçerik sağlayıcı
} android_app_type_t;

// Android uygulama durumları
typedef enum {
    ANDROID_APP_STATE_STOPPED,      // Durduruldu
    ANDROID_APP_STATE_STARTING,     // Başlatılıyor
    ANDROID_APP_STATE_RUNNING,      // Çalışıyor
    ANDROID_APP_STATE_PAUSED,       // Duraklatıldı
    ANDROID_APP_STATE_STOPPED,      // Durduruldu
    ANDROID_APP_STATE_ERROR         // Hata durumu
} android_app_state_t;

// Android uygulama izinleri
typedef enum {
    PERMISSION_INTERNET = (1 << 0),         // İnternet erişimi
    PERMISSION_STORAGE = (1 << 1),          // Depolama erişimi
    PERMISSION_LOCATION = (1 << 2),         // Konum servisleri
    PERMISSION_CAMERA = (1 << 3),           // Kamera erişimi
    PERMISSION_MICROPHONE = (1 << 4),       // Mikrofon erişimi
    PERMISSION_CONTACTS = (1 << 5),         // Kişiler erişimi
    PERMISSION_PHONE = (1 << 6),            // Telefon erişimi
    PERMISSION_SMS = (1 << 7),              // SMS erişimi
    PERMISSION_BLUETOOTH = (1 << 8),        // Bluetooth erişimi
    PERMISSION_ADMIN = (1 << 9)             // Yönetici erişimi
} android_permission_t;

// Android uygulama bilgileri
typedef struct {
    char package_name[256];         // Paket adı (com.example.app)
    char app_name[128];             // Uygulama adı
    char version_name[64];          // Sürüm adı
    uint32_t version_code;          // Sürüm kodu
    uint32_t min_sdk_version;       // Minimum SDK sürümü
    uint32_t target_sdk_version;    // Hedef SDK sürümü
    
    char app_icon_path[256];        // Uygulama simgesi yolu
    char app_data_path[256];        // Uygulama veri dizini
    
    uint64_t permissions;           // İzin bayrakları
    uint8_t is_system_app;          // Sistem uygulaması mı?
    uint8_t is_debuggable;          // Hata ayıklanabilir mi?
    
    // APK bilgileri
    char apk_path[256];             // APK dosya yolu
    uint64_t apk_size;              // APK dosya boyutu
    char apk_signature[256];        // APK imzası
} android_app_info_t;

// Android uygulama yapısı
typedef struct {
    android_app_info_t info;        // Uygulama bilgileri
    android_app_state_t state;      // Uygulama durumu
    android_container_t* container; // Konteyner
    
    uint32_t pid;                   // İşlem kimliği
    uint32_t uid;                   // Kullanıcı kimliği
    
    // Performans ve kaynak kullanımı
    uint64_t memory_usage;          // Bellek kullanımı
    float cpu_usage;                // CPU kullanımı
    uint64_t start_time;            // Başlangıç zamanı
    uint64_t last_active_time;      // Son aktif olma zamanı
} android_app_t;

// Android uygulama yöneticisi
typedef struct {
    uint32_t app_count;             // Kurulu uygulama sayısı
    android_app_t** apps;           // Uygulama listesi
    
    char apps_dir[256];             // Uygulamalar dizini
    char data_dir[256];             // Veri dizini
    
    uint8_t initialized;            // Başlatıldı mı?
    art_runtime_t* runtime;         // Android Runtime referansı
} android_manager_t;

// Uygulama yönetimi API
int android_manager_initialize();
android_manager_t* android_manager_get_instance();
int android_manager_refresh_app_list();
int android_manager_cleanup();

// APK yönetimi
int android_install_apk(const char* apk_path);
int android_uninstall_app(const char* package_name);
int android_update_app(const char* apk_path);
android_app_info_t* android_get_app_info(const char* apk_path);

// Uygulama yaşam döngüsü
android_app_t* android_launch_app(const char* package_name);
int android_stop_app(android_app_t* app);
int android_pause_app(android_app_t* app);
int android_resume_app(android_app_t* app);
int android_force_stop_app(android_app_t* app);

// Uygulama veri ve önbellek yönetimi
int android_clear_app_data(const char* package_name);
int android_clear_app_cache(const char* package_name);
int android_move_app_to_sd(const char* package_name, uint8_t to_sd);

// Uygulama bilgisi ve izin yönetimi
android_app_t* android_find_app_by_package(const char* package_name);
android_app_t* android_find_app_by_pid(uint32_t pid);
uint8_t android_check_permission(android_app_t* app, android_permission_t permission);
int android_grant_permission(android_app_t* app, android_permission_t permission);
int android_revoke_permission(android_app_t* app, android_permission_t permission);

// İşlevsel API
int android_send_intent(const char* action, const char* target_package, const char* target_component, void* extras);
int android_broadcast_intent(const char* action, void* extras);

#endif /* ANDROID_MANAGER_H */ 