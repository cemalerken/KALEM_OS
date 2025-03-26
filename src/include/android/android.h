#ifndef ANDROID_H
#define ANDROID_H

/**
 * KALEM OS Android Uyumluluk Katmanı
 * 
 * Bu başlık dosyası, KALEM OS'un Android uygulamalarını çalıştırmasını 
 * sağlayan tüm alt sistemleri içerir.
 */

#include "android_runtime.h"      // Android Runtime (ART)
#include "android_container.h"    // Konteyner yönetimi
#include "android_bridge.h"       // KALEM OS köprüsü
#include "android_manager.h"      // Uygulama yönetimi

// Android sistem sürüm bilgileri
#define ANDROID_VERSION_NAME      "11.0"
#define ANDROID_VERSION_CODE      30
#define ANDROID_API_LEVEL         30
#define ANDROID_CODENAME          "R"

// Uyumluluk katmanı sürüm bilgileri
#define ANDROID_COMPAT_VERSION    "1.0.0"
#define ANDROID_COMPAT_BUILD      1001

// Android entegrasyonu hata kodları
#define ANDROID_SUCCESS           0
#define ANDROID_ERROR_INIT        -1
#define ANDROID_ERROR_RUNTIME     -2
#define ANDROID_ERROR_CONTAINER   -3
#define ANDROID_ERROR_BRIDGE      -4
#define ANDROID_ERROR_MANAGER     -5
#define ANDROID_ERROR_APP         -6
#define ANDROID_ERROR_PERMISSION  -7
#define ANDROID_ERROR_RESOURCE    -8
#define ANDROID_ERROR_IO          -9
#define ANDROID_ERROR_UNKNOWN     -99

// Android uyumluluğu yapılandırma seçenekleri
typedef struct {
    uint8_t enable_hardware_acceleration; // Donanım hızlandırma etkin mi?
    uint8_t enable_audio;                // Ses desteği etkin mi?
    uint8_t enable_network;              // Ağ desteği etkin mi?
    uint8_t enable_camera;               // Kamera desteği etkin mi?
    uint8_t enable_sensors;              // Sensör desteği etkin mi?
    uint8_t enable_location;             // Konum desteği etkin mi?
    
    uint32_t container_memory_limit;     // Konteyner bellek limiti (MB)
    uint32_t container_storage_limit;    // Konteyner depolama limiti (MB)
    
    char system_image_path[256];         // Android sistem imajı yolu
    char data_path[256];                 // Android veri dizini
    char app_path[256];                  // Uygulama dizini
    
    uint8_t debug_mode;                  // Hata ayıklama modu
    char log_path[256];                  // Günlük dosyası yolu
} android_config_t;

// Ana Android uyumluluk işlevleri
int android_initialize(android_config_t* config);
int android_start_runtime();
int android_stop_runtime();
int android_cleanup();

// Durum kontrolü
uint8_t android_is_initialized();
uint8_t android_is_runtime_running();
int android_get_status(int* runtime, int* container, int* bridge, int* manager);

// Yapılandırma
int android_get_config(android_config_t* config);
int android_set_config(android_config_t* config);
int android_save_config(const char* file_path);
int android_load_config(const char* file_path);

// Sistem fonksiyonları
int android_pause_all();
int android_resume_all();
int android_kill_all();
int android_get_memory_usage(uint64_t* used, uint64_t* total);
int android_get_disk_usage(uint64_t* used, uint64_t* total);

// Loglama
int android_set_log_level(int level);
int android_get_last_error(char* error_msg, int max_len);

// Yardımcı işlevler
const char* android_error_to_string(int error_code);
int android_create_shortcut(const char* package_name, const char* desktop_path);

#endif /* ANDROID_H */ 