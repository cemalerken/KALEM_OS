#include "../include/user_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Hata kodları
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

// Kullanıcı veri dosyası yolu
#define USER_DATABASE_PATH          "/etc/kalem/users.dat"
#define USER_CONFIG_PATH            "/etc/kalem/user_config.dat"
#define USER_SESSION_PATH           "/var/kalem/sessions.dat"
#define HOSTNAME_PATH               "/etc/hostname"
#define MAX_USERS                   64
#define MAX_SESSIONS                16
#define HASH_ITERATIONS             10000
#define SALT_LENGTH                 16

// Kullanıcı veritabanı
static user_info_t* users = NULL;
static uint32_t user_count = 0;
static uint32_t next_user_id = 1000;

// Oturum veritabanı
static session_info_t* sessions = NULL;
static uint32_t session_count = 0;
static uint32_t next_session_id = 1;

// Sistem yapılandırması
static user_config_t system_config = {0};

// Başlatılma durumu
static uint8_t is_initialized = 0;

// Geçerli oturum
static uint32_t current_session_id = 0;
static uint32_t current_user_id = 0;

// Yerel fonksiyon bildirimleri
static int load_users();
static int save_users();
static int load_config();
static int save_config();
static int load_sessions();
static int save_sessions();
static int find_user_by_id(uint32_t user_id);
static int find_user_by_name(const char* username);
static int find_session_by_id(uint32_t session_id);
static int find_session_by_user(uint32_t user_id);
static int generate_hash(const char* password, const char* salt, char* hash_out, uint32_t hash_len);
static int generate_salt(char* salt_out, uint32_t salt_len);
static int verify_password(const char* password, const char* salt, const char* hash);
static int create_home_directory(const char* username);
static int remove_home_directory(const char* path);
static int check_admin_privileges();
static int load_hostname(char* hostname, uint32_t max_len);
static int save_hostname(const char* hostname);

/**
 * Kullanıcı yönetim sistemini başlatır
 */
int user_manager_init() {
    if (is_initialized) {
        return USER_ERROR_NONE; // Zaten başlatılmış
    }
    
    // Kullanıcı dizisini oluştur
    users = (user_info_t*)malloc(MAX_USERS * sizeof(user_info_t));
    if (!users) {
        return USER_ERROR_OUT_OF_MEMORY;
    }
    
    // Oturum dizisini oluştur
    sessions = (session_info_t*)malloc(MAX_SESSIONS * sizeof(session_info_t));
    if (!sessions) {
        free(users);
        users = NULL;
        return USER_ERROR_OUT_OF_MEMORY;
    }
    
    // Kullanıcı veritabanını yükle
    if (load_users() != USER_ERROR_NONE) {
        // Kullanıcı veritabanı yüklenemedi, varsayılan yönetici hesabı oluştur
        memset(users, 0, MAX_USERS * sizeof(user_info_t));
        user_count = 0;
        
        // Root kullanıcısı oluştur
        user_info_t admin_user = {0};
        admin_user.user_id = next_user_id++;
        strcpy(admin_user.username, "admin");
        strcpy(admin_user.full_name, "Sistem Yöneticisi");
        
        // Varsayılan parola: "admin"
        generate_salt(admin_user.salt, sizeof(admin_user.salt));
        generate_hash("admin", admin_user.salt, admin_user.password_hash, sizeof(admin_user.password_hash));
        
        admin_user.type = USER_TYPE_ADMIN;
        admin_user.permissions = USER_PERM_FILES_READ | USER_PERM_FILES_WRITE | 
                                USER_PERM_NETWORK | USER_PERM_INSTALL_APP | 
                                USER_PERM_SYSTEM_CONF | USER_PERM_USER_ADMIN | 
                                USER_PERM_ROOT;
        
        strcpy(admin_user.home_dir, "/home/admin");
        strcpy(admin_user.shell, "/bin/ksh");
        admin_user.created_time = time(NULL);
        admin_user.last_login_time = 0;
        admin_user.account_disabled = 0;
        
        // Kullanıcı listesine ekle
        users[user_count++] = admin_user;
        
        // Ev dizini oluştur
        create_home_directory("admin");
        
        // Değişiklikleri kaydet
        save_users();
    }
    
    // Yapılandırmayı yükle
    if (load_config() != USER_ERROR_NONE) {
        // Yapılandırma yüklenemedi, varsayılanları kullan
        memset(&system_config, 0, sizeof(system_config));
        strcpy(system_config.hostname, "kalem-pc");
        system_config.auto_login = 0;
        system_config.guest_enabled = 1;
        system_config.show_users_list = 1;
        system_config.session_timeout = 30 * 60; // 30 dakika
        system_config.user_switching = 1;
        
        // Değişiklikleri kaydet
        save_config();
        save_hostname(system_config.hostname);
    }
    
    // Oturumları yükle
    if (load_sessions() != USER_ERROR_NONE) {
        // Oturumlar yüklenemedi, boş başlat
        memset(sessions, 0, MAX_SESSIONS * sizeof(session_info_t));
        session_count = 0;
        current_session_id = 0;
        current_user_id = 0;
    }
    
    is_initialized = 1;
    return USER_ERROR_NONE;
}

/**
 * Kullanıcı yönetim sistemini kapatır
 */
int user_manager_cleanup() {
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Kullanıcı veritabanını kaydet
    save_users();
    
    // Yapılandırmayı kaydet
    save_config();
    
    // Oturumları kaydet
    save_sessions();
    
    // Hafızayı temizle
    free(users);
    users = NULL;
    
    free(sessions);
    sessions = NULL;
    
    is_initialized = 0;
    return USER_ERROR_NONE;
}

/**
 * Yeni kullanıcı oluşturur
 */
int user_create(const char* username, const char* password, user_type_t type, 
               const char* full_name, uint32_t* user_id_out) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!username || !password) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Kullanıcı adının uzunluğu
    size_t username_len = strlen(username);
    if (username_len < 2 || username_len >= sizeof(users[0].username)) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Kullanıcı adında geçersiz karakterler var mı?
    for (size_t i = 0; i < username_len; i++) {
        char c = username[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) {
            return USER_ERROR_INVALID_ARG;
        }
    }
    
    // Parolanın minimum uzunluğu
    if (strlen(password) < 4) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Kullanıcı zaten var mı?
    int index = find_user_by_name(username);
    if (index >= 0) {
        return USER_ERROR_USER_EXISTS;
    }
    
    // Kullanıcı sayısını kontrol et
    if (user_count >= MAX_USERS) {
        return USER_ERROR_OUT_OF_MEMORY;
    }
    
    // Yeni kullanıcı oluştur
    user_info_t* new_user = &users[user_count];
    memset(new_user, 0, sizeof(user_info_t));
    
    new_user->user_id = next_user_id++;
    strncpy(new_user->username, username, sizeof(new_user->username) - 1);
    
    // Tam adı kopyala
    if (full_name && strlen(full_name) > 0) {
        strncpy(new_user->full_name, full_name, sizeof(new_user->full_name) - 1);
    } else {
        // Tam ad belirtilmemişse, kullanıcı adını kullan
        strncpy(new_user->full_name, username, sizeof(new_user->full_name) - 1);
    }
    
    // Parola hash'i oluştur
    generate_salt(new_user->salt, sizeof(new_user->salt));
    generate_hash(password, new_user->salt, new_user->password_hash, sizeof(new_user->password_hash));
    
    // Kullanıcı tipini ayarla
    new_user->type = type;
    
    // Varsayılan izinleri ayarla
    switch (type) {
        case USER_TYPE_ADMIN:
            new_user->permissions = USER_PERM_FILES_READ | USER_PERM_FILES_WRITE | 
                                   USER_PERM_NETWORK | USER_PERM_INSTALL_APP | 
                                   USER_PERM_SYSTEM_CONF | USER_PERM_USER_ADMIN | 
                                   USER_PERM_ROOT;
            break;
            
        case USER_TYPE_STANDARD:
            new_user->permissions = USER_PERM_FILES_READ | USER_PERM_FILES_WRITE | 
                                   USER_PERM_NETWORK | USER_PERM_INSTALL_APP;
            break;
            
        case USER_TYPE_GUEST:
            new_user->permissions = USER_PERM_FILES_READ | USER_PERM_NETWORK;
            break;
            
        case USER_TYPE_SYSTEM:
            new_user->permissions = USER_PERM_FILES_READ | USER_PERM_FILES_WRITE | 
                                   USER_PERM_NETWORK | USER_PERM_ROOT;
            break;
            
        default:
            new_user->permissions = USER_PERM_NONE;
            break;
    }
    
    // Ev dizini ve kabuğu ayarla
    if (type == USER_TYPE_GUEST) {
        strcpy(new_user->home_dir, "/tmp/guest");
        strcpy(new_user->shell, "/bin/ksh");
    } else if (type == USER_TYPE_SYSTEM) {
        sprintf(new_user->home_dir, "/var/system/%s", username);
        strcpy(new_user->shell, "/bin/ksh");
    } else {
        sprintf(new_user->home_dir, "/home/%s", username);
        strcpy(new_user->shell, "/bin/ksh");
    }
    
    // Profil resmi yolu
    sprintf(new_user->icon_path, "/usr/share/kalem/icons/users/default.png");
    
    // Zaman bilgilerini ayarla
    new_user->created_time = time(NULL);
    new_user->last_login_time = 0;
    
    // Hesap durumunu ayarla
    new_user->account_disabled = 0;
    new_user->password_expires = 0;
    new_user->password_expiry = 0;
    
    // Kullanıcı sayısını artır
    user_count++;
    
    // Ev dizini oluştur
    if (type != USER_TYPE_GUEST) {
        create_home_directory(username);
    }
    
    // Değişiklikleri kaydet
    save_users();
    
    // Kullanıcı ID'sini döndür
    if (user_id_out) {
        *user_id_out = new_user->user_id;
    }
    
    // Kurtarma e-postası sistemi için callback
    extern int recovery_setup_for_new_user(const char* username, const char* email);
    recovery_setup_for_new_user(username, NULL);
    
    return USER_ERROR_NONE;
}

/**
 * Kullanıcıyı siler
 */
int user_delete(uint32_t user_id, uint8_t delete_home) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Yönetici yetkisi kontrolü
    if (check_admin_privileges() != USER_ERROR_NONE) {
        return USER_ERROR_PERMISSION;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(user_id);
    if (index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Kullanıcı aktif oturuma sahip mi kontrol et
    int session_index = find_session_by_user(user_id);
    if (session_index >= 0) {
        // Kullanıcının oturumunu sonlandır
        session_terminate_user(user_id);
    }
    
    // Ev dizini kaldırılacak mı?
    if (delete_home) {
        remove_home_directory(users[index].home_dir);
    }
    
    // Kullanıcıyı listeden kaldır
    if (index < user_count - 1) {
        // Son kullanıcı değilse, son kullanıcıyı bu konuma taşı
        memmove(&users[index], &users[user_count - 1], sizeof(user_info_t));
    }
    
    user_count--;
    
    // Değişiklikleri kaydet
    return save_users();
}

/**
 * Kullanıcı bilgilerini alır
 */
int user_get_info(uint32_t user_id, user_info_t* info) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!info) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(user_id);
    if (index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Kullanıcı bilgilerini kopyala
    *info = users[index];
    
    // Güvenlik için parola bilgilerini temizle
    memset(info->password_hash, 0, sizeof(info->password_hash));
    memset(info->salt, 0, sizeof(info->salt));
    
    return USER_ERROR_NONE;
}

/**
 * Kullanıcı bilgilerini kullanıcı adına göre alır
 */
int user_get_info_by_name(const char* username, user_info_t* info) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!username || !info) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_name(username);
    if (index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Kullanıcı bilgilerini kopyala
    *info = users[index];
    
    // Güvenlik için parola bilgilerini temizle
    memset(info->password_hash, 0, sizeof(info->password_hash));
    memset(info->salt, 0, sizeof(info->salt));
    
    return USER_ERROR_NONE;
}

/**
 * Kullanıcı bilgilerini günceller
 */
int user_update_info(uint32_t user_id, const user_info_t* info) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!info) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(user_id);
    if (index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Yetki kontrolü yap
    if (current_user_id != user_id) {
        // Başka bir kullanıcıyı düzenlemeye çalışıyor, yönetici mi kontrol et
        if (check_admin_privileges() != USER_ERROR_NONE) {
            return USER_ERROR_PERMISSION;
        }
    }
    
    // Kullanıcı bilgilerini güncelle
    // Not: Burada güvenlik açısından kritik alanlar (user_id, password_hash, salt) güncellenmez
    // Tam ad
    if (info->full_name[0] != '\0') {
        strncpy(users[index].full_name, info->full_name, sizeof(users[index].full_name) - 1);
    }
    
    // Kullanıcı tipi ve izinler - sadece yönetici değiştirebilir
    if (check_admin_privileges() == USER_ERROR_NONE) {
        users[index].type = info->type;
        users[index].permissions = info->permissions;
        users[index].account_disabled = info->account_disabled;
        users[index].password_expires = info->password_expires;
        users[index].password_expiry = info->password_expiry;
    }
    
    // Kabuk ve ikon yolu
    if (info->shell[0] != '\0') {
        strncpy(users[index].shell, info->shell, sizeof(users[index].shell) - 1);
    }
    
    if (info->icon_path[0] != '\0') {
        strncpy(users[index].icon_path, info->icon_path, sizeof(users[index].icon_path) - 1);
    }
    
    // Değişiklikleri kaydet
    return save_users();
}

/**
 * Kullanıcı parolasını değiştirir
 */
int user_change_password(uint32_t user_id, const char* old_password, const char* new_password) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!old_password || !new_password) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(user_id);
    if (index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Yetki kontrolü yap
    if (current_user_id != user_id) {
        // Başka bir kullanıcının parolasını değiştirmeye çalışıyor
        // Yönetici ise onun için farklı bir fonksiyon var (user_reset_password)
        return USER_ERROR_PERMISSION;
    }
    
    // Eski parolayı doğrula
    if (verify_password(old_password, users[index].salt, users[index].password_hash) != USER_ERROR_NONE) {
        return USER_ERROR_AUTH_FAILED;
    }
    
    // Yeni parola için hash oluştur (mevcut tuz korunur)
    if (generate_hash(new_password, users[index].salt, users[index].password_hash, sizeof(users[index].password_hash)) != USER_ERROR_NONE) {
        return USER_ERROR_INTERNAL;
    }
    
    // Parola süresini güncelle
    if (users[index].password_expires) {
        users[index].password_expiry = time(NULL) + (180 * 24 * 60 * 60); // 180 gün
    }
    
    // Değişiklikleri kaydet
    return save_users();
}

/**
 * Yönetici tarafından kullanıcı parolasını sıfırlar
 */
int user_reset_password(uint32_t user_id, const char* new_password) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!new_password) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Yönetici yetkisi kontrolü
    if (check_admin_privileges() != USER_ERROR_NONE) {
        return USER_ERROR_PERMISSION;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(user_id);
    if (index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Yeni tuz oluştur
    if (generate_salt(users[index].salt, sizeof(users[index].salt)) != USER_ERROR_NONE) {
        return USER_ERROR_INTERNAL;
    }
    
    // Yeni parola için hash oluştur
    if (generate_hash(new_password, users[index].salt, users[index].password_hash, sizeof(users[index].password_hash)) != USER_ERROR_NONE) {
        return USER_ERROR_INTERNAL;
    }
    
    // Parola süresini güncelle
    if (users[index].password_expires) {
        users[index].password_expiry = time(NULL) + (180 * 24 * 60 * 60); // 180 gün
    }
    
    // Değişiklikleri kaydet
    return save_users();
}

/**
 * Kullanıcı hesabını etkinleştirir veya devre dışı bırakır
 */
int user_set_enabled(uint32_t user_id, uint8_t enabled) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Yönetici yetkisi kontrolü
    if (check_admin_privileges() != USER_ERROR_NONE) {
        return USER_ERROR_PERMISSION;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(user_id);
    if (index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Hesap durumunu güncelle
    users[index].account_disabled = !enabled;
    
    // Değişiklikleri kaydet
    return save_users();
}

/**
 * Kullanıcı tipini değiştirir
 */
int user_set_type(uint32_t user_id, user_type_t type) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Yönetici yetkisi kontrolü
    if (check_admin_privileges() != USER_ERROR_NONE) {
        return USER_ERROR_PERMISSION;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(user_id);
    if (index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // Kullanıcı tipini güncelle
    users[index].type = type;
    
    // Tipe göre izinleri güncelle
    switch (type) {
        case USER_TYPE_ADMIN:
            users[index].permissions = USER_PERM_FILES_READ | USER_PERM_FILES_WRITE | 
                                    USER_PERM_NETWORK | USER_PERM_INSTALL_APP | 
                                    USER_PERM_SYSTEM_CONF | USER_PERM_USER_ADMIN;
            break;
            
        case USER_TYPE_STANDARD:
            users[index].permissions = USER_PERM_FILES_READ | USER_PERM_FILES_WRITE | 
                                    USER_PERM_NETWORK | USER_PERM_INSTALL_APP;
            break;
            
        case USER_TYPE_GUEST:
            users[index].permissions = USER_PERM_FILES_READ | USER_PERM_NETWORK;
            break;
            
        case USER_TYPE_SYSTEM:
            users[index].permissions = USER_PERM_FILES_READ | USER_PERM_FILES_WRITE | 
                                    USER_PERM_NETWORK | USER_PERM_INSTALL_APP | 
                                    USER_PERM_SYSTEM_CONF | USER_PERM_USER_ADMIN | 
                                    USER_PERM_ROOT;
            break;
    }
    
    // Değişiklikleri kaydet
    return save_users();
}

/**
 * Kullanıcı yetkilerini günceller
 */
int user_set_permissions(uint32_t user_id, uint32_t permissions) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Yönetici yetkisi kontrolü
    if (check_admin_privileges() != USER_ERROR_NONE) {
        return USER_ERROR_PERMISSION;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(user_id);
    if (index < 0) {
        return USER_ERROR_USER_NOT_FOUND;
    }
    
    // İzinleri güncelle
    users[index].permissions = permissions;
    
    // Değişiklikleri kaydet
    return save_users();
}

/**
 * Tüm kullanıcıları listeler
 */
int user_list_all(user_info_t* users_out, uint32_t max_count, uint32_t* count_out) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!users_out || !count_out) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Listelenecek kullanıcı sayısını belirle
    uint32_t copy_count = (max_count < user_count) ? max_count : user_count;
    
    // Kullanıcıları kopyala
    for (uint32_t i = 0; i < copy_count; i++) {
        users_out[i] = users[i];
        
        // Güvenlik için parola bilgilerini temizle
        memset(users_out[i].password_hash, 0, sizeof(users_out[i].password_hash));
        memset(users_out[i].salt, 0, sizeof(users_out[i].salt));
    }
    
    // Kullanıcı sayısını döndür
    *count_out = copy_count;
    
    return USER_ERROR_NONE;
}

/**
 * Kullanıcı sayısını döndürür
 */
int user_get_count(uint32_t* count_out) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!count_out) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Kullanıcı sayısını döndür
    *count_out = user_count;
    
    return USER_ERROR_NONE;
}

/**
 * Geçerli oturum açmış kullanıcı ID'sini döndürür
 */
int user_get_current(uint32_t* user_id_out) {
    // Başlatılma kontrolü
    if (!is_initialized) {
        return USER_ERROR_NOT_INITIALIZED;
    }
    
    // Parametreleri kontrol et
    if (!user_id_out) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Kullanıcı ID'sini döndür
    *user_id_out = current_user_id;
    
    return USER_ERROR_NONE;
}

/* 
 * Burada diğer fonksiyonlar (session_login, session_logout, vb.) olacak
 * Dosyayı bölmek için şimdilik bu kadarını yazıyoruz
 */ 