#ifndef _LOGIN_RECOVERY_H
#define _LOGIN_RECOVERY_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file login_recovery.h
 * @brief KALEM OS Hesap Kurtarma Sistemi
 * 
 * Bu modül, KALEM OS'ta kullanıcıların şifrelerini unuttukları durumlarda
 * hesaplarını kurtarmalarına olanak tanıyacak fonksiyonları sağlar.
 * E-posta tabanlı kurtarma kodu ve şifre sıfırlama işlemlerini içerir.
 */

/**
 * Kurtarma kodunun uzunluğu
 */
#define RECOVERY_CODE_LENGTH 8

/**
 * Kurtarma kodunun geçerlilik süresi (saniye)
 */
#define RECOVERY_CODE_VALIDITY 3600 // 1 saat

/**
 * Kurtarma kodu hata kodları
 */
typedef enum {
    RECOVERY_ERROR_NONE = 0,           // Hata yok
    RECOVERY_ERROR_INIT = -1,          // Başlatma hatası
    RECOVERY_ERROR_USER_NOT_FOUND = -2, // Kullanıcı bulunamadı
    RECOVERY_ERROR_NO_EMAIL = -3,       // E-posta adresi bulunamadı
    RECOVERY_ERROR_EMAIL_MISMATCH = -4, // E-posta adresi eşleşmiyor
    RECOVERY_ERROR_SEND_FAILED = -5,    // E-posta gönderimi başarısız
    RECOVERY_ERROR_CODE_INVALID = -6,   // Geçersiz kurtarma kodu
    RECOVERY_ERROR_CODE_EXPIRED = -7,   // Kurtarma kodunun süresi dolmuş
    RECOVERY_ERROR_UNKNOWN = -8         // Bilinmeyen hata
} recovery_error_t;

/**
 * Kurtarma kodu bilgisi
 */
typedef struct {
    char username[32];           // Kullanıcı adı
    char email[128];             // E-posta adresi
    char code[RECOVERY_CODE_LENGTH + 1]; // Kurtarma kodu
    uint64_t timestamp;          // Oluşturma zamanı
    uint8_t used;                // Kullanıldı mı?
} recovery_code_info_t;

/**
 * @brief Kurtarma sistemini başlatır
 * 
 * @return Hata kodu (RECOVERY_ERROR_NONE başarı)
 */
int recovery_init();

/**
 * Kurtarma sistemini temizler
 * 
 * @return Hata kodu (RECOVERY_ERROR_NONE başarı)
 */
int recovery_cleanup();

/**
 * @brief Kullanıcının kurtarma e-postasını alır
 * 
 * @param username Kullanıcı adı
 * @param email_out E-posta çıktısı
 * @param email_size E-posta arabelleği boyutu
 * @return Hata kodu (RECOVERY_ERROR_NONE başarı)
 */
int recovery_get_user_email(const char* username, char* email_out, size_t email_size);

/**
 * @brief Kullanıcının e-posta adresini ayarlar
 * 
 * @param username Kullanıcı adı
 * @param email E-posta adresi
 * @return Hata kodu (RECOVERY_ERROR_NONE başarı)
 */
int recovery_set_user_email(const char* username, const char* email);

/**
 * @brief Kurtarma kodu gönderir
 * 
 * @param username Kullanıcı adı
 * @param email E-posta adresi
 * @return Hata kodu (RECOVERY_ERROR_NONE başarı)
 */
int recovery_send_code(const char* username, const char* email);

/**
 * @brief Kurtarma kodunu doğrular
 * 
 * @param username Kullanıcı adı
 * @param code Kurtarma kodu
 * @return Hata kodu (RECOVERY_ERROR_NONE başarı)
 */
int recovery_verify_code(const char* username, const char* code);

/**
 * @brief Kurtarma kodu ile parolayı sıfırlar
 * 
 * @param username Kullanıcı adı
 * @param code Kurtarma kodu
 * @param new_password Yeni parola
 * @return Hata kodu (RECOVERY_ERROR_NONE başarı)
 */
int recovery_reset_password(const char* username, const char* code, const char* new_password);

/**
 * @brief Yeni kullanıcı için kurtarma e-postası ayarlar
 * 
 * @param username Kullanıcı adı
 * @param email E-posta adresi
 * @return Hata kodu (RECOVERY_ERROR_NONE başarı)
 */
int recovery_setup_for_new_user(const char* username, const char* email);

/**
 * @brief Kullanıcının e-posta adresi var mı kontrol eder
 * 
 * @param username Kullanıcı adı
 * @param has_email E-posta adresi var mı? (çıkış parametresi)
 * @return Hata kodu (RECOVERY_ERROR_NONE başarı)
 */
int recovery_has_email(const char* username, uint8_t* has_email);

#endif /* _LOGIN_RECOVERY_H */ 