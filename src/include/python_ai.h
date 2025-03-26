#ifndef PYTHON_AI_H
#define PYTHON_AI_H

#include <stdint.h>

/**
 * @file python_ai.h
 * @brief KALEM OS Python Yapay Zeka Desteği
 * 
 * Bu modül, KALEM OS'a Python tabanlı yapay zeka desteği ekler.
 * Kernel optimizasyonu, donanım yönetimi ve kullanıcı deneyimi
 * iyileştirmeleri için AI/ML işlevleri sağlar.
 */

/** Yapay zeka model tipleri */
typedef enum {
    AI_MODEL_NONE = 0,           // Model yok
    AI_MODEL_TEXT,               // Metin modeli
    AI_MODEL_IMAGE,              // Görüntü modeli
    AI_MODEL_AUDIO,              // Ses modeli
    AI_MODEL_HYBRID,             // Karma model
    AI_MODEL_OPTIMIZATION        // Optimizasyon modeli
} ai_model_type_t;

/** Yapay zeka optimizasyon hedefleri */
typedef enum {
    AI_OPTIMIZE_NONE = 0,        // Optimizasyon yok
    AI_OPTIMIZE_PERFORMANCE,     // Performans optimizasyonu
    AI_OPTIMIZE_POWER,           // Güç optimizasyonu
    AI_OPTIMIZE_MEMORY,          // Bellek optimizasyonu
    AI_OPTIMIZE_STORAGE,         // Depolama optimizasyonu
    AI_OPTIMIZE_BALANCE          // Dengeli optimizasyon
} ai_optimization_target_t;

/** Yapay zeka model yapılandırması */
typedef struct {
    char model_name[64];         // Model adı
    char model_path[256];        // Model dosya yolu
    float temperature;           // Sıcaklık (0.0 - 1.0)
    uint32_t max_tokens;         // Maksimum token sayısı
    uint8_t use_gpu;             // GPU kullanılsın mı?
    uint8_t quantized;           // Nicelleştirilmiş model mi?
    uint32_t max_memory_mb;      // Maksimum bellek kullanımı (MB)
    uint8_t thread_count;        // İş parçacığı sayısı
    uint8_t optimization_level;  // Optimizasyon seviyesi (0-5)
} ai_model_config_t;

/** Sistem kaynak kullanım bilgisi */
typedef struct {
    float cpu_usage;             // CPU kullanımı (%)
    uint32_t memory_used_mb;     // Kullanılan bellek (MB)
    uint32_t memory_free_mb;     // Boş bellek (MB)
    float disk_read_mbps;        // Disk okuma hızı (MB/s)
    float disk_write_mbps;       // Disk yazma hızı (MB/s)
    float network_rx_mbps;       // Ağ alım hızı (MB/s)
    float network_tx_mbps;       // Ağ gönderim hızı (MB/s)
    float battery_drain_rate;    // Batarya boşalma hızı (%/saat)
    uint8_t gpu_usage;           // GPU kullanımı (%)
} ai_system_metrics_t;

/** Yapay zeka optimizasyon önerileri */
typedef struct {
    uint8_t cpu_governor;        // CPU güç yönetimi modu
    uint8_t cpu_max_freq_pct;    // CPU maksimum frekans yüzdesi
    uint8_t enable_core_parking; // Çekirdek parklama etkin olsun mu?
    uint8_t available_core_pct;  // Kullanılabilir çekirdek yüzdesi
    uint8_t memory_compression;  // Bellek sıkıştırma seviyesi
    uint8_t disk_io_priority;    // Disk G/Ç önceliği
    uint8_t network_qos;         // Ağ QoS seviyesi
    uint8_t thermal_profile;     // Termal profil
    char process_limits[256];    // Süreç limitleri (JSON formatında)
} ai_optimization_recommendations_t;

/**
 * Yapay zeka modülünü başlatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_ai_init();

/**
 * Yapay zeka modülünü temizler
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_ai_cleanup();

/**
 * Yapay zeka modelini yükler
 * 
 * @param model_type Model tipi
 * @param config Model yapılandırması
 * @return int Model tanımlayıcısı veya <0: hata
 */
int python_ai_load_model(ai_model_type_t model_type, const ai_model_config_t* config);

/**
 * Yapay zeka modelini kaldırır
 * 
 * @param model_id Model tanımlayıcısı
 * @return int 0: başarılı, <0: hata
 */
int python_ai_unload_model(int model_id);

/**
 * Yapay zeka modeliyle metin üretir
 * 
 * @param model_id Model tanımlayıcısı
 * @param prompt İstek metni
 * @param result_out Sonuç metni (serbest bırakılmalıdır)
 * @return int 0: başarılı, <0: hata
 */
int python_ai_generate_text(int model_id, const char* prompt, char** result_out);

/**
 * Yapay zeka ile sistem metriklerini analiz eder
 * 
 * @param metrics Sistem metrikleri
 * @param recommendations_out Optimizasyon önerileri
 * @return int 0: başarılı, <0: hata
 */
int python_ai_analyze_system_metrics(const ai_system_metrics_t* metrics, 
                                     ai_optimization_recommendations_t* recommendations_out);

/**
 * Sistem kaynaklarını yapay zeka ile optimize eder
 * 
 * @param target Optimizasyon hedefi
 * @param strength Optimizasyon şiddeti (0-100)
 * @param result_out Optimizasyon sonucu (JSON formatında, serbest bırakılmalıdır)
 * @return int 0: başarılı, <0: hata
 */
int python_ai_optimize_resources(ai_optimization_target_t target, 
                                 uint8_t strength, char** result_out);

/**
 * Yapay zeka ile kod tamamlaması yapar
 * 
 * @param code_prefix Kod ön metni
 * @param result_out Tamamlama sonucu (serbest bırakılmalıdır)
 * @param max_suggestions Maksimum öneri sayısı
 * @return int 0: başarılı, <0: hata
 */
int python_ai_code_completion(const char* code_prefix, char** result_out, uint8_t max_suggestions);

/**
 * Yapay zeka ile kod açıklaması üretir
 * 
 * @param code Kod
 * @param result_out Açıklama (serbest bırakılmalıdır)
 * @return int 0: başarılı, <0: hata
 */
int python_ai_explain_code(const char* code, char** result_out);

/**
 * Yapay zeka ile kod analizi yapar
 * 
 * @param code Kod
 * @param result_out Analiz sonucu (JSON formatında, serbest bırakılmalıdır)
 * @return int 0: başarılı, <0: hata
 */
int python_ai_analyze_code(const char* code, char** result_out);

/**
 * Yapay zeka ile doğal dil komutunu yorumlar
 * 
 * @param nl_command Doğal dil komutu
 * @param result_out Yorum sonucu (JSON formatında, serbest bırakılmalıdır)
 * @return int 0: başarılı, <0: hata
 */
int python_ai_interpret_command(const char* nl_command, char** result_out);

/**
 * Yapay zeka ile doğal dil komutunu eyleme dönüştürür
 * 
 * @param nl_command Doğal dil komutu
 * @param action_out Eylem (JSON formatında, serbest bırakılmalıdır)
 * @return int 0: başarılı, <0: hata
 */
int python_ai_command_to_action(const char* nl_command, char** action_out);

/**
 * Yapay zeka modülleri listesini alır
 * 
 * @param models_out Modeller (JSON formatında, serbest bırakılmalıdır)
 * @return int 0: başarılı, <0: hata
 */
int python_ai_list_models(char** models_out);

/**
 * Yapay zeka model bilgisini alır
 * 
 * @param model_id Model tanımlayıcısı
 * @param info_out Model bilgisi (JSON formatında, serbest bırakılmalıdır)
 * @return int 0: başarılı, <0: hata
 */
int python_ai_get_model_info(int model_id, char** info_out);

#endif /* PYTHON_AI_H */ 