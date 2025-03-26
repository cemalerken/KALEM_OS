#ifndef _TASKBAR_H_
#define _TASKBAR_H_

#include <stdint.h>

// Taskbar sabitleri
#define TASKBAR_HEIGHT          30
#define DOCK_HEIGHT             TASKBAR_HEIGHT
#define SYSTRAY_HEIGHT          TASKBAR_HEIGHT
#define SYSTRAY_WIDTH           150
#define START_BUTTON_WIDTH      100
#define START_MENU_WIDTH        250
#define START_MENU_HEIGHT       350
#define MAX_DOCK_ITEM_NAME_LENGTH 32

// Fare olay tipleri
#define MOUSE_LEFT_BUTTON       1
#define MOUSE_RIGHT_BUTTON      2
#define MOUSE_MIDDLE_BUTTON     3
#define MOUSE_BUTTON_DOWN       1
#define MOUSE_BUTTON_UP         2
#define MOUSE_MOVE              3

// Taskbar konumları
typedef enum {
    TASKBAR_POSITION_BOTTOM = 0,
    TASKBAR_POSITION_TOP,
    TASKBAR_POSITION_LEFT,
    TASKBAR_POSITION_RIGHT
} taskbar_position_t;

// Dock öğesi durumları
typedef enum {
    DOCK_ITEM_STATE_NORMAL = 0,
    DOCK_ITEM_STATE_HOVER,
    DOCK_ITEM_STATE_ACTIVE,
    DOCK_ITEM_STATE_RUNNING
} dock_item_state_t;

// Dock animasyon durumları
typedef enum {
    DOCK_ANIMATION_NONE = 0,
    DOCK_ANIMATION_HOVER,
    DOCK_ANIMATION_BOUNCE,
    DOCK_ANIMATION_ZOOM
} dock_animation_state_t;

// Başlat menüsü animasyon durumları
typedef enum {
    START_MENU_ANIMATION_NONE = 0,
    START_MENU_ANIMATION_OPEN,
    START_MENU_ANIMATION_CLOSE
} startmenu_animation_state_t;

// Sistem tepsisi öğe tipleri
typedef enum {
    SYSTRAY_ITEM_NETWORK = 0,
    SYSTRAY_ITEM_VOLUME,
    SYSTRAY_ITEM_BATTERY,
    SYSTRAY_ITEM_CLOCK,
    SYSTRAY_ITEM_LANGUAGE,
    SYSTRAY_ITEM_NOTIFICATION
} systray_item_type_t;

// İleri tanım
struct dock_item;

// Dock öğesi tıklama işleyicisi tipi
typedef void (*dock_item_click_handler_t)(struct dock_item* item);

// Dock öğesi
typedef struct dock_item {
    uint8_t app_id;                          // Uygulama ID
    uint8_t icon_id;                         // İkon ID
    char name[MAX_DOCK_ITEM_NAME_LENGTH];    // Öğe adı
    dock_item_state_t state;                 // Öğe durumu
    dock_animation_state_t animation_state;  // Animasyon durumu
    dock_item_click_handler_t click_handler; // Tıklama işleyicisi
} dock_item_t;

// Sistem tepsisi öğesi
typedef struct {
    systray_item_type_t type;    // Öğe tipi
    uint8_t icon_id;             // İkon ID
    uint8_t state;               // Öğe durumu (tip başına değişir)
} systray_item_t;

// Dock
typedef struct {
    uint16_t x;                  // X konumu
    uint16_t y;                  // Y konumu
    uint16_t width;              // Genişlik
    uint16_t height;             // Yükseklik
    uint8_t icon_size;           // İkon boyutu
    uint8_t icon_spacing;        // İkonlar arası boşluk
    dock_animation_state_t animation_state; // Dock animasyon durumu
} dock_t;

// Sistem tepsisi
typedef struct {
    uint16_t x;                  // X konumu
    uint16_t y;                  // Y konumu
    uint16_t width;              // Genişlik
    uint16_t height;             // Yükseklik
    uint8_t item_spacing;        // Öğeler arası boşluk
} systray_t;

// Başlat menüsü
typedef struct {
    uint16_t x;                  // X konumu
    uint16_t y;                  // Y konumu
    uint16_t width;              // Genişlik
    uint16_t height;             // Yükseklik
    uint8_t visible;             // Görünürlük
    startmenu_animation_state_t animation_state; // Animasyon durumu
} startmenu_t;

// Taskbar
typedef struct {
    taskbar_position_t position; // Konum
    uint8_t height;              // Yükseklik
    uint8_t bg_color;            // Arkaplan rengi
    uint8_t visible;             // Görünürlük
} taskbar_t;

// Taskbar fonksiyonları
void taskbar_init();
void taskbar_draw();
void taskbar_cleanup();
uint8_t taskbar_handle_mouse(uint16_t x, uint16_t y, uint8_t button, uint8_t state);

// Dock fonksiyonları
void dock_draw();
void dock_init_items();
void dock_item_click(uint8_t index);
void dock_item_hover(uint8_t index);
uint8_t dock_add_item(uint8_t app_id, uint8_t icon_id, const char* name, dock_item_click_handler_t click_handler);
void dock_remove_item(uint8_t index);
void dock_update_item_state(uint8_t app_id, uint8_t state);
void dock_update_item_icon(uint8_t app_id, uint8_t icon_id);

// Sistem tepsisi fonksiyonları
void systray_draw();
void systray_init_items();
void systray_handle_click(uint16_t x, uint16_t y);
void systray_add_item(systray_item_t* item);

// Başlat menüsü fonksiyonları
void startmenu_draw();
void startmenu_draw_button();
void startmenu_toggle();
void startmenu_handle_click(uint16_t x, uint16_t y);

#endif /* _TASKBAR_H_ */ 