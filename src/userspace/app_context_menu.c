#include "../include/context_menu.h"
#include "../include/gui.h"
#include "../include/vga.h"
#include <string.h>
#include <stdlib.h>

// Uygulama bağlam menüsü
static context_menu_t* app_menu = NULL;

// Uygulama menü eylem işleyicileri
static void app_menu_move(void* data);
static void app_menu_copy(void* data);
static void app_menu_cut(void* data);
static void app_menu_paste(void* data);
static void app_menu_info(void* data);
static void app_menu_open_location(void* data);
static void app_menu_run_as_admin(void* data);
static void app_menu_close(void* data);

// Aktif pencere verisi
typedef struct {
    gui_window_t* window;
    uint32_t app_id;
    uint8_t is_desktop_icon;
} app_menu_data_t;

static app_menu_data_t current_app;

// Uygulama menüsünü oluştur
void app_context_menu_init() {
    // Önceki menü varsa yok et
    if (app_menu) {
        context_menu_destroy(app_menu);
    }
    
    // Yeni menü oluştur
    app_menu = context_menu_create("Uygulama", 0, 0, MENU_TYPE_APP);
    
    if (app_menu) {
        // Menüyü öğelerle doldur
        context_menu_add_item(app_menu, "Taşı", MENU_ITEM_NORMAL, app_menu_move, &current_app)->icon_id = 0;
        context_menu_add_separator(app_menu);
        context_menu_add_item(app_menu, "Kopyala", MENU_ITEM_NORMAL, app_menu_copy, &current_app)->icon_id = 8;
        context_menu_add_item(app_menu, "Kes", MENU_ITEM_NORMAL, app_menu_cut, &current_app)->icon_id = 7;
        context_menu_add_item(app_menu, "Yapıştır", MENU_ITEM_NORMAL, app_menu_paste, &current_app)->icon_id = 9;
        context_menu_add_separator(app_menu);
        context_menu_add_item(app_menu, "Uygulama bilgileri", MENU_ITEM_NORMAL, app_menu_info, &current_app)->icon_id = 0;
        context_menu_add_item(app_menu, "Dosya konumunu aç", MENU_ITEM_NORMAL, app_menu_open_location, &current_app)->icon_id = 1;
        context_menu_add_separator(app_menu);
        context_menu_add_item(app_menu, "Yönetici olarak çalıştır", MENU_ITEM_NORMAL, app_menu_run_as_admin, &current_app)->icon_id = 0;
        context_menu_add_separator(app_menu);
        context_menu_add_item(app_menu, "Kapat", MENU_ITEM_NORMAL, app_menu_close, &current_app)->icon_id = 0;
    }
}

// Masaüstü ikonu için özel bağlam menüsü oluştur
void app_context_menu_init_for_desktop_icon(uint32_t app_id) {
    // Önceki menü varsa yok et
    if (app_menu) {
        context_menu_destroy(app_menu);
    }
    
    // Yeni menü oluştur
    app_menu = context_menu_create(NULL, 0, 0, MENU_TYPE_APP);
    
    if (app_menu) {
        // Uygulama ID bilgisini sakla
        current_app.window = NULL;
        current_app.app_id = app_id;
        current_app.is_desktop_icon = 1;
        
        // Menüyü öğelerle doldur
        context_menu_add_item(app_menu, "Çalıştır", MENU_ITEM_NORMAL, app_menu_run_as_admin, &current_app)->icon_id = 3;
        context_menu_add_item(app_menu, "Yönetici olarak çalıştır", MENU_ITEM_NORMAL, app_menu_run_as_admin, &current_app)->icon_id = 3;
        context_menu_add_separator(app_menu);
        context_menu_add_item(app_menu, "Uygulama bilgileri", MENU_ITEM_NORMAL, app_menu_info, &current_app)->icon_id = 0;
        context_menu_add_item(app_menu, "Dosya konumunu aç", MENU_ITEM_NORMAL, app_menu_open_location, &current_app)->icon_id = 1;
    }
}

// Uygulama menüsünü göster
void app_context_menu_show(gui_window_t* window, uint32_t x, uint32_t y) {
    // Menü oluşturulmamış ise oluştur
    if (!app_menu) {
        app_context_menu_init();
    }
    
    // Aktif pencere bilgisini güncelle
    current_app.window = window;
    current_app.app_id = 0;  // Pencere üzerinde ID kullanmıyoruz
    current_app.is_desktop_icon = 0;
    
    // Menü başlığını güncelle (pencere başlığına göre)
    if (window && window->title) {
        strncpy(app_menu->title, window->title, sizeof(app_menu->title) - 1);
    } else {
        strncpy(app_menu->title, "Uygulama", sizeof(app_menu->title) - 1);
    }
    
    // Menüyü göster
    context_menu_show(app_menu, x, y);
}

// Uygulama menüsünü gizle
void app_context_menu_hide() {
    if (app_menu) {
        context_menu_hide(app_menu);
    }
}

// Uygulama başlık çubuğu sağ tık olayını işle
uint8_t app_context_menu_handle_titlebar_right_click(gui_window_t* window, uint32_t x, uint32_t y) {
    if (!window) return 0;
    
    // Uygulama menüsünü göster
    app_context_menu_show(window, x, y);
    return 1;
}

// Uygulama ikonu sağ tık olayını işle
uint8_t app_context_menu_handle_icon_right_click(uint32_t app_id, uint32_t x, uint32_t y) {
    // Masaüstü ikonu için özel menüyü oluştur
    app_context_menu_init_for_desktop_icon(app_id);
    
    // Menüyü göster
    context_menu_show(app_menu, x, y);
    return 1;
}

// Uygulamayı taşıma işleyicisi
static void app_menu_move(void* data) {
    app_menu_data_t* app_data = (app_menu_data_t*)data;
    
    if (app_data && app_data->window) {
        // Pencereyi taşıma moduna geçir
        app_data->window->is_dragging = 1;
    }
}

// Kopyalama işleyicisi
static void app_menu_copy(void* data) {
    app_menu_data_t* app_data = (app_menu_data_t*)data;
    
    if (app_data && app_data->window) {
        // TODO: Uygulama içeriğini kopyalama işlemi
    }
}

// Kesme işleyicisi
static void app_menu_cut(void* data) {
    app_menu_data_t* app_data = (app_menu_data_t*)data;
    
    if (app_data && app_data->window) {
        // TODO: Uygulama içeriğini kesme işlemi
    }
}

// Yapıştırma işleyicisi
static void app_menu_paste(void* data) {
    app_menu_data_t* app_data = (app_menu_data_t*)data;
    
    if (app_data && app_data->window) {
        // TODO: Uygulama içeriğine yapıştırma işlemi
    }
}

// Uygulama bilgileri işleyicisi
static void app_menu_info(void* data) {
    app_menu_data_t* app_data = (app_menu_data_t*)data;
    
    if (app_data) {
        // TODO: Uygulama bilgilerini gösterme işlemi
    }
}

// Dosya konumunu açma işleyicisi
static void app_menu_open_location(void* data) {
    app_menu_data_t* app_data = (app_menu_data_t*)data;
    
    if (app_data) {
        // TODO: Dosya konumunu gösterme işlemi
    }
}

// Yönetici olarak çalıştırma işleyicisi
static void app_menu_run_as_admin(void* data) {
    app_menu_data_t* app_data = (app_menu_data_t*)data;
    
    if (app_data && app_data->is_desktop_icon) {
        // TODO: Uygulamayı yönetici olarak çalıştırma
    }
}

// Uygulamayı kapatma işleyicisi
static void app_menu_close(void* data) {
    app_menu_data_t* app_data = (app_menu_data_t*)data;
    
    if (app_data && app_data->window) {
        // Pencereyi kapat
        gui_window_close(app_data->window);
    }
} 