#ifndef _CONTEXT_MENU_H
#define _CONTEXT_MENU_H

#include <stdint.h>

// İleri bildirim
struct gui_window;

// Menü öğe türleri
typedef enum {
    MENU_ITEM_NORMAL,      // Normal tıklanabilir öğe
    MENU_ITEM_SEPARATOR,   // Ayırıcı çizgi
    MENU_ITEM_SUBMENU,     // Alt menü
    MENU_ITEM_CHECKBOX,    // İşaretlenebilir öğe
    MENU_ITEM_RADIO,       // Radyo öğesi (sadece biri seçilebilir)
    MENU_ITEM_DISABLED     // Devre dışı bırakılmış öğe
} menu_item_type_t;

// Menü animasyon türleri
typedef enum {
    MENU_ANIM_NONE,        // Animasyon yok
    MENU_ANIM_FADE_IN,     // Soluklaşarak görünme
    MENU_ANIM_FADE_OUT,    // Soluklaşarak kaybolma
    MENU_ANIM_ZOOM_IN,     // Yakınlaşma
    MENU_ANIM_ZOOM_OUT,    // Uzaklaşma
    MENU_ANIM_SLIDE_DOWN,  // Aşağı doğru kayma
    MENU_ANIM_SLIDE_UP     // Yukarı doğru kayma
} menu_anim_type_t;

// Önceden tanımlanmış menü türleri
typedef enum {
    MENU_TYPE_DESKTOP,     // Masaüstü menüsü
    MENU_TYPE_APP,         // Uygulama menüsü
    MENU_TYPE_EDIT,        // Düzenleme menüsü
    MENU_TYPE_FILE,        // Dosya menüsü
    MENU_TYPE_CUSTOM       // Özel menü
} menu_type_t;

// İleri bildirim
typedef struct menu_item menu_item_t;
typedef struct context_menu context_menu_t;

// Menü öğesi
struct menu_item {
    char text[32];                     // Öğe metni
    menu_item_type_t type;             // Öğe türü
    uint8_t icon_id;                   // Simge kimliği (0 = simge yok)
    uint8_t state;                     // Öğe durumu (işaretli/işaretsiz)
    uint8_t enabled;                   // Etkin/devre dışı
    void (*on_click)(void* user_data); // Tıklama işleyici
    void* user_data;                   // Kullanıcı verisi
    
    // Görsel özellikler
    uint8_t text_color;                // Metin rengi
    uint8_t bg_color;                  // Arka plan rengi
    uint8_t highlight_color;           // Vurgu rengi
    
    // Alt menü (eğer varsa)
    context_menu_t* submenu;
    
    // Bağlı liste için işaretçiler
    menu_item_t* prev;
    menu_item_t* next;
};

// Bağlam menüsü
struct context_menu {
    uint32_t id;                     // Benzersiz menü kimliği
    char title[32];                  // Menü başlığı
    uint32_t x, y;                   // Konum
    uint32_t width, height;          // Boyutlar
    
    // Stil seçenekleri
    uint8_t has_title;               // Başlık çubuğu göster
    uint8_t has_icons;               // Simgeleri göster
    uint8_t has_shadow;              // Gölge efekti
    uint8_t transparent;             // Şeffaflık
    uint8_t rounded_corners;         // Yuvarlak köşeler
    
    // Renkler
    uint8_t border_color;            // Kenar rengi
    uint8_t bg_color;                // Arka plan rengi
    uint8_t title_bg_color;          // Başlık arka plan rengi
    uint8_t title_text_color;        // Başlık metin rengi
    
    // Öğe listesi
    menu_item_t* items;              // İlk öğe
    menu_item_t* hover_item;         // Fare üzerinde olan öğe
    uint32_t item_count;             // Öğe sayısı
    uint32_t visible_items;          // Görünür öğe sayısı
    
    // Animasyon ve durum
    uint8_t is_animating;            // Animasyon durumu
    uint8_t animation_frame;         // Animasyon karesi
    menu_anim_type_t animation_type; // Animasyon türü
    uint8_t is_visible;              // Görünürlük durumu
    uint8_t is_submenu;              // Alt menü mü?
    
    // Alt menü ve üst menü bağlantıları
    context_menu_t* parent;          // Üst menü (eğer alt menüyse)
    context_menu_t* active_submenu;  // Aktif alt menü
    
    // Kullanıcı verisi
    void* user_data;                 // Özel veri
};

// Menü oluşturma ve yönetme işlevleri
context_menu_t* context_menu_create(const char* title, uint32_t x, uint32_t y, menu_type_t type);
void context_menu_destroy(context_menu_t* menu);
void context_menu_show(context_menu_t* menu, uint32_t x, uint32_t y);
void context_menu_hide(context_menu_t* menu);
void context_menu_draw(context_menu_t* menu);
void context_menu_set_animation(context_menu_t* menu, menu_anim_type_t type);
void context_menu_animate(context_menu_t* menu);
void context_menu_update(context_menu_t* menu);

// Menü öğesi işlevleri
menu_item_t* context_menu_add_item(context_menu_t* menu, const char* text, menu_item_type_t type, void (*on_click)(void*), void* user_data);
menu_item_t* context_menu_add_submenu(context_menu_t* menu, const char* text, context_menu_t* submenu);
menu_item_t* context_menu_add_separator(context_menu_t* menu);
void context_menu_remove_item(context_menu_t* menu, menu_item_t* item);
void context_menu_clear(context_menu_t* menu);

// Kullanıcı girişi işlevleri
void context_menu_handle_mouse_move(uint32_t x, uint32_t y);
void context_menu_handle_mouse_click(uint32_t x, uint32_t y, uint8_t button);
void context_menu_handle_key(uint8_t key);

// Masaüstü sağ tık menüsü işlevleri
void desktop_menu_init(void);
void desktop_menu_show(uint32_t x, uint32_t y);
void desktop_menu_hide(void);
uint8_t desktop_menu_handle_right_click(uint32_t x, uint32_t y);

// Uygulama bağlam menüsü işlevleri
void app_context_menu_init(void);
void app_context_menu_init_for_desktop_icon(uint32_t app_id);
void app_context_menu_show(struct gui_window* window, uint32_t x, uint32_t y);
void app_context_menu_hide(void);
uint8_t app_context_menu_handle_titlebar_right_click(struct gui_window* window, uint32_t x, uint32_t y);
uint8_t app_context_menu_handle_icon_right_click(uint32_t app_id, uint32_t x, uint32_t y);

// Yardımcı işlevler
context_menu_t* context_menu_find_at(uint32_t x, uint32_t y);
menu_item_t* context_menu_find_item_at(context_menu_t* menu, uint32_t x, uint32_t y);

#endif /* _CONTEXT_MENU_H */ 