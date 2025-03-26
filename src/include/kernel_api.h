#ifndef KERNEL_API_H
#define KERNEL_API_H

#include <stdint.h>

/**
 * @file kernel_api.h
 * @brief KALEM OS Kernel API
 * 
 * Bu modül, KALEM OS kerneline programatik erişim sağlayan API'yi tanımlar.
 * Python gibi üst seviye diller bu API üzerinden kernel ile etkileşime geçebilir.
 */

/** Kernel API hata kodları */
typedef enum {
    KERNEL_API_SUCCESS = 0,        // Başarılı
    KERNEL_API_ERROR_INIT = -1,    // Başlatma hatası
    KERNEL_API_ERROR_PARAM = -2,   // Parametre hatası
    KERNEL_API_ERROR_ACCESS = -3,  // Erişim hatası
    KERNEL_API_ERROR_NOT_FOUND = -4, // Bulunamadı hatası
    KERNEL_API_ERROR_MEMORY = -5,  // Bellek hatası
    KERNEL_API_ERROR_IO = -6,      // G/Ç hatası
    KERNEL_API_ERROR_BUSY = -7,    // Meşgul hatası
    KERNEL_API_ERROR_TIMEOUT = -8, // Zaman aşımı hatası
    KERNEL_API_ERROR_UNKNOWN = -99 // Bilinmeyen hata
} kernel_api_error_t;

/** Donanım bilgileri */
typedef struct {
    char cpu_model[128];           // İşlemci modeli
    uint32_t cpu_speed;            // İşlemci hızı (MHz)
    uint8_t cpu_cores;             // İşlemci çekirdek sayısı
    uint32_t ram_mb;               // RAM miktarı (MB)
    uint64_t disk_gb;              // Disk kapasitesi (GB)
    uint8_t has_gpu;               // GPU var mı?
    char gpu_model[128];           // GPU modeli
    uint32_t gpu_vram_mb;          // GPU bellek (MB)
    uint8_t has_wifi;              // WiFi var mı?
    uint8_t has_ethernet;          // Ethernet var mı?
    uint8_t has_bluetooth;         // Bluetooth var mı?
    uint8_t has_touchscreen;       // Dokunmatik ekran var mı?
    uint8_t has_battery;           // Batarya var mı?
    uint8_t battery_percentage;    // Batarya yüzdesi
} hardware_info_t;

/** Sürücü bilgileri */
typedef struct {
    char name[64];                 // Sürücü adı
    uint8_t is_loaded;             // Yüklü mü?
    uint8_t is_builtin;            // Dahili mi?
    uint32_t version;              // Sürücü sürümü
    char device_path[128];         // Cihaz yolu
    char vendor[64];               // Üretici
    uint8_t status;                // Durum
} driver_info_t;

/** Süreç bilgileri */
typedef struct {
    uint32_t pid;                  // Süreç kimliği
    char name[64];                 // Süreç adı
    uint8_t priority;              // Öncelik
    float cpu_usage;               // CPU kullanımı (%)
    uint32_t memory_kb;            // Bellek kullanımı (KB)
    uint32_t start_time;           // Başlangıç zamanı (epoch)
    uint8_t status;                // Durum
    uint32_t parent_pid;           // Üst süreç kimliği
    uint8_t is_system;             // Sistem süreci mi?
} process_info_t;

/** Kernel istatistikleri */
typedef struct {
    uint64_t uptime_ms;            // Çalışma süresi (ms)
    float cpu_usage;               // CPU kullanımı (%)
    uint32_t memory_total_kb;      // Toplam bellek (KB)
    uint32_t memory_used_kb;       // Kullanılan bellek (KB)
    uint32_t memory_free_kb;       // Boş bellek (KB)
    uint32_t disk_total_kb;        // Toplam disk (KB)
    uint32_t disk_used_kb;         // Kullanılan disk (KB)
    uint32_t disk_free_kb;         // Boş disk (KB)
    uint32_t process_count;        // Süreç sayısı
    uint32_t thread_count;         // İş parçacığı sayısı
} kernel_stats_t;

/** Ağ arayüzü bilgileri */
typedef struct {
    char name[32];                 // Arayüz adı
    uint8_t is_up;                 // Aktif mi?
    uint8_t is_wireless;           // Kablosuz mu?
    char mac_address[18];          // MAC adresi
    char ip_address[16];           // IP adresi
    char netmask[16];              // Alt ağ maskesi
    char gateway[16];              // Ağ geçidi
    uint32_t bytes_received;       // Alınan bayt sayısı
    uint32_t bytes_sent;           // Gönderilen bayt sayısı
    int8_t signal_strength;        // Sinyal gücü (dBm)
} network_interface_t;

/** Dosya sistemi bilgileri */
typedef struct {
    char mount_point[128];         // Bağlama noktası
    char device_path[128];         // Cihaz yolu
    char filesystem_type[32];      // Dosya sistemi türü
    uint64_t total_bytes;          // Toplam boyut (bayt)
    uint64_t used_bytes;           // Kullanılan boyut (bayt)
    uint64_t free_bytes;           // Boş boyut (bayt)
    uint8_t is_readonly;           // Salt okunur mu?
} filesystem_info_t;

/** USB cihaz bilgileri */
typedef struct {
    uint16_t vendor_id;            // Üretici kimliği
    uint16_t product_id;           // Ürün kimliği
    char vendor_name[64];          // Üretici adı
    char product_name[64];         // Ürün adı
    char serial_number[32];        // Seri numarası
    uint8_t device_class;          // Cihaz sınıfı
    uint8_t device_protocol;       // Cihaz protokolü
    uint8_t is_connected;          // Bağlı mı?
    char mount_point[128];         // Bağlama noktası (varsa)
} usb_device_t;

/**
 * Donanım bilgilerini alır
 * 
 * @param info_out Donanım bilgisi yapısı
 * @return int 0: başarılı, <0: hata
 */
int kernel_get_hardware_info(hardware_info_t* info_out);

/**
 * Sürücü yükler
 * 
 * @param driver_name Sürücü adı
 * @return int 0: başarılı, <0: hata
 */
int kernel_load_driver(const char* driver_name);

/**
 * Sürücü kaldırır
 * 
 * @param driver_name Sürücü adı
 * @return int 0: başarılı, <0: hata
 */
int kernel_unload_driver(const char* driver_name);

/**
 * Sürücü durumunu alır
 * 
 * @param driver_name Sürücü adı
 * @param info_out Sürücü bilgisi
 * @return int 0: başarılı, <0: hata
 */
int kernel_get_driver_info(const char* driver_name, driver_info_t* info_out);

/**
 * Yüklü sürücü listesini alır
 * 
 * @param drivers_out Sürücü bilgisi dizisi (serbest bırakılmalıdır)
 * @param count_out Sürücü sayısı
 * @return int 0: başarılı, <0: hata
 */
int kernel_get_driver_list(driver_info_t** drivers_out, int* count_out);

/**
 * Süreç listesini alır
 * 
 * @param processes_out Süreç bilgisi dizisi (serbest bırakılmalıdır)
 * @param count_out Süreç sayısı
 * @return int 0: başarılı, <0: hata
 */
int kernel_get_process_list(process_info_t** processes_out, int* count_out);

/**
 * Süreç bilgilerini alır
 * 
 * @param pid Süreç kimliği
 * @param info_out Süreç bilgisi
 * @return int 0: başarılı, <0: hata
 */
int kernel_get_process_info(uint32_t pid, process_info_t* info_out);

/**
 * Süreç sonlandırır
 * 
 * @param pid Süreç kimliği
 * @param force Zorla sonlandırma
 * @return int 0: başarılı, <0: hata
 */
int kernel_kill_process(uint32_t pid, uint8_t force);

/**
 * Süreç önceliğini ayarlar
 * 
 * @param pid Süreç kimliği
 * @param priority Öncelik (0-255)
 * @return int 0: başarılı, <0: hata
 */
int kernel_set_process_priority(uint32_t pid, uint8_t priority);

/**
 * Kernel istatistiklerini alır
 * 
 * @param stats_out Kernel istatistikleri
 * @return int 0: başarılı, <0: hata
 */
int kernel_get_stats(kernel_stats_t* stats_out);

/**
 * Ağ arayüzlerini listeler
 * 
 * @param interfaces_out Arayüz bilgisi dizisi (serbest bırakılmalıdır)
 * @param count_out Arayüz sayısı
 * @return int 0: başarılı, <0: hata
 */
int kernel_get_network_interfaces(network_interface_t** interfaces_out, int* count_out);

/**
 * Ağ arayüzünü yönetir
 * 
 * @param interface_name Arayüz adı
 * @param command Komut (up, down, restart)
 * @return int 0: başarılı, <0: hata
 */
int kernel_manage_network_interface(const char* interface_name, const char* command);

/**
 * Dosya sistemlerini listeler
 * 
 * @param filesystems_out Dosya sistemi bilgisi dizisi (serbest bırakılmalıdır)
 * @param count_out Dosya sistemi sayısı
 * @return int 0: başarılı, <0: hata
 */
int kernel_get_filesystems(filesystem_info_t** filesystems_out, int* count_out);

/**
 * USB cihazları listeler
 * 
 * @param devices_out USB cihaz bilgisi dizisi (serbest bırakılmalıdır)
 * @param count_out Cihaz sayısı
 * @return int 0: başarılı, <0: hata
 */
int kernel_get_usb_devices(usb_device_t** devices_out, int* count_out);

/**
 * Sistem zamanını alır
 * 
 * @param time_out Sistem zamanı (epoch)
 * @return int 0: başarılı, <0: hata
 */
int kernel_get_system_time(uint64_t* time_out);

/**
 * Sistem zamanını ayarlar
 * 
 * @param time Sistem zamanı (epoch)
 * @return int 0: başarılı, <0: hata
 */
int kernel_set_system_time(uint64_t time);

/**
 * Sistemi yeniden başlatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int kernel_reboot();

/**
 * Sistemi kapatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int kernel_shutdown();

/**
 * CPU sıcaklığını alır
 * 
 * @param temp_out Sıcaklık (Celsius)
 * @return int 0: başarılı, <0: hata
 */
int kernel_get_cpu_temp(float* temp_out);

/**
 * Sistem logunu oku
 * 
 * @param buffer_out Çıktı arabelleği (serbest bırakılmalıdır)
 * @param max_lines En fazla kaç satır
 * @return int 0: başarılı, <0: hata
 */
int kernel_read_system_log(char** buffer_out, uint32_t max_lines);

/**
 * Sistem log seviyesini ayarlar
 * 
 * @param level Log seviyesi (0-5)
 * @return int 0: başarılı, <0: hata
 */
int kernel_set_log_level(uint8_t level);

#endif /* KERNEL_API_H */ 