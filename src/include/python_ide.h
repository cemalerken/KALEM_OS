#ifndef PYTHON_IDE_H
#define PYTHON_IDE_H

#include <stdint.h>
#include "gui.h"

/**
 * @file python_ide.h
 * @brief KALEM OS Python IDE
 * 
 * Bu modül, KALEM OS için Python geliştirme ortamı (IDE) sağlar.
 * Kullanıcıların Python betikleri yazmasına, düzenlemesine, 
 * hata ayıklamasına ve çalıştırmasına olanak tanır.
 */

/** Python IDE renk şeması */
typedef enum {
    PYTHON_IDE_THEME_LIGHT = 0,   // Açık tema
    PYTHON_IDE_THEME_DARK,        // Koyu tema
    PYTHON_IDE_THEME_BLUE,        // Mavi tema
    PYTHON_IDE_THEME_GREEN,       // Yeşil tema
    PYTHON_IDE_THEME_CUSTOM       // Özel tema
} python_ide_theme_t;

/** Python IDE renk yapılandırması */
typedef struct {
    uint8_t background_color;      // Arkaplan rengi
    uint8_t foreground_color;      // Yazı rengi
    uint8_t line_number_bg_color;  // Satır numarası arkaplan rengi
    uint8_t line_number_fg_color;  // Satır numarası yazı rengi
    uint8_t current_line_color;    // Geçerli satır vurgu rengi
    uint8_t error_line_color;      // Hata satırı vurgu rengi
    uint8_t breakpoint_color;      // Kesme noktası rengi
    uint8_t selection_color;       // Seçim rengi
    uint8_t toolbar_bg_color;      // Araç çubuğu arkaplan rengi
    uint8_t toolbar_fg_color;      // Araç çubuğu yazı rengi
    uint8_t terminal_bg_color;     // Terminal arkaplan rengi
    uint8_t terminal_fg_color;     // Terminal yazı rengi
    uint8_t hint_color;            // İpucu rengi
    uint8_t result_color;          // Sonuç rengi
} python_ide_colors_t;

/** Python IDE sözdizimi vurgulaması renkleri */
typedef struct {
    uint8_t keyword_color;         // Anahtar kelime rengi
    uint8_t function_color;        // Fonksiyon adı rengi
    uint8_t class_color;           // Sınıf adı rengi
    uint8_t string_color;          // String rengi
    uint8_t number_color;          // Sayı rengi
    uint8_t comment_color;         // Yorum rengi
    uint8_t decorator_color;       // Dekoratör rengi
    uint8_t operator_color;        // Operatör rengi
    uint8_t variable_color;        // Değişken rengi
    uint8_t builtin_color;         // Dahili fonksiyon rengi
    uint8_t constant_color;        // Sabit rengi
    uint8_t error_color;           // Hata rengi
} python_ide_syntax_colors_t;

/** Python IDE proje bilgisi */
typedef struct {
    char name[64];                // Proje adı
    char path[256];               // Proje yolu
    char main_file[256];          // Ana Python dosyası
    char venv_path[256];          // Sanal ortam yolu
    char requirements_file[256];  // Gereksinimler dosyası
    uint8_t use_venv;             // Sanal ortam kullanılsın mı?
} python_ide_project_t;

/** Python IDE yapılandırması */
typedef struct {
    uint8_t show_line_numbers;     // Satır numaraları gösterilsin mi?
    uint8_t syntax_highlighting;   // Sözdizimi vurgulaması yapılsın mı?
    uint8_t auto_indent;           // Otomatik girinti yapılsın mı?
    uint8_t auto_complete;         // Otomatik tamamlama yapılsın mı?
    uint8_t wrap_text;             // Metin kaydırılsın mı?
    uint8_t show_whitespace;       // Boşluk karakterleri gösterilsin mi?
    uint8_t highlight_current_line; // Geçerli satır vurgulansın mı?
    uint8_t auto_save;             // Otomatik kaydetme yapılsın mı?
    uint16_t auto_save_interval;   // Otomatik kaydetme aralığı (saniye)
    uint8_t tab_size;              // Tab genişliği
    uint8_t use_spaces_for_tab;    // Tab yerine boşluk kullanılsın mı?
    python_ide_theme_t theme;      // Tema
    python_ide_colors_t colors;    // Renk yapılandırması
    python_ide_syntax_colors_t syntax_colors; // Sözdizimi renkleri
} python_ide_config_t;

/** Python IDE hata bilgisi */
typedef struct {
    uint16_t line;                // Hata satırı
    uint16_t column;              // Hata sütunu
    char message[256];            // Hata mesajı
    char type[64];                // Hata tipi
    uint8_t severity;             // Hata şiddeti (0: bilgi, 1: uyarı, 2: hata)
} python_ide_error_t;

/**
 * Python IDE'yi başlatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_ide_init();

/**
 * Python IDE'yi gösterir
 */
void python_ide_show();

/**
 * Python IDE'yi temizler
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_ide_cleanup();

/**
 * Dosya açar
 * 
 * @param file_path Dosya yolu
 * @return int 0: başarılı, <0: hata
 */
int python_ide_open_file(const char* file_path);

/**
 * Geçerli dosyayı kaydeder
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_ide_save_file();

/**
 * Dosyayı farklı kaydeder
 * 
 * @param file_path Dosya yolu
 * @return int 0: başarılı, <0: hata
 */
int python_ide_save_as(const char* file_path);

/**
 * Geçerli dosyayı çalıştırır
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_ide_run_current_file();

/**
 * Geçerli dosyada hata ayıklama yapar
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_ide_debug_current_file();

/**
 * Python IDE yapılandırmasını alır
 * 
 * @param config_out Yapılandırma bilgisi
 * @return int 0: başarılı, <0: hata
 */
int python_ide_get_config(python_ide_config_t* config_out);

/**
 * Python IDE yapılandırmasını ayarlar
 * 
 * @param config Yapılandırma bilgisi
 * @return int 0: başarılı, <0: hata
 */
int python_ide_set_config(const python_ide_config_t* config);

/**
 * Proje açar
 * 
 * @param project_path Proje yolu
 * @return int 0: başarılı, <0: hata
 */
int python_ide_open_project(const char* project_path);

/**
 * Geçerli projeyi kapatır
 * 
 * @return int 0: başarılı, <0: hata
 */
int python_ide_close_project();

/**
 * Yeni proje oluşturur
 * 
 * @param project_name Proje adı
 * @param project_path Proje yolu
 * @param use_venv Sanal ortam kullanılsın mı?
 * @return int 0: başarılı, <0: hata
 */
int python_ide_create_project(const char* project_name, const char* project_path, uint8_t use_venv);

/**
 * Geçerli proje bilgisini alır
 * 
 * @param project_out Proje bilgisi
 * @return int 0: başarılı, <0: hata
 */
int python_ide_get_project(python_ide_project_t* project_out);

/**
 * Buton olay işleyicileri
 */
void on_run_button_clicked(gui_button_t* button, void* user_data);
void on_debug_button_clicked(gui_button_t* button, void* user_data);
void on_save_button_clicked(gui_button_t* button, void* user_data);
void on_open_button_clicked(gui_button_t* button, void* user_data);
void on_new_button_clicked(gui_button_t* button, void* user_data);
void on_find_button_clicked(gui_button_t* button, void* user_data);
void on_replace_button_clicked(gui_button_t* button, void* user_data);

/**
 * Python çıktı yakalama işleyicisi
 * 
 * @param output Çıktı
 */
void on_python_output(const char* output);

#endif /* PYTHON_IDE_H */ 