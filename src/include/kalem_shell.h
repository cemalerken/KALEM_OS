#ifndef __KALEM_SHELL_H__
#define __KALEM_SHELL_H__

#include <stdint.h>
#include <stddef.h>
#include "gui.h"

// Sabitler
#define KALEM_SHELL_CMD_MAX          256   // Maksimum komut uzunluğu
#define KALEM_SHELL_HISTORY_MAX      100   // Maksimum geçmiş sayısı
#define KALEM_SHELL_OUTPUT_MAX       8192  // Maksimum çıktı tamponu boyutu
#define KALEM_SHELL_PROMPT           "kalem@os:~$ "  // Komut istemi
#define KALEM_SHELL_LINE_COUNT       25    // Terminal satır sayısı
#define KALEM_SHELL_COLUMN_COUNT     80    // Terminal sütun sayısı

// Kabuk durumları
typedef enum {
    KALEM_SHELL_STATUS_NORMAL = 0,
    KALEM_SHELL_STATUS_EXECUTING,
    KALEM_SHELL_STATUS_ROOT
} kalem_shell_status_t;

// Komut türleri
typedef enum {
    KALEM_SHELL_CMD_NONE = 0,
    KALEM_SHELL_CMD_INTERNAL,
    KALEM_SHELL_CMD_EXTERNAL,
    KALEM_SHELL_CMD_ALIAS
} kalem_shell_cmd_type_t;

// Komut işlevi işaretçisi
typedef int (*kalem_shell_cmd_func_t)(int argc, char** argv);

// Dahili komut yapısı
typedef struct {
    const char* name;             // Komut adı
    kalem_shell_cmd_func_t func;  // Komut işlevi
    const char* help;             // Yardım metni
    uint8_t requires_root;        // Root yetkisi gerekiyor mu?
} kalem_shell_cmd_t;

// Kabuk yapısı
typedef struct {
    // GUI bileşenleri
    gui_window_t* window;                 // Pencere işaretçisi
    
    // Durum bilgisi
    kalem_shell_status_t status;          // Kabuk durumu
    
    // Komut tamponu
    char cmd_buffer[KALEM_SHELL_CMD_MAX]; // Komut tamponu
    int cmd_pos;                          // İmleç pozisyonu
    
    // Çıktı tamponu
    char output_buffer[KALEM_SHELL_OUTPUT_MAX]; // Çıktı tamponu
    int output_pos;                       // Çıktı pozisyonu
    
    // Geçmiş
    char* history[KALEM_SHELL_HISTORY_MAX]; // Komut geçmişi
    int history_count;                    // Geçmişteki komut sayısı
    int history_pos;                      // Geçmiş pozisyonu
    
    // İmleç durumu
    uint8_t cursor_visible;               // İmleç görünür mü?
    uint32_t cursor_blink_time;           // İmleç yanıp sönme zamanı
    
    // Dizin bilgisi
    char current_dir[256];                // Mevcut çalışma dizini
    
    // Yetki durumu
    uint8_t is_root;                      // Root yetkisi var mı?
} kalem_shell_t;

// Kabuk fonksiyonları
void kalem_shell_init();                 // Kabuğu başlat
void kalem_shell_show();                 // Kabuk penceresini göster
void kalem_shell_close();                // Kabuk penceresini kapat
void kalem_shell_paint(gui_window_t* window);  // Kabuk çizim fonksiyonu
void kalem_shell_handle_key(uint8_t key);      // Tuş girişini işle
void kalem_shell_execute_command();      // Komutu yürüt
void kalem_shell_add_to_output();        // Komut satırını çıktıya ekle
void kalem_shell_print(const char* text);      // Çıktıya metin ekle
void kalem_shell_println(const char* text);    // Çıktıya satır ekle
void kalem_shell_navigate_history(int direction); // Geçmişte gezin
void kalem_shell_tab_complete();         // Tab tamamlama
int kalem_shell_parse_command(char* cmd);     // Komutu yorumla
int kalem_shell_run_internal_command(const char* cmd_name, int argc, char** argv); // Dahili komutu çalıştır
int kalem_shell_run_external_command(const char* cmd_name, int argc, char** argv); // Harici komutu çalıştır
int kalem_shell_authenticate_root();     // Root yetkilendirme

// Dahili komut işlevleri
int kalem_shell_clear();                // Ekranı temizle
int kalem_shell_change_dir(const char* path); // Dizini değiştir
int kalem_shell_list_dir(const char* path);   // Dizini listele
int kalem_shell_cat(const char* filename);    // Dosya içeriğini göster
int kalem_shell_touch(const char* filename);  // Dosya oluştur
int kalem_shell_mkdir(const char* dirname);   // Dizin oluştur
int kalem_shell_rm(const char* path);         // Dosya/dizin sil
int kalem_shell_help();                 // Yardım göster
int kalem_shell_exit();                 // Kabuktan çık
int kalem_shell_history();              // Geçmişi göster
int kalem_shell_sysinfo();              // Sistem bilgilerini göster

// Uygulama ana fonksiyonu
void kalem_shell_app();                 // Shell uygulamasını başlat

#endif // __KALEM_SHELL_H__ 