#include "../include/taskbar.h"
#include "../include/gui.h"
#include "../include/vga.h"
#include "../include/launcher.h"
#include "../include/clock.h"
#include <string.h>
#include <stdlib.h>

// Taskbar yapıları
static taskbar_t* taskbar = NULL;
static dock_t* dock = NULL;
static systray_t* systray = NULL;
static startmenu_t* startmenu = NULL;

// Sistem tray ikonları
static systray_item_t network_icon;
static systray_item_t volume_icon;
static systray_item_t battery_icon;
static systray_item_t language_icon;
static systray_item_t notification_icon;

// Dock simgeleri
static dock_item_t* dock_items = NULL;
static uint8_t dock_item_count = 0;

// Startmenu görünürlük durumu
static uint8_t startmenu_visible = 0;

// Taskbar başlatma
void taskbar_init() {
    // Taskbar yapısını oluştur
    taskbar = (taskbar_t*)malloc(sizeof(taskbar_t));
    if (!taskbar) return;
    
    // Dock yapısını oluştur
    dock = (dock_t*)malloc(sizeof(dock_t));
    if (!dock) {
        free(taskbar);
        return;
    }
    
    // Sistem tepsisi yapısını oluştur
    systray = (systray_t*)malloc(sizeof(systray_t));
    if (!systray) {
        free(dock);
        free(taskbar);
        return;
    }
    
    // Başlat menüsü yapısını oluştur
    startmenu = (startmenu_t*)malloc(sizeof(startmenu_t));
    if (!startmenu) {
        free(systray);
        free(dock);
        free(taskbar);
        return;
    }
    
    // Taskbar ayarlarını yap
    taskbar->height = TASKBAR_HEIGHT;
    taskbar->bg_color = GUI_COLOR_TASKBAR_BG;
    taskbar->visible = 1;
    taskbar->position = TASKBAR_POSITION_BOTTOM;
    
    // Dock ayarlarını yap
    dock->width = VGA_WIDTH - SYSTRAY_WIDTH - START_BUTTON_WIDTH;
    dock->height = DOCK_HEIGHT;
    dock->x = START_BUTTON_WIDTH;
    dock->y = VGA_HEIGHT - DOCK_HEIGHT;
    dock->icon_spacing = 8;
    dock->icon_size = 32;
    dock->animation_state = DOCK_ANIMATION_NONE;
    
    // Sistem tepsisi ayarlarını yap
    systray->width = SYSTRAY_WIDTH;
    systray->height = SYSTRAY_HEIGHT;
    systray->x = VGA_WIDTH - SYSTRAY_WIDTH;
    systray->y = VGA_HEIGHT - SYSTRAY_HEIGHT;
    systray->item_spacing = 4;
    
    // Başlat menüsü ayarlarını yap
    startmenu->width = START_MENU_WIDTH;
    startmenu->height = START_MENU_HEIGHT;
    startmenu->x = 0;
    startmenu->y = VGA_HEIGHT - startmenu->height - TASKBAR_HEIGHT;
    startmenu->visible = 0;
    startmenu->animation_state = START_MENU_ANIMATION_NONE;
    
    // Sistem tepsisi ikonlarını başlat
    systray_init_items();
    
    // Dock simgelerini başlat
    dock_init_items();
}

// Taskbar'ı çiz
void taskbar_draw() {
    if (!taskbar || !taskbar->visible) return;
    
    // Taskbar arkaplanını çiz
    uint16_t taskbar_y = VGA_HEIGHT - taskbar->height;
    vga_fill_rect(0, taskbar_y, VGA_WIDTH, taskbar->height, taskbar->bg_color);
    
    // Başlat düğmesini çiz
    startmenu_draw_button();
    
    // Dock'u çiz
    dock_draw();
    
    // Sistem tepsisini çiz
    systray_draw();
    
    // Başlat menüsü görünür ise çiz
    if (startmenu->visible) {
        startmenu_draw();
    }
}

// Dock'u çiz
void dock_draw() {
    if (!dock) return;
    
    // Dock arkaplanı
    vga_fill_rect(dock->x, dock->y, dock->width, dock->height, GUI_COLOR_DOCK_BG);
    
    // Dock simgeleri
    for (uint8_t i = 0; i < dock_item_count; i++) {
        dock_item_t* item = &dock_items[i];
        
        // İkon pozisyonu
        uint16_t icon_x = dock->x + (i * (dock->icon_size + dock->icon_spacing)) + dock->icon_spacing;
        uint16_t icon_y = dock->y + (dock->height - dock->icon_size) / 2;
        
        // İkonun altı gölge
        if (item->state == DOCK_ITEM_STATE_ACTIVE) {
            vga_fill_rect(icon_x, dock->y + dock->height - 3, dock->icon_size, 3, GUI_COLOR_HIGHLIGHT);
        }
        
        // İkonu çiz
        gui_draw_icon(icon_x, icon_y, item->icon_id, dock->icon_size);
        
        // Animasyonlu efekti uygula
        if (item->animation_state == DOCK_ANIMATION_HOVER) {
            // Vurgu efekti
            vga_draw_rect(icon_x - 2, icon_y - 2, dock->icon_size + 4, dock->icon_size + 4, GUI_COLOR_HIGHLIGHT);
        }
        
        // Running indicator for applications
        if (item->state == DOCK_ITEM_STATE_RUNNING) {
            vga_fill_rect(icon_x + (dock->icon_size - 10) / 2, dock->y + dock->height - 5, 10, 2, GUI_COLOR_RUNNING);
        }
    }
}

// Sistem tepsisini çiz
void systray_draw() {
    if (!systray) return;
    
    // Sistem tepsisi arkaplanı
    vga_fill_rect(systray->x, systray->y, systray->width, systray->height, GUI_COLOR_SYSTRAY_BG);
    
    // Ayırıcı çizgi
    vga_draw_vline(systray->x, systray->y + 2, systray->height - 4, GUI_COLOR_GRAY);
    
    // Saat çiz
    char time_str[9]; // "HH:MM:SS\0"
    clock_get_time_str(time_str);
    
    uint16_t time_x = systray->x + systray->width - 8 * strlen(time_str) - 4;
    uint16_t time_y = systray->y + (systray->height - 16) / 2;
    vga_draw_text(time_x, time_y, time_str, GUI_COLOR_WHITE, GUI_COLOR_TRANSPARENT);
    
    // Sistem tepsisi ikonlarını çiz
    uint16_t icon_x = systray->x + systray->item_spacing;
    uint16_t icon_y = systray->y + (systray->height - 16) / 2;
    
    // Ağ durumu ikonu
    gui_draw_icon(icon_x, icon_y, network_icon.icon_id, 16);
    icon_x += 16 + systray->item_spacing;
    
    // Ses ikonu
    gui_draw_icon(icon_x, icon_y, volume_icon.icon_id, 16);
    icon_x += 16 + systray->item_spacing;
    
    // Pil ikonu
    gui_draw_icon(icon_x, icon_y, battery_icon.icon_id, 16);
    icon_x += 16 + systray->item_spacing;
    
    // Dil ikonu
    gui_draw_icon(icon_x, icon_y, language_icon.icon_id, 16);
    icon_x += 16 + systray->item_spacing;
    
    // Bildirim ikonu
    gui_draw_icon(icon_x, icon_y, notification_icon.icon_id, 16);
}

// Başlat menüsü düğmesini çiz
void startmenu_draw_button() {
    if (!startmenu) return;
    
    uint16_t btn_y = VGA_HEIGHT - TASKBAR_HEIGHT;
    uint8_t color = (startmenu->visible) ? GUI_COLOR_SELECTED : GUI_COLOR_BUTTON;
    
    // Başlat düğmesi arkaplanı
    vga_fill_rect(0, btn_y, START_BUTTON_WIDTH, TASKBAR_HEIGHT, color);
    
    // Kalem OS logosu
    gui_draw_icon(4, btn_y + (TASKBAR_HEIGHT - 24) / 2, 2, 24);
    
    // "Başlat" yazısı
    vga_draw_text(32, btn_y + (TASKBAR_HEIGHT - 16) / 2, "Başlat", GUI_COLOR_WHITE, GUI_COLOR_TRANSPARENT);
}

// Başlat menüsünü çiz
void startmenu_draw() {
    if (!startmenu || !startmenu->visible) return;
    
    // Arkaplan
    vga_fill_rect(startmenu->x, startmenu->y, startmenu->width, startmenu->height, GUI_COLOR_MENU_BG);
    vga_draw_rect(startmenu->x, startmenu->y, startmenu->width, startmenu->height, GUI_COLOR_BORDER);
    
    // Üst panel (kullanıcı bilgisi)
    vga_fill_rect(startmenu->x + 1, startmenu->y + 1, startmenu->width - 2, 40, GUI_COLOR_HIGHLIGHT);
    gui_draw_icon(startmenu->x + 8, startmenu->y + 4, 11, 32); // Kullanıcı ikonu
    vga_draw_text(startmenu->x + 48, startmenu->y + 12, "Kullanıcı", GUI_COLOR_WHITE, GUI_COLOR_TRANSPARENT);
    
    // Alt panel (güç seçenekleri)
    vga_fill_rect(startmenu->x + 1, startmenu->y + startmenu->height - 41, startmenu->width - 2, 40, GUI_COLOR_DARK_GRAY);
    
    // Güç ikonu
    gui_draw_icon(startmenu->x + 8, startmenu->y + startmenu->height - 36, 12, 24);
    vga_draw_text(startmenu->x + 40, startmenu->y + startmenu->height - 30, "Kapat", GUI_COLOR_WHITE, GUI_COLOR_TRANSPARENT);
    
    // Yeniden başlat ikonu
    gui_draw_icon(startmenu->x + 120, startmenu->y + startmenu->height - 36, 13, 24);
    vga_draw_text(startmenu->x + 152, startmenu->y + startmenu->height - 30, "Yeniden Başlat", GUI_COLOR_WHITE, GUI_COLOR_TRANSPARENT);
    
    // Uygulamalar listesi
    uint16_t app_y = startmenu->y + 50;
    uint16_t app_x = startmenu->x + 10;
    uint16_t app_height = 30;
    
    // Sık kullanılan uygulamalar
    const char* common_apps[] = {
        "Dosya Yöneticisi", "Terminal", "Tarayıcı", "Ayarlar", 
        "Metin Düzenleyici", "Hesap Makinesi", "Takvim", "Müzik Çalar"
    };
    
    for (uint8_t i = 0; i < 8; i++) {
        // Uygulama arkaplanı
        if (i % 2 == 0) {
            vga_fill_rect(app_x, app_y, startmenu->width - 20, app_height, GUI_COLOR_ALT_ROW);
        }
        
        // Uygulama ikonu (varsayılan ikon ID'lerini kullanıyoruz)
        gui_draw_icon(app_x + 4, app_y + (app_height - 24) / 2, 14 + i, 24);
        
        // Uygulama adı
        vga_draw_text(app_x + 36, app_y + (app_height - 16) / 2, common_apps[i], GUI_COLOR_WHITE, GUI_COLOR_TRANSPARENT);
        
        app_y += app_height;
    }
}

// Fare olayını işle
uint8_t taskbar_handle_mouse(uint16_t x, uint16_t y, uint8_t button, uint8_t state) {
    if (!taskbar || !taskbar->visible) return 0;
    
    // Y koordinatı taskbar alanında mı?
    if (y < VGA_HEIGHT - taskbar->height) {
        // Başlat menüsü açıksa fare menü içinde mi kontrol et
        if (startmenu->visible) {
            if (x >= startmenu->x && x < startmenu->x + startmenu->width &&
                y >= startmenu->y && y < startmenu->y + startmenu->height) {
                // Menü içinde tıklama işlemi
                if (button == MOUSE_LEFT_BUTTON && state == MOUSE_BUTTON_DOWN) {
                    startmenu_handle_click(x - startmenu->x, y - startmenu->y);
                    return 1;
                }
                return 1; // Fareyi yakala
            } else {
                // Menü dışına tıklama
                if (button == MOUSE_LEFT_BUTTON && state == MOUSE_BUTTON_DOWN) {
                    startmenu_toggle();
                    return 1;
                }
            }
        }
        return 0; // Taskbar dışı olayları geçir
    }
    
    // Başlat düğmesi tıklaması
    if (x < START_BUTTON_WIDTH) {
        if (button == MOUSE_LEFT_BUTTON && state == MOUSE_BUTTON_DOWN) {
            startmenu_toggle();
            return 1;
        }
    }
    
    // Dock içinde mi?
    if (x >= dock->x && x < dock->x + dock->width) {
        // Dock içinde tıklama
        if (button == MOUSE_LEFT_BUTTON && state == MOUSE_BUTTON_DOWN) {
            uint16_t relative_x = x - dock->x;
            uint8_t icon_index = relative_x / (dock->icon_size + dock->icon_spacing);
            
            if (icon_index < dock_item_count) {
                dock_item_click(icon_index);
                return 1;
            }
        }
        
        // Dock içinde hover
        if (state == MOUSE_MOVE) {
            uint16_t relative_x = x - dock->x;
            uint8_t icon_index = relative_x / (dock->icon_size + dock->icon_spacing);
            
            dock_item_hover(icon_index);
            return 1;
        }
    }
    
    // Sistem tepsisi içinde mi?
    if (x >= systray->x && x < systray->x + systray->width) {
        // Sistem tepsisinde tıklama
        if (button == MOUSE_LEFT_BUTTON && state == MOUSE_BUTTON_DOWN) {
            systray_handle_click(x - systray->x, y - systray->y);
            return 1;
        }
        
        return 1; // Fare olayını yakala
    }
    
    return 1; // Taskbar içinde tüm fare olaylarını yakala
}

// Dock öğesi tıklama işlemi
void dock_item_click(uint8_t index) {
    if (index >= dock_item_count) return;
    
    dock_item_t* item = &dock_items[index];
    
    // Dock öğesi işlevini çağır
    if (item->click_handler) {
        item->click_handler(item);
    }
    
    // Uygulamayı aç/öne getir
    if (item->app_id) {
        app_launch(item->app_id);
        item->state = DOCK_ITEM_STATE_RUNNING;
    }
    
    // Taskbar'ı yeniden çiz
    taskbar_draw();
}

// Dock öğesi üzerine gelme
void dock_item_hover(uint8_t index) {
    static uint8_t last_hover_index = 255;
    
    if (index >= dock_item_count) {
        // Hiçbir öğenin üzerinde değil
        if (last_hover_index != 255) {
            dock_items[last_hover_index].animation_state = DOCK_ANIMATION_NONE;
            last_hover_index = 255;
            taskbar_draw();
        }
        return;
    }
    
    // Yeni bir öğenin üzerine geldi
    if (index != last_hover_index) {
        // Önceki hover durumunu temizle
        if (last_hover_index != 255) {
            dock_items[last_hover_index].animation_state = DOCK_ANIMATION_NONE;
        }
        
        // Yeni hover durumunu ayarla
        dock_items[index].animation_state = DOCK_ANIMATION_HOVER;
        last_hover_index = index;
        
        // Taskbar'ı yeniden çiz
        taskbar_draw();
    }
}

// Başlat menüsü durumunu değiştir
void startmenu_toggle() {
    if (!startmenu) return;
    
    startmenu->visible = !startmenu->visible;
    taskbar_draw();
}

// Başlat menüsü içindeki tıklamaları işle
void startmenu_handle_click(uint16_t x, uint16_t y) {
    // Üst panel - kullanıcı hesabı tıklaması
    if (y < 40) {
        // TODO: Kullanıcı hesabı ekranı aç
        return;
    }
    
    // Alt panel - güç seçenekleri
    if (y > startmenu->height - 41) {
        if (x < 100) {
            // Kapatma
            // TODO: Sistemi kapat
        } else {
            // Yeniden başlat
            // TODO: Sistemi yeniden başlat
        }
        startmenu_toggle(); // Menüyü kapat
        return;
    }
    
    // Uygulamalar bölümü
    uint16_t app_y = 50;
    uint16_t app_height = 30;
    
    if (x > 10 && x < startmenu->width - 10 && y >= app_y) {
        uint8_t app_index = (y - app_y) / app_height;
        if (app_index < 8) { // 8 uygulama var
            // Uygulamayı başlat
            // TODO: İlgili uygulamayı başlat
            startmenu_toggle(); // Menüyü kapat
        }
    }
}

// Sistem tepsisi tıklama işle
void systray_handle_click(uint16_t x, uint16_t y) {
    uint16_t icon_x = systray->item_spacing;
    
    // Ağ ikonu
    if (x >= icon_x && x < icon_x + 16) {
        // TODO: Ağ ayarları penceresini aç
        return;
    }
    icon_x += 16 + systray->item_spacing;
    
    // Ses ikonu
    if (x >= icon_x && x < icon_x + 16) {
        // TODO: Ses ayarları penceresini aç
        return;
    }
    icon_x += 16 + systray->item_spacing;
    
    // Pil ikonu
    if (x >= icon_x && x < icon_x + 16) {
        // TODO: Güç ayarları penceresini aç
        return;
    }
    icon_x += 16 + systray->item_spacing;
    
    // Dil ikonu
    if (x >= icon_x && x < icon_x + 16) {
        // TODO: Dil ayarları penceresini aç
        return;
    }
    icon_x += 16 + systray->item_spacing;
    
    // Bildirim ikonu
    if (x >= icon_x && x < icon_x + 16) {
        // TODO: Bildirim merkezini aç
        return;
    }
    
    // Saat bölgesi - zaman tarih gösterimini aç
    if (x > systray->width - 80) {
        // TODO: Takvim/Saat penceresini aç
    }
}

// Sistem tepsisi öğelerini başlat
void systray_init_items() {
    // Ağ ikonu
    network_icon.icon_id = 20; // Varsayılan ikon ID'si
    network_icon.type = SYSTRAY_ITEM_NETWORK;
    network_icon.state = 1; // Bağlı
    
    // Ses ikonu
    volume_icon.icon_id = 21;
    volume_icon.type = SYSTRAY_ITEM_VOLUME;
    volume_icon.state = 75; // %75 ses
    
    // Pil ikonu
    battery_icon.icon_id = 22;
    battery_icon.type = SYSTRAY_ITEM_BATTERY;
    battery_icon.state = 80; // %80 şarj
    
    // Dil ikonu
    language_icon.icon_id = 23;
    language_icon.type = SYSTRAY_ITEM_LANGUAGE;
    language_icon.state = 0; // TR
    
    // Bildirim ikonu
    notification_icon.icon_id = 24;
    notification_icon.type = SYSTRAY_ITEM_NOTIFICATION;
    notification_icon.state = 0; // Bildirim yok
}

// Dock öğelerini başlat
void dock_init_items() {
    // Standart dock uygulamalarını tanımla
    const uint8_t default_dock_size = 5;
    
    // Dock öğeleri için bellek ayır
    dock_items = (dock_item_t*)malloc(sizeof(dock_item_t) * default_dock_size);
    if (!dock_items) return;
    
    dock_item_count = default_dock_size;
    
    // Dosya Yöneticisi
    dock_items[0].app_id = 1;
    dock_items[0].icon_id = 30;
    dock_items[0].state = DOCK_ITEM_STATE_NORMAL;
    dock_items[0].animation_state = DOCK_ANIMATION_NONE;
    dock_items[0].click_handler = NULL;
    strcpy(dock_items[0].name, "Dosya Yöneticisi");
    
    // Terminal
    dock_items[1].app_id = 2;
    dock_items[1].icon_id = 10;
    dock_items[1].state = DOCK_ITEM_STATE_NORMAL;
    dock_items[1].animation_state = DOCK_ANIMATION_NONE;
    dock_items[1].click_handler = NULL;
    strcpy(dock_items[1].name, "Terminal");
    
    // Web Tarayıcısı
    dock_items[2].app_id = 3;
    dock_items[2].icon_id = 31;
    dock_items[2].state = DOCK_ITEM_STATE_NORMAL;
    dock_items[2].animation_state = DOCK_ANIMATION_NONE;
    dock_items[2].click_handler = NULL;
    strcpy(dock_items[2].name, "Kalem Tarayıcı");
    
    // Metin Düzenleyici
    dock_items[3].app_id = 4;
    dock_items[3].icon_id = 32;
    dock_items[3].state = DOCK_ITEM_STATE_NORMAL;
    dock_items[3].animation_state = DOCK_ANIMATION_NONE;
    dock_items[3].click_handler = NULL;
    strcpy(dock_items[3].name, "Metin Düzenleyici");
    
    // Sistem Ayarları
    dock_items[4].app_id = 5;
    dock_items[4].icon_id = 33;
    dock_items[4].state = DOCK_ITEM_STATE_NORMAL;
    dock_items[4].animation_state = DOCK_ANIMATION_NONE;
    dock_items[4].click_handler = NULL;
    strcpy(dock_items[4].name, "Sistem Ayarları");
}

// Dock'a yeni bir öğe ekle
uint8_t dock_add_item(uint8_t app_id, uint8_t icon_id, const char* name, dock_item_click_handler_t click_handler) {
    // Dock öğeleri için bellek yeniden ayır
    dock_item_t* new_items = (dock_item_t*)realloc(dock_items, sizeof(dock_item_t) * (dock_item_count + 1));
    if (!new_items) return 0;
    
    dock_items = new_items;
    
    // Yeni öğeyi ekle
    dock_items[dock_item_count].app_id = app_id;
    dock_items[dock_item_count].icon_id = icon_id;
    dock_items[dock_item_count].state = DOCK_ITEM_STATE_NORMAL;
    dock_items[dock_item_count].animation_state = DOCK_ANIMATION_NONE;
    dock_items[dock_item_count].click_handler = click_handler;
    
    if (name) {
        strncpy(dock_items[dock_item_count].name, name, MAX_DOCK_ITEM_NAME_LENGTH);
        dock_items[dock_item_count].name[MAX_DOCK_ITEM_NAME_LENGTH - 1] = '\0';
    } else {
        dock_items[dock_item_count].name[0] = '\0';
    }
    
    dock_item_count++;
    
    // Taskbar'ı yeniden çiz
    taskbar_draw();
    
    return dock_item_count - 1;
}

// Dock'tan öğe kaldır
void dock_remove_item(uint8_t index) {
    if (index >= dock_item_count) return;
    
    // Son öğe değilse, diğer öğeleri kaydır
    if (index < dock_item_count - 1) {
        memmove(&dock_items[index], &dock_items[index + 1], sizeof(dock_item_t) * (dock_item_count - index - 1));
    }
    
    // Dock öğeleri için bellek yeniden ayır
    dock_item_count--;
    dock_item_t* new_items = (dock_item_t*)realloc(dock_items, sizeof(dock_item_t) * dock_item_count);
    if (new_items || dock_item_count == 0) {
        dock_items = new_items;
    }
    
    // Taskbar'ı yeniden çiz
    taskbar_draw();
}

// Dock öğesi durumunu güncelle
void dock_update_item_state(uint8_t app_id, uint8_t state) {
    for (uint8_t i = 0; i < dock_item_count; i++) {
        if (dock_items[i].app_id == app_id) {
            dock_items[i].state = state;
            taskbar_draw();
            return;
        }
    }
}

// Dock öğesi ikonunu güncelle
void dock_update_item_icon(uint8_t app_id, uint8_t icon_id) {
    for (uint8_t i = 0; i < dock_item_count; i++) {
        if (dock_items[i].app_id == app_id) {
            dock_items[i].icon_id = icon_id;
            taskbar_draw();
            return;
        }
    }
}

// Sistrem tray öğesi ekle
void systray_add_item(systray_item_t* item) {
    // Sistem tepsisi öğeleri şimdilik statik
    // TODO: Dinamik sistem tepsisi öğeleri ekleme kodu
}

// Taskbar'ı temizle ve yok et
void taskbar_cleanup() {
    if (dock_items) {
        free(dock_items);
        dock_items = NULL;
        dock_item_count = 0;
    }
    
    if (startmenu) {
        free(startmenu);
        startmenu = NULL;
    }
    
    if (systray) {
        free(systray);
        systray = NULL;
    }
    
    if (dock) {
        free(dock);
        dock = NULL;
    }
    
    if (taskbar) {
        free(taskbar);
        taskbar = NULL;
    }
} 