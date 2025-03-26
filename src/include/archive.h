#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include "gui.h"
#include <stdint.h>

// Arşiv dosya türleri
typedef enum {
    ARCHIVE_TYPE_UNKNOWN = 0,
    ARCHIVE_TYPE_ZIP,
    ARCHIVE_TYPE_RAR,
    ARCHIVE_TYPE_TAR,
    ARCHIVE_TYPE_GZ
} archive_type_t;

// Arşiv içindeki dosya bilgisi
typedef struct {
    char name[256];
    uint32_t size;
    uint32_t compressed_size;
    uint8_t is_directory;
    uint32_t time;
    uint32_t date;
} archive_file_t;

// Arşiv yapısı
typedef struct {
    archive_type_t type;
    char path[256];
    uint32_t file_count;
    archive_file_t* files;
    uint8_t is_open;
} archive_t;

// Arşiv penceresini temsil eden yapı
typedef struct {
    gui_window_t* window;
    archive_t* archive;
    uint32_t scroll_y;
    uint32_t selected_index;
    uint8_t show_details;
    char status_message[256];
    char extract_path[256];
} archive_window_t;

// Arşiv uygulamasını başlat
void archive_init();

// Arşiv uygulamasını göster
void archive_show_window();

// Arşiv açma fonksiyonu
archive_t* archive_open(const char* path);

// Arşiv kapatma fonksiyonu
void archive_close(archive_t* archive);

// Bir arşivden dosya çıkarma fonksiyonu
int archive_extract_file(archive_t* archive, const char* filename, const char* destination);

// Tüm arşivi çıkarma fonksiyonu
int archive_extract_all(archive_t* archive, const char* destination);

// Arşive dosya ekleme fonksiyonu
int archive_add_file(archive_t* archive, const char* filepath);

// Yeni arşiv oluşturma fonksiyonu
archive_t* archive_create(const char* path, archive_type_t type);

// Arşiv penceresini çizme fonksiyonu
void archive_paint(gui_window_t* window);

// Arşiv dosya listesini çiz
void archive_draw_file_list(archive_window_t* archive_win);

// Araç çubuğunu çiz
void archive_draw_toolbar(archive_window_t* archive_win);

// Durum çubuğunu çiz
void archive_draw_statusbar(archive_window_t* archive_win);

// Dosya detay panelini çiz
void archive_draw_details(archive_window_t* archive_win);

// Arşiv uygulamasının ana fonksiyonu
void app_archive();

#endif /* __ARCHIVE_H__ */ 