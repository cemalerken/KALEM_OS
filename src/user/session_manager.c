#include "../include/user_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Hata kodları ve sabit tanımlar için önceki dosyadan tanımlar burada tekrar edilecek
#define USER_ERROR_NONE              0
#define USER_ERROR_INIT_FAILED      -1
#define USER_ERROR_NOT_INITIALIZED  -2
#define USER_ERROR_INVALID_ARG      -3
#define USER_ERROR_USER_EXISTS      -4
#define USER_ERROR_USER_NOT_FOUND   -5
#define USER_ERROR_AUTH_FAILED      -6
#define USER_ERROR_PERMISSION       -7
#define USER_ERROR_SESSION          -8
#define USER_ERROR_FILE_IO          -9
#define USER_ERROR_OUT_OF_MEMORY    -10
#define USER_ERROR_INTERNAL         -99

// Bu dosyada kullanılan harici değişkenler ve fonksiyonlar (extern)
extern uint8_t is_initialized;
extern user_info_t* users;
extern uint32_t user_count;
extern session_info_t* sessions;
extern uint32_t session_count;
extern uint32_t next_session_id;
extern uint32_t current_session_id;
extern uint32_t current_user_id;

// Harici yardımcı fonksiyonlar
extern int find_user_by_id(uint32_t user_id);
extern int find_user_by_name(const char* username);
extern int find_session_by_id(uint32_t session_id);
extern int find_session_by_user(uint32_t user_id);
extern int verify_password(const char* password, const char* salt, const char* hash);
extern int save_sessions();
extern int save_users();

/**
 * Kullanıcı oturumu açar
 */
int session_login(const char* username, const char* password, uint32_t* session_id_out) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!username || !password) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Kullanıcıyı bul
    int user_index = find_user_by_name(username);
    if (user_index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Hesap devre dışı mı kontrol et
    if (users[user_index].account_disabled) {
        return USER_ERROR_AUTH_FAILED;
    }
    
    // Parolayı doğrula
    if (verify_password(password, users[user_index].salt, users[user_index].password_hash) != USER_ERROR_NONE) {
        return USER_ERROR_AUTH_FAILED;
    }
    
    // Kullanıcının aktif oturumu var mı kontrol et
    int session_index = find_session_by_user(users[user_index].user_id);
    if (session_index >= 0) {
        // Kullanıcının zaten bir oturumu var, bu oturumu sonlandır
        // veya hata döndür
        return USER_ERROR_SESSION;
    }
    
    // Oturum sınırı kontrolü
    if (session_count >= MAX_SESSIONS) {
        return USER_ERROR_OUT_OF_MEMORY;
    }
    
    // Yeni oturum oluştur
    session_info_t new_session = {0};
    new_session.session_id = next_session_id++;
    new_session.user_id = users[user_index].user_id;
    strncpy(new_session.username, username, sizeof(new_session.username) - 1);
    new_session.state = SESSION_STATE_ACTIVE;
    new_session.login_time = time(NULL);
    new_session.last_active_time = new_session.login_time;
    
    // Terminal ve ekran bilgilerini doldur
    strcpy(new_session.terminal, "tty1"); // Örnek değer
    strcpy(new_session.display, ":0");    // Örnek değer
    
    // Oturum listesine ekle
    sessions[session_count++] = new_session;
    
    // Geçerli oturum ve kullanıcı ID'sini güncelle
    current_session_id = new_session.session_id;
    current_user_id = new_session.user_id;
    
    // Son giriş zamanını güncelle
    users[user_index].last_login_time = new_session.login_time;
    save_users();
    
    // Oturum bilgilerini kaydet
    save_sessions();
    
    // Oturum ID'sini döndür (isteğe bağlı)
    if (session_id_out) {
        *session_id_out = new_session.session_id;
    }
    
    return USER_ERROR_NONE;
}

/**
 * Kullanıcı oturumunu kapatır
 */
int session_logout(uint32_t session_id) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Oturum ID'si belirtilmemişse, geçerli oturumu kullan
    if (session_id == 0) {
        session_id = current_session_id;
    }
    
    // Geçerli oturum yoksa hata döndür
    if (session_id == 0) {
        return USER_ERROR_SESSION;
    }
    
    // Oturumu bul
    int index = find_session_by_id(session_id);
    if (index < 0) {
        return USER_ERROR_SESSION;
    }
    
    // Oturumu listeden kaldır
    if (index < session_count - 1) {
        // Son oturum değilse, son oturumu bu konuma taşı
        memmove(&sessions[index], &sessions[session_count - 1], sizeof(session_info_t));
    }
    
    session_count--;
    
    // Eğer geçerli oturum kapatıldıysa, geçerli oturum ve kullanıcı ID'sini sıfırla
    if (current_session_id == session_id) {
        current_session_id = 0;
        current_user_id = 0;
    }
    
    // Oturum bilgilerini kaydet
    return save_sessions();
}

/**
 * Kullanıcı oturumunu kilitler
 */
int session_lock(uint32_t session_id) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Oturum ID'si belirtilmemişse, geçerli oturumu kullan
    if (session_id == 0) {
        session_id = current_session_id;
    }
    
    // Geçerli oturum yoksa hata döndür
    if (session_id == 0) {
        return USER_ERROR_SESSION;
    }
    
    // Oturumu bul
    int index = find_session_by_id(session_id);
    if (index < 0) {
        return USER_ERROR_SESSION;
    }
    
    // Oturum zaten kilitli mi kontrol et
    if (sessions[index].state == SESSION_STATE_LOCKED) {
        return USER_ERROR_NONE; // Zaten kilitli
    }
    
    // Oturumu kilitle
    sessions[index].state = SESSION_STATE_LOCKED;
    
    // Oturum bilgilerini kaydet
    return save_sessions();
}

/**
 * Kilitli oturumun kilidini açar
 */
int session_unlock(uint32_t session_id, const char* password) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!password) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Oturum ID'si belirtilmemişse, geçerli oturumu kullan
    if (session_id == 0) {
        session_id = current_session_id;
    }
    
    // Geçerli oturum yoksa hata döndür
    if (session_id == 0) {
        return USER_ERROR_SESSION;
    }
    
    // Oturumu bul
    int session_index = find_session_by_id(session_id);
    if (session_index < 0) {
        return USER_ERROR_SESSION;
    }
    
    // Oturum kilitli değilse hata döndür
    if (sessions[session_index].state != SESSION_STATE_LOCKED) {
        return USER_ERROR_SESSION;
    }
    
    // Kullanıcıyı bul
    int user_index = find_user_by_id(sessions[session_index].user_id);
    if (user_index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Parolayı doğrula
    if (verify_password(password, users[user_index].salt, users[user_index].password_hash) != USER_ERROR_NONE) {
        return USER_ERROR_AUTH_FAILED;
    }
    
    // Oturumun kilidini aç
    sessions[session_index].state = SESSION_STATE_ACTIVE;
    sessions[session_index].last_active_time = time(NULL);
    
    // Oturum bilgilerini kaydet
    return save_sessions();
}

/**
 * Geçerli oturum bilgilerini alır
 */
int session_get_info(uint32_t session_id, session_info_t* info) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!info) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Oturum ID'si belirtilmemişse, geçerli oturumu kullan
    if (session_id == 0) {
        session_id = current_session_id;
    }
    
    // Geçerli oturum yoksa hata döndür
    if (session_id == 0) {
        return USER_ERROR_SESSION;
    }
    
    // Oturumu bul
    int index = find_session_by_id(session_id);
    if (index < 0) {
        return USER_ERROR_SESSION;
    }
    
    // Oturum bilgilerini kopyala
    *info = sessions[index];
    
    return USER_ERROR_NONE;
}

/**
 * Tüm aktif oturumları listeler
 */
int session_list_all(session_info_t* sessions_out, uint32_t max_count, uint32_t* count_out) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!sessions_out || !count_out) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Listelenecek oturum sayısını belirle
    uint32_t copy_count = (max_count < session_count) ? max_count : session_count;
    
    // Oturumları kopyala
    for (uint32_t i = 0; i < copy_count; i++) {
        sessions_out[i] = sessions[i];
    }
    
    // Oturum sayısını döndür
    *count_out = copy_count;
    
    return USER_ERROR_NONE;
}

/**
 * Kullanıcı oturumlarını sonlandırır
 */
int session_terminate_user(uint32_t user_id) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Kullanıcının oturumlarını sonlandır
    for (int i = session_count - 1; i >= 0; i--) {
        if (sessions[i].user_id == user_id) {
            // Eğer geçerli oturum sonlandırılıyorsa, geçerli oturum ve kullanıcı ID'sini sıfırla
            if (current_session_id == sessions[i].session_id) {
                current_session_id = 0;
                current_user_id = 0;
            }
            
            // Oturumu listeden kaldır
            if (i < session_count - 1) {
                // Son oturum değilse, son oturumu bu konuma taşı
                memmove(&sessions[i], &sessions[session_count - 1], sizeof(session_info_t));
            }
            
            session_count--;
        }
    }
    
    // Oturum bilgilerini kaydet
    return save_sessions();
}

/**
 * Bilgisayar adını alır
 */
int system_get_hostname(char* hostname, uint32_t max_len) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!hostname || max_len == 0) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Bilgisayar adını döndür
    extern char system_config_hostname[64];
    strncpy(hostname, system_config_hostname, max_len - 1);
    hostname[max_len - 1] = '\0';
    
    return USER_ERROR_NONE;
}

/**
 * Bilgisayar adını değiştirir
 */
int system_set_hostname(const char* hostname) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!hostname) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Yönetici yetkisi kontrolü
    extern int check_admin_privileges();
    if (check_admin_privileges() != USER_ERROR_NONE) {
        return USER_ERROR_PERMISSION;
    }
    
    // Bilgisayar adını güncelle
    extern char system_config_hostname[64];
    strncpy(system_config_hostname, hostname, sizeof(system_config_hostname) - 1);
    system_config_hostname[sizeof(system_config_hostname) - 1] = '\0';
    
    // Bilgisayar adını dosyaya kaydet
    extern int save_hostname(const char* hostname);
    return save_hostname(hostname);
}

/**
 * Kullanıcı yönetim yapılandırmasını alır
 */
int config_get(user_config_t* config) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!config) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Yapılandırmayı döndür
    extern user_config_t system_config;
    *config = system_config;
    
    return USER_ERROR_NONE;
}

/**
 * Kullanıcı yönetim yapılandırmasını ayarlar
 */
int config_set(const user_config_t* config) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!config) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Yönetici yetkisi kontrolü
    extern int check_admin_privileges();
    if (check_admin_privileges() != USER_ERROR_NONE) {
        return USER_ERROR_PERMISSION;
    }
    
    // Yapılandırmayı güncelle
    extern user_config_t system_config;
    system_config = *config;
    
    // Değişiklikleri kaydet
    extern int save_config();
    int result = save_config();
    
    // Bilgisayar adını ayrıca kaydet
    extern int save_hostname(const char* hostname);
    save_hostname(system_config.hostname);
    
    return result;
}

/**
 * Misafir hesabını etkinleştirir veya devre dışı bırakır
 */
int config_set_guest_enabled(uint8_t enabled) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Yönetici yetkisi kontrolü
    extern int check_admin_privileges();
    if (check_admin_privileges() != USER_ERROR_NONE) {
        return USER_ERROR_PERMISSION;
    }
    
    // Misafir hesabını güncelle
    extern user_config_t system_config;
    system_config.guest_enabled = enabled;
    
    // Değişiklikleri kaydet
    extern int save_config();
    return save_config();
}

/**
 * Otomatik giriş yapılandırmasını ayarlar
 */
int config_set_auto_login(uint8_t enabled, const char* username) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Yönetici yetkisi kontrolü
    extern int check_admin_privileges();
    if (check_admin_privileges() != USER_ERROR_NONE) {
        return USER_ERROR_PERMISSION;
    }
    
    // Otomatik giriş ayarını güncelle
    extern user_config_t system_config;
    system_config.auto_login = enabled;
    
    if (enabled && username) {
        // Kullanıcının varlığını kontrol et
        if (find_user_by_name(username) < 0) {
            return USER_ERROR_USER_NOT_FOUND;
        }
        
        strncpy(system_config.auto_login_user, username, sizeof(system_config.auto_login_user) - 1);
    } else {
        // Otomatik giriş devre dışı bırakılıyorsa, kullanıcı adını temizle
        system_config.auto_login_user[0] = '\0';
    }
    
    // Değişiklikleri kaydet
    extern int save_config();
    return save_config();
}

/**
 * Eğer kullanıcının istenen izni varsa 1, yoksa 0 döndürür
 */
int user_has_permission(uint32_t user_id, user_permission_t permission, uint8_t* result_out) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!result_out) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Kullanıcı ID'si belirtilmemişse, geçerli kullanıcıyı kullan
    if (user_id == 0) {
        user_id = current_user_id;
    }
    
    // Geçerli kullanıcı yoksa hata döndür
    if (user_id == 0) {
        *result_out = 0;
        return USER_ERROR_NONE;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(user_id);
    if (index < 0) {
        *result_out = 0;
        return USER_ERROR_NONE;
    }
    
    // İzin kontrolü yap
    *result_out = (users[index].permissions & permission) ? 1 : 0;
    
    return USER_ERROR_NONE;
}

/**
 * Komutu yükseltilmiş yetkilerle (root) çalıştırır
 */
int user_run_as_root(const char* command, const char* password) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!command || !password) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Geçerli kullanıcı yoksa hata döndür
    if (current_user_id == 0) {
        return USER_ERROR_PERMISSION;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(current_user_id);
    if (index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Kullanıcı zaten root yetkisine sahip mi kontrol et
    if (users[index].permissions & USER_PERM_ROOT) {
        // Root yetkisi var, komutu doğrudan çalıştır
        // Not: Gerçek uygulamada burada komut çalıştırma işlemi yapılır
        printf("Root olarak çalıştırılıyor: %s\n", command);
        return USER_ERROR_NONE;
    }
    
    // Admin hesabını bul
    int admin_index = -1;
    for (uint32_t i = 0; i < user_count; i++) {
        if (users[i].type == USER_TYPE_ADMIN) {
            admin_index = i;
            break;
        }
    }
    
    if (admin_index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Admin parolasını doğrula
    if (verify_password(password, users[admin_index].salt, users[admin_index].password_hash) != USER_ERROR_NONE) {
        return USER_ERROR_AUTH_FAILED;
    }
    
    // Parola doğrulandı, komutu root olarak çalıştır
    // Not: Gerçek uygulamada burada komut çalıştırma işlemi yapılır
    printf("Root olarak çalıştırılıyor: %s\n", command);
    
    return USER_ERROR_NONE;
}

/**
 * Yönetici yetkisini talep etmeye çalışır
 */
int user_request_admin(const char* password, uint32_t timeout_sec) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!password) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Geçerli kullanıcı yoksa hata döndür
    if (current_user_id == 0) {
        return USER_ERROR_PERMISSION;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(current_user_id);
    if (index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Kullanıcı zaten yönetici mi kontrol et
    if (users[index].type == USER_TYPE_ADMIN || users[index].type == USER_TYPE_SYSTEM) {
        // Kullanıcı zaten yönetici, parolasını doğrula
        if (verify_password(password, users[index].salt, users[index].password_hash) == USER_ERROR_NONE) {
            // Başarılı, geçici yönetici yetkileri ver
            // Not: Gerçek uygulamada bu yetkiler geçici bir süre için tanımlanır
            return USER_ERROR_NONE;
        } else {
            return USER_ERROR_AUTH_FAILED;
        }
    }
    
    // Admin hesabını bul
    int admin_index = -1;
    for (uint32_t i = 0; i < user_count; i++) {
        if (users[i].type == USER_TYPE_ADMIN) {
            admin_index = i;
            break;
        }
    }
    
    if (admin_index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Admin parolasını doğrula
    if (verify_password(password, users[admin_index].salt, users[admin_index].password_hash) != USER_ERROR_NONE) {
        return USER_ERROR_AUTH_FAILED;
    }
    
    // Parola doğrulandı, geçici yönetici yetkileri ver
    // Not: Gerçek uygulamada bu yetkiler geçici bir süre için tanımlanır
    
    return USER_ERROR_NONE;
} 