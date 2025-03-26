#include "../include/user_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// Hata kodları
#define RECOVERY_ERROR_NONE              0
#define RECOVERY_ERROR_INIT_FAILED      -1
#define RECOVERY_ERROR_INVALID_ARG      -2
#define RECOVERY_ERROR_USER_NOT_FOUND   -3
#define RECOVERY_ERROR_NO_EMAIL         -4
#define RECOVERY_ERROR_EMAIL_SEND       -5
#define RECOVERY_ERROR_CODE_INVALID     -6
#define RECOVERY_ERROR_CODE_EXPIRED     -7
#define RECOVERY_ERROR_FILE_IO          -8
#define RECOVERY_ERROR_INTERNAL         -99

// Kurtarma dosyası ve süresi
#define RECOVERY_CODES_PATH     "/etc/kalem/recovery_codes.dat"
#define RECOVERY_CODE_EXPIRY    3600    // 1 saat (saniye)
#define RECOVERY_CODE_LENGTH    8       // 8 karakter
#define MAX_RECOVERY_CODES      32      // Maksimum 32 kod

// Şifremi unuttum e-posta şablonu
const char* EMAIL_TEMPLATE = 
    "Konu: KALEM OS - Hesap Kurtarma Kodu\n"
    "Kimden: noreply@kalemos.org\n"
    "Kime: %s\n"
    "\n"
    "Merhaba %s,\n"
    "\n"
    "KALEM OS hesabınız için bir şifre sıfırlama talebinde bulundunuz.\n"
    "Hesabınıza erişmek için aşağıdaki kurtarma kodunu kullanabilirsiniz:\n"
    "\n"
    "Kurtarma Kodu: %s\n"
    "\n"
    "Bu kod %d dakika içinde geçerliliğini yitirecektir.\n"
    "Eğer bu talebi siz yapmadıysanız, lütfen bu e-postayı dikkate almayın.\n"
    "\n"
    "Saygılarımızla,\n"
    "KALEM OS Güvenlik Ekibi\n";

// Kurtarma kodu yapısı
typedef struct {
    uint32_t user_id;           // Kullanıcı ID
    char username[32];          // Kullanıcı adı
    char email[128];            // E-posta adresi
    char code[16];              // Kurtarma kodu
    uint32_t creation_time;     // Oluşturma zamanı
    uint32_t expiry_time;       // Son geçerlilik zamanı
    uint8_t used;               // Kullanıldı mı?
} recovery_code_t;

// Kurtarma kodları
static recovery_code_t recovery_codes[MAX_RECOVERY_CODES];
static uint32_t recovery_code_count = 0;

// E-posta veritabanı
typedef struct {
    char username[32];
    char email[128];
} email_entry_t;

static email_entry_t* email_db = NULL;
static int email_count = 0;

// Başlatma durumu
static int is_initialized = 0;

// İleri bildirimler
static int load_recovery_codes();
static int save_recovery_codes();
static void generate_recovery_code(char* code, int length);
static int send_recovery_email(const char* email, const char* username, const char* code, int expiry_minutes);
static int find_recovery_code(const char* code);
static int cleanup_expired_codes();

/**
 * Kurtarma kodları veritabanını yükler
 */
static int load_recovery_codes() {
    FILE* file = fopen(RECOVERY_CODES_PATH, "rb");
    if (!file) {
        // Dosya yoksa, yeni oluştur
        return 0;
    }
    
    // Dosyayı oku
    fread(&recovery_code_count, sizeof(int), 1, file);
    
    // Geçerlilik kontrolü
    if (recovery_code_count < 0 || recovery_code_count > MAX_RECOVERY_CODES) {
        recovery_code_count = 0;
        fclose(file);
        return -1;
    }
    
    // Kodları oku
    if (recovery_code_count > 0) {
        fread(recovery_codes, sizeof(recovery_code_t), recovery_code_count, file);
    }
    
    fclose(file);
    return 0;
}

/**
 * Kurtarma kodları veritabanını kaydeder
 */
static int save_recovery_codes() {
    // Dizin kontrolü
    struct stat st = {0};
    if (stat("/etc/kalem", &st) == -1) {
        mkdir("/etc/kalem", 0755);
    }
    
    FILE* file = fopen(RECOVERY_CODES_PATH, "wb");
    if (!file) {
        log_error("Kurtarma kodları veritabanı kaydedilemedi: %s", RECOVERY_CODES_PATH);
        return -1;
    }
    
    // Dosyayı yaz
    fwrite(&recovery_code_count, sizeof(int), 1, file);
    
    // Kodları yaz
    if (recovery_code_count > 0) {
        fwrite(recovery_codes, sizeof(recovery_code_t), recovery_code_count, file);
    }
    
    fclose(file);
    return 0;
}

/**
 * E-posta veritabanını yükler
 */
static int load_email_db() {
    FILE* file = fopen("/etc/kalem/recovery_emails.dat", "rb");
    if (!file) {
        // Dosya yoksa, yeni oluştur
        email_count = 0;
        email_db = NULL;
        return 0;
    }
    
    // Dosyayı oku
    fread(&email_count, sizeof(int), 1, file);
    
    // Geçerlilik kontrolü
    if (email_count <= 0) {
        email_count = 0;
        email_db = NULL;
        fclose(file);
        return 0;
    }
    
    // E-postaları oku
    email_db = (email_entry_t*)malloc(sizeof(email_entry_t) * email_count);
    if (!email_db) {
        email_count = 0;
        fclose(file);
        return -1;
    }
    
    fread(email_db, sizeof(email_entry_t), email_count, file);
    
    fclose(file);
    return 0;
}

/**
 * E-posta veritabanını kaydeder
 */
static int save_email_db() {
    // Dizin kontrolü
    struct stat st = {0};
    if (stat("/etc/kalem", &st) == -1) {
        mkdir("/etc/kalem", 0755);
    }
    
    FILE* file = fopen("/etc/kalem/recovery_emails.dat", "wb");
    if (!file) {
        log_error("E-posta veritabanı kaydedilemedi: %s", "/etc/kalem/recovery_emails.dat");
        return -1;
    }
    
    // Dosyayı yaz
    fwrite(&email_count, sizeof(int), 1, file);
    
    // E-postaları yaz
    if (email_count > 0 && email_db) {
        fwrite(email_db, sizeof(email_entry_t), email_count, file);
    }
    
    fclose(file);
    return 0;
}

/**
 * Rastgele kurtarma kodu oluşturur
 */
static void generate_recovery_code(char* code, int length) {
    static const char charset[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    
    // Rastgele sayı üretici için tohum
    srand(time(NULL) + getpid());
    
    // Kodu oluştur
    for (int i = 0; i < length; i++) {
        int index = rand() % (sizeof(charset) - 1);
        code[i] = charset[index];
    }
    
    code[length] = '\0';
}

/**
 * Kurtarma sistemini başlatır
 */
int recovery_init() {
    if (is_initialized) {
        return RECOVERY_ERROR_NONE;
    }
    
    // Kurtarma kodları veritabanını yükle
    if (load_recovery_codes() != 0) {
        log_error("Kurtarma kodları veritabanı yüklenemedi");
        return RECOVERY_ERROR_INIT;
    }
    
    // E-posta veritabanını yükle
    if (load_email_db() != 0) {
        log_error("E-posta veritabanı yüklenemedi");
        return RECOVERY_ERROR_INIT;
    }
    
    // Süresi dolmuş kodları temizle
    time_t now = time(NULL);
    int i = 0;
    while (i < recovery_code_count) {
        if (now - recovery_codes[i].creation_time > RECOVERY_CODE_EXPIRY || recovery_codes[i].used) {
            // Kodu sil
            if (i < recovery_code_count - 1) {
                memcpy(&recovery_codes[i], &recovery_codes[i + 1], 
                       sizeof(recovery_code_t) * (recovery_code_count - i - 1));
            }
            recovery_code_count--;
        } else {
            i++;
        }
    }
    
    // Değişiklikleri kaydet
    save_recovery_codes();
    
    is_initialized = 1;
    return RECOVERY_ERROR_NONE;
}

/**
 * Kurtarma sistemini temizler
 */
int recovery_cleanup() {
    if (!is_initialized) {
        return RECOVERY_ERROR_NONE;
    }
    
    // Kurtarma kodları veritabanını kaydet
    save_recovery_codes();
    
    // E-posta veritabanını kaydet
    save_email_db();
    
    // E-posta veritabanını temizle
    if (email_db) {
        free(email_db);
        email_db = NULL;
    }
    email_count = 0;
    
    is_initialized = 0;
    return RECOVERY_ERROR_NONE;
}

/**
 * Kullanıcının e-posta adresini alır
 */
int recovery_get_user_email(const char* username, char* email_out, size_t email_size) {
    if (!is_initialized) {
        if (recovery_init() != RECOVERY_ERROR_NONE) {
            return RECOVERY_ERROR_INIT;
        }
    }
    
    if (!username || !email_out || email_size == 0) {
        return RECOVERY_ERROR_UNKNOWN;
    }
    
    // E-posta veritabanında ara
    for (int i = 0; i < email_count; i++) {
        if (strcmp(email_db[i].username, username) == 0) {
            strncpy(email_out, email_db[i].email, email_size - 1);
            email_out[email_size - 1] = '\0';
            return RECOVERY_ERROR_NONE;
        }
    }
    
    return RECOVERY_ERROR_NO_EMAIL;
}

/**
 * Kullanıcının e-posta adresini ayarlar
 */
int recovery_set_user_email(const char* username, const char* email) {
    if (!is_initialized) {
        if (recovery_init() != RECOVERY_ERROR_NONE) {
            return RECOVERY_ERROR_INIT;
        }
    }
    
    if (!username || !email) {
        return RECOVERY_ERROR_UNKNOWN;
    }
    
    // Kullanıcı var mı kontrol et
    user_info_t user_info;
    if (user_get_info_by_name(username, &user_info) != 0) {
        return RECOVERY_ERROR_USER_NOT_FOUND;
    }
    
    // Mevcut bir e-posta var mı kontrol et
    for (int i = 0; i < email_count; i++) {
        if (strcmp(email_db[i].username, username) == 0) {
            // Güncelle
            strncpy(email_db[i].email, email, sizeof(email_db[i].email) - 1);
            email_db[i].email[sizeof(email_db[i].email) - 1] = '\0';
            save_email_db();
            return RECOVERY_ERROR_NONE;
        }
    }
    
    // Yeni e-posta ekle
    email_entry_t* new_db = (email_entry_t*)realloc(email_db, sizeof(email_entry_t) * (email_count + 1));
    if (!new_db) {
        return RECOVERY_ERROR_UNKNOWN;
    }
    
    email_db = new_db;
    strncpy(email_db[email_count].username, username, sizeof(email_db[email_count].username) - 1);
    email_db[email_count].username[sizeof(email_db[email_count].username) - 1] = '\0';
    
    strncpy(email_db[email_count].email, email, sizeof(email_db[email_count].email) - 1);
    email_db[email_count].email[sizeof(email_db[email_count].email) - 1] = '\0';
    
    email_count++;
    
    save_email_db();
    return RECOVERY_ERROR_NONE;
}

/**
 * Kullanıcının e-posta adresi var mı kontrol eder
 */
int recovery_has_email(const char* username, uint8_t* has_email) {
    if (!is_initialized) {
        if (recovery_init() != RECOVERY_ERROR_NONE) {
            return RECOVERY_ERROR_INIT;
        }
    }
    
    if (!username || !has_email) {
        return RECOVERY_ERROR_UNKNOWN;
    }
    
    // E-posta veritabanında ara
    for (int i = 0; i < email_count; i++) {
        if (strcmp(email_db[i].username, username) == 0) {
            *has_email = 1;
            return RECOVERY_ERROR_NONE;
        }
    }
    
    *has_email = 0;
    return RECOVERY_ERROR_NONE;
}

/**
 * Kurtarma kodu gönderir
 */
int recovery_send_code(const char* username, const char* email) {
    if (!is_initialized) {
        if (recovery_init() != RECOVERY_ERROR_NONE) {
            return RECOVERY_ERROR_INIT;
        }
    }
    
    if (!username || !email) {
        return RECOVERY_ERROR_UNKNOWN;
    }
    
    // Kullanıcı var mı kontrol et
    user_info_t user_info;
    if (user_get_info_by_name(username, &user_info) != 0) {
        return RECOVERY_ERROR_USER_NOT_FOUND;
    }
    
    // Kullanıcının kayıtlı e-postası ile eşleşiyor mu kontrol et
    char stored_email[128] = {0};
    if (recovery_get_user_email(username, stored_email, sizeof(stored_email)) != RECOVERY_ERROR_NONE) {
        return RECOVERY_ERROR_NO_EMAIL;
    }
    
    if (strcmp(stored_email, email) != 0) {
        return RECOVERY_ERROR_EMAIL_MISMATCH;
    }
    
    // Yeni kurtarma kodu oluştur
    recovery_code_t new_code = {0};
    strncpy(new_code.username, username, sizeof(new_code.username) - 1);
    strncpy(new_code.email, email, sizeof(new_code.email) - 1);
    generate_recovery_code(new_code.code, RECOVERY_CODE_LENGTH);
    new_code.creation_time = time(NULL);
    new_code.expiry_time = new_code.creation_time + RECOVERY_CODE_EXPIRY;
    new_code.used = 0;
    
    // Kurtarma kodunu ekle
    if (recovery_code_count >= MAX_RECOVERY_CODES) {
        // En eski kodu kaldır
        for (int i = 0; i < recovery_code_count - 1; i++) {
            recovery_codes[i] = recovery_codes[i + 1];
        }
        recovery_code_count--;
    }
    
    recovery_codes[recovery_code_count++] = new_code;
    save_recovery_codes();
    
    // E-posta gönder
    char subject[64] = {0};
    snprintf(subject, sizeof(subject), "KALEM OS Hesap Kurtarma Kodu");
    
    char body[512] = {0};
    snprintf(body, sizeof(body), 
            "Sayın %s,\n\n"
            "KALEM OS hesabınız için kurtarma kodu talebinde bulundunuz.\n"
            "Kurtarma kodunuz: %s\n\n"
            "Bu kod %d dakika boyunca geçerlidir.\n\n"
            "Eğer bu talebi siz yapmadıysanız, lütfen bu e-postayı dikkate almayın.\n\n"
            "Saygılarımızla,\n"
            "KALEM OS Ekibi",
            username, new_code.code, RECOVERY_CODE_EXPIRY / 60);
    
    if (send_recovery_email(email, username, new_code.code, RECOVERY_CODE_EXPIRY / 60) != RECOVERY_ERROR_NONE) {
        return RECOVERY_ERROR_SEND_FAILED;
    }
    
    return RECOVERY_ERROR_NONE;
}

/**
 * Kurtarma kodunu doğrular
 */
int recovery_verify_code(const char* username, const char* code) {
    if (!is_initialized) {
        if (recovery_init() != RECOVERY_ERROR_NONE) {
            return RECOVERY_ERROR_INIT;
        }
    }
    
    if (!username || !code) {
        return RECOVERY_ERROR_UNKNOWN;
    }
    
    // Kullanıcı var mı kontrol et
    user_info_t user_info;
    if (user_get_info_by_name(username, &user_info) != 0) {
        return RECOVERY_ERROR_USER_NOT_FOUND;
    }
    
    // Kurtarma kodunu ara
    time_t now = time(NULL);
    for (int i = 0; i < recovery_code_count; i++) {
        if (strcmp(recovery_codes[i].username, username) == 0 && 
            strcmp(recovery_codes[i].code, code) == 0 && 
            !recovery_codes[i].used) {
            
            // Kodun süresi dolmuş mu?
            if (now > recovery_codes[i].expiry_time) {
                return RECOVERY_ERROR_CODE_EXPIRED;
            }
            
            return RECOVERY_ERROR_NONE;
        }
    }
    
    return RECOVERY_ERROR_CODE_INVALID;
}

/**
 * Kurtarma kodu ile parolayı sıfırlar
 */
int recovery_reset_password(const char* username, const char* code, const char* new_password) {
    if (!is_initialized) {
        if (recovery_init() != RECOVERY_ERROR_NONE) {
            return RECOVERY_ERROR_INIT;
        }
    }
    
    if (!username || !code || !new_password) {
        return RECOVERY_ERROR_UNKNOWN;
    }
    
    // Kodu doğrula
    int verify_result = recovery_verify_code(username, code);
    if (verify_result != RECOVERY_ERROR_NONE) {
        return verify_result;
    }
    
    // Parolayı sıfırla
    if (user_reset_password(username, new_password) != 0) {
        return RECOVERY_ERROR_INTERNAL;
    }
    
    // Kodu kullanıldı olarak işaretle
    for (int i = 0; i < recovery_code_count; i++) {
        if (strcmp(recovery_codes[i].username, username) == 0 && 
            strcmp(recovery_codes[i].code, code) == 0 && 
            !recovery_codes[i].used) {
            
            recovery_codes[i].used = 1;
            save_recovery_codes();
            break;
        }
    }
    
    return RECOVERY_ERROR_NONE;
}

/**
 * Yeni kullanıcı için kurtarma e-postası ayarlar
 */
int recovery_setup_for_new_user(const char* username, const char* email) {
    if (!is_initialized) {
        if (recovery_init() != RECOVERY_ERROR_NONE) {
            return RECOVERY_ERROR_INIT;
        }
    }
    
    if (!username) {
        return RECOVERY_ERROR_UNKNOWN;
    }
    
    // E-posta adresi yoksa, işlem yapma
    if (!email || strlen(email) == 0) {
        return RECOVERY_ERROR_NONE;
    }
    
    // Kullanıcı var mı kontrol et
    user_info_t user_info;
    if (user_get_info_by_name(username, &user_info) != 0) {
        return RECOVERY_ERROR_USER_NOT_FOUND;
    }
    
    // E-posta adresini ayarla
    return recovery_set_user_email(username, email);
} 