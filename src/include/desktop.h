#ifndef _DESKTOP_H_
#define _DESKTOP_H_

#include "gui.h"

// Masaüstü sabitleri
#define DESKTOP_MAX_ICONS              100
#define DESKTOP_ICON_WIDTH             64
#define DESKTOP_ICON_HEIGHT            64
#define DESKTOP_ICON_LABEL_HEIGHT      20
#define DESKTOP_ICON_SPACING_X         80
#define DESKTOP_ICON_SPACING_Y         100
#define DESKTOP_MARGIN_X               20
#define DESKTOP_MARGIN_Y               20
#define MAX_ICON_NAME_LENGTH           32
#define MAX_ICON_PATH_LENGTH           256
#define MAX_CLIPBOARD_ITEMS            10
#define MAX_WALLPAPER_PATH_LENGTH      256
#define MAX_SLIDESHOW_IMAGES           10
#define WALLPAPER_CHANGE_INTERVAL      60 // Saniye cinsinden duvar kağıdı değişim süresi

// Masaüstü sürükle bırak sabitleri
#define DESKTOP_DND_MIN_DRAG_DISTANCE  5
#define DESKTOP_DND_ANIMATION_FRAMES   10

// Masaüstü simge tipleri
typedef enum {
    DESKTOP_ICON_TYPE_FILE = 0,
    DESKTOP_ICON_TYPE_FOLDER,
    DESKTOP_ICON_TYPE_APPLICATION,
    DESKTOP_ICON_TYPE_SHORTCUT,
    DESKTOP_ICON_TYPE_DRIVE
} desktop_icon_type_t;

// Masaüstü düzen tipleri
typedef enum {
    DESKTOP_LAYOUT_FREE = 0,     // Simgeler serbest yerleştirilebilir
    DESKTOP_LAYOUT_GRID,         // Simgeler ızgara üzerinde
    DESKTOP_LAYOUT_AUTO          // Simgeler otomatik düzenlenir
} desktop_layout_t;

// Masaüstü arkaplan modları
typedef enum {
    DESKTOP_BG_MODE_SOLID = 0,   // Düz renk
    DESKTOP_BG_MODE_WALLPAPER,   // Tek duvar kağıdı
    DESKTOP_BG_MODE_SLIDESHOW,   // Slayt gösterisi
    DESKTOP_BG_MODE_ANIMATED     // Animasyonlu arkaplan
} desktop_background_mode_t;

// Arkaplan ölçeklendirme modu
typedef enum {
    DESKTOP_BG_SCALE_STRETCH = 0, // Ekrana sığdır
    DESKTOP_BG_SCALE_FIT,         // En boy oranını koru, sığdır
    DESKTOP_BG_SCALE_FILL,        // En boy oranını koru, doldur
    DESKTOP_BG_SCALE_TILE        // Döşe
} desktop_background_scale_t;

// Masaüstü simge yapısı
typedef struct {
    desktop_icon_type_t type;        // Simge tipi
    char name[MAX_ICON_NAME_LENGTH]; // Simge adı
    char path[MAX_ICON_PATH_LENGTH]; // Dosya/uygulama yolu
    uint16_t x;                      // Ekran X konumu
    uint16_t y;                      // Ekran Y konumu
    uint16_t width;                  // Genişlik
    uint16_t height;                 // Yükseklik
    uint8_t visible;                 // Görünürlük
    uint8_t selected;                // Seçili mi?
    uint8_t highlighted;             // Vurgulanmış mı?
    uint8_t icon_id;                 // Simge ID
    uint32_t icon_color;             // Simge rengi
    uint8_t draggable;               // Sürüklenebilir mi?

    // Fare olayı işleyicileri
    void (*on_click)(struct desktop_icon* icon, uint16_t x, uint16_t y);
    void (*on_double_click)(struct desktop_icon* icon, uint16_t x, uint16_t y);
    void (*on_right_click)(struct desktop_icon* icon, uint16_t x, uint16_t y);
    void (*on_drag_start)(struct desktop_icon* icon, uint16_t x, uint16_t y);
    void (*on_drag_end)(struct desktop_icon* icon, uint16_t x, uint16_t y);
} desktop_icon_t;

// Masaüstü arkaplan yapısı
typedef struct {
    desktop_background_mode_t mode;    // Arkaplan modu
    desktop_background_scale_t scale;  // Ölçeklendirme modu
    
    // Düz renk
    uint32_t bg_color;
    
    // Duvar kağıdı
    char wallpaper_path[MAX_WALLPAPER_PATH_LENGTH];
    uint16_t* wallpaper_data;
    uint16_t wallpaper_width;
    uint16_t wallpaper_height;
    
    // Slayt gösterisi
    char slideshow_paths[MAX_SLIDESHOW_IMAGES][MAX_WALLPAPER_PATH_LENGTH];
    uint8_t slideshow_count;
    uint8_t slideshow_current;
    uint32_t slideshow_timer;
    
    // Animasyonlu arkaplan
    uint8_t animation_enabled;
    uint8_t animation_frame;
    uint8_t animation_total_frames;
    uint32_t animation_timer;
    void (*animation_update)(void);
} desktop_background_t;

// Masaüstü seçim alanı
typedef struct {
    uint16_t start_x;   // Başlangıç X
    uint16_t start_y;   // Başlangıç Y
    uint16_t end_x;     // Bitiş X
    uint16_t end_y;     // Bitiş Y
    uint8_t active;     // Aktif mi?
} desktop_selection_t;

// Masaüstü yapısı
typedef struct {
    desktop_layout_t layout;         // Masaüstü düzeni
    desktop_background_t background; // Arkaplan
    
    // Simgeler
    desktop_icon_t icons[DESKTOP_MAX_ICONS];
    uint16_t icon_count;
    
    // Seçim ve sürükleme
    desktop_selection_t selection;
    desktop_icon_t* dragged_icon;
    uint16_t drag_start_x;
    uint16_t drag_start_y;
    uint16_t drag_current_x;
    uint16_t drag_current_y;
    uint8_t drag_active;
    
    // Pano
    desktop_icon_t clipboard_icons[MAX_CLIPBOARD_ITEMS];
    uint8_t clipboard_count;
    uint8_t clipboard_is_cut;
    
    // Düzenleme modu
    uint8_t edit_mode;
} desktop_t;

// Global masaüstü
extern desktop_t* desktop;

// Masaüstü yönetimi için fonksiyonlar
void desktop_init();
void desktop_draw();
void desktop_cleanup();
uint8_t desktop_handle_mouse(uint16_t x, uint16_t y, uint8_t button, uint8_t state);
uint8_t desktop_handle_keyboard(uint8_t key, uint8_t state);
void desktop_auto_arrange_icons();
void desktop_select_all_icons();
void desktop_deselect_all_icons();
void desktop_invert_selection();
void desktop_begin_selection(uint16_t x, uint16_t y);
void desktop_update_selection(uint16_t x, uint16_t y);
void desktop_end_selection();
void desktop_set_layout(desktop_layout_t layout);
void desktop_edit_mode(uint8_t enabled);
void desktop_arrange_icons();

// Masaüstü arkaplan yönetimi
void desktop_set_background_color(uint32_t color);
void desktop_set_wallpaper(const char* path);
void desktop_set_wallpaper_mode(desktop_background_mode_t mode);
void desktop_set_wallpaper_scale(desktop_background_scale_t scale);
void desktop_add_slideshow_image(const char* path);
void desktop_clear_slideshow();
void desktop_update_background();
void desktop_set_animated_background(void (*update_func)(void));

// Masaüstü simge yönetimi
desktop_icon_t* desktop_add_icon(const char* name, const char* path, desktop_icon_type_t type, uint8_t icon_id);
void desktop_remove_icon(desktop_icon_t* icon);
void desktop_remove_icon_by_index(uint16_t index);
void desktop_update_icon(desktop_icon_t* icon);
void desktop_set_icon_position(desktop_icon_t* icon, uint16_t x, uint16_t y);
desktop_icon_t* desktop_find_icon_at(uint16_t x, uint16_t y);
desktop_icon_t* desktop_find_icon_by_name(const char* name);
desktop_icon_t* desktop_find_icon_by_path(const char* path);
void desktop_show_icon_context_menu(desktop_icon_t* icon, uint16_t x, uint16_t y);

// Masaüstü sürükle bırak yönetimi
void desktop_begin_drag(desktop_icon_t* icon, uint16_t x, uint16_t y);
void desktop_update_drag(uint16_t x, uint16_t y);
void desktop_end_drag(uint16_t x, uint16_t y);
void desktop_cancel_drag();

// Masaüstü pano yönetimi
void desktop_copy_selected_icons();
void desktop_cut_selected_icons();
void desktop_paste();
void desktop_clear_clipboard();

// Masaüstü bağlam menüsü yönetimi
void desktop_context_menu_init();
void desktop_context_menu_show(uint16_t x, uint16_t y);
void desktop_context_menu_hide();
uint8_t desktop_context_menu_handle_mouse(uint16_t x, uint16_t y, uint8_t button, uint8_t state);

#endif /* _DESKTOP_H_ */ 