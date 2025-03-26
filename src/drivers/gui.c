#include "../include/gui.h"
#include "../include/vga.h"
#include "../include/font.h"
#include "../include/context_menu.h"
#include "../include/desktop.h"
#include "../include/taskbar.h"
#include "../include/mouse.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Global masaüstü nesnesi
gui_desktop_t* gui_desktop = NULL;

// Benzersiz pencere ID üreticisi
static uint32_t window_id_counter = 1;

// Benzersiz buton ID üreticisi
static uint32_t button_id_counter = 1;

// Masaüstü
desktop_t* desktop = NULL;

// Sürükleme durumu değişkenleri
static uint8_t window_drag_active = 0;
static gui_window_t* dragged_window = NULL;
static int16_t drag_offset_x = 0;
static int16_t drag_offset_y = 0;

// Pencere listesi
gui_window_t* gui_window_list = NULL;
gui_window_t* gui_active_window = NULL;
uint16_t gui_window_count = 0;

// GUI başlatma
void gui_init() {
    // VGA modunu ayarla
    vga_init();
    
    // Font sistemini başlat
    font_init();
    
    // Masaüstü ve taskbar başlat
    desktop_init();
    taskbar_init();
}

// Masaüstü başlatma
void gui_desktop_init() {
    gui_desktop = (gui_desktop_t*)alloc_kheap(sizeof(gui_desktop_t));
    if (!gui_desktop) return;
    
    memset(gui_desktop, 0, sizeof(gui_desktop_t));
    
    gui_desktop->width = vga_width;
    gui_desktop->height = vga_height;
    gui_desktop->background_color = GUI_COLOR_DESKTOP_BG;
    gui_desktop->windows = NULL;
    gui_desktop->focused_window = NULL;
    gui_desktop->dragging_window = NULL;
    
    // Masaüstünü çiz
    gui_desktop_draw();
}

// Masaüstü çizme
void gui_desktop_draw() {
    // Masaüstü arkaplanını çiz
    if (desktop) {
        // Masaüstü modülü yüklüyse onu kullan
        desktop_draw();
    } else {
        // Değilse basit arkaplan çiz
        vga_fill_rect(0, 0, gui_desktop->width, gui_desktop->height, gui_desktop->background_color);
    }
    
    // Masaüstünü çiz
    desktop_draw();
    
    // Taskbar'ı çiz
    taskbar_draw();
    
    // Pencereleri çiz
    gui_window_t* window = gui_window_list;
    while (window) {
        if (window->visible) {
            gui_draw_window(window);
        }
        window = window->next;
    }
    
    // Görev çubuğunu çiz
    taskbar_draw();
    
    // Sürükleme animasyonu veya seçim kutusu
    if (desktop && desktop->drag_active && desktop->dragged_icon) {
        // Sürüklenen simgeyi yarı saydam çiz
        gui_draw_icon(
            desktop->drag_current_x - (desktop->dragged_icon->width / 2),
            desktop->drag_current_y - (desktop->dragged_icon->height / 2),
            desktop->dragged_icon->icon_id,
            desktop->dragged_icon->width
        );
    }
    
    // Bağlam menüsünü çiz
    context_menu_draw_all();
}

// Arkaplan rengini ayarla
void gui_desktop_set_background(uint8_t color) {
    if (gui_desktop) {
        gui_desktop->background_color = color;
        gui_desktop_draw();
    }
}

// Pencere oluşturma
gui_window_t* gui_window_create(const char* title, uint32_t x, uint32_t y, 
                              uint32_t width, uint32_t height, uint8_t style) {
    // Bellek tahsisi
    gui_window_t* window = (gui_window_t*)alloc_kheap(sizeof(gui_window_t));
    if (!window) return NULL;
    
    // Pencere özelliklerini ayarla
    memset(window, 0, sizeof(gui_window_t));
    window->id = window_id_counter++;
    strncpy(window->title, title, sizeof(window->title) - 1);
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->style = style;
    window->visible = 0;
    
    // İçerik alanını hesapla
    if (!(style & GUI_WINDOW_STYLE_NO_TITLE)) {
        window->client.x = GUI_WINDOW_BORDER_WIDTH;
        window->client.y = GUI_WINDOW_TITLE_HEIGHT;
        window->client.width = width - (GUI_WINDOW_BORDER_WIDTH * 2);
        window->client.height = height - GUI_WINDOW_TITLE_HEIGHT - GUI_WINDOW_BORDER_WIDTH;
    } else {
        window->client.x = GUI_WINDOW_BORDER_WIDTH;
        window->client.y = GUI_WINDOW_BORDER_WIDTH;
        window->client.width = width - (GUI_WINDOW_BORDER_WIDTH * 2);
        window->client.height = height - (GUI_WINDOW_BORDER_WIDTH * 2);
    }
    
    window->on_click = NULL;
    window->on_right_click = NULL;  // Sağ tıklama olay işleyicisi
    window->on_key = NULL;
    window->on_draw = NULL;
    window->user_data = NULL;
    
    // Pencereyi masaüstü listesine ekle
    if (gui_desktop) {
        window->next = gui_desktop->windows;
        if (window->next) {
            window->next->prev = window;
        }
        gui_desktop->windows = window;
    }
    
    return window;
}

// Pencere yok etme
void gui_window_destroy(gui_window_t* window) {
    if (!window) return;
    
    // Pencereyi listeden çıkar
    if (window->prev) {
        window->prev->next = window->next;
    } else if (gui_desktop) {
        gui_desktop->windows = window->next;
    }
    
    if (window->next) {
        window->next->prev = window->prev;
    }
    
    // Odaklanmış pencereyi güncelle
    if (gui_desktop && gui_desktop->focused_window == window) {
        gui_desktop->focused_window = NULL;
    }
    
    // Belleği serbest bırak
    free_kheap(window);
    
    // Masaüstünü güncelle
    gui_desktop_draw();
}

// Pencere gösterme
void gui_window_show(gui_window_t* window) {
    if (!window) return;
    
    window->visible = 1;
    gui_window_bring_to_front(window);
    gui_desktop_draw();
}

// Pencere gizleme
void gui_window_hide(gui_window_t* window) {
    if (!window) return;
    
    window->visible = 0;
    
    // Odaklanmış pencereyi güncelle
    if (gui_desktop && gui_desktop->focused_window == window) {
        gui_desktop->focused_window = NULL;
    }
    
    gui_desktop_draw();
}

// Pencere taşıma
void gui_window_move(gui_window_t* window, uint32_t x, uint32_t y) {
    if (!window) return;
    
    window->x = x;
    window->y = y;
    
    if (window->on_move) {
        window->on_move(window, x, y);
    }
    
    gui_desktop_draw();
}

// Pencere yeniden boyutlandırma
void gui_window_resize(gui_window_t* window, uint32_t width, uint32_t height) {
    if (!window) return;
    
    window->width = width;
    window->height = height;
    
    // İçerik alanını güncelle
    if (!(window->style & GUI_WINDOW_STYLE_NO_TITLE)) {
        window->client.width = width - (GUI_WINDOW_BORDER_WIDTH * 2);
        window->client.height = height - GUI_WINDOW_TITLE_HEIGHT - GUI_WINDOW_BORDER_WIDTH;
    } else {
        window->client.width = width - (GUI_WINDOW_BORDER_WIDTH * 2);
        window->client.height = height - (GUI_WINDOW_BORDER_WIDTH * 2);
    }
    
    if (window->on_resize) {
        window->on_resize(window, width, height);
    }
    
    gui_desktop_draw();
}

// Pencere çizme
void gui_window_draw(gui_window_t* window) {
    if (!window || !window->visible) return;
    
    // Pencere arkaplanı
    vga_fill_rect(window->x, window->y, window->width, window->height, GUI_COLOR_WINDOW_BG);
    
    // Pencere kenarları
    gui_window_draw_border(window);
    
    // Başlık çubuğu
    if (!(window->style & GUI_WINDOW_STYLE_NO_TITLE)) {
        gui_window_draw_title(window);
    }
    
    // İçerik alanı
    gui_window_draw_client(window);
}

// Pencere başlığı çizme
void gui_window_draw_title(gui_window_t* window) {
    if (!window || (window->style & GUI_WINDOW_STYLE_NO_TITLE)) return;
    
    // Başlık çubuğu arkaplanı
    vga_fill_rect(window->x, window->y, window->width, GUI_WINDOW_TITLE_HEIGHT, 
                 GUI_COLOR_WINDOW_TITLE);
    
    // Başlık metni
    font_draw_string(window->x + 5, window->y + 2, window->title, 
                    GUI_COLOR_WINDOW_TITLE_TEXT, 0xFF);
    
    // Kapat düğmesi (X)
    if (!(window->style & GUI_WINDOW_STYLE_NO_CLOSE)) {
        vga_fill_rect(window->x + window->width - 18, window->y + 2, 
                     16, 16, GUI_COLOR_LIGHT_RED);
        font_draw_string(window->x + window->width - 15, window->y + 2, 
                        "X", GUI_COLOR_WHITE, 0xFF);
    }
}

// Pencere kenarları çizme
void gui_window_draw_border(gui_window_t* window) {
    if (!window) return;
    
    uint8_t border_color = window == gui_desktop->focused_window ? 
                           GUI_COLOR_LIGHT_BLUE : GUI_COLOR_WINDOW_BORDER;
    
    // Üst kenar
    vga_fill_rect(window->x, window->y, 
                 window->width, GUI_WINDOW_BORDER_WIDTH, 
                 border_color);
    
    // Alt kenar
    vga_fill_rect(window->x, window->y + window->height - GUI_WINDOW_BORDER_WIDTH, 
                 window->width, GUI_WINDOW_BORDER_WIDTH, 
                 border_color);
    
    // Sol kenar
    vga_fill_rect(window->x, window->y, 
                 GUI_WINDOW_BORDER_WIDTH, window->height, 
                 border_color);
    
    // Sağ kenar
    vga_fill_rect(window->x + window->width - GUI_WINDOW_BORDER_WIDTH, window->y, 
                 GUI_WINDOW_BORDER_WIDTH, window->height, 
                 border_color);
}

// Pencere içerik alanı çizme
void gui_window_draw_client(gui_window_t* window) {
    if (!window) return;
    
    // Özel çizim olayını çağır
    if (window->on_paint) {
        window->on_paint(window);
    }
}

// Belirli bir koordinattaki pencereyi bul
gui_window_t* gui_window_find_at(uint32_t x, uint32_t y) {
    if (!gui_desktop) return NULL;
    
    gui_window_t* window = gui_desktop->windows;
    while (window) {
        if (window->visible &&
            x >= window->x && x < window->x + window->width &&
            y >= window->y && y < window->y + window->height) {
            return window;
        }
        window = window->next;
    }
    
    return NULL;
}

// Pencereyi öne getir
void gui_window_bring_to_front(gui_window_t* window) {
    if (!window || !gui_desktop) return;
    
    // Pencere zaten en önde ise bir şey yapma
    if (gui_desktop->windows == window) {
        gui_desktop->focused_window = window;
        return;
    }
    
    // Pencereyi mevcut listeden çıkar
    if (window->prev) {
        window->prev->next = window->next;
    }
    
    if (window->next) {
        window->next->prev = window->prev;
    }
    
    // Pencereyi listenin başına ekle
    window->next = gui_desktop->windows;
    window->prev = NULL;
    
    if (gui_desktop->windows) {
        gui_desktop->windows->prev = window;
    }
    
    gui_desktop->windows = window;
    gui_desktop->focused_window = window;
}

// Buton oluşturma
gui_button_t* gui_button_create(gui_window_t* window, const char* label, 
                              uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    // Bellek tahsisi
    gui_button_t* button = (gui_button_t*)alloc_kheap(sizeof(gui_button_t));
    if (!button) return NULL;
    
    // Buton özelliklerini ayarla
    memset(button, 0, sizeof(gui_button_t));
    button->id = button_id_counter++;
    strncpy(button->label, label, sizeof(button->label) - 1);
    button->x = x;
    button->y = y;
    button->width = width;
    button->height = height;
    button->state = 0;
    
    return button;
}

// Buton yok etme
void gui_button_destroy(gui_button_t* button) {
    if (button) {
        free_kheap(button);
    }
}

// Buton çizme
void gui_button_draw(gui_window_t* window, gui_button_t* button) {
    if (!window || !button) return;
    
    uint32_t x = window->x + window->client.x + button->x;
    uint32_t y = window->y + window->client.y + button->y;
    
    // Buton arkaplanı
    vga_fill_rect(x, y, button->width, button->height, GUI_COLOR_BUTTON);
    
    // Buton kenarları (3D görünüm)
    uint8_t light_color = GUI_COLOR_WHITE;
    uint8_t dark_color = GUI_COLOR_DARK_GRAY;
    
    if (button->state) { // Basılı buton
        light_color = GUI_COLOR_DARK_GRAY;
        dark_color = GUI_COLOR_WHITE;
    }
    
    // Üst ve sol kenar (açık renk)
    vga_draw_hline(x, y, button->width - 1, light_color);
    vga_draw_vline(x, y + 1, button->height - 2, light_color);
    
    // Alt ve sağ kenar (koyu renk)
    vga_draw_hline(x + 1, y + button->height - 1, button->width - 1, dark_color);
    vga_draw_vline(x + button->width - 1, y, button->height - 1, dark_color);
    
    // Buton metni
    uint32_t text_width = font_get_string_width(button->label);
    uint32_t text_x = x + (button->width - text_width) / 2;
    uint32_t text_y = y + (button->height - system_font->height) / 2;
    
    if (button->state) { // Basılı buton
        text_x += 1;
        text_y += 1;
    }
    
    font_draw_string(text_x, text_y, button->label, GUI_COLOR_BUTTON_TEXT, 0xFF);
}

// Fare olaylarını işleyen fonksiyonu düzenliyorum
void gui_handle_mouse_event(uint32_t x, uint32_t y, uint8_t button_state) {
    // Önceki fare durumunu sakla
    static uint32_t prev_x = 0;
    static uint32_t prev_y = 0;
    static uint8_t prev_button_state = 0;
    static uint8_t dragging = 0;
    static gui_window_t* drag_window = NULL;
    static uint32_t drag_offset_x = 0;
    static uint32_t drag_offset_y = 0;
    
    // Fare pozisyonunu güncelle
    gui_desktop->mouse_x = x;
    gui_desktop->mouse_y = y;
    
    // Fare hareket etmişse menüye bildir
    if (x != prev_x || y != prev_y) {
        context_menu_handle_mouse_move(x, y);
    }
    
    // Önce taskbar fare olaylarını işle
    if (taskbar_handle_mouse(x, y, button_state)) {
        return;
    }
    
    // Sonra masaüstü fare olaylarını işle
    if (desktop_handle_mouse(x, y, button_state)) {
        return;
    }
    
    // Sonra pencere fare olaylarını işle
    // Sol tıklama olayı (basıldı)
    if ((button_state & 1) && !(prev_button_state & 1)) {
        // Önce aktif menüyü kontrol et
        context_menu_t* menu = context_menu_find_at(x, y);
        if (menu) {
            // Menü tıklamasını işle
            context_menu_handle_mouse_click(x, y, 1);
            prev_button_state = button_state;
            prev_x = x;
            prev_y = y;
            return;
        }
        
        // Pencere tıklaması
        gui_window_t* window = gui_window_find_at(x, y);
        if (window) {
            // Pencere başlık çubuğu tıklaması
            if (y >= window->y && y < window->y + GUI_WINDOW_TITLE_HEIGHT) {
                dragging = 1;
                drag_window = window;
                drag_offset_x = x - window->x;
                drag_offset_y = y - window->y;
                window->is_dragging = 1;
            }
            
            // Pencereyi öne getir
            gui_window_bring_to_front(window);
            
            // Pencere içeriğine tıklama olayını ilet
            if (window->on_click) {
                window->on_click(window, x - window->x, y - window->y);
            }
        } else {
            // Masaüstü tıklaması - uygulama ikonları için launcher'ı kontrol et
            if (launcher_handle_click(x, y)) {
                // Launcher bir şey yaptı, devam etme
            } else {
                // Boş masaüstü tıklaması
            }
        }
    }
    
    // Sol tıklama bırakıldı
    if (!(button_state & 1) && (prev_button_state & 1)) {
        if (dragging && drag_window) {
            drag_window->is_dragging = 0;
            dragging = 0;
            drag_window = NULL;
        }
    }
    
    // Sağ tıklama olayı (basıldı)
    if ((button_state & 2) && !(prev_button_state & 2)) {
        // Önce aktif menüyü kontrol et
        context_menu_t* menu = context_menu_find_at(x, y);
        if (menu) {
            // Menü tıklamasını işle
            context_menu_handle_mouse_click(x, y, 2);
            prev_button_state = button_state;
            prev_x = x;
            prev_y = y;
            return;
        }
        
        // Pencere üzerinde sağ tıklama mı?
        gui_window_t* window = gui_window_find_at(x, y);
        if (window) {
            // Başlık çubuğu sağ tıklaması
            if (y >= window->y && y < window->y + GUI_WINDOW_TITLE_HEIGHT) {
                app_context_menu_handle_titlebar_right_click(window, x, y);
            } else {
                // Pencere içeriğine sağ tıklama olayını ilet
                if (window->on_right_click) {
                    window->on_right_click(window, x - window->x, y - window->y);
                }
            }
        } else {
            // Masaüstü sağ tıklaması
            // Önce uygulama ikonu üzerinde mi kontrol et
            uint32_t app_id = launcher_get_icon_at(x, y);
            if (app_id != 0) {
                app_context_menu_handle_icon_right_click(app_id, x, y);
            } else {
                // Boş masaüstüne sağ tıklama
                desktop_menu_handle_right_click(x, y);
            }
        }
    }
    
    // Sürükleme işlemi
    if (dragging && drag_window && (button_state & 1)) {
        uint32_t new_x = x - drag_offset_x;
        uint32_t new_y = y - drag_offset_y;
        
        // Ekran sınırlarını kontrol et
        if (new_x < 0) new_x = 0;
        if (new_y < 0) new_y = 0;
        if (new_x + drag_window->width > gui_desktop->width) {
            new_x = gui_desktop->width - drag_window->width;
        }
        
        // Pencereyi taşı
        gui_window_move(drag_window, new_x, new_y);
    }
    
    // Durumu güncelle
    prev_button_state = button_state;
    prev_x = x;
    prev_y = y;
}

// Klavye olaylarını işleyen fonksiyonu güncelliyorum
void gui_handle_key_event(uint8_t key) {
    // İlk olarak aktif menüye yönlendir
    context_menu_t* menu = context_menu_find_at(gui_desktop->mouse_x, gui_desktop->mouse_y);
    if (menu) {
        context_menu_handle_key(key);
        return;
    }
    
    // Önce masaüstü klavye olaylarını işle
    if (desktop_handle_keyboard(key, KEYBOARD_KEY_DOWN)) {
        return;
    }
    
    // ... existing code ...
}

// Pencereyi çiz
static void gui_draw_window(gui_window_t* window) {
    if (!window || !window->visible) return;
    
    // Minimized ise sadece başlık çubuğunu çiz
    if (window->minimized) {
        // TODO: Minimize edilmiş pencere çizimi
        return;
    }
    
    // Pencere kenarlığı
    uint8_t border_color = (window == gui_active_window) ? GUI_COLOR_WINDOW_ACTIVE_BORDER : GUI_COLOR_WINDOW_BORDER;
    vga_draw_rect(window->x, window->y, window->width, window->height, border_color);
    
    // Başlık çubuğu
    uint8_t titlebar_color = (window == gui_active_window) ? GUI_COLOR_WINDOW_ACTIVE_TITLEBAR : GUI_COLOR_WINDOW_TITLEBAR;
    vga_fill_rect(window->x + 1, window->y + 1, window->width - 2, GUI_WINDOW_TITLEBAR_HEIGHT - 1, titlebar_color);
    
    // Başlık metni
    uint16_t title_x = window->x + 5;
    uint16_t title_y = window->y + (GUI_WINDOW_TITLEBAR_HEIGHT - 16) / 2;
    vga_draw_text(title_x, title_y, window->title, GUI_COLOR_WINDOW_TITLE_TEXT, GUI_COLOR_TRANSPARENT);
    
    // Kapat düğmesi
    uint16_t close_btn_x = window->x + window->width - 20;
    uint16_t close_btn_y = window->y + 5;
    vga_fill_rect(close_btn_x, close_btn_y, 15, 15, GUI_COLOR_WINDOW_CLOSE_BTN);
    vga_draw_rect(close_btn_x, close_btn_y, 15, 15, GUI_COLOR_WINDOW_BORDER);
    
    // X şekli
    vga_draw_line(close_btn_x + 3, close_btn_y + 3, close_btn_x + 11, close_btn_y + 11, GUI_COLOR_WINDOW_CLOSE_X);
    vga_draw_line(close_btn_x + 11, close_btn_y + 3, close_btn_x + 3, close_btn_y + 11, GUI_COLOR_WINDOW_CLOSE_X);
    
    // Küçült düğmesi
    if (window->style & GUI_WINDOW_MINIMIZABLE) {
        uint16_t min_btn_x = close_btn_x - 20;
        uint16_t min_btn_y = close_btn_y;
        vga_fill_rect(min_btn_x, min_btn_y, 15, 15, GUI_COLOR_WINDOW_MIN_BTN);
        vga_draw_rect(min_btn_x, min_btn_y, 15, 15, GUI_COLOR_WINDOW_BORDER);
        vga_draw_line(min_btn_x + 3, min_btn_y + 7, min_btn_x + 11, min_btn_y + 7, GUI_COLOR_WINDOW_MIN_SYMBOL);
    }
    
    // Büyüt/Eski haline getir düğmesi
    if (window->style & GUI_WINDOW_MAXIMIZABLE) {
        uint16_t max_btn_x = close_btn_x - 40;
        uint16_t max_btn_y = close_btn_y;
        vga_fill_rect(max_btn_x, max_btn_y, 15, 15, GUI_COLOR_WINDOW_MAX_BTN);
        vga_draw_rect(max_btn_x, max_btn_y, 15, 15, GUI_COLOR_WINDOW_BORDER);
        
        if (window->maximized) {
            // Eski haline getir simgesi
            vga_draw_rect(max_btn_x + 3, max_btn_y + 5, 7, 7, GUI_COLOR_WINDOW_MAX_SYMBOL);
            vga_draw_rect(max_btn_x + 5, max_btn_y + 3, 7, 7, GUI_COLOR_WINDOW_MAX_SYMBOL);
        } else {
            // Büyüt simgesi
            vga_draw_rect(max_btn_x + 3, max_btn_y + 3, 9, 9, GUI_COLOR_WINDOW_MAX_SYMBOL);
        }
    }
    
    // İçerik alanı
    vga_fill_rect(
        window->client.x,
        window->client.y,
        window->client.width,
        window->client.height,
        GUI_COLOR_WINDOW_CLIENT
    );
    
    // İçeriği çiz (on_paint olayını çağır)
    if (window->on_paint) {
        window->on_paint(window);
    }
}

// Pencereyi çiz
void gui_draw_icon(uint16_t x, uint16_t y, uint8_t icon_id, uint16_t size) {
    // Temel ikonlar
    uint32_t colors[] = {
        GUI_COLOR_ICON_1,  // 0: Yardım ikonu
        GUI_COLOR_ICON_2,  // 1: Klasör ikonu
        GUI_COLOR_ICON_3,  // 2: Kalem OS logosu
        GUI_COLOR_ICON_4,  // 3: Kısayol ikonu
        GUI_COLOR_ICON_5,  // 4: Arama ikonu
        GUI_COLOR_ICON_6,  // 5: Ayarlar ikonu
        GUI_COLOR_ICON_7,  // 6: Yenile ikonu
        GUI_COLOR_ICON_8,  // 7: Kes ikonu
        GUI_COLOR_ICON_9,  // 8: Kopyala ikonu
        GUI_COLOR_ICON_10, // 9: Yapıştır ikonu
        GUI_COLOR_ICON_11, // 10: Terminal ikonu
        GUI_COLOR_ICON_12, // 11: Kullanıcı ikonu
        GUI_COLOR_ICON_13, // 12: Güç ikonu
        GUI_COLOR_ICON_14  // 13: Yeniden başlat ikonu
    };
    
    if (icon_id < sizeof(colors) / sizeof(colors[0])) {
        // Basit kare ikon
        vga_fill_rect(x, y, size, size, colors[icon_id]);
        vga_draw_rect(x, y, size, size, GUI_COLOR_ICON_BORDER);
    } else {
        // Varsayılan ikon
        vga_fill_rect(x, y, size, size, GUI_COLOR_ICON_DEFAULT);
        vga_draw_rect(x, y, size, size, GUI_COLOR_ICON_BORDER);
    }
}

// GUI sistemini kapat
void gui_cleanup() {
    // Masaüstü ve taskbar'ı temizle
    desktop_cleanup();
    taskbar_cleanup();
    
    // Tüm pencereleri kapat
    while (gui_window_list) {
        gui_window_t* window = gui_window_list;
        gui_window_list = window->next;
        free(window);
    }
    
    gui_window_count = 0;
    gui_active_window = NULL;
} 