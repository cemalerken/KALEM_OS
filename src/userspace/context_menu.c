#include "../include/context_menu.h"
#include "../include/gui.h"
#include "../include/vga.h"
#include "../include/font.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Sabitler
#define MAX_CONTEXT_MENUS 16
#define MENU_ITEM_HEIGHT 24
#define MENU_PADDING 5
#define MENU_BORDER_WIDTH 1
#define MENU_SHADOW_OFFSET 3
#define MENU_TITLE_HEIGHT 20

// Benzersiz menü kimliği oluşturucu
static uint32_t menu_id_counter = 1;

// Aktif bağlam menüsü
context_menu_t* active_menu = NULL;

// Tüm bağlam menülerinin kümesi
static context_menu_t* menus[MAX_CONTEXT_MENUS] = {0};
static uint32_t menu_count = 0;

// Varsayılan menü renkleri
static uint8_t default_border_color = GUI_COLOR_DARK_GRAY;
static uint8_t default_bg_color = GUI_COLOR_LIGHT_GRAY;
static uint8_t default_title_bg_color = GUI_COLOR_BLUE;
static uint8_t default_title_text_color = GUI_COLOR_WHITE;
static uint8_t default_text_color = GUI_COLOR_BLACK;
static uint8_t default_highlight_color = GUI_COLOR_LIGHT_BLUE;
static uint8_t default_separator_color = GUI_COLOR_DARK_GRAY;
static uint8_t default_disabled_color = GUI_COLOR_DARK_GRAY;

// Önceden tanımlanmış simgeler
static uint8_t menu_icons[] = {
    0, // Boş simge
    1, // Klasör simgesi
    2, // Dosya simgesi
    3, // Uygulama simgesi
    4, // Arama simgesi
    5, // Ayarlar simgesi
    6, // Yenile simgesi
    7, // Kes simgesi
    8, // Kopyala simgesi
    9, // Yapıştır simgesi
    10 // Terminal simgesi
};

// İleri bildirimler
static void context_menu_calculate_size(context_menu_t* menu);
static void context_menu_draw_item(context_menu_t* menu, menu_item_t* item, uint32_t x, uint32_t y, uint8_t is_hover);
static void context_menu_handle_item_click(context_menu_t* menu, menu_item_t* item);
static void context_menu_register(context_menu_t* menu);
static void context_menu_unregister(context_menu_t* menu);
static void context_menu_animate_frame(context_menu_t* menu);

// Yeni bağlam menüsü oluştur
context_menu_t* context_menu_create(const char* title, uint32_t x, uint32_t y, menu_type_t type) {
    context_menu_t* menu = (context_menu_t*)malloc(sizeof(context_menu_t));
    if (!menu) return NULL;
    
    // Menü özelliklerini ayarla
    memset(menu, 0, sizeof(context_menu_t));
    menu->id = menu_id_counter++;
    
    if (title) {
        strncpy(menu->title, title, sizeof(menu->title) - 1);
        menu->has_title = 1;
    } else {
        menu->has_title = 0;
    }
    
    menu->x = x;
    menu->y = y;
    menu->width = 150; // Varsayılan genişlik, sonra hesaplanacak
    menu->height = 0;  // İçerik boyutuna göre hesaplanacak
    
    // Görsel stili ayarla
    menu->has_icons = 1;
    menu->has_shadow = 1;
    menu->transparent = 0;
    menu->rounded_corners = 1;
    
    // Renkleri ayarla
    menu->border_color = default_border_color;
    menu->bg_color = default_bg_color;
    menu->title_bg_color = default_title_bg_color;
    menu->title_text_color = default_title_text_color;
    
    // Animasyon ve durumu ayarla
    menu->is_animating = 0;
    menu->animation_frame = 0;
    menu->animation_type = MENU_ANIM_NONE;
    menu->is_visible = 0;
    menu->is_submenu = 0;
    
    // Metin rengi ve yüksekliği ayarla
    menu->item_count = 0;
    menu->visible_items = 0;
    menu->items = NULL;
    menu->hover_item = NULL;
    
    // Menüyü kaydet
    context_menu_register(menu);
    
    return menu;
}

// Menüyü yok et
void context_menu_destroy(context_menu_t* menu) {
    if (!menu) return;
    
    // Önce tüm öğeleri temizle
    context_menu_clear(menu);
    
    // Kaydını kaldır
    context_menu_unregister(menu);
    
    // Aktif menüyü güncelle
    if (active_menu == menu) {
        active_menu = NULL;
    }
    
    // Alt menülerin ebeveyn bağlantısını kaldır
    for (int i = 0; i < menu_count; i++) {
        if (menus[i] && menus[i]->parent == menu) {
            menus[i]->parent = NULL;
        }
    }
    
    // Belleği serbest bırak
    free(menu);
}

// Menüyü belirli bir konumda göster
void context_menu_show(context_menu_t* menu, uint32_t x, uint32_t y) {
    if (!menu) return;
    
    // Önceki aktif menüyü gizle
    if (active_menu && active_menu != menu) {
        context_menu_hide(active_menu);
    }
    
    // Menü konumunu ayarla
    menu->x = x;
    menu->y = y;
    
    // Menünün boyutunu hesapla
    context_menu_calculate_size(menu);
    
    // Animasyon ayarla
    menu->is_animating = 1;
    menu->animation_frame = 0;
    menu->animation_type = MENU_ANIM_FADE_IN;
    
    // Ekran sınırlarını kontrol et
    if (menu->x + menu->width > gui_desktop->width) {
        menu->x = gui_desktop->width - menu->width;
    }
    
    if (menu->y + menu->height > gui_desktop->height) {
        menu->y = gui_desktop->height - menu->height;
    }
    
    // Görünür olarak ayarla
    menu->is_visible = 1;
    active_menu = menu;
    
    // İlk çizim ve güncelleme
    context_menu_draw(menu);
    gui_desktop_draw();
}

// Menüyü gizle
void context_menu_hide(context_menu_t* menu) {
    if (!menu || !menu->is_visible) return;
    
    // Önce alt menüyü gizle (varsa)
    if (menu->active_submenu) {
        context_menu_hide(menu->active_submenu);
        menu->active_submenu = NULL;
    }
    
    // Animasyon ayarla
    menu->is_animating = 1;
    menu->animation_frame = 0;
    menu->animation_type = MENU_ANIM_FADE_OUT;
    
    // Durumu güncelle
    menu->is_visible = 0;
    menu->hover_item = NULL;
    
    // Aktif menüyü güncelle
    if (active_menu == menu) {
        active_menu = menu->parent;
    }
    
    // Ekranı güncelle
    gui_desktop_draw();
}

// Menü içeriğinin boyutunu hesapla
static void context_menu_calculate_size(context_menu_t* menu) {
    if (!menu) return;
    
    // Öğelerin toplam yüksekliğini hesapla
    uint32_t total_height = 0;
    uint32_t max_width = 150; // Minimum genişlik
    menu->visible_items = 0;
    
    menu_item_t* item = menu->items;
    while (item) {
        if (item->type != MENU_ITEM_SEPARATOR) {
            // Metin genişliğini hesapla
            uint32_t text_width = strlen(item->text) * 8; // Yaklaşık genişlik
            if (text_width > max_width - 40) { // Simge ve boşluk için 40 piksel ayır
                max_width = text_width + 40;
            }
            
            // Öğe yüksekliği
            total_height += MENU_ITEM_HEIGHT;
        } else {
            // Ayırıcı yüksekliği
            total_height += 5;
        }
        
        menu->visible_items++;
        item = item->next;
    }
    
    // Başlık yüksekliğini ekle
    if (menu->has_title) {
        total_height += MENU_TITLE_HEIGHT;
    }
    
    // Kenar boşluklarını ekle
    total_height += MENU_PADDING * 2;
    max_width += MENU_PADDING * 2;
    
    // Boyutları güncelle
    menu->width = max_width;
    menu->height = total_height;
}

// Menüyü çiz
void context_menu_draw(context_menu_t* menu) {
    if (!menu || !menu->is_visible) return;
    
    // Animasyon sırasında alfa/boyut değerlerini hesapla
    float alpha = 1.0;
    uint32_t width = menu->width;
    uint32_t height = menu->height;
    
    if (menu->is_animating) {
        switch (menu->animation_type) {
            case MENU_ANIM_FADE_IN:
                alpha = menu->animation_frame / 10.0;
                break;
            case MENU_ANIM_FADE_OUT:
                alpha = 1.0 - (menu->animation_frame / 10.0);
                break;
            case MENU_ANIM_ZOOM_IN:
                width = menu->width * (menu->animation_frame / 10.0);
                height = menu->height * (menu->animation_frame / 10.0);
                break;
            case MENU_ANIM_ZOOM_OUT:
                width = menu->width * (1.0 - (menu->animation_frame / 10.0));
                height = menu->height * (1.0 - (menu->animation_frame / 10.0));
                break;
            default:
                break;
        }
    }
    
    // Gölgeyi çiz
    if (menu->has_shadow) {
        vga_fill_rect(menu->x + MENU_SHADOW_OFFSET, menu->y + MENU_SHADOW_OFFSET, 
                    width, height, GUI_COLOR_BLACK);
    }
    
    // Menü arkaplanını çiz
    vga_fill_rect(menu->x, menu->y, width, height, menu->bg_color);
    
    // Menü kenarlarını çiz
    vga_draw_rect(menu->x, menu->y, width, height, menu->border_color);
    
    // Başlık çubuğunu çiz
    if (menu->has_title) {
        vga_fill_rect(menu->x + 1, menu->y + 1, width - 2, MENU_TITLE_HEIGHT, menu->title_bg_color);
        font_draw_string(menu->x + 5, menu->y + 5, menu->title, menu->title_text_color, 0);
    }
    
    // Öğeleri çiz
    uint32_t y_offset = menu->has_title ? MENU_TITLE_HEIGHT + MENU_PADDING : MENU_PADDING;
    menu_item_t* item = menu->items;
    
    while (item) {
        context_menu_draw_item(menu, item, menu->x, menu->y + y_offset, (item == menu->hover_item));
        
        if (item->type != MENU_ITEM_SEPARATOR) {
            y_offset += MENU_ITEM_HEIGHT;
        } else {
            y_offset += 5;
        }
        
        item = item->next;
    }
    
    // Alt menüyü çiz (varsa)
    if (menu->active_submenu && menu->active_submenu->is_visible) {
        context_menu_draw(menu->active_submenu);
    }
}

// Menü öğesini çiz
static void context_menu_draw_item(context_menu_t* menu, menu_item_t* item, uint32_t x, uint32_t y, uint8_t is_hover) {
    if (!menu || !item) return;
    
    // Ayırıcı çizgi
    if (item->type == MENU_ITEM_SEPARATOR) {
        vga_draw_hline(x + 5, y + 2, menu->width - 10, default_separator_color);
        return;
    }
    
    // Öğe arka planı
    uint8_t bg_color = is_hover ? default_highlight_color : menu->bg_color;
    vga_fill_rect(x + 1, y, menu->width - 2, MENU_ITEM_HEIGHT, bg_color);
    
    // Öğe metni
    uint8_t text_color = item->enabled ? 
                        (item->text_color ? item->text_color : default_text_color) : 
                        default_disabled_color;
    
    // Simge (varsa)
    if (menu->has_icons && item->icon_id > 0) {
        // TODO: Gerçek simge çiz
        vga_fill_rect(x + 5, y + 4, 16, 16, GUI_COLOR_DARK_GRAY);
        
        // Metni simge genişliği kadar kaydır
        font_draw_string(x + 26, y + 5, item->text, text_color, 0);
    } else {
        font_draw_string(x + 10, y + 5, item->text, text_color, 0);
    }
    
    // Alt menü okunu çiz
    if (item->type == MENU_ITEM_SUBMENU) {
        font_draw_string(x + menu->width - 15, y + 5, ">", text_color, 0);
    }
    
    // İşaretlenebilir öğelerin durumunu göster
    if (item->type == MENU_ITEM_CHECKBOX) {
        if (item->state) {
            font_draw_string(x + 5, y + 5, "✓", text_color, 0);
        } else {
            font_draw_string(x + 5, y + 5, "□", text_color, 0);
        }
    } else if (item->type == MENU_ITEM_RADIO) {
        if (item->state) {
            font_draw_string(x + 5, y + 5, "◉", text_color, 0);
        } else {
            font_draw_string(x + 5, y + 5, "○", text_color, 0);
        }
    }
}

// Menüye öğe ekle
menu_item_t* context_menu_add_item(context_menu_t* menu, const char* text, menu_item_type_t type, void (*on_click)(void*), void* user_data) {
    if (!menu) return NULL;
    
    // Yeni öğe oluştur
    menu_item_t* item = (menu_item_t*)malloc(sizeof(menu_item_t));
    if (!item) return NULL;
    
    // Öğe özelliklerini ayarla
    memset(item, 0, sizeof(menu_item_t));
    strncpy(item->text, text ? text : "", sizeof(item->text) - 1);
    item->type = type;
    item->icon_id = 0; // Varsayılan olarak simge yok
    item->state = 0;
    item->enabled = 1;
    item->on_click = on_click;
    item->user_data = user_data;
    item->submenu = NULL;
    
    // Renkleri ayarla
    item->text_color = default_text_color;
    item->bg_color = menu->bg_color;
    item->highlight_color = default_highlight_color;
    
    // Öğeyi listeye ekle
    if (!menu->items) {
        menu->items = item;
    } else {
        // Listenin sonuna ekle
        menu_item_t* last = menu->items;
        while (last->next) {
            last = last->next;
        }
        last->next = item;
        item->prev = last;
    }
    
    menu->item_count++;
    
    // Menü boyutunu güncelle
    context_menu_calculate_size(menu);
    
    return item;
}

// Menüye alt menü ekle
menu_item_t* context_menu_add_submenu(context_menu_t* menu, const char* text, context_menu_t* submenu) {
    if (!menu || !submenu) return NULL;
    
    // Alt menü olarak işaretle
    submenu->is_submenu = 1;
    submenu->parent = menu;
    
    // Yeni öğe oluştur ve alt menüye bağla
    menu_item_t* item = context_menu_add_item(menu, text, MENU_ITEM_SUBMENU, NULL, NULL);
    if (item) {
        item->submenu = submenu;
    }
    
    return item;
}

// Menüye ayırıcı ekle
menu_item_t* context_menu_add_separator(context_menu_t* menu) {
    return context_menu_add_item(menu, NULL, MENU_ITEM_SEPARATOR, NULL, NULL);
}

// Menüden öğe sil
void context_menu_remove_item(context_menu_t* menu, menu_item_t* item) {
    if (!menu || !item) return;
    
    // Öğe menüde mi kontrol et
    menu_item_t* current = menu->items;
    uint8_t found = 0;
    
    while (current) {
        if (current == item) {
            found = 1;
            break;
        }
        current = current->next;
    }
    
    if (!found) return;
    
    // Bağlantıları güncelle
    if (item->prev) {
        item->prev->next = item->next;
    } else {
        menu->items = item->next;
    }
    
    if (item->next) {
        item->next->prev = item->prev;
    }
    
    // Hover durumunu güncelle
    if (menu->hover_item == item) {
        menu->hover_item = NULL;
    }
    
    // Belleği serbest bırak
    free(item);
    menu->item_count--;
    
    // Menü boyutunu güncelle
    context_menu_calculate_size(menu);
}

// Menüyü temizle (tüm öğeleri kaldır)
void context_menu_clear(context_menu_t* menu) {
    if (!menu) return;
    
    // Tüm öğeleri kaldır
    menu_item_t* item = menu->items;
    while (item) {
        menu_item_t* next = item->next;
        free(item);
        item = next;
    }
    
    menu->items = NULL;
    menu->item_count = 0;
    menu->hover_item = NULL;
    
    // Menü boyutunu güncelle
    context_menu_calculate_size(menu);
}

// Fare hareketi işleme
void context_menu_handle_mouse_move(uint32_t x, uint32_t y) {
    if (!active_menu || !active_menu->is_visible) return;
    
    // Menünün dışında mı kontrol et
    if (x < active_menu->x || x >= active_menu->x + active_menu->width ||
        y < active_menu->y || y >= active_menu->y + active_menu->height) {
        
        // Alt menü açıksa ve fare alt menünün üzerindeyse işlemi iptal et
        if (active_menu->active_submenu && active_menu->active_submenu->is_visible) {
            context_menu_t* submenu = active_menu->active_submenu;
            
            if (x >= submenu->x && x < submenu->x + submenu->width &&
                y >= submenu->y && y < submenu->y + submenu->height) {
                return;
            }
        }
        
        active_menu->hover_item = NULL;
        return;
    }
    
    // Başlık alanında mı kontrol et
    if (active_menu->has_title && y < active_menu->y + MENU_TITLE_HEIGHT) {
        active_menu->hover_item = NULL;
        return;
    }
    
    // Hangi öğenin üzerinde olduğunu bul
    uint32_t y_offset = active_menu->has_title ? MENU_TITLE_HEIGHT + MENU_PADDING : MENU_PADDING;
    menu_item_t* item = active_menu->items;
    menu_item_t* hover_item = NULL;
    
    while (item) {
        if (item->type != MENU_ITEM_SEPARATOR) {
            if (y >= active_menu->y + y_offset && y < active_menu->y + y_offset + MENU_ITEM_HEIGHT) {
                hover_item = item;
                break;
            }
            y_offset += MENU_ITEM_HEIGHT;
        } else {
            y_offset += 5;
        }
        
        item = item->next;
    }
    
    // Hover durumunu güncelle
    if (active_menu->hover_item != hover_item) {
        active_menu->hover_item = hover_item;
        
        // Mevcut alt menüyü kapat
        if (active_menu->active_submenu && 
            (!hover_item || hover_item->submenu != active_menu->active_submenu)) {
            context_menu_hide(active_menu->active_submenu);
            active_menu->active_submenu = NULL;
        }
        
        // Alt menüyü aç (varsa)
        if (hover_item && hover_item->type == MENU_ITEM_SUBMENU && hover_item->submenu) {
            // Alt menüyü konumlandır
            uint32_t submenu_x = active_menu->x + active_menu->width;
            uint32_t submenu_y = active_menu->y + y_offset;
            
            hover_item->submenu->parent = active_menu;
            context_menu_show(hover_item->submenu, submenu_x, submenu_y);
            active_menu->active_submenu = hover_item->submenu;
        }
        
        // Ekranı yeniden çiz
        gui_desktop_draw();
    }
}

// Fare tıklama işleme
void context_menu_handle_mouse_click(uint32_t x, uint32_t y, uint8_t button) {
    if (!active_menu || !active_menu->is_visible || button != 1) return;
    
    // Menünün dışında mı kontrol et
    if (x < active_menu->x || x >= active_menu->x + active_menu->width ||
        y < active_menu->y || y >= active_menu->y + active_menu->height) {
        
        // Alt menü açıksa ve fare alt menünün üzerindeyse işlemi iptal et
        if (active_menu->active_submenu && active_menu->active_submenu->is_visible) {
            context_menu_t* submenu = active_menu->active_submenu;
            
            if (x >= submenu->x && x < submenu->x + submenu->width &&
                y >= submenu->y && y < submenu->y + submenu->height) {
                return;
            }
        }
        
        // Menü dışı tıklama - menüyü kapat
        context_menu_hide(active_menu);
        return;
    }
    
    // Başlık alanında mı kontrol et
    if (active_menu->has_title && y < active_menu->y + MENU_TITLE_HEIGHT) {
        return; // Başlıkta tıklama, bir şey yapma
    }
    
    // Hover durumundaki öğeyi bul
    if (active_menu->hover_item) {
        // Alt menüye sahip öğe için, sadece alt menüyü aç
        if (active_menu->hover_item->type == MENU_ITEM_SUBMENU) {
            if (!active_menu->active_submenu) {
                // Alt menü henüz açık değilse aç
                uint32_t y_offset = 0;
                
                // Öğenin y ofsetini hesapla
                uint32_t item_y_offset = active_menu->has_title ? MENU_TITLE_HEIGHT + MENU_PADDING : MENU_PADDING;
                menu_item_t* item = active_menu->items;
                
                while (item && item != active_menu->hover_item) {
                    if (item->type != MENU_ITEM_SEPARATOR) {
                        item_y_offset += MENU_ITEM_HEIGHT;
                    } else {
                        item_y_offset += 5;
                    }
                    item = item->next;
                }
                
                // Alt menüyü göster
                uint32_t submenu_x = active_menu->x + active_menu->width;
                uint32_t submenu_y = active_menu->y + item_y_offset;
                
                active_menu->hover_item->submenu->parent = active_menu;
                context_menu_show(active_menu->hover_item->submenu, submenu_x, submenu_y);
                active_menu->active_submenu = active_menu->hover_item->submenu;
            }
        } else if (active_menu->hover_item->type != MENU_ITEM_SEPARATOR && 
                  active_menu->hover_item->enabled) {
            // Normal öğe tıklaması
            context_menu_handle_item_click(active_menu, active_menu->hover_item);
            context_menu_hide(active_menu);
        }
    }
}

// Tuş girişi işleme
void context_menu_handle_key(uint8_t key) {
    if (!active_menu || !active_menu->is_visible) return;
    
    // ESC tuşu - menüyü kapat
    if (key == 0x1B) {
        context_menu_hide(active_menu);
        return;
    }
    
    // Yukarı ok
    if (key == 0x11) {
        if (!active_menu->hover_item) {
            // Henüz seçim yoksa, son öğeyi seç
            menu_item_t* item = active_menu->items;
            while (item && item->next) {
                item = item->next;
            }
            active_menu->hover_item = item;
        } else {
            // Bir önceki öğeye git
            menu_item_t* prev = active_menu->hover_item->prev;
            while (prev && prev->type == MENU_ITEM_SEPARATOR) {
                prev = prev->prev;
            }
            
            if (prev) {
                active_menu->hover_item = prev;
            }
        }
        
        gui_desktop_draw();
        return;
    }
    
    // Aşağı ok
    if (key == 0x12) {
        if (!active_menu->hover_item) {
            // Henüz seçim yoksa, ilk öğeyi seç
            active_menu->hover_item = active_menu->items;
        } else {
            // Bir sonraki öğeye git
            menu_item_t* next = active_menu->hover_item->next;
            while (next && next->type == MENU_ITEM_SEPARATOR) {
                next = next->next;
            }
            
            if (next) {
                active_menu->hover_item = next;
            }
        }
        
        gui_desktop_draw();
        return;
    }
    
    // Sağ ok - alt menü aç
    if (key == 0x14) {
        if (active_menu->hover_item && 
            active_menu->hover_item->type == MENU_ITEM_SUBMENU && 
            active_menu->hover_item->submenu) {
            
            // Alt menüyü konumlandır ve göster
            uint32_t submenu_x = active_menu->x + active_menu->width;
            uint32_t submenu_y = active_menu->y + 10; // Yaklaşık bir değer, gerçekte hesaplanmalı
            
            active_menu->hover_item->submenu->parent = active_menu;
            context_menu_show(active_menu->hover_item->submenu, submenu_x, submenu_y);
            active_menu->active_submenu = active_menu->hover_item->submenu;
        }
        return;
    }
    
    // Sol ok - üst menüye git
    if (key == 0x13) {
        if (active_menu->is_submenu && active_menu->parent) {
            context_menu_hide(active_menu);
        }
        return;
    }
    
    // Enter - öğeyi tıkla
    if (key == '\n' || key == '\r') {
        if (active_menu->hover_item && 
            active_menu->hover_item->type != MENU_ITEM_SEPARATOR && 
            active_menu->hover_item->enabled) {
            
            context_menu_handle_item_click(active_menu, active_menu->hover_item);
            context_menu_hide(active_menu);
        }
        return;
    }
}

// Öğe tıklama işleme
static void context_menu_handle_item_click(context_menu_t* menu, menu_item_t* item) {
    if (!menu || !item || item->type == MENU_ITEM_SEPARATOR || !item->enabled) return;
    
    // İşaretlenebilir öğelerin durumunu değiştir
    if (item->type == MENU_ITEM_CHECKBOX) {
        item->state = !item->state;
    } else if (item->type == MENU_ITEM_RADIO) {
        // Diğer radyo öğelerini temizle
        menu_item_t* other = menu->items;
        while (other) {
            if (other != item && other->type == MENU_ITEM_RADIO) {
                other->state = 0;
            }
            other = other->next;
        }
        
        item->state = 1;
    }
    
    // Tıklama işleyiciyi çağır
    if (item->on_click) {
        item->on_click(item->user_data);
    }
}

// Koordinattaki menüyü bul
context_menu_t* context_menu_find_at(uint32_t x, uint32_t y) {
    // Önce aktif alt menüyü kontrol et
    if (active_menu && active_menu->active_submenu) {
        context_menu_t* submenu = active_menu->active_submenu;
        
        if (submenu->is_visible && 
            x >= submenu->x && x < submenu->x + submenu->width &&
            y >= submenu->y && y < submenu->y + submenu->height) {
            return submenu;
        }
    }
    
    // Sonra aktif menüyü kontrol et
    if (active_menu && active_menu->is_visible && 
        x >= active_menu->x && x < active_menu->x + active_menu->width &&
        y >= active_menu->y && y < active_menu->y + active_menu->height) {
        return active_menu;
    }
    
    return NULL;
}

// Koordinattaki menü öğesini bul
menu_item_t* context_menu_find_item_at(context_menu_t* menu, uint32_t x, uint32_t y) {
    if (!menu || !menu->is_visible || !menu->items) return NULL;
    
    // Menü alanı içinde mi kontrol et
    if (x < menu->x || x >= menu->x + menu->width ||
        y < menu->y || y >= menu->y + menu->height) {
        return NULL;
    }
    
    // Başlık alanında mı kontrol et
    if (menu->has_title && y < menu->y + MENU_TITLE_HEIGHT) {
        return NULL;
    }
    
    // Hangi öğenin koordinatları olduğunu bul
    uint32_t y_offset = menu->has_title ? MENU_TITLE_HEIGHT + MENU_PADDING : MENU_PADDING;
    menu_item_t* item = menu->items;
    
    while (item) {
        if (item->type != MENU_ITEM_SEPARATOR) {
            if (y >= menu->y + y_offset && y < menu->y + y_offset + MENU_ITEM_HEIGHT) {
                return item;
            }
            y_offset += MENU_ITEM_HEIGHT;
        } else {
            y_offset += 5;
        }
        
        item = item->next;
    }
    
    return NULL;
}

// Menüyü kaydet
static void context_menu_register(context_menu_t* menu) {
    if (!menu) return;
    
    // Kullanılmayan bir slot bul
    for (int i = 0; i < MAX_CONTEXT_MENUS; i++) {
        if (menus[i] == NULL) {
            menus[i] = menu;
            menu_count++;
            return;
        }
    }
}

// Menü kaydını kaldır
static void context_menu_unregister(context_menu_t* menu) {
    if (!menu) return;
    
    // Menünün kaydını bul ve kaldır
    for (int i = 0; i < MAX_CONTEXT_MENUS; i++) {
        if (menus[i] == menu) {
            menus[i] = NULL;
            menu_count--;
            return;
        }
    }
}

// Menü animasyonunu ayarla
void context_menu_set_animation(context_menu_t* menu, menu_anim_type_t type) {
    if (!menu) return;
    
    menu->animation_type = type;
    menu->is_animating = 1;
    menu->animation_frame = 0;
}

// Menüyü animasyonla göster (her çerçeve için çağrılır)
void context_menu_animate(context_menu_t* menu) {
    if (!menu || !menu->is_animating) return;
    
    // Bir sonraki kareyi yap
    menu->animation_frame++;
    
    // Animasyon bitti mi?
    if (menu->animation_frame >= 10) {
        menu->is_animating = 0;
        menu->animation_frame = 0;
        
        // Kapanma animasyonu tamamlandı, menüyü tamamen kapat
        if (menu->animation_type == MENU_ANIM_FADE_OUT || 
            menu->animation_type == MENU_ANIM_SLIDE_UP || 
            menu->animation_type == MENU_ANIM_ZOOM_OUT) {
            
            menu->is_visible = 0;
        }
    }
    
    // Ekranı yeniden çiz
    gui_desktop_draw();
}

// Menü animasyonu kare işleme
void context_menu_update(context_menu_t* menu) {
    if (!menu) return;
    
    // Animasyon varsa kare güncelle
    if (menu->is_animating) {
        context_menu_animate_frame(menu);
    }
    
    // Alt menü varsa onu da güncelle
    if (menu->active_submenu) {
        context_menu_update(menu->active_submenu);
    }
}

// Animasyon karesini işle
static void context_menu_animate_frame(context_menu_t* menu) {
    if (!menu || !menu->is_animating) return;
    
    // Bir sonraki kareyi yap
    context_menu_animate(menu);
} 