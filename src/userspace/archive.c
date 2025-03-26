#include "../include/archive.h"
#include "../include/gui.h"
#include "../include/font.h"
#include "../include/vga.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Aktif arşiv penceresi
static archive_window_t* active_archive_window = NULL;

// Varsayılan arşiv yolu
static const char* default_extract_path = "/home/user/extracted";

// Demo dosya listesi
static archive_file_t demo_files[] = {
    {"belge.txt", 1024, 512, 0, 12345678, 20230512},
    {"resim.png", 153600, 124928, 0, 12345679, 20230512},
    {"dosyalar/", 0, 0, 1, 12345680, 20230512},
    {"dosyalar/rapor.doc", 65536, 45056, 0, 12345681, 20230512},
    {"dosyalar/tablo.xls", 32768, 24576, 0, 12345682, 20230512},
    {"sunum.ppt", 262144, 212992, 0, 12345683, 20230512},
    {"kaynak/", 0, 0, 1, 12345684, 20230512},
    {"kaynak/README.md", 2048, 1024, 0, 12345685, 20230512},
    {"kaynak/main.c", 4096, 2048, 0, 12345686, 20230512},
    {"kaynak/helper.c", 3072, 1536, 0, 12345687, 20230512},
    {"kaynak/header.h", 1024, 512, 0, 12345688, 20230512},
    {"kaynak/makefile", 512, 256, 0, 12345689, 20230512}
};

// Arşiv uygulamasını başlat
void archive_init() {
    // İlk çalıştırma kontrolü
    if (active_archive_window == NULL) {
        active_archive_window = (archive_window_t*)malloc(sizeof(archive_window_t));
        if (!active_archive_window) return;
        
        memset(active_archive_window, 0, sizeof(archive_window_t));
        
        // Demo arşiv oluştur
        active_archive_window->archive = (archive_t*)malloc(sizeof(archive_t));
        if (!active_archive_window->archive) {
            free(active_archive_window);
            active_archive_window = NULL;
            return;
        }
        
        memset(active_archive_window->archive, 0, sizeof(archive_t));
        
        // Demo bilgileri doldur
        active_archive_window->archive->type = ARCHIVE_TYPE_ZIP;
        strcpy(active_archive_window->archive->path, "ornek_arsiv.zip");
        active_archive_window->archive->file_count = sizeof(demo_files) / sizeof(archive_file_t);
        active_archive_window->archive->files = demo_files;
        active_archive_window->archive->is_open = 1;
        
        // Varsayılan ayarlar
        active_archive_window->scroll_y = 0;
        active_archive_window->selected_index = 0;
        active_archive_window->show_details = 1;
        strcpy(active_archive_window->status_message, "12 öğe, toplam 525 KB");
        strcpy(active_archive_window->extract_path, default_extract_path);
    }
}

// Arşiv penceresini göster
void archive_show_window() {
    // Uygulama başlatılmadıysa başlat
    if (active_archive_window == NULL) {
        archive_init();
    }
    
    // Pencere zaten açıksa odakla
    if (active_archive_window->window) {
        gui_window_bring_to_front(active_archive_window->window);
        return;
    }
    
    // Pencereyi oluştur
    active_archive_window->window = gui_window_create("Arşiv Yöneticisi", 150, 100, 560, 380, GUI_WINDOW_STYLE_NORMAL);
    if (!active_archive_window->window) return;
    
    // Pencere kapatıldığında referansı temizleyecek kapanış fonksiyonu
    active_archive_window->window->on_close = [](gui_window_t* window) {
        if (active_archive_window) {
            active_archive_window->window = NULL;
        }
    };
    
    // Çizim fonksiyonunu ayarla
    active_archive_window->window->on_paint = archive_paint;
    
    // Pencereyi göster
    gui_window_show(active_archive_window->window);
}

// Arşiv açma fonksiyonu
archive_t* archive_open(const char* path) {
    if (!path) return NULL;
    
    // Bu demo sürümünde sadece mevcut arşiv döndür
    if (active_archive_window && active_archive_window->archive) {
        // Yolu güncelle
        strcpy(active_archive_window->archive->path, path);
        return active_archive_window->archive;
    }
    
    return NULL;
}

// Arşiv kapatma fonksiyonu
void archive_close(archive_t* archive) {
    if (!archive) return;
    
    // Demo sürümünde sadece is_open bayrağını temizle
    archive->is_open = 0;
}

// Bir arşivden dosya çıkarma fonksiyonu
int archive_extract_file(archive_t* archive, const char* filename, const char* destination) {
    if (!archive || !filename || !destination) return -1;
    
    // Demo sürümünde sadece başarı mesajı güncelle
    if (active_archive_window) {
        sprintf(active_archive_window->status_message, "'%s' dosyası '%s' konumuna çıkarıldı", 
                filename, destination);
    }
    
    return 0;
}

// Tüm arşivi çıkarma fonksiyonu
int archive_extract_all(archive_t* archive, const char* destination) {
    if (!archive || !destination) return -1;
    
    // Demo sürümünde sadece başarı mesajı güncelle
    if (active_archive_window) {
        sprintf(active_archive_window->status_message, "Tüm dosyalar '%s' konumuna çıkarıldı", 
                destination);
    }
    
    return 0;
}

// Arşive dosya ekleme fonksiyonu
int archive_add_file(archive_t* archive, const char* filepath) {
    if (!archive || !filepath) return -1;
    
    // Demo sürümünde sadece başarı mesajı güncelle
    if (active_archive_window) {
        sprintf(active_archive_window->status_message, "'%s' dosyası arşive eklendi", 
                filepath);
    }
    
    return 0;
}

// Yeni arşiv oluşturma fonksiyonu
archive_t* archive_create(const char* path, archive_type_t type) {
    if (!path) return NULL;
    
    // Demo sürümünde mevcut arşivi yeni yol ve tür ile güncelle
    if (active_archive_window && active_archive_window->archive) {
        strcpy(active_archive_window->archive->path, path);
        active_archive_window->archive->type = type;
        active_archive_window->archive->is_open = 1;
        
        sprintf(active_archive_window->status_message, "Yeni arşiv oluşturuldu: %s", path);
        
        return active_archive_window->archive;
    }
    
    return NULL;
}

// Dosya uzantısından arşiv türünü belirle
archive_type_t archive_get_type_from_extension(const char* path) {
    if (!path) return ARCHIVE_TYPE_UNKNOWN;
    
    const char* ext = strrchr(path, '.');
    if (!ext) return ARCHIVE_TYPE_UNKNOWN;
    
    if (strcasecmp(ext, ".zip") == 0) return ARCHIVE_TYPE_ZIP;
    if (strcasecmp(ext, ".rar") == 0) return ARCHIVE_TYPE_RAR;
    if (strcasecmp(ext, ".tar") == 0) return ARCHIVE_TYPE_TAR;
    if (strcasecmp(ext, ".gz") == 0 || strcasecmp(ext, ".tgz") == 0) return ARCHIVE_TYPE_GZ;
    
    return ARCHIVE_TYPE_UNKNOWN;
}

// Arşiv türünden string elde et
const char* archive_get_type_string(archive_type_t type) {
    switch (type) {
        case ARCHIVE_TYPE_ZIP: return "ZIP Arşivi";
        case ARCHIVE_TYPE_RAR: return "RAR Arşivi";
        case ARCHIVE_TYPE_TAR: return "TAR Arşivi";
        case ARCHIVE_TYPE_GZ: return "GZip Arşivi";
        default: return "Bilinmeyen Arşiv";
    }
}

// Dosya boyutunu insanların okuyabileceği formata dönüştür
void format_size(uint32_t size, char* buffer, size_t buffer_size) {
    if (size < 1024) {
        snprintf(buffer, buffer_size, "%u B", size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.1f KB", (float)size / 1024);
    } else {
        snprintf(buffer, buffer_size, "%.1f MB", (float)size / (1024 * 1024));
    }
}

// Tarih formatını dönüştür
void format_date(uint32_t date, char* buffer, size_t buffer_size) {
    uint32_t year = (date / 10000);
    uint32_t month = (date / 100) % 100;
    uint32_t day = date % 100;
    
    snprintf(buffer, buffer_size, "%02u/%02u/%04u", day, month, year);
}

// Arşiv dosya listesini çiz
void archive_draw_file_list(archive_window_t* archive_win) {
    if (!archive_win || !archive_win->window || !archive_win->archive) return;
    
    gui_window_t* window = archive_win->window;
    archive_t* archive = archive_win->archive;
    
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    uint32_t width = window->client.width;
    uint32_t height = window->client.height;
    
    // Detay paneli genişliği
    uint32_t details_width = archive_win->show_details ? 180 : 0;
    
    // Liste arkaplanı
    vga_fill_rect(x, y + 30, width - details_width, height - 50, GUI_COLOR_WHITE);
    
    // Liste başlığı
    vga_fill_rect(x, y + 30, width - details_width, 20, GUI_COLOR_LIGHT_GRAY);
    vga_draw_hline(x, y + 50, width - details_width, GUI_COLOR_DARK_GRAY);
    
    // Başlık metinleri
    font_draw_string(x + 10, y + 35, "Dosya Adı", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 250, y + 35, "Boyut", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 320, y + 35, "Sık. Boyut", GUI_COLOR_BLACK, 0xFF);
    font_draw_string(x + 390, y + 35, "Tarih", GUI_COLOR_BLACK, 0xFF);
    
    // Kaydırma çubuğu arkaplanı
    vga_fill_rect(x + width - details_width - 10, y + 50, 10, height - 70, GUI_COLOR_WINDOW_BG);
    vga_draw_rect(x + width - details_width - 10, y + 50, 10, height - 70, GUI_COLOR_DARK_GRAY);
    
    // Dosya listesi
    if (archive->file_count == 0) {
        font_draw_string(x + 10, y + 60, "Arşivde dosya bulunamadı.", GUI_COLOR_BLACK, 0xFF);
        return;
    }
    
    // Görünür satır sayısını hesapla
    uint32_t line_height = system_font->height + 4;
    uint32_t visible_lines = (height - 70) / line_height;
    
    // Kaydırma kontrolü
    if (archive_win->scroll_y > archive->file_count - visible_lines && archive->file_count > visible_lines) {
        archive_win->scroll_y = archive->file_count - visible_lines;
    }
    
    // Kaydırma çubuğu gezdirici
    if (archive->file_count > visible_lines) {
        uint32_t scrollbar_height = height - 70;
        uint32_t thumb_height = scrollbar_height * visible_lines / archive->file_count;
        if (thumb_height < 10) thumb_height = 10;
        
        uint32_t thumb_pos = scrollbar_height * archive_win->scroll_y / archive->file_count;
        if (thumb_pos > scrollbar_height - thumb_height) {
            thumb_pos = scrollbar_height - thumb_height;
        }
        
        vga_fill_rect(x + width - details_width - 9, y + 51 + thumb_pos, 8, thumb_height, GUI_COLOR_BUTTON);
    }
    
    // Dosyaları listele
    uint32_t line_y = y + 52;
    for (uint32_t i = archive_win->scroll_y; i < archive->file_count && i < archive_win->scroll_y + visible_lines; i++) {
        archive_file_t* file = &archive->files[i];
        
        // Seçili dosya arkaplanı
        if (i == archive_win->selected_index) {
            vga_fill_rect(x, line_y, width - details_width - 10, line_height, GUI_COLOR_SELECTED);
        }
        
        // Dosya adı
        uint8_t text_color = (i == archive_win->selected_index) ? GUI_COLOR_WHITE : GUI_COLOR_BLACK;
        
        if (file->is_directory) {
            font_draw_string(x + 10, line_y + 2, file->name, GUI_COLOR_BLUE, 0xFF);
        } else {
            font_draw_string(x + 10, line_y + 2, file->name, text_color, 0xFF);
        }
        
        // Boyut
        if (!file->is_directory) {
            char size_str[16];
            format_size(file->size, size_str, sizeof(size_str));
            font_draw_string(x + 250, line_y + 2, size_str, text_color, 0xFF);
            
            // Sıkıştırılmış boyut
            format_size(file->compressed_size, size_str, sizeof(size_str));
            font_draw_string(x + 320, line_y + 2, size_str, text_color, 0xFF);
        }
        
        // Tarih
        char date_str[16];
        format_date(file->date, date_str, sizeof(date_str));
        font_draw_string(x + 390, line_y + 2, date_str, text_color, 0xFF);
        
        line_y += line_height;
    }
}

// Araç çubuğunu çiz
void archive_draw_toolbar(archive_window_t* archive_win) {
    if (!archive_win || !archive_win->window) return;
    
    gui_window_t* window = archive_win->window;
    
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    uint32_t width = window->client.width;
    
    // Araç çubuğu arkaplanı
    vga_fill_rect(x, y, width, 30, GUI_COLOR_LIGHT_GRAY);
    
    // Araç çubuğu ayırıcı çizgisi
    vga_draw_hline(x, y + 29, width, GUI_COLOR_DARK_GRAY);
    
    // Butonlar
    const char* button_labels[] = {"Aç", "Oluştur", "Çıkar", "Tümünü Çıkar", "Ekle", "Sil", "Detaylar"};
    uint32_t button_x = x + 5;
    
    for (int i = 0; i < 7; i++) {
        uint32_t btn_width = strlen(button_labels[i]) * 8 + 10;
        
        vga_fill_rect(button_x, y + 5, btn_width, 20, GUI_COLOR_BUTTON);
        
        // 3D kenarlar
        vga_draw_hline(button_x, y + 5, btn_width, GUI_COLOR_WHITE);
        vga_draw_vline(button_x, y + 5, 20, GUI_COLOR_WHITE);
        vga_draw_hline(button_x, y + 24, btn_width, GUI_COLOR_DARK_GRAY);
        vga_draw_vline(button_x + btn_width - 1, y + 5, 20, GUI_COLOR_DARK_GRAY);
        
        // Buton metni
        font_draw_string(button_x + 5, y + 8, button_labels[i], GUI_COLOR_BLACK, 0xFF);
        
        button_x += btn_width + 5;
    }
}

// Durum çubuğunu çiz
void archive_draw_statusbar(archive_window_t* archive_win) {
    if (!archive_win || !archive_win->window) return;
    
    gui_window_t* window = archive_win->window;
    
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    uint32_t width = window->client.width;
    uint32_t height = window->client.height;
    
    // Durum çubuğu arkaplanı
    vga_fill_rect(x, y + height - 20, width, 20, GUI_COLOR_LIGHT_GRAY);
    
    // Durum çubuğu ayırıcı çizgisi
    vga_draw_hline(x, y + height - 20, width, GUI_COLOR_DARK_GRAY);
    
    // Durum mesajı
    font_draw_string(x + 10, y + height - 15, archive_win->status_message, GUI_COLOR_BLACK, 0xFF);
    
    // Arşiv yolu
    if (archive_win->archive && archive_win->archive->is_open) {
        char path_info[64];
        sprintf(path_info, "Arşiv: %s", archive_win->archive->path);
        font_draw_string(x + width - 200, y + height - 15, path_info, GUI_COLOR_DARK_BLUE, 0xFF);
    }
}

// Dosya detay panelini çiz
void archive_draw_details(archive_window_t* archive_win) {
    if (!archive_win || !archive_win->window || !archive_win->show_details) return;
    
    gui_window_t* window = archive_win->window;
    archive_t* archive = archive_win->archive;
    
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    uint32_t width = window->client.width;
    uint32_t height = window->client.height;
    
    // Detay paneli genişliği
    uint32_t details_width = 180;
    uint32_t details_x = x + width - details_width;
    
    // Detay paneli arkaplanı
    vga_fill_rect(details_x, y + 30, details_width, height - 50, GUI_COLOR_LIGHT_GRAY);
    
    // Detay paneli ayırıcı çizgisi
    vga_draw_vline(details_x, y + 30, height - 50, GUI_COLOR_DARK_GRAY);
    
    // Başlık
    font_draw_string(details_x + 10, y + 40, "Arşiv Detayları", GUI_COLOR_BLACK, 0xFF);
    vga_draw_hline(details_x, y + 55, details_width, GUI_COLOR_DARK_GRAY);
    
    // Arşiv bilgileri
    if (archive && archive->is_open) {
        // Arşiv türü
        font_draw_string(details_x + 10, y + 65, "Tür:", GUI_COLOR_BLACK, 0xFF);
        font_draw_string(details_x + 50, y + 65, archive_get_type_string(archive->type), GUI_COLOR_DARK_BLUE, 0xFF);
        
        // Dosya sayısı
        char count_str[32];
        sprintf(count_str, "Dosya sayısı: %u", archive->file_count);
        font_draw_string(details_x + 10, y + 85, count_str, GUI_COLOR_BLACK, 0xFF);
        
        // Toplam boyut
        uint32_t total_size = 0;
        uint32_t total_compressed = 0;
        for (uint32_t i = 0; i < archive->file_count; i++) {
            total_size += archive->files[i].size;
            total_compressed += archive->files[i].compressed_size;
        }
        
        char size_str[32];
        font_draw_string(details_x + 10, y + 105, "Toplam boyut:", GUI_COLOR_BLACK, 0xFF);
        format_size(total_size, size_str, sizeof(size_str));
        font_draw_string(details_x + 10, y + 120, size_str, GUI_COLOR_DARK_BLUE, 0xFF);
        
        font_draw_string(details_x + 10, y + 140, "Sık. boyut:", GUI_COLOR_BLACK, 0xFF);
        format_size(total_compressed, size_str, sizeof(size_str));
        font_draw_string(details_x + 10, y + 155, size_str, GUI_COLOR_DARK_BLUE, 0xFF);
        
        // Sıkıştırma oranı
        if (total_size > 0) {
            float ratio = (float)total_compressed / total_size * 100.0f;
            char ratio_str[32];
            sprintf(ratio_str, "Oran: %.1f%%", ratio);
            font_draw_string(details_x + 10, y + 175, ratio_str, GUI_COLOR_BLACK, 0xFF);
        }
        
        // Seçili dosya bilgileri
        if (archive_win->selected_index < archive->file_count) {
            archive_file_t* file = &archive->files[archive_win->selected_index];
            
            vga_draw_hline(details_x, y + 200, details_width, GUI_COLOR_DARK_GRAY);
            font_draw_string(details_x + 10, y + 215, "Seçili Dosya", GUI_COLOR_BLACK, 0xFF);
            
            // Dosya adı
            font_draw_string(details_x + 10, y + 235, "Adı:", GUI_COLOR_BLACK, 0xFF);
            
            // Uzun dosya adlarını kısalt
            char short_name[20] = {0};
            if (strlen(file->name) > 18) {
                strncpy(short_name, file->name, 15);
                strcat(short_name, "...");
                font_draw_string(details_x + 10, y + 250, short_name, GUI_COLOR_DARK_BLUE, 0xFF);
            } else {
                font_draw_string(details_x + 10, y + 250, file->name, GUI_COLOR_DARK_BLUE, 0xFF);
            }
            
            // Dosya boyutu
            if (!file->is_directory) {
                char file_size[16];
                format_size(file->size, file_size, sizeof(file_size));
                font_draw_string(details_x + 10, y + 270, "Boyut:", GUI_COLOR_BLACK, 0xFF);
                font_draw_string(details_x + 10, y + 285, file_size, GUI_COLOR_DARK_BLUE, 0xFF);
            } else {
                font_draw_string(details_x + 10, y + 270, "Tür:", GUI_COLOR_BLACK, 0xFF);
                font_draw_string(details_x + 10, y + 285, "Klasör", GUI_COLOR_DARK_BLUE, 0xFF);
            }
            
            // Dosya tarihi
            char date_str[16];
            format_date(file->date, date_str, sizeof(date_str));
            font_draw_string(details_x + 10, y + 305, "Tarih:", GUI_COLOR_BLACK, 0xFF);
            font_draw_string(details_x + 10, y + 320, date_str, GUI_COLOR_DARK_BLUE, 0xFF);
        }
    }
}

// Arşiv penceresini çiz
void archive_paint(gui_window_t* window) {
    if (!window || !active_archive_window || active_archive_window->window != window) return;
    
    // Araç çubuğunu çiz
    archive_draw_toolbar(active_archive_window);
    
    // Dosya listesini çiz
    archive_draw_file_list(active_archive_window);
    
    // Detay panelini çiz
    archive_draw_details(active_archive_window);
    
    // Durum çubuğunu çiz
    archive_draw_statusbar(active_archive_window);
}

// Arşiv uygulamasının ana fonksiyonu
void app_archive() {
    archive_show_window();
} 