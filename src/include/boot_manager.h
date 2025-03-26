#ifndef BOOT_MANAGER_H
#define BOOT_MANAGER_H

#include <stdint.h>

/**
 * @file boot_manager.h
 * @brief KALEM OS Başlangıç ve Açılış Yöneticisi
 * 
 * Bu modül, KALEM OS'un başlangıç sürecini, açılış ekranını ve
 * oturum yönetimine geçiş işlemlerini kontrol eder.
 */

/** Açılış modu sabitleri */
typedef enum {
    BOOT_MODE_NORMAL = 0,      // Normal açılış modu
    BOOT_MODE_SAFE = 1,        // Güvenli mod
    BOOT_MODE_RECOVERY = 2,    // Kurtarma modu
    BOOT_MODE_DEBUG = 3        // Hata ayıklama modu
} boot_mode_t;

/** Açılış durumu sabitleri */
typedef enum {
    BOOT_STATE_INIT = 0,           // Başlangıç durumu
    BOOT_STATE_HARDWARE_DETECT,    // Donanım algılama
    BOOT_STATE_KERNEL_LOAD,        // Kernel yükleniyor
    BOOT_STATE_DRIVERS_INIT,       // Sürücüler başlatılıyor
    BOOT_STATE_SERVICES_START,     // Servisler başlatılıyor
    BOOT_STATE_USER_INIT,          // Kullanıcı ortamı hazırlanıyor
    BOOT_STATE_COMPLETE,           // Açılış tamamlandı
    BOOT_STATE_ERROR               // Hata durumu
} boot_state_t;

/** Açılış ekranı animasyon tipi */
typedef enum {
    BOOT_ANIM_NONE = 0,        // Animasyon yok
    BOOT_ANIM_SIMPLE,          // Basit animasyon
    BOOT_ANIM_PENCIL,          // Kalem animasyonu
    BOOT_ANIM_SPLASH,          // Splash animasyonu
    BOOT_ANIM_CUSTOM           // Özel animasyon
} boot_animation_t;

/** Açılış ekranı stilleri */
typedef enum {
    BOOT_STYLE_DARK = 0,       // Koyu tema
    BOOT_STYLE_LIGHT,          // Açık tema
    BOOT_STYLE_MODERN,         // Modern tema
    BOOT_STYLE_CLASSIC,        // Klasik tema
    BOOT_STYLE_CUSTOM          // Özel tema
} boot_style_t;

/** Açılış ekranı yapılandırması */
typedef struct {
    uint8_t show_logo;             // Logo gösterilsin mi?
    uint8_t show_progress;         // İlerleme çubuğu gösterilsin mi?
    uint8_t show_text;             // Metin gösterilsin mi?
    uint8_t show_animation;        // Animasyon gösterilsin mi?
    boot_animation_t animation;    // Animasyon tipi
    boot_style_t style;            // Ekran stili
    char logo_path[256];           // Logo dosya yolu
    char background_path[256];     // Arkaplan dosya yolu
    uint32_t timeout;              // Otomatik geçiş süresi (ms)
    uint8_t auto_login;            // Otomatik oturum açılsın mı?
    char auto_login_user[32];      // Otomatik oturum kullanıcısı
    uint8_t safe_mode_enabled;     // Güvenli mod etkin mi?
    uint8_t show_boot_messages;    // Açılış mesajları gösterilsin mi?
} boot_config_t;

/** Açılış ilerleme bilgisi */
typedef struct {
    boot_state_t state;            // Güncel açılış durumu
    uint8_t progress;              // İlerleme yüzdesi (0-100)
    char status_message[128];      // Durum mesajı
    uint32_t current_step;         // Geçerli adım
    uint32_t total_steps;          // Toplam adım sayısı
    uint8_t error_occurred;        // Hata oluştu mu?
    char error_message[128];       // Hata mesajı (varsa)
} boot_progress_t;

/**
 * @brief Açılış yöneticisini başlatır
 * 
 * @param mode Açılış modu
 * @return int 0: başarılı, <0: hata
 */
int boot_manager_init(boot_mode_t mode);

/**
 * @brief Açılış yöneticisini kapatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int boot_manager_cleanup();

/**
 * @brief Açılış ekranını başlatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int boot_screen_start();

/**
 * @brief Açılış ekranını kapatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int boot_screen_stop();

/**
 * @brief Açılış ilerlemesini günceller
 * 
 * @param state Açılış durumu
 * @param progress İlerleme yüzdesi (0-100)
 * @param message Durum mesajı
 * @return int 0: başarılı, <0: hata
 */
int boot_update_progress(boot_state_t state, uint8_t progress, const char* message);

/**
 * @brief Açılış hatasını raporlar
 * 
 * @param error_message Hata mesajı
 * @return int 0: başarılı, <0: hata
 */
int boot_report_error(const char* error_message);

/**
 * @brief Açılış ilerlemesini alır
 * 
 * @param progress İlerleme bilgisi
 * @return int 0: başarılı, <0: hata
 */
int boot_get_progress(boot_progress_t* progress);

/**
 * @brief Açılış yapılandırmasını alır
 * 
 * @param config Yapılandırma bilgisi
 * @return int 0: başarılı, <0: hata
 */
int boot_get_config(boot_config_t* config);

/**
 * @brief Açılış yapılandırmasını ayarlar
 * 
 * @param config Yapılandırma bilgisi
 * @return int 0: başarılı, <0: hata
 */
int boot_set_config(const boot_config_t* config);

/**
 * @brief Oturum açma ekranına geçiş yapar
 * 
 * @return int 0: başarılı, <0: hata
 */
int boot_show_login_screen();

/**
 * @brief Otomatik oturum açma ayarlarını kontrol eder ve uygular
 * 
 * @return int 0: başarılı (oturum açıldı), <0: hata (oturum açılamadı)
 */
int boot_auto_login();

/**
 * @brief Sistem açılış zamanını milisaniye cinsinden döndürür
 * 
 * @return uint64_t Açılış zamanı (ms)
 */
uint64_t boot_get_uptime_ms();

#endif /* BOOT_MANAGER_H */ 