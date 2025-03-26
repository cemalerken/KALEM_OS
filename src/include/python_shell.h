#ifndef PYTHON_SHELL_H
#define PYTHON_SHELL_H

#include <stdint.h>
#include "gui.h"

/**
 * @file python_shell.h
 * @brief KALEM OS Python Shell
 * 
 * Bu modül, KALEM OS için Python kabuğu (PythonShell) sağlar.
 * Kullanıcıların komut satırından Python komutları çalıştırmasına
 * ve sistem yönetim işlevlerine erişmesine olanak tanır.
 */

/** Python shell renk yapılandırması */
typedef struct {
    uint8_t background_color;      // Arkaplan rengi
    uint8_t foreground_color;      // Yazı rengi
    uint8_t prompt_color;          // Komut istemi rengi
    uint8_t error_color;           // Hata mesajı rengi
    uint8_t success_color;         // Başarı mesajı rengi
    uint8_t highlight_color;       // Vurgulama rengi
    uint8_t comment_color;         // Yorum rengi
    uint8_t keyword_color;         // Anahtar kelime rengi
    uint8_t function_color;        // Fonksiyon adı rengi
    uint8_t string_color;          // String rengi
    uint8_t number_color;          // Sayı rengi
} python_shell_colors_t;

/** Python shell komut geçmişi yapısı */
typedef struct {
    char** commands;              // Komut listesi
    uint16_t capacity;            // Kapasite
    uint16_t count;               // Kayıtlı komut sayısı
    uint16_t current_index;       // Geçerli komut indeksi
} python_shell_history_t;

/** Python shell yapılandırması */
typedef struct {
    uint8_t show_line_numbers;     // Satır numaraları gösterilsin mi?
    uint8_t syntax_highlighting;   // Sözdizimi vurgulaması yapılsın mı?
    uint8_t auto_indent;           // Otomatik girinti yapılsın mı?
    uint8_t tab_complete;          // Tab ile tamamlama yapılsın mı?
    uint8_t save_history;          // Komut geçmişi kaydedilsin mi?
    uint16_t history_size;         // Komut geçmişi boyutu
    uint8_t show_suggestions;      // Öneri gösterilsin mi?
    uint8_t multiline_mode;        // Çok satırlı mod etkin mi?
    char history_file[256];        // Komut geçmişi dosyası
    python_shell_colors_t colors;  // Renk yapılandırması
} python_shell_config_t;

/**
 * Python kabuğunu başlatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_shell_init();

/**
 * Python kabuğunu gösterir
 */
void python_shell_show();

/**
 * Python kabuğunu gizler
 */
void python_shell_hide();

/**
 * Python kabuğunu temizler
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_shell_cleanup();

/**
 * Python kabuğu yapılandırmasını alır
 * 
 * @param config_out Yapılandırma bilgisi
 * @return int 0: başarılı, <0: hata
 */
int python_shell_get_config(python_shell_config_t* config_out);

/**
 * Python kabuğu yapılandırmasını ayarlar
 * 
 * @param config Yapılandırma bilgisi
 * @return int 0: başarılı, <0: hata
 */
int python_shell_set_config(const python_shell_config_t* config);

/**
 * Python kabuğunda komut çalıştırır
 * 
 * @param command Komut
 * @return int 0: başarılı, <0: hata
 */
int python_shell_execute(const char* command);

/**
 * Python kabuğuna metin ekler
 * 
 * @param text Metin
 * @param color Renk
 */
void python_shell_append_text(const char* text, uint8_t color);

/**
 * Python kabuğuna yeni satır ekler
 */
void python_shell_append_newline();

/**
 * Python kabuğunu temizler
 */
void python_shell_clear();

/**
 * Python kabuğu komut geçmişini kaydeder
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_shell_save_history();

/**
 * Python kabuğu komut geçmişini yükler
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_shell_load_history();

/**
 * Klavye olay işleyicisi
 * 
 * @param terminal Terminal
 * @param key Tuş kodu
 * @param modifier Değiştirici tuşlar
 */
void on_key_press(gui_terminal_t* terminal, int key, int modifier);

#endif /* PYTHON_SHELL_H */ 