#ifndef PYTHON_MANAGER_H
#define PYTHON_MANAGER_H

#include <stdint.h>

/**
 * @file python_manager.h
 * @brief KALEM OS Python Yöneticisi
 * 
 * Bu modül, KALEM OS'a Python programlama dili desteği ekler.
 * Python yorumlayıcısını başlatma, Python betikleri çalıştırma
 * ve kernel API'sini Python'a sunma işlevlerini içerir.
 */

/** Python hata kodları */
typedef enum {
    PYTHON_ERROR_NONE = 0,        // Hata yok
    PYTHON_ERROR_INIT = -1,       // Başlatma hatası
    PYTHON_ERROR_SCRIPT = -2,     // Betik çalıştırma hatası
    PYTHON_ERROR_MODULE = -3,     // Modül yükleme hatası
    PYTHON_ERROR_MEMORY = -4,     // Bellek hatası
    PYTHON_ERROR_API = -5,        // API hatası
    PYTHON_ERROR_PERMISSION = -6, // İzin hatası
    PYTHON_ERROR_IO = -7,         // G/Ç hatası
    PYTHON_ERROR_UNKNOWN = -99    // Bilinmeyen hata
} python_error_t;

/** Python paket yöneticisi yapılandırması */
typedef struct {
    uint8_t auto_install_deps;    // Bağımlılıklar otomatik yüklensin mi?
    uint8_t use_pip_cache;        // pip önbelleği kullanılsın mı?
    uint8_t offline_mode;         // Çevrimdışı mod etkin mi?
    char custom_repo_url[256];    // Özel paket deposu URL'si
    char venv_path[256];          // Sanal ortam yolu
    char pip_cache_dir[256];      // pip önbellek dizini
} python_package_config_t;

/** Python ortam bilgisi */
typedef struct {
    char version[32];             // Python sürümü
    char path[256];               // Python yolu
    char home[256];               // Python ana dizini
    char exec[256];               // Python çalıştırılabilir dosya yolu
    char site_packages[256];      // Site paketleri dizini
    uint8_t is_cpython;           // CPython mı?
    uint8_t is_micropython;       // MicroPython mı?
    uint32_t memory_limit_kb;     // Bellek sınırı (KB)
} python_environment_t;

/** Python script kayıt bilgisi */
typedef struct python_script {
    char path[256];               // Betik yolu
    char name[64];                // Betik adı
    char description[256];        // Betik açıklaması
    uint8_t autorun;              // Otomatik çalıştırılsın mı?
    uint8_t system_script;        // Sistem betiği mi?
    uint32_t last_run_time;       // Son çalıştırma zamanı (epoch)
    uint8_t exit_code;            // Son çıkış kodu
    struct python_script* next;   // Sonraki betik
} python_script_t;

/**
 * Python yöneticisini başlatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_manager_init();

/**
 * Python yöneticisini temizler
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_manager_cleanup();

/**
 * Python betiği çalıştırır
 * 
 * @param script_path Betik dosya yolu
 * @return int 0: başarılı, <0: hata
 */
int python_run_script(const char* script_path);

/**
 * Python kodu çalıştırır
 * 
 * @param code Python kodu
 * @return int 0: başarılı, <0: hata
 */
int python_run_string(const char* code);

/**
 * Python kodu çalıştırır ve çıktıyı döndürür
 * 
 * @param code Python kodu
 * @param output_out Çıktı arabelleği (serbest bırakılmalıdır)
 * @return int 0: başarılı, <0: hata
 */
int python_run_string_with_output(const char* code, char** output_out);

/**
 * Son Python hatasını döndürür
 * 
 * @return const char* Hata mesajı
 */
const char* python_get_last_error();

/**
 * Python ortam bilgisini alır
 * 
 * @param env_out Ortam bilgisi
 * @return int 0: başarılı, <0: hata
 */
int python_get_environment(python_environment_t* env_out);

/**
 * Python paket yöneticisi yapılandırmasını alır
 * 
 * @param config_out Yapılandırma bilgisi
 * @return int 0: başarılı, <0: hata
 */
int python_get_package_config(python_package_config_t* config_out);

/**
 * Python paket yöneticisi yapılandırmasını ayarlar
 * 
 * @param config Yapılandırma bilgisi
 * @return int 0: başarılı, <0: hata
 */
int python_set_package_config(const python_package_config_t* config);

/**
 * Python paketi kurar
 * 
 * @param package_name Paket adı
 * @param version Paket sürümü (NULL: en son)
 * @return int 0: başarılı, <0: hata
 */
int python_install_package(const char* package_name, const char* version);

/**
 * Python paketini kaldırır
 * 
 * @param package_name Paket adı
 * @return int 0: başarılı, <0: hata
 */
int python_uninstall_package(const char* package_name);

/**
 * Python paket bilgisini alır
 * 
 * @param package_name Paket adı
 * @param info_out Paket bilgisi (JSON formatında, serbest bırakılmalıdır)
 * @return int 0: başarılı, <0: hata
 */
int python_get_package_info(const char* package_name, char** info_out);

/**
 * Python paketlerini listeler
 * 
 * @param list_out Paket listesi (JSON formatında, serbest bırakılmalıdır)
 * @return int 0: başarılı, <0: hata
 */
int python_list_packages(char** list_out);

/**
 * Python betik taramasını başlatır
 * 
 * @param directory Taranacak dizin
 * @return int Bulunan betik sayısı, <0: hata
 */
int python_scan_scripts(const char* directory);

/**
 * Python betiklerini listeler
 * 
 * @param system_only Sadece sistem betikleri
 * @return python_script_t* Betik listesinin başı (NULL: hata)
 */
python_script_t* python_list_scripts(uint8_t system_only);

/**
 * Python betiğini kayıt eder
 * 
 * @param script_path Betik yolu
 * @param description Betik açıklaması
 * @param autorun Otomatik çalıştırılsın mı?
 * @param system_script Sistem betiği mi?
 * @return int 0: başarılı, <0: hata
 */
int python_register_script(const char* script_path, const char* description,
                           uint8_t autorun, uint8_t system_script);

/**
 * Python betiğini kayıttan çıkarır
 * 
 * @param script_path Betik yolu
 * @return int 0: başarılı, <0: hata
 */
int python_unregister_script(const char* script_path);

/**
 * Python çıktı yakalama işlevini ayarlar
 * 
 * @param callback Çıktı yakalama işlevi
 */
void python_set_output_callback(void (*callback)(const char* output));

/**
 * Python çıktı renklerini ayarlar
 * 
 * @param stdout_color Standart çıktı rengi
 * @param stderr_color Hata çıktısı rengi
 * @param prompt_color Komut istemi rengi
 */
void python_set_output_colors(uint8_t stdout_color, uint8_t stderr_color, uint8_t prompt_color);

#endif /* PYTHON_MANAGER_H */ 