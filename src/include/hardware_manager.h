#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * @file hardware_manager.h
 * @brief KALEM OS Donanım Yönetim Merkezi
 * 
 * Bu modül, KALEM OS'un donanım bileşenlerini algılama, yönetme ve izleme
 * fonksiyonlarını sağlar. Tüm donanım etkileşimleri için merkezi bir arayüz sunar.
 */

/**
 * Hata kodları
 */
typedef enum {
    HW_ERROR_NONE              = 0,    /* Başarılı */
    HW_ERROR_UNKNOWN           = -1,   /* Bilinmeyen hata */
    HW_ERROR_NOT_INITIALIZED   = -2,   /* Yönetici başlatılmamış */
    HW_ERROR_ALREADY_INIT      = -3,   /* Yönetici zaten başlatılmış */
    HW_ERROR_MEMORY            = -4,   /* Bellek hatası */
    HW_ERROR_INVALID_ARG       = -5,   /* Geçersiz parametre */
    HW_ERROR_NOT_FOUND         = -6,   /* Bileşen bulunamadı */
    HW_ERROR_NOT_SUPPORTED     = -7,   /* Özellik desteklenmiyor */
    HW_ERROR_BUSY              = -8,   /* Yönetici meşgul */
    HW_ERROR_ACCESS            = -9,   /* Erişim reddedildi */
    HW_ERROR_TIMEOUT           = -10,  /* İşlem zaman aşımına uğradı */
    HW_ERROR_DEVICE            = -11,  /* Aygıt hatası */
    HW_ERROR_IO                = -12,  /* I/O hatası */
    HW_ERROR_NOT_LOADED        = -13,  /* Sürücü yüklü değil */
    HW_ERROR_DRIVER            = -14,  /* Sürücü hatası */
    HW_ERROR_FILE              = -15,  /* Dosya hatası */
    HW_ERROR_THREAD            = -16,  /* Thread oluşturma hatası */
    HW_ERROR_MUTEX             = -17,  /* Mutex hatası */
    HW_ERROR_OVERRUN           = -18,  /* Taşma hatası */
    HW_ERROR_CONFIG            = -19,  /* Yapılandırma hatası */
    HW_ERROR_INVALID_STATE     = -20   /* Geçersiz durum */
} hw_error_t;

/**
 * @file hardware_manager.h
 * @brief KALEM OS Donanım Yönetim Merkezi
 * 
 * Bu modül, KALEM OS'un donanım bileşenlerini algılama, yönetme ve izleme
 * fonksiyonlarını sağlar. Tüm donanım etkileşimleri için merkezi bir arayüz sunar.
 */

/**
 * Donanım bileşeni türleri
 */
typedef enum {
    HW_TYPE_UNKNOWN        = 0,
    HW_TYPE_CPU            = 1,
    HW_TYPE_GPU            = 2,
    HW_TYPE_MEMORY         = 3,
    HW_TYPE_STORAGE        = 4,
    HW_TYPE_NETWORK        = 5,
    HW_TYPE_AUDIO          = 6,
    HW_TYPE_USB            = 7,
    HW_TYPE_BLUETOOTH      = 8,
    HW_TYPE_DISPLAY        = 9,
    HW_TYPE_INPUT          = 10,
    HW_TYPE_BATTERY        = 11,
    HW_TYPE_SENSOR         = 12,
    HW_TYPE_CAMERA         = 13,
    HW_TYPE_PRINTER        = 14,
    HW_TYPE_OTHER          = 15
} hw_component_type_t;

/**
 * Donanım sürücüsü durumu
 */
typedef enum {
    DRIVER_STATUS_UNKNOWN      = 0,
    DRIVER_STATUS_MISSING      = 1,
    DRIVER_STATUS_INSTALLED    = 2,
    DRIVER_STATUS_LOADED       = 3,
    DRIVER_STATUS_ACTIVE       = 4,
    DRIVER_STATUS_ERROR        = 5,
    DRIVER_STATUS_DISABLED     = 6,
    DRIVER_STATUS_OUTDATED     = 7
} driver_status_t;

/**
 * Donanım bileşeni durumu
 */
typedef enum {
    HW_STATUS_UNKNOWN      = 0,
    HW_STATUS_OK           = 1,
    HW_STATUS_WARNING      = 2,
    HW_STATUS_ERROR        = 3,
    HW_STATUS_CRITICAL     = 4,
    HW_STATUS_DISABLED     = 5,
    HW_STATUS_SUSPENDED    = 6,
    HW_STATUS_NOT_PRESENT  = 7
} hw_status_t;

/**
 * Donanım güç durumu
 */
typedef enum {
    HW_POWER_UNKNOWN       = 0,
    HW_POWER_NORMAL        = 1,
    HW_POWER_SAVING        = 2,
    HW_POWER_HIGH_PERF     = 3,
    HW_POWER_OFF           = 4,
    HW_POWER_SLEEP         = 5
} hw_power_state_t;

/**
 * Donanım bileşeni adresi (bus/device/function gibi)
 */
typedef struct {
    uint8_t bus_type;      // PCI, USB, I2C, vb.
    uint8_t bus;           // Bus numarası
    uint8_t device;        // Aygıt numarası
    uint8_t function;      // Fonksiyon numarası
    uint32_t domain;       // PCI domain
    char path[64];         // Aygıt yolu (örn. "/dev/sda")
} hw_address_t;

/**
 * Donanım bileşeni temel bilgileri
 */
typedef struct {
    uint32_t id;                   // Benzersiz ID
    hw_component_type_t type;      // Donanım türü
    char name[64];                 // Aygıt adı
    char vendor[32];               // Üretici
    char model[64];                // Model bilgisi
    char serial[32];               // Seri numarası
    char firmware_version[32];     // Aygıt yazılımı sürümü
    hw_address_t address;          // Aygıt adresi
    uint64_t capacity;             // Kapasite/Büyüklük (varsa)
    uint32_t flags;                // Özellik bayrakları
} hw_component_info_t;

/**
 * Donanım sürücüsü bilgileri
 */
typedef struct {
    char name[64];                 // Sürücü adı
    char version[32];              // Sürücü sürümü
    char path[128];                // Sürücü dosya yolu
    driver_status_t status;        // Sürücü durumu
    bool is_opensource;            // Açık kaynak mı?
    bool is_kernel_module;         // Kernel modülü mü?
    bool supports_hotplug;         // Sıcak takıp-çıkarma desteği var mı?
    uint32_t load_time;            // Yüklenme zamanı (epoch)
    uint32_t flags;                // Ek bayraklar
} hw_driver_info_t;

/**
 * Donanım durum ve performans bilgileri
 */
typedef struct {
    hw_status_t status;            // Genel durum
    hw_power_state_t power_state;  // Güç durumu
    uint8_t temperature;           // Sıcaklık (Celsius)
    uint8_t utilization;           // Kullanım oranı (%)
    uint32_t power_usage;          // Güç tüketimi (mW)
    uint32_t error_count;          // Hata sayısı
    uint32_t last_error_time;      // Son hata zamanı
    char last_error_msg[128];      // Son hata mesajı
    uint32_t uptime;               // Çalışma süresi (saniye)
} hw_status_info_t;

/**
 * Tam donanım bileşeni yapısı
 */
typedef struct {
    hw_component_info_t info;      // Temel bilgiler
    hw_driver_info_t driver;       // Sürücü bilgileri
    hw_status_info_t status;       // Durum bilgileri
    void* extra_data;              // Tür-spesifik ek veriler
    uint32_t extra_data_size;      // Ek veri boyutu
} hw_component_t;

/**
 * CPU özel bilgileri
 */
typedef struct {
    uint8_t core_count;            // Çekirdek sayısı
    uint8_t thread_count;          // İş parçacığı sayısı
    uint32_t base_freq;            // Temel frekans (MHz)
    uint32_t max_freq;             // Maksimum frekans (MHz)
    uint32_t cache_l1;             // L1 önbellek boyutu (KB)
    uint32_t cache_l2;             // L2 önbellek boyutu (KB)
    uint32_t cache_l3;             // L3 önbellek boyutu (KB)
    uint8_t architecture;          // Mimari (x86, ARM, vb.)
    uint8_t bit_width;             // Bit genişliği (32, 64)
    char features[128];            // CPU özellikleri
} hw_cpu_info_t;

/**
 * GPU özel bilgileri
 */
typedef struct {
    uint32_t core_count;           // GPU çekirdeği sayısı
    uint32_t core_freq;            // Çekirdek frekansı (MHz)
    uint64_t memory_size;          // Bellek boyutu (byte)
    uint32_t memory_type;          // Bellek türü
    uint32_t memory_freq;          // Bellek frekansı (MHz)
    uint32_t api_version;          // API sürümü (OpenGL, Vulkan, DirectX)
    char features[128];            // GPU özellikleri
} hw_gpu_info_t;

/**
 * Depolama birimi özel bilgileri
 */
typedef struct {
    uint8_t type;                  // HDD, SSD, NVMe, vb.
    uint64_t total_size;           // Toplam boyut (byte)
    uint64_t free_size;            // Boş alan (byte)
    uint32_t block_size;           // Blok boyutu (byte)
    uint32_t read_speed;           // Okuma hızı (MB/s)
    uint32_t write_speed;          // Yazma hızı (MB/s)
    char filesystem[16];           // Dosya sistemi
    char mount_point[64];          // Bağlama noktası
    bool removable;                // Çıkarılabilir mi?
    uint32_t partition_count;      // Bölüm sayısı
} hw_storage_info_t;

/**
 * Ağ arayüzü özel bilgileri
 */
typedef struct {
    char mac_address[18];          // MAC adresi
    char ip_address[16];           // IP adresi
    char netmask[16];              // Alt ağ maskesi
    char gateway[16];              // Ağ geçidi
    uint8_t type;                  // Ethernet, WiFi, vb.
    uint32_t speed;                // Bağlantı hızı (Mbps)
    bool is_connected;             // Bağlı mı?
    uint64_t rx_bytes;             // Alınan byte
    uint64_t tx_bytes;             // Gönderilen byte
    bool dhcp_enabled;             // DHCP etkin mi?
} hw_network_info_t;

/**
 * Donanım yöneticisi yapılandırması
 */
typedef struct {
    bool enable_hotplug;           // Sıcak tak-çıkar tespiti
    bool enable_monitoring;        // Donanım izleme
    uint32_t scan_interval;        // Tarama aralığı (saniye)
    uint32_t monitor_interval;     // İzleme aralığı (saniye)
    bool enable_ai_optimization;   // AI optimizasyonu
    bool auto_driver_update;       // Otomatik sürücü güncellemesi
    bool security_checks;          // Güvenlik kontrolleri
    char driver_repo_url[128];     // Sürücü depo URL'si
    uint32_t log_level;            // Günlük seviyesi
} hw_manager_config_t;

/**
 * Donanım yöneticisi durumu
 */
typedef struct {
    bool initialized;              // Başlatıldı mı?
    uint32_t component_count;      // Toplam bileşen sayısı
    uint32_t driver_count;         // Toplam sürücü sayısı
    uint32_t last_scan_time;       // Son tarama zamanı
    uint32_t uptime;               // Çalışma süresi
    uint8_t system_health;         // Sistem sağlığı (%)
    uint32_t total_errors;         // Toplam hata sayısı
    bool scan_in_progress;         // Tarama devam ediyor
    bool update_in_progress;       // Güncelleme devam ediyor
} hw_manager_status_t;

/**
 * @brief Donanım yöneticisini başlatır
 * 
 * @param config Yapılandırma parametreleri (NULL: varsayılan)
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_init(const hw_manager_config_t* config);

/**
 * @brief Donanım yöneticisini kapatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_cleanup();

/**
 * @brief Donanım taraması başlatır
 * 
 * @param full_scan Tam tarama yapılsın mı? (false: sadece yeni/değişen aygıtlar)
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_scan(bool full_scan);

/**
 * @brief Donanım yöneticisi durumunu alır
 * 
 * @param status Durum çıktısı
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_get_status(hw_manager_status_t* status);

/**
 * @brief Donanım bileşeni listesini alır
 * 
 * @param type Donanım türü (HW_TYPE_UNKNOWN: tüm türler)
 * @param components Bileşen dizisi
 * @param max_count Maksimum bileşen sayısı
 * @param count_out Bulunan bileşen sayısı
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_get_components(hw_component_type_t type, 
                             hw_component_t* components, 
                             uint32_t max_count, 
                             uint32_t* count_out);

/**
 * @brief Donanım bileşeni bilgilerini alır
 * 
 * @param component_id Bileşen ID
 * @param component Bileşen çıktısı
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_get_component(uint32_t component_id, hw_component_t* component);

/**
 * @brief Donanım bileşeninin durumunu günceller
 * 
 * @param component_id Bileşen ID
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_update_status(uint32_t component_id);

/**
 * @brief Donanım sürücüsünü yükler
 * 
 * @param component_id Bileşen ID
 * @param force_reload Yeniden yükleme yapılsın mı?
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_load_driver(uint32_t component_id, bool force_reload);

/**
 * @brief Donanım sürücüsünü kaldırır
 * 
 * @param component_id Bileşen ID
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_unload_driver(uint32_t component_id);

/**
 * @brief Donanım sürücüsünü günceller
 * 
 * @param component_id Bileşen ID (0: tüm sürücüler)
 * @param check_only Sadece kontrol et, güncelleme yapma
 * @param update_info Güncelleme bilgisi çıktısı
 * @return int 0: başarılı, <0: hata, >0: güncelleme sayısı
 */
int hw_manager_update_driver(uint32_t component_id, bool check_only, void* update_info);

/**
 * @brief Donanım bileşeninin güç durumunu değiştirir
 * 
 * @param component_id Bileşen ID
 * @param power_state Yeni güç durumu
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_set_power_state(uint32_t component_id, hw_power_state_t power_state);

/**
 * @brief Donanım bileşenini etkinleştirir veya devre dışı bırakır
 * 
 * @param component_id Bileşen ID
 * @param enable Etkinleştir (true) veya devre dışı bırak (false)
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_set_enabled(uint32_t component_id, bool enable);

/**
 * @brief Donanım durumunu izlemeyi başlatır
 * 
 * @param component_id Bileşen ID (0: tüm bileşenler)
 * @param interval_ms İzleme aralığı (ms)
 * @param callback İzleme geri çağırma fonksiyonu
 * @return int 0: başarılı, <0: hata, >0: izleme ID'si
 */
int hw_manager_start_monitoring(uint32_t component_id, uint32_t interval_ms, void* callback);

/**
 * @brief Donanım durumunu izlemeyi durdurur
 * 
 * @param monitor_id İzleme ID'si (0: tüm izlemeler)
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_stop_monitoring(uint32_t monitor_id);

/**
 * @brief Yapay zeka destekli optimizasyonu yapılandırır
 * 
 * @param enable Etkinleştir/devre dışı bırak
 * @param optimization_level Optimizasyon seviyesi (0-3)
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_configure_ai(bool enable, uint8_t optimization_level);

/**
 * @brief Sistem donanımı durum raporunu oluşturur
 * 
 * @param report_file Rapor dosya yolu (NULL: standart çıktı)
 * @param format Rapor formatı (0: özet, 1: ayrıntılı, 2: XML, 3: JSON)
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_generate_report(const char* report_file, uint8_t format);

/**
 * @brief Donanım bileşeni için özel işlemi çalıştırır
 * 
 * @param component_id Bileşen ID
 * @param command Komut
 * @param params Parametreler
 * @param result Sonuç çıktısı
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_execute_command(uint32_t component_id, const char* command, 
                              const void* params, void* result);

/**
 * @brief Donanım yöneticisi günlük ayarlarını yapılandırır
 * 
 * @param log_level Günlük seviyesi
 * @param log_file Günlük dosyası (NULL: standart hata)
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_configure_logging(uint32_t log_level, const char* log_file);

/**
 * @brief Donanım yöneticisi yapılandırmasını alır
 * 
 * @param config Yapılandırma çıktısı
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_get_config(hw_manager_config_t* config);

/**
 * @brief Donanım yöneticisi yapılandırmasını ayarlar
 * 
 * @param config Yeni yapılandırma
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_set_config(const hw_manager_config_t* config);

/**
 * @brief Hata kodunu metne dönüştürür
 * 
 * @param error_code Hata kodu
 * @return const char* Hata metni
 */
const char* hw_manager_error_string(int error_code);

/* Hata Kodları */
#define HW_ERROR_NONE                   0
#define HW_ERROR_INIT_FAILED           -1
#define HW_ERROR_NOT_INITIALIZED       -2
#define HW_ERROR_INVALID_ARG           -3
#define HW_ERROR_NOT_FOUND             -4
#define HW_ERROR_NOT_SUPPORTED         -5
#define HW_ERROR_DRIVER                -6
#define HW_ERROR_PERMISSION            -7
#define HW_ERROR_BUSY                  -8
#define HW_ERROR_TIMEOUT               -9
#define HW_ERROR_MEMORY                -10
#define HW_ERROR_IO                    -11
#define HW_ERROR_NETWORK               -12
#define HW_ERROR_INTERNAL              -99

#endif /* HARDWARE_MANAGER_H */ 