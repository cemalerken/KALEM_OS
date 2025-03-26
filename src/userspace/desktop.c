#include "../include/desktop.h"
#include "../include/gui.h"
#include "../include/vga.h"
#include "../include/context_menu.h"
#include <stdlib.h>
#include <string.h>

// Global masaüstü değişkeni
desktop_t* desktop = NULL;

// Masaüstü ikonları için varsayılan ayarlar
#define ICON_START_X DESKTOP_MARGIN_X
#define ICON_START_Y DESKTOP_MARGIN_Y
#define ICON_MAX_PER_ROW ((VGA_WIDTH - (2 * DESKTOP_MARGIN_X)) / DESKTOP_ICON_SPACING_X)

// Masaüstü başlatma
void desktop_init() {
    // Masaüstü için bellek ayır
    desktop = (desktop_t*)malloc(sizeof(desktop_t));
    if (!desktop) return;
    
    // Masaüstü yapısını sıfırla
    memset(desktop, 0, sizeof(desktop_t));
    
    // Varsayılan değerleri ayarla
    desktop->layout = DESKTOP_LAYOUT_GRID;
    desktop->icon_count = 0;
    desktop->edit_mode = 0;
    desktop->drag_active = 0;
    desktop->clipboard_count = 0;
    desktop->clipboard_is_cut = 0;
    
    // Arkaplan varsayılan değerlerini ayarla
    desktop->background.mode = DESKTOP_BG_MODE_SOLID;
    desktop->background.scale = DESKTOP_BG_SCALE_STRETCH;
    desktop->background.bg_color = GUI_COLOR_DESKTOP_BG;
    desktop->background.wallpaper_data = NULL;
    desktop->background.slideshow_count = 0;
    desktop->background.animation_enabled = 0;
    
    // Seçim alanını sıfırla
    desktop->selection.active = 0;
    
    // Masaüstü menüsünü başlat
    desktop_context_menu_init();
    
    // Örnek simgeler ekle
    desktop_add_icon("Dosya Yöneticisi", "/bin/filemanager", DESKTOP_ICON_TYPE_APPLICATION, 30);
    desktop_add_icon("Terminal", "/bin/terminal", DESKTOP_ICON_TYPE_APPLICATION, 10);
    desktop_add_icon("Kalem Tarayıcı", "/bin/browser", DESKTOP_ICON_TYPE_APPLICATION, 31);
    desktop_add_icon("Ayarlar", "/bin/settings", DESKTOP_ICON_TYPE_APPLICATION, 33);
    desktop_add_icon("Belgelerim", "/home/user/Documents", DESKTOP_ICON_TYPE_FOLDER, 34);
    
    // Simgeleri otomatik düzenle
    desktop_auto_arrange_icons();
}

// Masaüstünü çiz
void desktop_draw() {
    if (!desktop) return;
    
    // Arkaplanı çiz
    desktop_update_background();
    
    // Seçim alanını çiz (aktifse)
    if (desktop->selection.active) {
        uint16_t sel_x = (desktop->selection.start_x < desktop->selection.end_x) ? 
                          desktop->selection.start_x : desktop->selection.end_x;
        uint16_t sel_y = (desktop->selection.start_y < desktop->selection.end_y) ? 
                          desktop->selection.start_y : desktop->selection.end_y;
        uint16_t sel_width = abs(desktop->selection.end_x - desktop->selection.start_x);
        uint16_t sel_height = abs(desktop->selection.end_y - desktop->selection.start_y);
        
        // Seçim alanı çerçevesi
        vga_draw_rect(sel_x, sel_y, sel_width, sel_height, GUI_COLOR_SELECTED);
        
        // Yarı saydam dolgu
        vga_fill_rect_alpha(sel_x, sel_y, sel_width, sel_height, GUI_COLOR_SELECTED, 128);
    }
    
    // Tüm simgeleri çiz
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        desktop_icon_t* icon = &desktop->icons[i];
        if (!icon->visible) continue;
        
        // Simge arkaplanı (seçiliyse)
        if (icon->selected) {
            vga_fill_rect(icon->x - 2, icon->y - 2, 
                          icon->width + 4, icon->height + DESKTOP_ICON_LABEL_HEIGHT + 4, 
                          GUI_COLOR_SELECTED);
        }
        
        // İkonu çiz
        gui_draw_icon(icon->x + (icon->width - 32) / 2, 
                      icon->y + (icon->height - 32) / 2, 
                      icon->icon_id, 32);
        
        // İkon adını çiz
        uint16_t text_x = icon->x + (icon->width - strlen(icon->name) * 8) / 2;
        uint16_t text_y = icon->y + icon->height;
        vga_draw_text(text_x, text_y, icon->name, GUI_COLOR_WHITE, GUI_COLOR_TRANSPARENT);
    }
    
    // Sürükleme durumunda, sürüklenen simgenin yarı saydam kopyasını çiz
    if (desktop->drag_active && desktop->dragged_icon) {
        // Yarı saydam simge
        uint16_t drag_x = desktop->drag_current_x - (desktop->dragged_icon->width / 2);
        uint16_t drag_y = desktop->drag_current_y - (desktop->dragged_icon->height / 2);
        
        // İkonu yarı saydam çiz
        gui_draw_icon(drag_x + (desktop->dragged_icon->width - 32) / 2, 
                      drag_y + (desktop->dragged_icon->height - 32) / 2, 
                      desktop->dragged_icon->icon_id, 32);
    }
}

// Masaüstü arkaplanını güncelle
void desktop_update_background() {
    if (!desktop) return;
    
    switch (desktop->background.mode) {
        case DESKTOP_BG_MODE_SOLID:
            // Düz renk arkaplan
            vga_fill_rect(0, 0, desktop->width, desktop->height, desktop->background.bg_color);
            break;
            
        case DESKTOP_BG_MODE_WALLPAPER:
            // Duvar kağıdı
            if (desktop->background.wallpaper_data) {
                // Duvar kağıdı verisini çiz
                switch (desktop->background.scale) {
                    case DESKTOP_BG_SCALE_STRETCH:
                        // TODO: Duvar kağıdını ekrana sığdır
                        break;
                    case DESKTOP_BG_SCALE_FIT:
                        // TODO: En-boy oranını koru ve sığdır
                        break;
                    case DESKTOP_BG_SCALE_FILL:
                        // TODO: En-boy oranını koru ve doldur
                        break;
                    case DESKTOP_BG_SCALE_TILE:
                        // TODO: Döşe
                        break;
                }
            } else {
                // Duvar kağıdı yoksa düz renk göster
                vga_fill_rect(0, 0, VGA_WIDTH, VGA_HEIGHT, desktop->background.bg_color);
            }
            break;
            
        case DESKTOP_BG_MODE_SLIDESHOW:
            // Slayt gösterisi
            // TODO: Slayt gösterisi güncelleme kodu
            break;
            
        case DESKTOP_BG_MODE_ANIMATED:
            // Animasyonlu arkaplan
            if (desktop->background.animation_enabled && desktop->background.animation_update) {
                desktop->background.animation_update();
            } else {
                // Animasyon yoksa düz renk göster
                vga_fill_rect(0, 0, VGA_WIDTH, VGA_HEIGHT, desktop->background.bg_color);
            }
            break;
    }
}

// Masaüstü arkaplan rengini ayarla
void desktop_set_background_color(uint32_t color) {
    if (!desktop) return;
    
    desktop->background.bg_color = color;
    desktop->background.mode = DESKTOP_BG_MODE_SOLID;
    
    // Masaüstünü yeniden çiz
    desktop_draw();
}

// Duvar kağıdını ayarla
void desktop_set_wallpaper(const char* path) {
    if (!desktop || !path) return;
    
    // Duvar kağıdı yolunu kaydet
    strncpy(desktop->background.wallpaper_path, path, MAX_WALLPAPER_PATH_LENGTH - 1);
    desktop->background.wallpaper_path[MAX_WALLPAPER_PATH_LENGTH - 1] = '\0';
    
    // Duvar kağıdı modu
    desktop->background.mode = DESKTOP_BG_MODE_WALLPAPER;
    
    // Eski duvar kağıdını temizle
    if (desktop->background.wallpaper_data) {
        free(desktop->background.wallpaper_data);
        desktop->background.wallpaper_data = NULL;
    }
    
    // Duvar kağıdı verilerini yükle
    // TODO: Gerçek bir sistemde burada görüntü dosyası yüklenmeli
    
    // Masaüstünü yeniden çiz
    desktop_draw();
}

// Fare olayını işle
uint8_t desktop_handle_mouse(uint16_t x, uint16_t y, uint8_t button, uint8_t state) {
    if (!desktop) return 0;
    
    // Düzenleme modunda
    if (desktop->edit_mode) {
        // TODO: Düzenleme modu fare işleme
        return 1;
    }
    
    // Simge seçimi ve sürükleme
    if (button == MOUSE_LEFT_BUTTON) {
        // Fare butonu basıldıysa
        if (state == MOUSE_BUTTON_DOWN) {
            // Tıklanan konumda simge var mı?
            desktop_icon_t* icon = desktop_find_icon_at(x, y);
            
            if (icon) {
                // Simge üstüne tıklandı
                
                // Ctrl tuşu basılı değilse ve simge seçili değilse, tüm seçimi temizle
                if (!(keyboard_modifiers & KEYBOARD_MOD_CTRL) && !icon->selected) {
                    desktop_deselect_all_icons();
                }
                
                // Simgenin seçim durumunu değiştir (Ctrl ile çoklu seçim için)
                if (keyboard_modifiers & KEYBOARD_MOD_CTRL) {
                    icon->selected = !icon->selected;
                } else {
                    icon->selected = 1;
                }
                
                // Sürükleme başlat
                desktop_begin_drag(icon, x, y);
                
                // Masaüstünü yeniden çiz
                desktop_draw();
            } else {
                // Boş alana tıklandı
                
                // Ctrl tuşu basılı değilse seçimi temizle
                if (!(keyboard_modifiers & KEYBOARD_MOD_CTRL)) {
                    desktop_deselect_all_icons();
                    desktop_draw();
                }
                
                // Seçim alanını başlat
                desktop_begin_selection(x, y);
            }
            
            return 1;
        } 
        // Fare butonu bırakıldıysa
        else if (state == MOUSE_BUTTON_UP) {
            // Sürükleme aktifse bitir
            if (desktop->drag_active) {
                desktop_end_drag(x, y);
                return 1;
            }
            
            // Seçim alanı aktifse bitir
            if (desktop->selection.active) {
                desktop_end_selection();
                return 1;
            }
        }
        // Fare hareket ediyorsa
        else if (state == MOUSE_MOVE) {
            // Sürükleme aktifse güncelle
            if (desktop->drag_active) {
                desktop_update_drag(x, y);
                return 1;
            }
            
            // Seçim alanı aktifse güncelle
            if (desktop->selection.active) {
                desktop_update_selection(x, y);
                return 1;
            }
        }
    }
    // Sağ tıklama
    else if (button == MOUSE_RIGHT_BUTTON && state == MOUSE_BUTTON_DOWN) {
        return desktop_menu_handle_right_click(x, y);
    }
    
    return 0;
}

// Klavye olayını işle
uint8_t desktop_handle_keyboard(uint8_t key, uint8_t state) {
    if (!desktop) return 0;
    
    // Tuşa basıldıysa
    if (state == KEYBOARD_KEY_DOWN) {
        switch (key) {
            case KEYBOARD_KEY_ESCAPE:
                // Esc: Seçimi temizle, sürüklemeyi iptal et, düzenleme modunu kapat
                desktop_deselect_all_icons();
                desktop_cancel_drag();
                desktop_edit_mode(0);
                desktop_draw();
                return 1;
                
            case KEYBOARD_KEY_DELETE:
                // Delete: Seçili simgeleri sil
                // TODO: Seçili simgeleri silme işlevi
                return 1;
                
            case KEYBOARD_KEY_A:
                // Ctrl+A: Tümünü seç
                if (keyboard_modifiers & KEYBOARD_MOD_CTRL) {
                    desktop_select_all_icons();
                    desktop_draw();
                    return 1;
                }
                break;
                
            case KEYBOARD_KEY_C:
                // Ctrl+C: Kopyala
                if (keyboard_modifiers & KEYBOARD_MOD_CTRL) {
                    desktop_copy_selected_icons();
                    return 1;
                }
                break;
                
            case 'X':
                // Ctrl+X: Kes
                if (keyboard_modifiers & KEYBOARD_MOD_CTRL) {
                    desktop_cut_selected_icons();
                    return 1;
                }
                break;
                
            case KEYBOARD_KEY_V:
                // Ctrl+V: Yapıştır
                if (keyboard_modifiers & KEYBOARD_MOD_CTRL) {
                    desktop_paste();
                    return 1;
                }
                break;
        }
    }
    
    return 0;
}

// Simgeleri otomatik düzenle
void desktop_auto_arrange_icons() {
    if (!desktop) return;
    
    uint16_t x = ICON_START_X;
    uint16_t y = ICON_START_Y;
    uint16_t count_in_row = 0;
    
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        desktop_icon_t* icon = &desktop->icons[i];
        
        // Simgenin konumunu ayarla
        icon->x = x;
        icon->y = y;
        
        // Sonraki simge konumu
        x += DESKTOP_ICON_SPACING_X;
        count_in_row++;
        
        // Satır sonu kontrolü
        if (count_in_row >= ICON_MAX_PER_ROW) {
            x = ICON_START_X;
            y += DESKTOP_ICON_SPACING_Y;
            count_in_row = 0;
        }
    }
    
    // Masaüstünü yeniden çiz
    desktop_draw();
}

// Tüm simgeleri seç
void desktop_select_all_icons() {
    if (!desktop) return;
    
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        desktop->icons[i].selected = 1;
    }
}

// Tüm simgelerin seçimini kaldır
void desktop_deselect_all_icons() {
    if (!desktop) return;
    
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        desktop->icons[i].selected = 0;
    }
}

// Seçimi tersine çevir
void desktop_invert_selection() {
    if (!desktop) return;
    
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        desktop->icons[i].selected = !desktop->icons[i].selected;
    }
    
    // Masaüstünü yeniden çiz
    desktop_draw();
}

// Seçim alanını başlat
void desktop_begin_selection(uint16_t x, uint16_t y) {
    if (!desktop) return;
    
    desktop->selection.start_x = x;
    desktop->selection.start_y = y;
    desktop->selection.end_x = x;
    desktop->selection.end_y = y;
    desktop->selection.active = 1;
}

// Seçim alanını güncelle
void desktop_update_selection(uint16_t x, uint16_t y) {
    if (!desktop || !desktop->selection.active) return;
    
    desktop->selection.end_x = x;
    desktop->selection.end_y = y;
    
    // Seçim alanı içindeki simgeleri belirle
    uint16_t sel_x1 = (desktop->selection.start_x < desktop->selection.end_x) ? 
                      desktop->selection.start_x : desktop->selection.end_x;
    uint16_t sel_y1 = (desktop->selection.start_y < desktop->selection.end_y) ? 
                      desktop->selection.start_y : desktop->selection.end_y;
    uint16_t sel_x2 = (desktop->selection.start_x > desktop->selection.end_x) ? 
                      desktop->selection.start_x : desktop->selection.end_x;
    uint16_t sel_y2 = (desktop->selection.start_y > desktop->selection.end_y) ? 
                      desktop->selection.start_y : desktop->selection.end_y;
    
    // Her simge için seçim kontrolü
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        desktop_icon_t* icon = &desktop->icons[i];
        
        // Simge seçim alanıyla kesişiyor mu?
        uint8_t intersects = !(icon->x + icon->width < sel_x1 || 
                              icon->x > sel_x2 || 
                              icon->y + icon->height + DESKTOP_ICON_LABEL_HEIGHT < sel_y1 || 
                              icon->y > sel_y2);
        
        // Ctrl tuşu ile çoklu seçim durumu
        if (keyboard_modifiers & KEYBOARD_MOD_CTRL) {
            // Sadece yeni kesişenleri seç, kesişmeyenlere dokunma
            if (intersects) {
                icon->selected = 1;
            }
        } else {
            // Normal seçim: sadece kesişenleri seç
            icon->selected = intersects;
        }
    }
    
    // Masaüstünü yeniden çiz
    desktop_draw();
}

// Seçim alanını bitir
void desktop_end_selection() {
    if (!desktop) return;
    
    desktop->selection.active = 0;
    
    // Masaüstünü yeniden çiz
    desktop_draw();
}

// Masaüstü düzenini ayarla
void desktop_set_layout(desktop_layout_t layout) {
    if (!desktop) return;
    
    desktop->layout = layout;
    
    if (layout == DESKTOP_LAYOUT_GRID || layout == DESKTOP_LAYOUT_AUTO) {
        desktop_auto_arrange_icons();
    }
}

// Düzenleme modunu aç/kapat
void desktop_edit_mode(uint8_t enabled) {
    if (!desktop) return;
    
    desktop->edit_mode = enabled;
    
    // TODO: Düzenleme modu göstergeleri
    
    // Masaüstünü yeniden çiz
    desktop_draw();
}

// Masaüstü simge ekle
desktop_icon_t* desktop_add_icon(const char* name, const char* path, desktop_icon_type_t type, uint8_t icon_id) {
    if (!desktop || !name || !path || desktop->icon_count >= DESKTOP_MAX_ICONS) return NULL;
    
    // Yeni simge indeksi
    uint16_t index = desktop->icon_count;
    desktop_icon_t* icon = &desktop->icons[index];
    
    // Simge özelliklerini ayarla
    strncpy(icon->name, name, MAX_ICON_NAME_LENGTH - 1);
    icon->name[MAX_ICON_NAME_LENGTH - 1] = '\0';
    
    strncpy(icon->path, path, MAX_ICON_PATH_LENGTH - 1);
    icon->path[MAX_ICON_PATH_LENGTH - 1] = '\0';
    
    icon->type = type;
    icon->icon_id = icon_id;
    icon->x = 0;
    icon->y = 0;
    icon->width = DESKTOP_ICON_WIDTH;
    icon->height = DESKTOP_ICON_HEIGHT;
    icon->visible = 1;
    icon->selected = 0;
    icon->highlighted = 0;
    icon->draggable = 1;
    icon->icon_color = GUI_COLOR_ICON_DEFAULT;
    
    // Öğe sayısını artır
    desktop->icon_count++;
    
    return icon;
}

// Simgeyi kaldır
void desktop_remove_icon(desktop_icon_t* icon) {
    if (!desktop || !icon) return;
    
    // Simge indeksini bul
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        if (&desktop->icons[i] == icon) {
            desktop_remove_icon_by_index(i);
            return;
        }
    }
}

// Simgeyi indeksle kaldır
void desktop_remove_icon_by_index(uint16_t index) {
    if (!desktop || index >= desktop->icon_count) return;
    
    // Son simge değilse diğerlerini kaydır
    if (index < desktop->icon_count - 1) {
        memmove(&desktop->icons[index], &desktop->icons[index + 1], 
                sizeof(desktop_icon_t) * (desktop->icon_count - index - 1));
    }
    
    // Öğe sayısını azalt
    desktop->icon_count--;
    
    // Masaüstünü yeniden çiz
    desktop_draw();
}

// X,Y koordinatındaki simgeyi bul
desktop_icon_t* desktop_find_icon_at(uint16_t x, uint16_t y) {
    if (!desktop) return NULL;
    
    // Sondan başa doğru kontrol (üstteki simgeleri önce bulmak için)
    for (int16_t i = desktop->icon_count - 1; i >= 0; i--) {
        desktop_icon_t* icon = &desktop->icons[i];
        
        if (!icon->visible) continue;
        
        // Fare simge sınırları içinde mi?
        if (x >= icon->x && x < icon->x + icon->width &&
            y >= icon->y && y < icon->y + icon->height + DESKTOP_ICON_LABEL_HEIGHT) {
            return icon;
        }
    }
    
    return NULL;
}

// İsimle simge bul
desktop_icon_t* desktop_find_icon_by_name(const char* name) {
    if (!desktop || !name) return NULL;
    
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        if (strcmp(desktop->icons[i].name, name) == 0) {
            return &desktop->icons[i];
        }
    }
    
    return NULL;
}

// Yolla simge bul
desktop_icon_t* desktop_find_icon_by_path(const char* path) {
    if (!desktop || !path) return NULL;
    
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        if (strcmp(desktop->icons[i].path, path) == 0) {
            return &desktop->icons[i];
        }
    }
    
    return NULL;
}

// Sürüklemeyi başlat
void desktop_begin_drag(desktop_icon_t* icon, uint16_t x, uint16_t y) {
    if (!desktop || !icon || !icon->draggable) return;
    
    desktop->dragged_icon = icon;
    desktop->drag_start_x = x;
    desktop->drag_start_y = y;
    desktop->drag_current_x = x;
    desktop->drag_current_y = y;
    desktop->drag_active = 1;
}

// Sürüklemeyi güncelle
void desktop_update_drag(uint16_t x, uint16_t y) {
    if (!desktop || !desktop->drag_active || !desktop->dragged_icon) return;
    
    desktop->drag_current_x = x;
    desktop->drag_current_y = y;
    
    // Masaüstünü yeniden çiz
    desktop_draw();
}

// Sürüklemeyi bitir
void desktop_end_drag(uint16_t x, uint16_t y) {
    if (!desktop || !desktop->drag_active || !desktop->dragged_icon) return;
    
    // Hareket mesafesi hesapla
    int16_t dx = x - desktop->drag_start_x;
    int16_t dy = y - desktop->drag_start_y;
    
    // Tüm seçili simgeleri taşı
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        if (desktop->icons[i].selected) {
            desktop->icons[i].x += dx;
            desktop->icons[i].y += dy;
            
            // Ekran sınırları kontrolü
            if (desktop->icons[i].x < 0) desktop->icons[i].x = 0;
            if (desktop->icons[i].y < 0) desktop->icons[i].y = 0;
            if (desktop->icons[i].x > VGA_WIDTH - desktop->icons[i].width) 
                desktop->icons[i].x = VGA_WIDTH - desktop->icons[i].width;
            if (desktop->icons[i].y > VGA_HEIGHT - desktop->icons[i].height - DESKTOP_ICON_LABEL_HEIGHT) 
                desktop->icons[i].y = VGA_HEIGHT - desktop->icons[i].height - DESKTOP_ICON_LABEL_HEIGHT;
        }
    }
    
    // Sürüklemeyi temizle
    desktop->dragged_icon = NULL;
    desktop->drag_active = 0;
    
    // Masaüstünü yeniden çiz
    desktop_draw();
}

// Sürüklemeyi iptal et
void desktop_cancel_drag() {
    if (!desktop) return;
    
    desktop->dragged_icon = NULL;
    desktop->drag_active = 0;
}

// Seçili simgeleri kopyala
void desktop_copy_selected_icons() {
    if (!desktop) return;
    
    // Panoyu temizle
    desktop->clipboard_count = 0;
    desktop->clipboard_is_cut = 0;
    
    // Seçili simgeleri panoya ekle
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        if (desktop->icons[i].selected && desktop->clipboard_count < MAX_CLIPBOARD_ITEMS) {
            // Simgeyi panoya kopyala
            memcpy(&desktop->clipboard_icons[desktop->clipboard_count], &desktop->icons[i], sizeof(desktop_icon_t));
            desktop->clipboard_count++;
        }
    }
}

// Seçili simgeleri kes
void desktop_cut_selected_icons() {
    if (!desktop) return;
    
    // Panoyu temizle
    desktop->clipboard_count = 0;
    desktop->clipboard_is_cut = 1;
    
    // Seçili simgeleri panoya ekle
    for (uint16_t i = 0; i < desktop->icon_count; i++) {
        if (desktop->icons[i].selected && desktop->clipboard_count < MAX_CLIPBOARD_ITEMS) {
            // Simgeyi panoya kopyala
            memcpy(&desktop->clipboard_icons[desktop->clipboard_count], &desktop->icons[i], sizeof(desktop_icon_t));
            desktop->clipboard_count++;
        }
    }
    
    // Kesme işlemi olduğu için, seçili simgeleri kaldır
    if (desktop->clipboard_is_cut) {
        for (int16_t i = desktop->icon_count - 1; i >= 0; i--) {
            if (desktop->icons[i].selected) {
                desktop_remove_icon_by_index(i);
            }
        }
    }
}

// Panodan yapıştır
void desktop_paste() {
    if (!desktop || desktop->clipboard_count == 0) return;
    
    // Fare konumunu al
    uint16_t paste_x = mouse_x;
    uint16_t paste_y = mouse_y;
    
    // Simgeleri uygun konumlara yerleştir
    for (uint16_t i = 0; i < desktop->clipboard_count; i++) {
        // Simge ekle
        desktop_icon_t* new_icon = desktop_add_icon(
            desktop->clipboard_icons[i].name,
            desktop->clipboard_icons[i].path,
            desktop->clipboard_icons[i].type,
            desktop->clipboard_icons[i].icon_id
        );
        
        // Konum ayarla
        if (new_icon) {
            new_icon->x = paste_x + (i * 20);
            new_icon->y = paste_y + (i * 20);
            
            // Ekran sınırları kontrolü
            if (new_icon->x > VGA_WIDTH - new_icon->width) 
                new_icon->x = VGA_WIDTH - new_icon->width;
            if (new_icon->y > VGA_HEIGHT - new_icon->height - DESKTOP_ICON_LABEL_HEIGHT) 
                new_icon->y = VGA_HEIGHT - new_icon->height - DESKTOP_ICON_LABEL_HEIGHT;
            
            // Önceki seçimleri temizle ve yeni simgeyi seç
            desktop_deselect_all_icons();
            new_icon->selected = 1;
        }
    }
    
    // Kesme işlemiyse panoyu temizle
    if (desktop->clipboard_is_cut) {
        desktop->clipboard_count = 0;
        desktop->clipboard_is_cut = 0;
    }
    
    // Masaüstünü yeniden çiz
    desktop_draw();
}

// Simge sağ tık menüsü göster
void desktop_show_icon_context_menu(desktop_icon_t* icon, uint16_t x, uint16_t y) {
    if (!desktop || !icon) return;
    
    // Simge seçili değilse, diğerlerini temizle ve bunu seç
    if (!icon->selected) {
        desktop_deselect_all_icons();
        icon->selected = 1;
        desktop_draw();
    }
    
    // Uygulama ikonuysa
    if (icon->type == DESKTOP_ICON_TYPE_APPLICATION) {
        app_context_menu_show(NULL, x, y);
    } else {
        // TODO: Diğer simge tipleri için bağlam menüsü
        desktop_menu_show(x, y);
    }
}

// Simgeleri düzenle
void desktop_arrange_icons() {
    if (!desktop) return;
    
    desktop_auto_arrange_icons();
}

// Belleği temizle
void desktop_cleanup() {
    if (!desktop) return;
    
    // Duvar kağıdı verisini temizle
    if (desktop->background.wallpaper_data) {
        free(desktop->background.wallpaper_data);
        desktop->background.wallpaper_data = NULL;
    }
    
    // Masaüstü belleğini serbest bırak
    free(desktop);
    desktop = NULL;
} 