#ifndef KALEMOS_NOTEPLUS_H
#define KALEMOS_NOTEPLUS_H

#include <stdint.h>
#include "../include/gui.h"

// Maksimum dosya boyutu
#define NOTEPLUS_MAX_TEXT_SIZE 8192

// Maksimum satır sayısı
#define NOTEPLUS_MAX_LINES 200

// Not++ uygulaması pencere yapısı
typedef struct {
    gui_window_t* window;       // Pencere referansı
    char* text_buffer;          // Metin tamponu
    uint32_t buffer_size;       // Tamponda mevcut metin boyutu
    uint32_t cursor_pos;        // İmleç pozisyonu (karakter indeksi)
    uint32_t cursor_line;       // İmleç satır numarası
    uint32_t cursor_col;        // İmleç sütun numarası
    uint32_t scroll_y;          // Dikey kaydırma pozisyonu (satır)
    uint32_t scroll_x;          // Yatay kaydırma pozisyonu (karakter)
    uint32_t visible_lines;     // Görünür satır sayısı
    uint32_t visible_cols;      // Görünür sütun sayısı
    uint8_t modified;           // Değişiklik durumu
    char current_file[256];     // Açılan dosya adı
    uint8_t show_line_numbers;  // Satır numaralarını göster
    uint8_t syntax_highlight;   // Sözdizimi vurgulama
} noteplus_editor_t;

// Sözdizimi vurgulama türleri
typedef enum {
    NOTEPLUS_TOKEN_NORMAL,
    NOTEPLUS_TOKEN_KEYWORD,
    NOTEPLUS_TOKEN_STRING,
    NOTEPLUS_TOKEN_NUMBER,
    NOTEPLUS_TOKEN_COMMENT,
    NOTEPLUS_TOKEN_PREPROCESSOR
} noteplus_token_type_t;

// Not++ uygulamasını başlat
void noteplus_init();

// Not++ penceresini göster
void noteplus_show_window();

// Not++ uygulama penceresini çiz
void noteplus_paint(gui_window_t* window);

// Düzenleyici yapısını oluştur
noteplus_editor_t* noteplus_create_editor();

// Düzenleyici yapısını yok et
void noteplus_destroy_editor(noteplus_editor_t* editor);

// Kaydırma çubuğunu çiz
void noteplus_draw_scrollbar(noteplus_editor_t* editor);

// Metin alanını çiz
void noteplus_draw_text_area(noteplus_editor_t* editor);

// Araç çubuğunu çiz
void noteplus_draw_toolbar(noteplus_editor_t* editor);

// Durum çubuğunu çiz
void noteplus_draw_statusbar(noteplus_editor_t* editor);

// Metni ekle
void noteplus_insert_text(noteplus_editor_t* editor, const char* text);

// Karakter ekle
void noteplus_insert_char(noteplus_editor_t* editor, char c);

// Karakteri sil
void noteplus_delete_char(noteplus_editor_t* editor);

// İmleci taşı
void noteplus_move_cursor(noteplus_editor_t* editor, int delta_x, int delta_y);

// İmleç pozisyonunu ayarla
void noteplus_set_cursor(noteplus_editor_t* editor, uint32_t line, uint32_t col);

// Satırı al
const char* noteplus_get_line(noteplus_editor_t* editor, uint32_t line);

// Satır uzunluğunu al
uint32_t noteplus_get_line_length(noteplus_editor_t* editor, uint32_t line);

// Toplam satır sayısını al
uint32_t noteplus_get_line_count(noteplus_editor_t* editor);

// Açık noteplus editörünü al
noteplus_editor_t* noteplus_get_active_editor();

#endif // KALEMOS_NOTEPLUS_H 