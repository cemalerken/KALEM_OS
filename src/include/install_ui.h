/**
 * KALEM OS - Kurulum Arayüzü
 * Şık ve modern bir kurulum arayüzü sağlar.
 */

#ifndef INSTALL_UI_H
#define INSTALL_UI_H

#include "gui.h"
#include <stdint.h>

// Eksik tipleri tanımla
#ifndef GUI_TYPES_DEFINED
#define GUI_TYPES_DEFINED
// Mevcut GUI türlerini kullan
typedef struct gui_window gui_window_t;
typedef struct gui_client_area gui_client_area_t;
// Kurulum arayüzü için gerekli türler
typedef struct gui_layout gui_layout_t;
typedef struct gui_image gui_image_t;
typedef struct gui_label gui_label_t;
typedef struct gui_progress_bar gui_progress_bar_t;
typedef struct gui_button gui_button_t;
typedef struct gui_checkbox gui_checkbox_t;
typedef struct gui_listview gui_listview_t;
typedef struct gui_radio_button gui_radio_button_t;
typedef struct gui_timer gui_timer_t;
typedef struct gui_rect gui_rect_t;
typedef struct gui_event gui_event_t;
#endif

// GUI kurulum sabitlerini tanımla
#define GUI_WINDOW_STYLE_FULLSCREEN   0x80 // gui.h'deki mevcut pencere stillerine ek olarak
#define GUI_LAYOUT_VERTICAL           0x01
#define GUI_LAYOUT_HORIZONTAL         0x02
#define GUI_ALIGN_CENTER              0x01

// Kurulum aşamaları
typedef enum {
    INSTALL_STAGE_WELCOME,         // Karşılama ekranı
    INSTALL_STAGE_DISCLAIMER,      // Sorumluluk beyanı
    INSTALL_STAGE_DISK_SELECT,     // Disk seçimi
    INSTALL_STAGE_PARTITIONING,    // Bölümlendirme
    INSTALL_STAGE_INSTALLING,      // Kurulum aşaması
    INSTALL_STAGE_COMPLETE         // Tamamlanma ekranı
} install_stage_t;

// Disk formatı
typedef enum {
    DISK_FORMAT_GPT,
    DISK_FORMAT_MBR
} disk_format_t;

// Disk türü
typedef enum {
    DISK_TYPE_HDD,
    DISK_TYPE_SSD,
    DISK_TYPE_M2_SSD,
    DISK_TYPE_USB
} disk_type_t;

// Disk yapısı
typedef struct {
    char device_name[32];      // /dev/sda gibi
    char model[64];            // Model adı
    uint64_t size_bytes;       // Boyut (byte)
    disk_type_t type;          // Disk türü
    uint8_t is_bootable;       // Önyüklenebilir mi?
    uint8_t has_os;            // İşletim sistemi içeriyor mu?
} install_disk_t;

// Disk bölümü
typedef struct {
    char name[32];             // /dev/sda1 gibi
    uint64_t size_bytes;       // Boyut (byte)
    uint64_t start_sector;     // Başlangıç sektörü
    uint64_t end_sector;       // Bitiş sektörü
    char fs_type[16];          // Dosya sistemi türü
    char mount_point[32];      // Bağlama noktası
    uint8_t is_bootable;       // Önyüklenebilir mi?
    uint8_t is_system;         // Sistem bölümü mü?
} disk_partition_t;

// Kurulum durum bilgisi
typedef struct {
    install_stage_t current_stage;         // Mevcut aşama
    install_disk_t* selected_disk;         // Seçilen disk
    disk_format_t selected_format;         // Seçilen format (GPT/MBR)
    disk_partition_t* partitions;          // Bölümler
    uint32_t partition_count;              // Bölüm sayısı
    uint8_t format_entire_disk;            // Tüm diski formatla
    uint8_t install_progress;              // Kurulum ilerleme yüzdesi
    char current_operation[128];           // Mevcut işlem
    uint8_t live_mode;                     // Canlı mod
} install_state_t;

// Kurulum arayüzü öğeleri
typedef struct {
    gui_window_t* main_window;             // Ana pencere
    gui_layout_t* main_layout;             // Ana düzen
    gui_image_t* logo_image;               // Logo
    gui_label_t* title_label;              // Başlık
    gui_label_t* subtitle_label;           // Alt başlık
    gui_label_t* status_label;             // Durum metni
    gui_progress_bar_t* progress_bar;      // İlerleme çubuğu
    gui_button_t* next_button;             // İleri butonu
    gui_button_t* back_button;             // Geri butonu
    gui_button_t* cancel_button;           // İptal butonu
    
    // Aşamaya özgü bileşenler
    union {
        // Karşılama ekranı
        struct {
            gui_button_t* install_button;  // Kuruluma başla
            gui_button_t* live_button;     // Canlı mod
        } welcome;
        
        // Sorumluluk beyanı
        struct {
            gui_checkbox_t* accept_checkbox; // Kabul onay kutusu
        } disclaimer;
        
        // Disk seçimi
        struct {
            gui_listview_t* disk_list;     // Disk listesi
        } disk_select;
        
        // Bölümlendirme
        struct {
            gui_radio_button_t* gpt_radio; // GPT radyo düğmesi
            gui_radio_button_t* mbr_radio; // MBR radyo düğmesi
            gui_checkbox_t* entire_disk_checkbox; // Tüm diski kullan
            gui_button_t* add_partition_button;   // Bölüm ekle
            gui_button_t* edit_partition_button;  // Bölüm düzenle
            gui_button_t* delete_partition_button; // Bölüm sil
            gui_listview_t* partition_list; // Bölüm listesi
        } partitioning;
    };
} install_ui_t;

// GUI event yapısı
typedef struct gui_event {
    int type;
    union {
        struct {
            int id;
        } button;
        struct {
            int code;
        } key;
    };
} gui_event_t;

// GUI rect yapısı
typedef struct gui_rect {
    int x;
    int y;
    int width;
    int height;
} gui_rect_t;

// Global kurulum durumu
extern install_state_t install_state;

// API fonksiyonları
void install_ui_init(void);
void install_ui_show(void);
void install_ui_close(void);
void install_ui_update_progress(uint8_t progress, const char* operation);
void install_ui_complete(void);
void install_ui_error(const char* error_message);

#endif /* INSTALL_UI_H */ 