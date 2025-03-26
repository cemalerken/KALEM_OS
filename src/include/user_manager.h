#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <stdint.h>

/**
 * @file user_manager.h
 * @brief KALEM OS kullanıcı yönetimi modülü
 * 
 * Bu modül, KALEM OS'ta kullanıcı hesaplarının yönetimi için gerekli
 * fonksiyonları sağlar. Kullanıcı ekleme, silme, düzenleme, oturum açma/kapama 
 * ve yetkilendirme işlemleri burada tanımlanır.
 */

/** Kullanıcı tipi sabitleri */
typedef enum {
    USER_TYPE_STANDARD = 0,    // Standart kullanıcı
    USER_TYPE_ADMIN = 1,       // Yönetici kullanıcı
    USER_TYPE_GUEST = 2,       // Misafir kullanıcı
    USER_TYPE_SYSTEM = 3       // Sistem kullanıcısı
} user_type_t;

/** Kullanıcı yetki bayrakları */
typedef enum {
    USER_PERM_NONE          = 0,
    USER_PERM_FILES_READ    = (1 << 0),  // Dosya okuma
    USER_PERM_FILES_WRITE   = (1 << 1),  // Dosya yazma
    USER_PERM_NETWORK       = (1 << 2),  // Ağ erişimi
    USER_PERM_INSTALL_APP   = (1 << 3),  // Uygulama kurma
    USER_PERM_SYSTEM_CONF   = (1 << 4),  // Sistem ayarlarını değiştirme
    USER_PERM_USER_ADMIN    = (1 << 5),  // Kullanıcı yönetimi
    USER_PERM_ROOT          = (1 << 6)   // Tam erişim (root)
} user_permission_t;

/** Oturum durumu */
typedef enum {
    SESSION_STATE_NONE,       // Oturum yok
    SESSION_STATE_ACTIVE,     // Aktif oturum
    SESSION_STATE_LOCKED,     // Kilitli oturum
    SESSION_STATE_SUSPENDED   // Askıya alınmış oturum
} session_state_t;

/** Kullanıcı bilgisi yapısı */
typedef struct {
    uint32_t user_id;          // Kullanıcı ID
    char username[32];         // Kullanıcı adı
    char full_name[64];        // Tam ad
    char password_hash[64];    // Parola hash'i
    char salt[16];             // Parola tuzu
    user_type_t type;          // Kullanıcı tipi
    uint32_t permissions;      // Yetki bayrakları
    char home_dir[256];        // Ev dizini
    char shell[128];           // Kabuk yolu
    char icon_path[256];       // Profil resmi yolu
    uint32_t created_time;     // Oluşturulma zamanı
    uint32_t last_login_time;  // Son giriş zamanı
    uint8_t account_disabled;  // Hesap devre dışı mı?
    uint8_t password_expires;  // Parola süresi dolacak mı?
    uint32_t password_expiry;  // Parola son geçerlilik tarihi
} user_info_t;

/** Aktif oturum bilgisi */
typedef struct {
    uint32_t session_id;       // Oturum ID
    uint32_t user_id;          // Kullanıcı ID
    char username[32];         // Kullanıcı adı
    session_state_t state;     // Oturum durumu
    uint32_t login_time;       // Giriş zamanı
    uint32_t last_active_time; // Son aktif olma zamanı
    char terminal[64];         // Terminal adı
    char display[16];          // Ekran adı
    char remote_host[128];     // Uzak bağlantı adresi (varsa)
} session_info_t;

/** Sistem kullanıcı yapılandırması */
typedef struct {
    char hostname[64];         // Bilgisayar adı
    char domain[128];          // Etki alanı (varsa)
    uint8_t auto_login;        // Otomatik giriş aktif mi?
    char auto_login_user[32];  // Otomatik giriş kullanıcısı
    uint8_t guest_enabled;     // Misafir hesabı aktif mi?
    uint8_t show_users_list;   // Kullanıcı listesi gösterilsin mi?
    uint32_t session_timeout;  // Oturum zaman aşımı (saniye)
    uint8_t user_switching;    // Kullanıcı değiştirme aktif mi?
} user_config_t;

/**
 * @brief Kullanıcı yönetim sistemini başlatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int user_manager_init();

/**
 * @brief Kullanıcı yönetim sistemini kapatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int user_manager_cleanup();

/**
 * @brief Yeni kullanıcı oluşturur
 *
 * @param username Kullanıcı adı
 * @param password Parola (düz metin)
 * @param type Kullanıcı tipi
 * @param full_name Tam adı (isteğe bağlı)
 * @param user_id_out Oluşturulan kullanıcı ID'si (isteğe bağlı)
 * @return int 0: başarılı, <0: hata
 */
int user_create(const char* username, const char* password, user_type_t type, 
                const char* full_name, uint32_t* user_id_out);

/**
 * @brief Kullanıcıyı siler
 *
 * @param user_id Kullanıcı ID
 * @param delete_home Ev dizini de silinsin mi?
 * @return int 0: başarılı, <0: hata
 */
int user_delete(uint32_t user_id, uint8_t delete_home);

/**
 * @brief Kullanıcı bilgilerini alır
 *
 * @param user_id Kullanıcı ID
 * @param info Kullanıcı bilgisi çıktısı
 * @return int 0: başarılı, <0: hata
 */
int user_get_info(uint32_t user_id, user_info_t* info);

/**
 * @brief Kullanıcı bilgilerini kullanıcı adına göre alır
 *
 * @param username Kullanıcı adı
 * @param info Kullanıcı bilgisi çıktısı
 * @return int 0: başarılı, <0: hata
 */
int user_get_info_by_name(const char* username, user_info_t* info);

/**
 * @brief Kullanıcı bilgilerini günceller
 *
 * @param user_id Kullanıcı ID
 * @param info Güncellenecek bilgiler
 * @return int 0: başarılı, <0: hata
 */
int user_update_info(uint32_t user_id, const user_info_t* info);

/**
 * @brief Kullanıcı parolasını değiştirir
 *
 * @param user_id Kullanıcı ID
 * @param old_password Eski parola (doğrulama için)
 * @param new_password Yeni parola
 * @return int 0: başarılı, <0: hata
 */
int user_change_password(uint32_t user_id, const char* old_password, const char* new_password);

/**
 * @brief Yönetici tarafından kullanıcı parolasını sıfırlar
 *
 * @param user_id Kullanıcı ID
 * @param new_password Yeni parola
 * @return int 0: başarılı, <0: hata
 */
int user_reset_password(uint32_t user_id, const char* new_password);

/**
 * @brief Kullanıcı hesabını etkinleştirir veya devre dışı bırakır
 *
 * @param user_id Kullanıcı ID
 * @param enabled 0: devre dışı, 1: etkin
 * @return int 0: başarılı, <0: hata
 */
int user_set_enabled(uint32_t user_id, uint8_t enabled);

/**
 * @brief Kullanıcı tipini değiştirir
 *
 * @param user_id Kullanıcı ID
 * @param type Yeni kullanıcı tipi
 * @return int 0: başarılı, <0: hata
 */
int user_set_type(uint32_t user_id, user_type_t type);

/**
 * @brief Kullanıcı yetkilerini günceller
 *
 * @param user_id Kullanıcı ID
 * @param permissions Yeni yetki bayrakları
 * @return int 0: başarılı, <0: hata
 */
int user_set_permissions(uint32_t user_id, uint32_t permissions);

/**
 * @brief Tüm kullanıcıları listeler
 *
 * @param users Kullanıcı bilgileri dizisi
 * @param max_count En fazla kaç kullanıcı döndürüleceği
 * @param count_out Bulunan kullanıcı sayısı
 * @return int 0: başarılı, <0: hata
 */
int user_list_all(user_info_t* users, uint32_t max_count, uint32_t* count_out);

/**
 * @brief Kullanıcı sayısını döndürür
 *
 * @param count_out Kullanıcı sayısı çıktısı
 * @return int 0: başarılı, <0: hata
 */
int user_get_count(uint32_t* count_out);

/**
 * @brief Geçerli oturum açmış kullanıcı ID'sini döndürür
 *
 * @param user_id_out Kullanıcı ID çıktısı
 * @return int 0: başarılı, <0: hata
 */
int user_get_current(uint32_t* user_id_out);

/**
 * @brief Kullanıcı oturumu açar
 *
 * @param username Kullanıcı adı
 * @param password Parola
 * @param session_id_out Oluşturulan oturum ID'si (isteğe bağlı)
 * @return int 0: başarılı, <0: hata
 */
int session_login(const char* username, const char* password, uint32_t* session_id_out);

/**
 * @brief Kullanıcı oturumunu kapatır
 *
 * @param session_id Oturum ID (0: geçerli oturum)
 * @return int 0: başarılı, <0: hata
 */
int session_logout(uint32_t session_id);

/**
 * @brief Kullanıcı oturumunu kilitler
 *
 * @param session_id Oturum ID (0: geçerli oturum)
 * @return int 0: başarılı, <0: hata
 */
int session_lock(uint32_t session_id);

/**
 * @brief Kilitli oturumun kilidini açar
 *
 * @param session_id Oturum ID (0: geçerli oturum)
 * @param password Parola
 * @return int 0: başarılı, <0: hata
 */
int session_unlock(uint32_t session_id, const char* password);

/**
 * @brief Geçerli oturum bilgilerini alır
 *
 * @param session_id Oturum ID (0: geçerli oturum)
 * @param info Oturum bilgisi çıktısı
 * @return int 0: başarılı, <0: hata
 */
int session_get_info(uint32_t session_id, session_info_t* info);

/**
 * @brief Tüm aktif oturumları listeler
 *
 * @param sessions Oturum bilgileri dizisi
 * @param max_count En fazla kaç oturum döndürüleceği
 * @param count_out Bulunan oturum sayısı
 * @return int 0: başarılı, <0: hata
 */
int session_list_all(session_info_t* sessions, uint32_t max_count, uint32_t* count_out);

/**
 * @brief Kullanıcı oturumlarını sonlandırır
 *
 * @param user_id Kullanıcı ID
 * @return int 0: başarılı, <0: hata
 */
int session_terminate_user(uint32_t user_id);

/**
 * @brief Bilgisayar adını alır
 *
 * @param hostname Bilgisayar adı çıktısı
 * @param max_len Maksimum karakter sayısı
 * @return int 0: başarılı, <0: hata
 */
int system_get_hostname(char* hostname, uint32_t max_len);

/**
 * @brief Bilgisayar adını değiştirir
 *
 * @param hostname Yeni bilgisayar adı
 * @return int 0: başarılı, <0: hata
 */
int system_set_hostname(const char* hostname);

/**
 * @brief Kullanıcı yönetim yapılandırmasını alır
 *
 * @param config Yapılandırma çıktısı
 * @return int 0: başarılı, <0: hata
 */
int config_get(user_config_t* config);

/**
 * @brief Kullanıcı yönetim yapılandırmasını ayarlar
 *
 * @param config Yeni yapılandırma
 * @return int 0: başarılı, <0: hata
 */
int config_set(const user_config_t* config);

/**
 * @brief Misafir hesabını etkinleştirir veya devre dışı bırakır
 *
 * @param enabled 0: devre dışı, 1: etkin
 * @return int 0: başarılı, <0: hata
 */
int config_set_guest_enabled(uint8_t enabled);

/**
 * @brief Otomatik giriş yapılandırmasını ayarlar
 *
 * @param enabled 0: devre dışı, 1: etkin
 * @param username Otomatik giriş yapılacak kullanıcı (NULL: devre dışı)
 * @return int 0: başarılı, <0: hata
 */
int config_set_auto_login(uint8_t enabled, const char* username);

/**
 * @brief Eğer kullanıcının istenen izni varsa 1, yoksa 0 döndürür
 *
 * @param user_id Kullanıcı ID (0: geçerli kullanıcı)
 * @param permission Kontrol edilecek izin
 * @param result_out İzin sonucu (1: var, 0: yok)
 * @return int 0: başarılı, <0: hata
 */
int user_has_permission(uint32_t user_id, user_permission_t permission, uint8_t* result_out);

/**
 * @brief Komutu yükseltilmiş yetkilerle (root) çalıştırır
 *
 * @param command Çalıştırılacak komut
 * @param password Yönetici parolası
 * @return int 0: başarılı, <0: hata
 */
int user_run_as_root(const char* command, const char* password);

/**
 * @brief Yönetici yetkisini talep etmeye çalışır
 *
 * @param password Yönetici parolası
 * @param timeout_sec Yetki süresi (saniye, 0: kalıcı)
 * @return int 0: başarılı, <0: hata
 */
int user_request_admin(const char* password, uint32_t timeout_sec);

#endif /* USER_MANAGER_H */ 