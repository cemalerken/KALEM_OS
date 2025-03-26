#include "../include/context_menu.h"
#include "../include/gui.h"
#include "../include/vga.h"
#include "../include/launcher.h"
#include "../include/terminal.h"
#include "../include/desktop.h"
#include <string.h>
#include <stdlib.h>

// Masaüstü bağlam menüsü
static context_menu_t* desktop_menu = NULL;

// Alt menüler
static context_menu_t* view_menu = NULL;
static context_menu_t* sort_menu = NULL;
static context_menu_t* create_menu = NULL;
static context_menu_t* display_menu = NULL;

// Masaüstü menü eylem işleyicileri
static void desktop_menu_terminal(void* data);
static void desktop_menu_root_terminal(void* data);
static void desktop_menu_refresh(void* data);
static void desktop_menu_search(void* data);
static void desktop_menu_settings(void* data);
static void desktop_menu_copy(void* data);
static void desktop_menu_paste(void* data);
static void desktop_menu_cut(void* data);
static void desktop_menu_help(void* data);
static void desktop_menu_about(void* data);

// Yeni eklenen işleyiciler
static void desktop_menu_edit_desktop(void* data);
static void desktop_menu_arrange_icons(void* data);
static void desktop_menu_change_wallpaper(void* data);
static void desktop_menu_display_settings(void* data);
static void desktop_menu_create_folder(void* data);
static void desktop_menu_create_shortcut(void* data);
static void desktop_menu_create_text_file(void* data);
static void desktop_menu_sort_by_name(void* data);
static void desktop_menu_sort_by_size(void* data);
static void desktop_menu_sort_by_type(void* data);
static void desktop_menu_sort_by_date(void* data);
static void desktop_menu_view_large_icons(void* data);
static void desktop_menu_view_small_icons(void* data);
static void desktop_menu_view_list(void* data);
static void desktop_menu_view_details(void* data);

// Alt menüleri oluştur
static void create_view_menu() {
    if (view_menu) {
        context_menu_destroy(view_menu);
    }
    
    view_menu = context_menu_create("Görünüm", 0, 0, MENU_TYPE_CUSTOM);
    if (view_menu) {
        context_menu_add_item(view_menu, "Büyük Simgeler", MENU_ITEM_RADIO, desktop_menu_view_large_icons, NULL)->state = 1;
        context_menu_add_item(view_menu, "Küçük Simgeler", MENU_ITEM_RADIO, desktop_menu_view_small_icons, NULL);
        context_menu_add_item(view_menu, "Liste", MENU_ITEM_RADIO, desktop_menu_view_list, NULL);
        context_menu_add_item(view_menu, "Ayrıntılar", MENU_ITEM_RADIO, desktop_menu_view_details, NULL);
    }
}

static void create_sort_menu() {
    if (sort_menu) {
        context_menu_destroy(sort_menu);
    }
    
    sort_menu = context_menu_create("Sıralama", 0, 0, MENU_TYPE_CUSTOM);
    if (sort_menu) {
        context_menu_add_item(sort_menu, "Ada Göre", MENU_ITEM_RADIO, desktop_menu_sort_by_name, NULL)->state = 1;
        context_menu_add_item(sort_menu, "Boyuta Göre", MENU_ITEM_RADIO, desktop_menu_sort_by_size, NULL);
        context_menu_add_item(sort_menu, "Türe Göre", MENU_ITEM_RADIO, desktop_menu_sort_by_type, NULL);
        context_menu_add_item(sort_menu, "Tarihe Göre", MENU_ITEM_RADIO, desktop_menu_sort_by_date, NULL);
        context_menu_add_separator(sort_menu);
        context_menu_add_item(sort_menu, "Artan Sırada", MENU_ITEM_CHECKBOX, NULL, NULL)->state = 1;
        context_menu_add_item(sort_menu, "Azalan Sırada", MENU_ITEM_CHECKBOX, NULL, NULL);
    }
}

static void create_create_menu() {
    if (create_menu) {
        context_menu_destroy(create_menu);
    }
    
    create_menu = context_menu_create("Oluştur", 0, 0, MENU_TYPE_CUSTOM);
    if (create_menu) {
        context_menu_add_item(create_menu, "Klasör", MENU_ITEM_NORMAL, desktop_menu_create_folder, NULL)->icon_id = 1;
        context_menu_add_item(create_menu, "Kısayol", MENU_ITEM_NORMAL, desktop_menu_create_shortcut, NULL)->icon_id = 3;
        context_menu_add_item(create_menu, "Metin Belgesi", MENU_ITEM_NORMAL, desktop_menu_create_text_file, NULL);
    }
}

static void create_display_menu() {
    if (display_menu) {
        context_menu_destroy(display_menu);
    }
    
    display_menu = context_menu_create("Ekran", 0, 0, MENU_TYPE_CUSTOM);
    if (display_menu) {
        context_menu_add_item(display_menu, "Duvar Kağıdını Değiştir", MENU_ITEM_NORMAL, desktop_menu_change_wallpaper, NULL);
        context_menu_add_item(display_menu, "Ekran Ayarları", MENU_ITEM_NORMAL, desktop_menu_display_settings, NULL);
    }
}

// Masaüstü sağ tık menüsünü oluştur
void desktop_menu_init() {
    // Önce alt menüleri oluştur
    create_view_menu();
    create_sort_menu();
    create_create_menu();
    create_display_menu();
    
    // Önceki menü varsa yok et
    if (desktop_menu) {
        context_menu_destroy(desktop_menu);
    }
    
    // Yeni menü oluştur
    desktop_menu = context_menu_create("Masaüstü", 0, 0, MENU_TYPE_DESKTOP);
    
    if (desktop_menu) {
        // Masaüstü yönetimi
        context_menu_add_item(desktop_menu, "Masaüstünü Düzenle", MENU_ITEM_NORMAL, desktop_menu_edit_desktop, NULL)->icon_id = 5;
        context_menu_add_item(desktop_menu, "Simgeleri Düzenle", MENU_ITEM_NORMAL, desktop_menu_arrange_icons, NULL)->icon_id = 6;
        context_menu_add_separator(desktop_menu);
        
        // Görünüm alt menüsü
        context_menu_add_submenu(desktop_menu, "Görünüm", view_menu);
        context_menu_add_submenu(desktop_menu, "Simgeleri Sırala", sort_menu);
        context_menu_add_submenu(desktop_menu, "Ekran", display_menu);
        context_menu_add_submenu(desktop_menu, "Yeni", create_menu);
        context_menu_add_separator(desktop_menu);
        
        // Terminal ve arama
        context_menu_add_item(desktop_menu, "Terminal", MENU_ITEM_NORMAL, desktop_menu_terminal, NULL)->icon_id = 10;
        context_menu_add_item(desktop_menu, "Root olarak terminal", MENU_ITEM_NORMAL, desktop_menu_root_terminal, NULL)->icon_id = 10;
        context_menu_add_item(desktop_menu, "Ara", MENU_ITEM_NORMAL, desktop_menu_search, NULL)->icon_id = 4;
        context_menu_add_separator(desktop_menu);
        
        // Düzenleme öğeleri
        context_menu_add_item(desktop_menu, "Kes", MENU_ITEM_NORMAL, desktop_menu_cut, NULL)->icon_id = 7;
        context_menu_add_item(desktop_menu, "Kopyala", MENU_ITEM_NORMAL, desktop_menu_copy, NULL)->icon_id = 8;
        context_menu_add_item(desktop_menu, "Yapıştır", MENU_ITEM_NORMAL, desktop_menu_paste, NULL)->icon_id = 9;
        context_menu_add_separator(desktop_menu);
        
        // Ekranı yenile
        context_menu_add_item(desktop_menu, "Ekranı yenile", MENU_ITEM_NORMAL, desktop_menu_refresh, NULL)->icon_id = 6;
        context_menu_add_separator(desktop_menu);
        
        // Sistem öğeleri
        context_menu_add_item(desktop_menu, "Sistem Ayarları", MENU_ITEM_NORMAL, desktop_menu_settings, NULL)->icon_id = 5;
        context_menu_add_separator(desktop_menu);
        context_menu_add_item(desktop_menu, "Yardım", MENU_ITEM_NORMAL, desktop_menu_help, NULL)->icon_id = 0;
        context_menu_add_item(desktop_menu, "Hakkında", MENU_ITEM_NORMAL, desktop_menu_about, NULL)->icon_id = 0;
    }
}

// Masaüstü menü göster
void desktop_menu_show(uint32_t x, uint32_t y) {
    // Menü oluşturulmamış ise oluştur
    if (!desktop_menu) {
        desktop_menu_init();
    }
    
    // Menüyü göster
    context_menu_show(desktop_menu, x, y);
}

// Masaüstü menü kapat
void desktop_menu_hide() {
    if (desktop_menu) {
        context_menu_hide(desktop_menu);
    }
}

// Masaüstü sağ tık olayını işle
uint8_t desktop_menu_handle_right_click(uint32_t x, uint32_t y) {
    // Önce uygulama ikonlarını kontrol et
    // Eğer uygulama ikonu üzerinde sağ tık yapıldıysa app_context_menu'yü aç
    desktop_icon_t* icon = desktop_find_icon_at(x, y);
    if (icon) {
        desktop_show_icon_context_menu(icon, x, y);
        return 1;
    }
    
    // Değilse masaüstü menüsünü göster
    desktop_menu_show(x, y);
    return 1;
}

// Terminal açma işleyicisi
static void desktop_menu_terminal(void* data) {
    // Terminal uygulamasını başlat
    terminal_create();
}

// Root olarak terminal açma işleyicisi
static void desktop_menu_root_terminal(void* data) {
    // Root haklarına sahip terminal uygulamasını başlat
    terminal_t* term = terminal_create();
    if (term) {
        term->is_root = 1;
        strcpy(term->prompt, "root# ");
    }
}

// Ekranı yenileme işleyicisi
static void desktop_menu_refresh(void* data) {
    // Masaüstünü yeniden çiz
    gui_desktop_draw();
}

// Arama işleyicisi
static void desktop_menu_search(void* data) {
    // Arama uygulamasını başlat veya arama çubuğunu göster
    // TODO: Arama uygulaması entegrasyonu
}

// Sistem ayarları işleyicisi
static void desktop_menu_settings(void* data) {
    // Sistem ayarları uygulamasını aç
    // TODO: Sistem ayarları uygulaması entegrasyonu
}

// Kopyalama işleyicisi
static void desktop_menu_copy(void* data) {
    // Seçili simgeleri kopyala
    desktop_copy_selected_icons();
}

// Yapıştırma işleyicisi
static void desktop_menu_paste(void* data) {
    // Panodaki içeriği yapıştır
    desktop_paste();
}

// Kesme işleyicisi
static void desktop_menu_cut(void* data) {
    // Seçili simgeleri kes
    desktop_cut_selected_icons();
}

// Yardım işleyicisi
static void desktop_menu_help(void* data) {
    // Yardım penceresini aç
    // TODO: Yardım uygulaması entegrasyonu
}

// Hakkında işleyicisi
static void desktop_menu_about(void* data) {
    // Sistem bilgisi penceresini aç
    // TODO: Sistem bilgisi uygulaması entegrasyonu
}

// Masaüstünü düzenleme işleyicisi
static void desktop_menu_edit_desktop(void* data) {
    // Masaüstü düzenleme modunu aç
    desktop_edit_mode(1);
}

// Simgeleri düzenleme işleyicisi
static void desktop_menu_arrange_icons(void* data) {
    // Simgeleri otomatik olarak düzenle
    desktop_arrange_icons();
}

// Duvar kağıdı değiştirme işleyicisi
static void desktop_menu_change_wallpaper(void* data) {
    // Duvar kağıdı değiştirme penceresini aç
    // TODO: Duvar kağıdı seçme uygulaması entegrasyonu
}

// Ekran ayarları işleyicisi
static void desktop_menu_display_settings(void* data) {
    // Ekran ayarları penceresini aç
    // TODO: Ekran ayarları entegrasyonu
}

// Klasör oluşturma işleyicisi
static void desktop_menu_create_folder(void* data) {
    // Masaüstünde yeni klasör oluştur
    // TODO: Klasör oluşturma işlevi
}

// Kısayol oluşturma işleyicisi
static void desktop_menu_create_shortcut(void* data) {
    // Masaüstünde yeni kısayol oluştur
    // TODO: Kısayol oluşturma işlevi
}

// Metin belgesi oluşturma işleyicisi
static void desktop_menu_create_text_file(void* data) {
    // Masaüstünde yeni metin belgesi oluştur
    // TODO: Metin belgesi oluşturma işlevi
}

// İsim sıralama işleyicisi
static void desktop_menu_sort_by_name(void* data) {
    // Simgeleri isme göre sırala
    // TODO: İsme göre sıralama işlevi
}

// Boyut sıralama işleyicisi
static void desktop_menu_sort_by_size(void* data) {
    // Simgeleri boyuta göre sırala
    // TODO: Boyuta göre sıralama işlevi
}

// Tür sıralama işleyicisi
static void desktop_menu_sort_by_type(void* data) {
    // Simgeleri türüne göre sırala
    // TODO: Türe göre sıralama işlevi
}

// Tarih sıralama işleyicisi
static void desktop_menu_sort_by_date(void* data) {
    // Simgeleri tarihine göre sırala
    // TODO: Tarihe göre sıralama işlevi
}

// Büyük simge görünümü işleyicisi
static void desktop_menu_view_large_icons(void* data) {
    // Büyük simge görünümünü ayarla
    // TODO: Büyük simge görünümü işlevi
}

// Küçük simge görünümü işleyicisi
static void desktop_menu_view_small_icons(void* data) {
    // Küçük simge görünümünü ayarla
    // TODO: Küçük simge görünümü işlevi
}

// Liste görünümü işleyicisi
static void desktop_menu_view_list(void* data) {
    // Liste görünümünü ayarla
    // TODO: Liste görünümü işlevi
}

// Ayrıntı görünümü işleyicisi
static void desktop_menu_view_details(void* data) {
    // Ayrıntı görünümünü ayarla
    // TODO: Ayrıntı görünümü işlevi
} 