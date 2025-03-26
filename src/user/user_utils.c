#include "../include/user_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

// Kriptografik fonksiyonlar için
#include <openssl/sha.h>
#include <openssl/rand.h>

// Dosya yolları ve sabitler
#define USER_DATABASE_PATH  "/etc/kalem/users.dat"
#define USER_CONFIG_PATH    "/etc/kalem/user_config.dat"
#define USER_SESSION_PATH   "/var/kalem/sessions.dat"
#define USER_HOME_BASE      "/home"
#define DEFAULT_SHELL       "/bin/kalem_shell"
#define SALT_LENGTH         16
#define HASH_ITERATIONS     10000

// Global yapılandırma
user_config_t system_config = {0};
char system_config_hostname[64] = "kalem-pc";

/**
 * Rastgele bir tuz (salt) oluşturur
 */
int generate_salt(char* salt_out, uint32_t len) {
    // Tuz için rastgele baytlar oluştur
    unsigned char random_bytes[SALT_LENGTH];
    if (RAND_bytes(random_bytes, SALT_LENGTH) != 1) {
        return -1;
    }
    
    // Hex formatına dönüştür
    for (uint32_t i = 0; i < SALT_LENGTH && i * 2 < len - 1; i++) {
        sprintf(salt_out + (i * 2), "%02x", random_bytes[i]);
    }
    
    return 0;
}

/**
 * Parola hash fonksiyonu (PBKDF2 benzeri basit implementasyon)
 */
int hash_password(const char* password, const char* salt, char* hash_out, uint32_t hash_len) {
    if (!password || !salt || !hash_out || hash_len < SHA256_DIGEST_LENGTH * 2 + 1) {
        return -1;
    }
    
    // İlk hash değerini hesapla
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password, strlen(password));
    SHA256_Update(&sha256, salt, strlen(salt));
    SHA256_Final(hash, &sha256);
    
    // Iterasyonları uygula
    for (int i = 0; i < HASH_ITERATIONS; i++) {
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, hash, SHA256_DIGEST_LENGTH);
        SHA256_Final(hash, &sha256);
    }
    
    // Hash'i hex formatına dönüştür
    for (uint32_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hash_out + (i * 2), "%02x", hash[i]);
    }
    
    return 0;
}

/**
 * Verilen parolayı doğrular
 */
int verify_password(const char* password, const char* salt, const char* stored_hash) {
    char calculated_hash[SHA256_DIGEST_LENGTH * 2 + 1] = {0};
    
    if (hash_password(password, salt, calculated_hash, sizeof(calculated_hash)) != 0) {
        return USER_ERROR_INTERNAL;
    }
    
    return (strcmp(calculated_hash, stored_hash) == 0) ? USER_ERROR_NONE : USER_ERROR_AUTH_FAILED;
}

/**
 * Kullanıcı ID'sine göre kullanıcıyı bulur, bulunursa indisini döndürür
 */
int find_user_by_id(uint32_t user_id) {
    extern user_info_t* users;
    extern uint32_t user_count;
    
    for (uint32_t i = 0; i < user_count; i++) {
        if (users[i].user_id == user_id) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Kullanıcı adına göre kullanıcıyı bulur, bulunursa indisini döndürür
 */
int find_user_by_name(const char* username) {
    extern user_info_t* users;
    extern uint32_t user_count;
    
    if (!username) {
        return -1;
    }
    
    for (uint32_t i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Oturum ID'sine göre oturumu bulur, bulunursa indisini döndürür
 */
int find_session_by_id(uint32_t session_id) {
    extern session_info_t* sessions;
    extern uint32_t session_count;
    
    for (uint32_t i = 0; i < session_count; i++) {
        if (sessions[i].session_id == session_id) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Kullanıcı ID'sine göre oturumu bulur, bulunursa indisini döndürür
 */
int find_session_by_user(uint32_t user_id) {
    extern session_info_t* sessions;
    extern uint32_t session_count;
    
    for (uint32_t i = 0; i < session_count; i++) {
        if (sessions[i].user_id == user_id) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Kullanıcı veritabanını dosyadan yükler
 */
int load_users() {
    extern user_info_t* users;
    extern uint32_t user_count;
    extern uint32_t next_user_id;
    
    FILE* fp = fopen(USER_DATABASE_PATH, "rb");
    if (!fp) {
        // Dosya bulunamadı, ilk çalıştırma olabilir
        return USER_ERROR_FILE_IO;
    }
    
    // Kullanıcı sayısını oku
    if (fread(&user_count, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    // Sonraki kullanıcı ID'sini oku
    if (fread(&next_user_id, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    // Bellek tahsis et
    if (users) {
        free(users);
    }
    
    users = (user_info_t*)calloc(user_count, sizeof(user_info_t));
    if (!users) {
        fclose(fp);
        return USER_ERROR_OUT_OF_MEMORY;
    }
    
    // Kullanıcı bilgilerini oku
    if (fread(users, sizeof(user_info_t), user_count, fp) != user_count) {
        free(users);
        users = NULL;
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    fclose(fp);
    return USER_ERROR_NONE;
}

/**
 * Kullanıcı veritabanını dosyaya kaydeder
 */
int save_users() {
    extern user_info_t* users;
    extern uint32_t user_count;
    extern uint32_t next_user_id;
    
    // Dizini oluştur
    mkdir("/etc", 0755);
    mkdir("/etc/kalem", 0755);
    
    FILE* fp = fopen(USER_DATABASE_PATH, "wb");
    if (!fp) {
        return USER_ERROR_FILE_IO;
    }
    
    // Kullanıcı sayısını yaz
    if (fwrite(&user_count, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    // Sonraki kullanıcı ID'sini yaz
    if (fwrite(&next_user_id, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    // Kullanıcı bilgilerini yaz
    if (fwrite(users, sizeof(user_info_t), user_count, fp) != user_count) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    fclose(fp);
    return USER_ERROR_NONE;
}

/**
 * Oturum bilgilerini dosyadan yükler
 */
int load_sessions() {
    extern session_info_t* sessions;
    extern uint32_t session_count;
    extern uint32_t next_session_id;
    
    FILE* fp = fopen(USER_SESSION_PATH, "rb");
    if (!fp) {
        // Dosya bulunamadı, ilk çalıştırma olabilir
        return USER_ERROR_FILE_IO;
    }
    
    // Oturum sayısını oku
    if (fread(&session_count, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    // Sonraki oturum ID'sini oku
    if (fread(&next_session_id, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    // Bellek tahsis et
    if (sessions) {
        free(sessions);
    }
    
    sessions = (session_info_t*)calloc(session_count, sizeof(session_info_t));
    if (!sessions) {
        fclose(fp);
        return USER_ERROR_OUT_OF_MEMORY;
    }
    
    // Oturum bilgilerini oku
    if (fread(sessions, sizeof(session_info_t), session_count, fp) != session_count) {
        free(sessions);
        sessions = NULL;
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    fclose(fp);
    return USER_ERROR_NONE;
}

/**
 * Oturum bilgilerini dosyaya kaydeder
 */
int save_sessions() {
    extern session_info_t* sessions;
    extern uint32_t session_count;
    extern uint32_t next_session_id;
    
    // Dizini oluştur
    mkdir("/var", 0755);
    mkdir("/var/kalem", 0755);
    
    FILE* fp = fopen(USER_SESSION_PATH, "wb");
    if (!fp) {
        return USER_ERROR_FILE_IO;
    }
    
    // Oturum sayısını yaz
    if (fwrite(&session_count, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    // Sonraki oturum ID'sini yaz
    if (fwrite(&next_session_id, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    // Oturum bilgilerini yaz
    if (fwrite(sessions, sizeof(session_info_t), session_count, fp) != session_count) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    fclose(fp);
    return USER_ERROR_NONE;
}

/**
 * Yapılandırmayı dosyadan yükler
 */
int load_config() {
    FILE* fp = fopen(USER_CONFIG_PATH, "rb");
    if (!fp) {
        // Dosya bulunamadı, varsayılan yapılandırmayı kullan
        return USER_ERROR_FILE_IO;
    }
    
    // Yapılandırmayı oku
    if (fread(&system_config, sizeof(user_config_t), 1, fp) != 1) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    fclose(fp);
    return USER_ERROR_NONE;
}

/**
 * Yapılandırmayı dosyaya kaydeder
 */
int save_config() {
    // Dizini oluştur
    mkdir("/etc", 0755);
    mkdir("/etc/kalem", 0755);
    
    FILE* fp = fopen(USER_CONFIG_PATH, "wb");
    if (!fp) {
        return USER_ERROR_FILE_IO;
    }
    
    // Yapılandırmayı yaz
    if (fwrite(&system_config, sizeof(user_config_t), 1, fp) != 1) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    fclose(fp);
    return USER_ERROR_NONE;
}

/**
 * Bilgisayar adını dosyadan yükler
 */
int load_hostname() {
    FILE* fp = fopen("/etc/hostname", "r");
    if (!fp) {
        // Dosya bulunamadı, varsayılan bilgisayar adını kullan
        return USER_ERROR_FILE_IO;
    }
    
    // Bilgisayar adını oku
    if (fgets(system_config_hostname, sizeof(system_config_hostname), fp) == NULL) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    // Yeni satır karakterini kaldır
    size_t len = strlen(system_config_hostname);
    if (len > 0 && system_config_hostname[len - 1] == '\n') {
        system_config_hostname[len - 1] = '\0';
    }
    
    fclose(fp);
    return USER_ERROR_NONE;
}

/**
 * Bilgisayar adını dosyaya kaydeder
 */
int save_hostname(const char* hostname) {
    if (!hostname) {
        return USER_ERROR_INVALID_ARG;
    }
    
    FILE* fp = fopen("/etc/hostname", "w");
    if (!fp) {
        return USER_ERROR_FILE_IO;
    }
    
    // Bilgisayar adını yaz
    if (fputs(hostname, fp) < 0) {
        fclose(fp);
        return USER_ERROR_FILE_IO;
    }
    
    fclose(fp);
    
    // hosts dosyasını da güncelle
    fp = fopen("/etc/hosts", "w");
    if (!fp) {
        return USER_ERROR_FILE_IO;
    }
    
    fprintf(fp, "127.0.0.1\tlocalhost\n");
    fprintf(fp, "127.0.1.1\t%s\n", hostname);
    fprintf(fp, "::1\t\tlocalhost ip6-localhost ip6-loopback\n");
    
    fclose(fp);
    
    return USER_ERROR_NONE;
}

/**
 * Yönetici yetkilerini kontrol eder
 */
int check_admin_privileges() {
    extern uint32_t current_user_id;
    extern user_info_t* users;
    
    // Eğer oturum açılmamışsa, yetki yok
    if (current_user_id == 0) {
        return USER_ERROR_PERMISSION;
    }
    
    // Kullanıcıyı bul
    int index = find_user_by_id(current_user_id);
    if (index < 0) {
        return USER_ERROR_PERMISSION;
    }
    
    // Kullanıcı tipi kontrolü
    if (users[index].type == USER_TYPE_ADMIN || users[index].type == USER_TYPE_SYSTEM) {
        return USER_ERROR_NONE;
    }
    
    // Root yetkisi kontrolü
    if (users[index].permissions & USER_PERM_ROOT) {
        return USER_ERROR_NONE;
    }
    
    return USER_ERROR_PERMISSION;
}

/**
 * Kullanıcı için ev dizini yolunu oluşturur
 */
void get_user_home_dir(const char* username, char* path_out, uint32_t max_len) {
    snprintf(path_out, max_len, "%s/%s", USER_HOME_BASE, username);
}

/**
 * Kullanıcı için ev dizini oluşturur
 */
int create_user_home_dir(const char* username) {
    if (!username) {
        return USER_ERROR_INVALID_ARG;
    }
    
    char home_path[256];
    get_user_home_dir(username, home_path, sizeof(home_path));
    
    // Dizini oluştur
    if (mkdir(home_path, 0700) != 0 && errno != EEXIST) {
        return USER_ERROR_FILE_IO;
    }
    
    // Varsayılan dizinleri oluştur
    char dir_path[300];
    
    snprintf(dir_path, sizeof(dir_path), "%s/Desktop", home_path);
    mkdir(dir_path, 0700);
    
    snprintf(dir_path, sizeof(dir_path), "%s/Documents", home_path);
    mkdir(dir_path, 0700);
    
    snprintf(dir_path, sizeof(dir_path), "%s/Downloads", home_path);
    mkdir(dir_path, 0700);
    
    snprintf(dir_path, sizeof(dir_path), "%s/Music", home_path);
    mkdir(dir_path, 0700);
    
    snprintf(dir_path, sizeof(dir_path), "%s/Pictures", home_path);
    mkdir(dir_path, 0700);
    
    snprintf(dir_path, sizeof(dir_path), "%s/Videos", home_path);
    mkdir(dir_path, 0700);
    
    // Sahiplik ve izinleri ayarla
    int uid = -1;
    int gid = -1;
    
    // Not: Gerçek bir sistemde uid ve gid değerleri passwd dosyasından alınır
    // Bu örnek kodda basitlik için varsayalım ki uid ve gid var
    
    // Ana dizini ve alt dizinlerini kullanıcıya ata
    if (uid >= 0 && gid >= 0) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "chown -R %d:%d %s", uid, gid, home_path);
        system(cmd);
    }
    
    return USER_ERROR_NONE;
}

/**
 * Kullanıcı ev dizinini siler
 */
int remove_user_home_dir(const char* username) {
    if (!username) {
        return USER_ERROR_INVALID_ARG;
    }
    
    char home_path[256];
    get_user_home_dir(username, home_path, sizeof(home_path));
    
    // Güvenlik kontrolü - ana dizinleri silmemek için
    if (strcmp(home_path, "/") == 0 || strcmp(home_path, "/home") == 0 || 
        strlen(home_path) < 6) {
        return USER_ERROR_INVALID_ARG;
    }
    
    // Dizini sil
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", home_path);
    int res = system(cmd);
    
    return (res == 0) ? USER_ERROR_NONE : USER_ERROR_FILE_IO;
}

/**
 * Yönetici yetkisi kontrolü yapar
 */
int check_user_permission(uint32_t user_id, user_permission_t perm) {
    // Kullanıcıyı bul
    int index = find_user_by_id(user_id);
    if (index < 0) {
        return 0;
    }
    
    extern user_info_t* users;
    
    // Yönetici kullanıcıları tüm izinlere sahiptir
    if (users[index].type == USER_TYPE_ADMIN || users[index].type == USER_TYPE_SYSTEM) {
        return 1;
    }
    
    // İzin kontrolü
    return (users[index].permissions & perm) ? 1 : 0;
} 