#ifndef ANDROID_CONTAINER_H
#define ANDROID_CONTAINER_H

#include <stdint.h>

// Android konteyner durumları
typedef enum {
    CONTAINER_STATUS_STOPPED,      // Konteyner durduruldu
    CONTAINER_STATUS_STARTING,     // Konteyner başlatılıyor
    CONTAINER_STATUS_RUNNING,      // Konteyner çalışıyor
    CONTAINER_STATUS_PAUSED,       // Konteyner duraklatıldı
    CONTAINER_STATUS_STOPPING,     // Konteyner durduruluyor
    CONTAINER_STATUS_ERROR         // Konteyner hata durumunda
} container_status_t;

// İzolasyon seviyeleri
typedef enum {
    ISOLATION_NONE,                // İzolasyon yok (tehlikeli)
    ISOLATION_BASIC,               // Temel izolasyon
    ISOLATION_ADVANCED,            // Gelişmiş izolasyon
    ISOLATION_FULL                 // Tam izolasyon
} isolation_level_t;

// Konteyner kaynak limitleri
typedef struct {
    uint32_t max_memory_mb;        // Maksimum bellek (MB)
    uint32_t max_cpu_percent;      // Maksimum CPU kullanımı (%)
    uint32_t max_disk_mb;          // Maksimum disk kullanımı (MB)
    uint32_t max_processes;        // Maksimum işlem sayısı
} container_limits_t;

// Android konteyner yapısı
typedef struct android_container {
    uint32_t id;                   // Konteyner kimliği
    char name[64];                 // Konteyner adı
    char root_path[256];           // Kök dizin yolu
    isolation_level_t isolation;   // İzolasyon seviyesi
    container_status_t status;     // Durum
    container_limits_t limits;     // Kaynak limitleri
    
    // İşlem yönetimi
    uint32_t main_pid;             // Ana işlem kimliği
    uint32_t process_count;        // Toplam işlem sayısı
    
    // Sistem kaynakları
    void* filesystem;              // Dosya sistemi yapısı
    void* network;                 // Ağ yapısı
    void* ipc;                     // Süreçler arası iletişim
    
    // İstatistikler
    uint64_t start_time;           // Başlangıç zamanı
    uint64_t memory_usage;         // Bellek kullanımı
    float cpu_usage;               // CPU kullanımı
} android_container_t;

// Konteyner yönetim API
int container_initialize();
android_container_t* container_create(const char* name, isolation_level_t isolation);
int container_start(android_container_t* container);
int container_stop(android_container_t* container);
int container_pause(android_container_t* container);
int container_resume(android_container_t* container);
int container_destroy(android_container_t* container);

// Dosya sistemi yönetimi
int container_mount_android_fs(android_container_t* container, const char* system_image);
int container_mount_data(android_container_t* container, const char* data_path);
int container_bind_mount(android_container_t* container, const char* host_path, const char* container_path);

// İşlem yönetimi
int container_exec(android_container_t* container, const char* command, char* const argv[]);
int container_kill_process(android_container_t* container, uint32_t pid);
int container_signal(android_container_t* container, int signal);

// Kaynak yönetimi
int container_set_limits(android_container_t* container, container_limits_t* limits);
int container_get_usage(android_container_t* container, container_limits_t* usage);

// İstatistik ve izleme
int container_get_stats(android_container_t* container, void* stats);
int container_attach_logger(android_container_t* container, void (*log_callback)(const char*, void*), void* user_data);

#endif /* ANDROID_CONTAINER_H */ 