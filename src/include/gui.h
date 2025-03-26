#ifndef KALEMOS_GUI_H
#define KALEMOS_GUI_H

#include <stdint.h>
#include <stddef.h>

// GUI renk tanımlamaları
#define GUI_COLOR_BLACK          0
#define GUI_COLOR_BLUE           1
#define GUI_COLOR_GREEN          2
#define GUI_COLOR_CYAN           3
#define GUI_COLOR_RED            4
#define GUI_COLOR_MAGENTA        5
#define GUI_COLOR_BROWN          6
#define GUI_COLOR_LIGHT_GRAY     7
#define GUI_COLOR_DARK_GRAY      8
#define GUI_COLOR_LIGHT_BLUE     9
#define GUI_COLOR_LIGHT_GREEN    10
#define GUI_COLOR_LIGHT_CYAN     11
#define GUI_COLOR_LIGHT_RED      12
#define GUI_COLOR_LIGHT_MAGENTA  13
#define GUI_COLOR_YELLOW         14
#define GUI_COLOR_WHITE          15
#define GUI_COLOR_TRANSPARENT    0xFF

// Yeni GUI renk tanımlamaları
#define GUI_COLOR_DESKTOP_BG         GUI_COLOR_BLUE
#define GUI_COLOR_WINDOW_BG          GUI_COLOR_LIGHT_GRAY
#define GUI_COLOR_WINDOW_BORDER      GUI_COLOR_DARK_GRAY
#define GUI_COLOR_WINDOW_ACTIVE_BORDER GUI_COLOR_LIGHT_BLUE
#define GUI_COLOR_WINDOW_TITLEBAR    GUI_COLOR_DARK_GRAY
#define GUI_COLOR_WINDOW_ACTIVE_TITLEBAR GUI_COLOR_LIGHT_BLUE
#define GUI_COLOR_WINDOW_TITLE_TEXT  GUI_COLOR_WHITE
#define GUI_COLOR_WINDOW_CLIENT      GUI_COLOR_LIGHT_GRAY
#define GUI_COLOR_WINDOW_CLOSE_BTN   GUI_COLOR_LIGHT_RED
#define GUI_COLOR_WINDOW_MIN_BTN     GUI_COLOR_LIGHT_CYAN
#define GUI_COLOR_WINDOW_MAX_BTN     GUI_COLOR_LIGHT_GREEN
#define GUI_COLOR_WINDOW_CLOSE_X     GUI_COLOR_WHITE
#define GUI_COLOR_WINDOW_MIN_SYMBOL  GUI_COLOR_BLACK
#define GUI_COLOR_WINDOW_MAX_SYMBOL  GUI_COLOR_BLACK

#define GUI_COLOR_TEXT_NORMAL    GUI_COLOR_BLACK
#define GUI_COLOR_TEXT_HIGHLIGHT GUI_COLOR_WHITE
#define GUI_COLOR_BUTTON         GUI_COLOR_LIGHT_GRAY
#define GUI_COLOR_BUTTON_TEXT    GUI_COLOR_BLACK
#define GUI_COLOR_SELECTED       GUI_COLOR_LIGHT_BLUE
#define GUI_COLOR_HIGHLIGHT      GUI_COLOR_LIGHT_BLUE
#define GUI_COLOR_BORDER         GUI_COLOR_DARK_GRAY
#define GUI_COLOR_GRAY           GUI_COLOR_DARK_GRAY
#define GUI_COLOR_ALT_ROW        GUI_COLOR_LIGHT_CYAN
#define GUI_COLOR_RUNNING        GUI_COLOR_GREEN

// Taskbar ve dock renkleri
#define GUI_COLOR_TASKBAR_BG     GUI_COLOR_DARK_GRAY
#define GUI_COLOR_DOCK_BG        GUI_COLOR_DARK_GRAY
#define GUI_COLOR_SYSTRAY_BG     GUI_COLOR_DARK_GRAY
#define GUI_COLOR_MENU_BG        GUI_COLOR_LIGHT_GRAY

// İkon renkleri
#define GUI_COLOR_ICON_1         GUI_COLOR_LIGHT_BLUE
#define GUI_COLOR_ICON_2         GUI_COLOR_YELLOW
#define GUI_COLOR_ICON_3         GUI_COLOR_LIGHT_GREEN
#define GUI_COLOR_ICON_4         GUI_COLOR_LIGHT_CYAN
#define GUI_COLOR_ICON_5         GUI_COLOR_LIGHT_MAGENTA
#define GUI_COLOR_ICON_6         GUI_COLOR_LIGHT_RED
#define GUI_COLOR_ICON_7         GUI_COLOR_GREEN
#define GUI_COLOR_ICON_8         GUI_COLOR_RED
#define GUI_COLOR_ICON_9         GUI_COLOR_BLUE
#define GUI_COLOR_ICON_10        GUI_COLOR_CYAN
#define GUI_COLOR_ICON_11        GUI_COLOR_MAGENTA
#define GUI_COLOR_ICON_12        GUI_COLOR_BROWN
#define GUI_COLOR_ICON_13        GUI_COLOR_RED
#define GUI_COLOR_ICON_14        GUI_COLOR_GREEN
#define GUI_COLOR_ICON_DEFAULT   GUI_COLOR_LIGHT_GRAY
#define GUI_COLOR_ICON_BORDER    GUI_COLOR_BLACK

// GUI sistem sabitleri
#define GUI_WINDOW_BORDER_WIDTH    2
#define GUI_WINDOW_TITLEBAR_HEIGHT 20
#define GUI_WINDOW_MIN_WIDTH       100
#define GUI_WINDOW_MIN_HEIGHT      80
#define GUI_BUTTON_HEIGHT          24
#define GUI_MAX_WINDOW_TITLE_LENGTH 64

// Masaüstü durumları
typedef enum {
    GUI_DESKTOP_NORMAL = 0,  // Normal durum
    GUI_DESKTOP_SELECTING,   // Seçim yapılıyor
    GUI_DESKTOP_DRAGGING     // Sürükleme yapılıyor
} gui_desktop_state_t;

// Pencere stilleri
#define GUI_WINDOW_NORMAL         0x00  // Standart pencere
#define GUI_WINDOW_BORDERLESS     0x01  // Kenarsız pencere
#define GUI_WINDOW_RESIZABLE      0x02  // Yeniden boyutlandırılabilir
#define GUI_WINDOW_MOVABLE        0x04  // Taşınabilir
#define GUI_WINDOW_MAXIMIZABLE    0x08  // Büyütülebilir
#define GUI_WINDOW_MINIMIZABLE    0x10  // Küçültülebilir
#define GUI_WINDOW_ALWAYS_ON_TOP  0x20  // Her zaman üstte
#define GUI_WINDOW_MODAL          0x40  // Modal pencere

// İleri tanım
struct gui_window;

// Pencere olay işleyicileri
typedef uint8_t (*gui_window_paint_handler_t)(struct gui_window* window);
typedef uint8_t (*gui_window_close_handler_t)(struct gui_window* window);
typedef void (*gui_window_resize_handler_t)(struct gui_window* window, uint16_t width, uint16_t height);
typedef void (*gui_window_move_handler_t)(struct gui_window* window, uint16_t x, uint16_t y);
typedef void (*gui_window_mouse_handler_t)(struct gui_window* window, uint16_t x, uint16_t y, uint8_t button, uint8_t state);
typedef void (*gui_window_keyboard_handler_t)(struct gui_window* window, uint8_t key, uint8_t state);
typedef void (*gui_window_right_click_handler_t)(struct gui_window* window, uint16_t x, uint16_t y);

// Pencere içerik alanı yapısı
typedef struct {
    uint16_t x;             // İçerik alanı X konumu (pencere içinde)
    uint16_t y;             // İçerik alanı Y konumu (pencere içinde)
    uint16_t width;         // İçerik alanı genişliği
    uint16_t height;        // İçerik alanı yüksekliği
} gui_client_area_t;

// Pencere yapısı
typedef struct gui_window {
    // Kimlik ve başlık
    uint16_t id;                      // Benzersiz pencere kimliği
    char title[GUI_MAX_WINDOW_TITLE_LENGTH]; // Pencere başlığı
    
    // Konum ve boyut
    uint16_t x, y;                    // Ekrandaki konum
    uint16_t width, height;           // Pencere boyutu
    
    // Geri yükleme bilgileri (maximized için)
    uint16_t restore_x, restore_y;    // Geri yükleme konumu
    uint16_t restore_width, restore_height; // Geri yükleme boyutu
    
    // İçerik alanı
    gui_client_area_t client;         // İçerik alanı
    
    // Durum
    uint8_t visible;                  // Görünürlük
    uint8_t minimized;                // Küçültülmüş mü?
    uint8_t maximized;                // Büyütülmüş mü?
    uint8_t style;                    // Pencere stili
    uint8_t resizable;                // Yeniden boyutlandırılabilir mi?
    uint8_t movable;                  // Taşınabilir mi?
    
    // Olay işleyicileri
    gui_window_paint_handler_t on_paint;       // Çizim olayı
    gui_window_close_handler_t on_close;       // Kapatma olayı
    gui_window_resize_handler_t on_resize;     // Yeniden boyutlandırma olayı
    gui_window_move_handler_t on_move;         // Taşıma olayı
    gui_window_mouse_handler_t on_mouse;       // Fare olayı
    gui_window_keyboard_handler_t on_keyboard; // Klavye olayı
    gui_window_right_click_handler_t on_right_click; // Sağ tık olayı
    
    // Kullanıcı verisi
    void* user_data;                  // Özel kullanıcı verisi
    
    // Pencere listesi için bağlantı
    struct gui_window* next;          // Sonraki pencere
} gui_window_t;

// Masaüstü yapısı
typedef struct {
    uint16_t width, height;           // Ekran boyutu
    uint8_t background_color;         // Arkaplan rengi
    gui_desktop_state_t state;        // Masaüstü durumu
} gui_desktop_t;

// GUI başlatma ve kapatma
void gui_init();
void gui_cleanup();

// Masaüstü işlemleri
void gui_desktop_draw();

// Pencere işlemleri
gui_window_t* gui_create_window(const char* title, uint16_t x, uint16_t y, 
                                uint16_t width, uint16_t height, uint8_t style);
void gui_close_window(gui_window_t* window);
void gui_set_window_visibility(gui_window_t* window, uint8_t visible);
void gui_set_window_position(gui_window_t* window, uint16_t x, uint16_t y);
void gui_set_window_size(gui_window_t* window, uint16_t width, uint16_t height);
void gui_set_window_title(gui_window_t* window, const char* title);

// Grafik çizim işlevleri
void gui_draw_icon(uint16_t x, uint16_t y, uint8_t icon_id, uint16_t size);

// Olay işleme
void gui_handle_mouse(uint16_t x, uint16_t y, uint8_t button, uint8_t state);
void gui_handle_keyboard(uint8_t key, uint8_t state);

// Global değişkenler
extern gui_desktop_t gui_desktop;
extern gui_window_t* gui_window_list;
extern gui_window_t* gui_active_window;

#endif // KALEMOS_GUI_H 