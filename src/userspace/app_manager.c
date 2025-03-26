#include "../include/app_manager.h"
#include "../include/gui.h"
#include "../include/font.h"
#include "../include/vga.h"
#include "../include/launcher.h"
#include <string.h>
#include <stdlib.h>

#define MAX_APPS 100
#define MAX_INSTALLED_APPS 50

// İleri fonksiyon bildirimleri (forward declarations)
static void android_app_paint(gui_window_t* window);
static void app_manager_paint(gui_window_t* window);
static void app_store_paint(gui_window_t* window);
void settings_paint_apps(gui_window_t* window);

// Uygulama listesi
static app_t apps[MAX_APPS];
static uint32_t app_count = 0;

// Kurulu uygulamalar
static app_t* installed_apps[MAX_INSTALLED_APPS];
static uint32_t installed_app_count = 0;

// Uygulama mağazası penceresi
static gui_window_t* store_window = NULL;

// Android uyumluluğu için sanal makine durumu
static struct {
    uint8_t initialized;
    uint8_t is_running;
    uint32_t available_memory;
    uint32_t used_memory;
    uint8_t sdk_version;
} android_vm = {0};

// Uygulama başlatma fonksiyonu pointer dizisi
static void (*app_launchers[MAX_APPS])() = {NULL};

// Her uygulama için dinamik başlatıcı fonksiyon oluştur
static void create_app_launcher(uint32_t app_index) {
    // Uygulamaya özel başlatıcı fonksiyon
    app_launchers[app_index] = apps[app_index].app_function;
    
    // Eğer uygulama fonksiyonu yoksa varsayılanı kullan
    if (app_launchers[app_index] == NULL) {
        app_launchers[app_index] = app_manager_show;
    }
}

// Android uygulamalarını başlatmak için fonksiyon
static void launch_android_app(const char* package_name) {
    app_manager_run_android_app(package_name);
}

// Varsayılan uygulama başlatıcı
static void default_app_launcher() {
    app_manager_show();
}

// Demo uygulama verileri oluştur
static void app_manager_create_demo_apps() {
    // Sistem uygulamaları
    app_t system_app1 = {
        .name = "Dosya Yöneticisi",
        .package_name = "tr.gov.kalemos.filemanager",
        .version = "1.0.0",
        .developer = "KALEM OS Geliştirme Ekibi",
        .description = "Dosya ve klasörleri yönetmek için basit bir uygulama",
        .icon_id = 1,
        .size_kb = 1024,
        .category = APP_CATEGORY_SYSTEM,
        .status = APP_STATUS_INSTALLED,
        .source = APP_SOURCE_SYSTEM,
        .is_android = 0,
        .desktop_shortcut = 1,
        .autostart = 0,
        .app_function = NULL
    };
    app_manager_add_app(&system_app1);
    
    app_t system_app2 = {
        .name = "Görev Yöneticisi",
        .package_name = "tr.gov.kalemos.taskmanager",
        .version = "1.0.0",
        .developer = "KALEM OS Geliştirme Ekibi",
        .description = "Sistem kaynaklarını ve çalışan uygulamaları izlemek için araç",
        .icon_id = 2,
        .size_kb = 768,
        .category = APP_CATEGORY_SYSTEM,
        .status = APP_STATUS_INSTALLED,
        .source = APP_SOURCE_SYSTEM,
        .is_android = 0,
        .desktop_shortcut = 0,
        .autostart = 0,
        .app_function = NULL
    };
    app_manager_add_app(&system_app2);
    
    // Ofis uygulamaları
    app_t office_app = {
        .name = "KALEM OS Ofis",
        .package_name = "tr.gov.kalemos.office",
        .version = "1.0.0",
        .developer = "KALEM OS Yazılım",
        .description = "Kelime işlemci, hesap tablosu ve sunum uygulamaları içeren ofis paketi",
        .icon_id = 3,
        .size_kb = 15360,
        .category = APP_CATEGORY_OFFICE,
        .status = APP_STATUS_INSTALLED,
        .source = APP_SOURCE_STORE,
        .is_android = 0,
        .desktop_shortcut = 1,
        .autostart = 0,
        .app_function = NULL
    };
    app_manager_add_app(&office_app);
    
    // Medya uygulamaları
    app_t media_app = {
        .name = "KALEM OS Media Player",
        .package_name = "tr.gov.kalemos.mediaplayer",
        .version = "1.0.0",
        .developer = "KALEM OS Multimedya",
        .description = "Ses ve video dosyalarını oynatmak için medya oynatıcı",
        .icon_id = 4,
        .size_kb = 8192,
        .category = APP_CATEGORY_MEDIA,
        .status = APP_STATUS_INSTALLED,
        .source = APP_SOURCE_STORE,
        .is_android = 0,
        .desktop_shortcut = 1,
        .autostart = 0,
        .app_function = NULL
    };
    app_manager_add_app(&media_app);
    
    // İnternet uygulamaları
    app_t internet_app = {
        .name = "E-posta",
        .package_name = "tr.gov.kalemos.email",
        .version = "1.0.0",
        .developer = "KALEM OS İnternet",
        .description = "E-posta gönderme ve alma uygulaması",
        .icon_id = 5,
        .size_kb = 4096,
        .category = APP_CATEGORY_INTERNET,
        .status = APP_STATUS_INSTALLED,
        .source = APP_SOURCE_STORE,
        .is_android = 0,
        .desktop_shortcut = 1,
        .autostart = 0,
        .app_function = NULL
    };
    app_manager_add_app(&internet_app);
    
    // Android uygulamaları
    app_t android_app1 = {
        .name = "Android Mesajlaşma",
        .package_name = "com.android.messaging",
        .version = "2.3.1",
        .developer = "Android",
        .description = "Android mesajlaşma uygulaması",
        .icon_id = 10,
        .size_kb = 12288,
        .category = APP_CATEGORY_INTERNET,
        .status = APP_STATUS_INSTALLED,
        .source = APP_SOURCE_ANDROID_APK,
        .is_android = 1,
        .desktop_shortcut = 1,
        .autostart = 0,
        .app_function = NULL
    };
    app_manager_add_app(&android_app1);
    
    app_t android_app2 = {
        .name = "Android Galeri",
        .package_name = "com.android.gallery",
        .version = "1.9.2",
        .developer = "Android",
        .description = "Android fotoğraf galerisi uygulaması",
        .icon_id = 11,
        .size_kb = 9216,
        .category = APP_CATEGORY_MEDIA,
        .status = APP_STATUS_INSTALLED,
        .source = APP_SOURCE_ANDROID_APK,
        .is_android = 1,
        .desktop_shortcut = 1,
        .autostart = 0,
        .app_function = NULL
    };
    app_manager_add_app(&android_app2);
    
    // Mağazadan kurulabilir uygulamalar
    app_t store_app1 = {
        .name = "KALEM OS Oyun Merkezi",
        .package_name = "tr.gov.kalemos.games",
        .version = "1.0.0",
        .developer = "KALEM OS Eğlence",
        .description = "Eğlenceli oyunlar ve aktiviteler içeren uygulama",
        .icon_id = 6,
        .size_kb = 20480,
        .category = APP_CATEGORY_GAMES,
        .status = APP_STATUS_NOT_INSTALLED,
        .source = APP_SOURCE_STORE,
        .is_android = 0,
        .desktop_shortcut = 0,
        .autostart = 0,
        .app_function = NULL
    };
    app_manager_add_app(&store_app1);
    
    app_t store_app2 = {
        .name = "KALEM OS Geliştirici Araçları",
        .package_name = "tr.gov.kalemos.devtools",
        .version = "1.0.0",
        .developer = "KALEM OS Geliştirme",
        .description = "Yazılım geliştirme araçları",
        .icon_id = 7,
        .size_kb = 51200,
        .category = APP_CATEGORY_DEVELOPMENT,
        .status = APP_STATUS_NOT_INSTALLED,
        .source = APP_SOURCE_STORE,
        .is_android = 0,
        .desktop_shortcut = 0,
        .autostart = 0,
        .app_function = NULL
    };
    app_manager_add_app(&store_app2);
}

// Uygulama yöneticisini başlat
void app_manager_init() {
    // Android VM'i başlat
    android_vm.initialized = 1;
    android_vm.is_running = 0;
    android_vm.available_memory = 256 * 1024; // 256 MB
    android_vm.used_memory = 0;
    android_vm.sdk_version = 23; // Android 6.0
    
    // Demo uygulamaları oluştur
    app_manager_create_demo_apps();
    
    // Kurulu uygulamaları işaretle
    installed_app_count = 0;
    for (uint32_t i = 0; i < app_count; i++) {
        if (apps[i].status == APP_STATUS_INSTALLED) {
            installed_apps[installed_app_count++] = &apps[i];
            
            // Masaüstü kısayolu varsa ekle
            if (apps[i].desktop_shortcut) {
                // Launcher'a simge ekle
                launcher_add_icon(apps[i].name, 50 + (installed_app_count % 5) * 100, 
                                 50 + (installed_app_count / 5) * 100, 
                                 apps[i].is_android ? GUI_COLOR_LIGHT_GREEN : GUI_COLOR_LIGHT_BLUE,
                                 app_launchers[i]);
            }
        }
    }
}

// Uygulama yöneticisi penceresini göster
void app_manager_show() {
    gui_window_t* window = gui_window_create("KALEM OS Uygulama Yöneticisi", 100, 50, 600, 500, GUI_WINDOW_STYLE_NORMAL);
    if (!window) return;
    
    window->on_paint = app_manager_paint;
    gui_window_show(window);
}

// Uygulama mağazasını göster
void app_store() {
    if (store_window) {
        gui_window_focus(store_window);
        return;
    }
    
    store_window = gui_window_create("KALEM OS Uygulama Mağazası", 80, 40, 640, 520, GUI_WINDOW_STYLE_NORMAL);
    if (!store_window) return;
    
    store_window->on_paint = app_store_paint;
    store_window->user_data = NULL; // Gerekli veri yoksa NULL geçilir
    gui_window_show(store_window);
}

// Kategori adını al
const char* app_manager_get_category_name(app_category_t category) {
    static const char* names[] = {
        "Sistem",
        "Ofis",
        "Medya",
        "İnternet",
        "Oyunlar",
        "Geliştirme",
        "Araçlar",
        "Diğer"
    };
    
    if (category < APP_CATEGORY_SYSTEM || category > APP_CATEGORY_OTHER) {
        return "Bilinmeyen";
    }
    
    return names[category];
}

// Durum adını al
const char* app_manager_get_status_name(app_status_t status) {
    static const char* names[] = {
        "Kurulu Değil",
        "Kuruluyor",
        "Kurulu",
        "Güncelleniyor",
        "Kaldırılıyor"
    };
    
    if (status < APP_STATUS_NOT_INSTALLED || status > APP_STATUS_REMOVING) {
        return "Bilinmeyen";
    }
    
    return names[status];
}

// Kaynak adını al
const char* app_manager_get_source_name(app_source_t source) {
    static const char* names[] = {
        "Sistem",
        "Mağaza",
        "Dosya",
        "Android APK"
    };
    
    if (source < APP_SOURCE_SYSTEM || source > APP_SOURCE_ANDROID_APK) {
        return "Bilinmeyen";
    }
    
    return names[source];
}

// Uygulama ekle
int app_manager_add_app(const app_t* app) {
    if (!app || app_count >= MAX_APPS) return -1;
    
    // Uygulamayı listeye ekle
    memcpy(&apps[app_count], app, sizeof(app_t));
    
    // Uygulama için başlatıcı oluştur
    create_app_launcher(app_count);
    
    app_count++;
    
    return 0;
}

// Uygulama kur
int app_manager_install_app(const char* package_name) {
    if (!package_name) return -1;
    
    // Uygulamayı bul
    for (uint32_t i = 0; i < app_count; i++) {
        if (strcmp(apps[i].package_name, package_name) == 0) {
            // Zaten kurulu mu?
            if (apps[i].status == APP_STATUS_INSTALLED) {
                return 0; // Başarılı, zaten kurulu
            }
            
            // Kurulum durumunu güncelle
            apps[i].status = APP_STATUS_INSTALLING;
            
            // Kurulum simülasyonu (gerçek sistemde dosya kopyalama vb. olacak)
            // Burada sadece durumu güncelliyoruz
            apps[i].status = APP_STATUS_INSTALLED;
            
            // Kurulu uygulamalar listesine ekle
            if (installed_app_count < MAX_INSTALLED_APPS) {
                installed_apps[installed_app_count++] = &apps[i];
            }
            
            // Masaüstü kısayolu isteniyorsa ekle
            if (apps[i].desktop_shortcut) {
                app_manager_add_desktop_shortcut(package_name);
            }
            
            return 0; // Başarılı
        }
    }
    
    return -1; // Uygulama bulunamadı
}

// Uygulama kaldır
int app_manager_uninstall_app(const char* package_name) {
    if (!package_name) return -1;
    
    // Uygulamayı bul
    for (uint32_t i = 0; i < app_count; i++) {
        if (strcmp(apps[i].package_name, package_name) == 0) {
            // Sistem uygulamaları kaldırılamaz
            if (apps[i].source == APP_SOURCE_SYSTEM) {
                return -2; // Sistem uygulaması kaldırılamaz
            }
            
            // Zaten kurulu değil mi?
            if (apps[i].status != APP_STATUS_INSTALLED) {
                return 0; // Başarılı, zaten kurulu değil
            }
            
            // Kaldırma durumunu güncelle
            apps[i].status = APP_STATUS_REMOVING;
            
            // Masaüstü kısayolunu kaldır
            if (apps[i].desktop_shortcut) {
                app_manager_remove_desktop_shortcut(package_name);
            }
            
            // Kaldırma simülasyonu (gerçek sistemde dosya silme vb. olacak)
            // Burada sadece durumu güncelliyoruz
            apps[i].status = APP_STATUS_NOT_INSTALLED;
            apps[i].desktop_shortcut = 0;
            
            // Kurulu uygulamalar listesinden çıkar
            for (uint32_t j = 0; j < installed_app_count; j++) {
                if (installed_apps[j] == &apps[i]) {
                    // Son elemanı buraya taşı ve sayacı azalt
                    installed_apps[j] = installed_apps[--installed_app_count];
                    break;
                }
            }
            
            return 0; // Başarılı
        }
    }
    
    return -1; // Uygulama bulunamadı
}

// Uygulama başlat
int app_manager_launch_app(const char* package_name) {
    if (!package_name) return -1;
    
    // Uygulamayı bul
    for (uint32_t i = 0; i < app_count; i++) {
        if (strcmp(apps[i].package_name, package_name) == 0) {
            // Kurulu değil mi?
            if (apps[i].status != APP_STATUS_INSTALLED) {
                return -2; // Kurulu değil
            }
            
            // Android uygulaması mı?
            if (apps[i].is_android) {
                return app_manager_run_android_app(apps[i].package_name);
            }
            
            // Başlatma fonksiyonu var mı?
            if (apps[i].app_function) {
                apps[i].app_function();
                return 0; // Başarılı
            }
            
            return -3; // Başlatma fonksiyonu yok
        }
    }
    
    return -1; // Uygulama bulunamadı
}

// APK dosyasını kur
int app_manager_install_apk(const char* apk_path) {
    if (!apk_path) return -1;
    
    // Android VM başlatılmış mı?
    if (!android_vm.initialized) {
        // Android VM'i başlat
        android_vm.initialized = 1;
        android_vm.is_running = 0;
        android_vm.available_memory = 256 * 1024; // 256 MB
        android_vm.used_memory = 0;
        android_vm.sdk_version = 23; // Android 6.0
    }
    
    // APK bilgilerini çıkar
    apk_info_t* apk_info = app_manager_extract_apk_info(apk_path);
    if (!apk_info) return -2; // APK bilgileri alınamadı
    
    // Uygulama zaten var mı kontrol et
    for (uint32_t i = 0; i < app_count; i++) {
        if (strcmp(apps[i].package_name, apk_info->package_name) == 0) {
            // Güncelleme yap
            apps[i].status = APP_STATUS_UPDATING;
            
            // Versiyon güncelle
            strncpy(apps[i].version, apk_info->version, sizeof(apps[i].version) - 1);
            
            // Kurulum simülasyonu
            apps[i].status = APP_STATUS_INSTALLED;
            
            free(apk_info);
            return 0; // Başarılı (güncelleme)
        }
    }
    
    // Yeni uygulama oluştur
    app_t new_app = {0};
    strncpy(new_app.name, apk_info->app_name, sizeof(new_app.name) - 1);
    strncpy(new_app.package_name, apk_info->package_name, sizeof(new_app.package_name) - 1);
    strncpy(new_app.version, apk_info->version, sizeof(new_app.version) - 1);
    strncpy(new_app.developer, "Android Uygulaması", sizeof(new_app.developer) - 1);
    new_app.icon_id = apk_info->icon_id;
    new_app.size_kb = 10240; // Varsayılan 10MB
    new_app.category = APP_CATEGORY_OTHER; // Varsayılan kategori
    new_app.status = APP_STATUS_INSTALLED;
    new_app.source = APP_SOURCE_ANDROID_APK;
    new_app.is_android = 1;
    new_app.desktop_shortcut = 1; // Varsayılan olarak masaüstüne ekle
    
    // Uygulamayı ekle
    int result = app_manager_add_app(&new_app);
    
    // Masaüstü kısayolu ekle
    if (result == 0 && new_app.desktop_shortcut) {
        app_manager_add_desktop_shortcut(new_app.package_name);
    }
    
    free(apk_info);
    return result;
}

// APK bilgilerini çıkar
apk_info_t* app_manager_extract_apk_info(const char* apk_path) {
    if (!apk_path) return NULL;
    
    // Demo amaçlı sabit bir APK bilgisi döndür
    apk_info_t* info = (apk_info_t*)malloc(sizeof(apk_info_t));
    if (!info) return NULL;
    
    // Demo bilgilerini doldur
    strcpy(info->package_name, "com.example.demoapp");
    strcpy(info->app_name, "Demo Android Uygulaması");
    strcpy(info->version, "1.0.0");
    info->min_sdk = 19; // Android 4.4
    info->target_sdk = 23; // Android 6.0
    info->icon_id = 15; // Varsayılan ikon ID
    
    // Tüm izinleri sıfırla
    memset(info->has_permission, 0, sizeof(info->has_permission));
    
    // Bazı demo izinleri ayarla
    info->has_permission[0] = 1; // INTERNET
    info->has_permission[1] = 1; // STORAGE
    
    return info;
}

// Masaüstü kısayolu ekle
int app_manager_add_desktop_shortcut(const char* package_name) {
    if (!package_name) return -1;
    
    // Uygulamayı bul
    for (uint32_t i = 0; i < app_count; i++) {
        if (strcmp(apps[i].package_name, package_name) == 0) {
            // Zaten masaüstü kısayolu var mı?
            if (apps[i].desktop_shortcut) {
                return 0; // Başarılı, zaten var
            }
            
            // Masaüstü kısayolu ekle
            apps[i].desktop_shortcut = 1;
            
            // Launcher'a simge ekle
            launcher_add_icon(apps[i].name, 50 + (i % 5) * 100, 50 + (i / 5) * 100, 
                             apps[i].is_android ? GUI_COLOR_LIGHT_GREEN : GUI_COLOR_LIGHT_BLUE,
                             app_launchers[i]);
            
            return 0; // Başarılı
        }
    }
    
    return -1; // Uygulama bulunamadı
}

// Masaüstü kısayolu kaldır
int app_manager_remove_desktop_shortcut(const char* package_name) {
    if (!package_name) return -1;
    
    // Uygulamayı bul
    for (uint32_t i = 0; i < app_count; i++) {
        if (strcmp(apps[i].package_name, package_name) == 0) {
            // Masaüstü kısayolu var mı?
            if (!apps[i].desktop_shortcut) {
                return 0; // Başarılı, zaten yok
            }
            
            // Masaüstü kısayolunu kaldır
            apps[i].desktop_shortcut = 0;
            
            // Gerçek sistemde launcher'dan simgeyi kaldırmak gerekiyor
            // Demo sürümünde sadece işaretliyoruz
            
            return 0; // Başarılı
        }
    }
    
    return -1; // Uygulama bulunamadı
}

// Android uygulamasını çalıştır
int app_manager_run_android_app(const char* package_name) {
    if (!package_name) return -1;
    
    // Uygulamayı bul
    app_t* target_app = NULL;
    for (uint32_t i = 0; i < app_count; i++) {
        if (strcmp(apps[i].package_name, package_name) == 0) {
            target_app = &apps[i];
            break;
        }
    }
    
    if (!target_app) return -1; // Uygulama bulunamadı
    
    // Android uygulaması değil mi?
    if (!target_app->is_android) {
        return -2; // Android uygulaması değil
    }
    
    // Android VM çalışıyor mu?
    if (!android_vm.is_running) {
        // Android VM'i başlat
        android_vm.is_running = 1;
    }
    
    // Android uygulama penceresini göster
    gui_window_t* window = gui_window_create(target_app->name, 100, 100, 480, 720, GUI_WINDOW_STYLE_NORMAL);
    if (!window) return -3;
    
    // Android uygulama penceresi çizim fonksiyonu
    window->on_paint = android_app_paint;
    window->user_data = target_app; // Uygulama bilgisini kaydet
    
    // Pencereyi göster
    gui_window_show(window);
    
    return 0; // Başarılı
}

// Android uygulama çizimi
static void android_app_paint(gui_window_t* window) {
    if (!window || !window->user_data) return;
    
    app_t* app = (app_t*)window->user_data;
    
    // Pencere içeriğini çiz
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    uint32_t width = window->client.width;
    uint32_t height = window->client.height;
    
    // Android uygulama arayüzü
    // Üst çubuk
    vga_fill_rect(x, y, width, 40, GUI_COLOR_DARK_GREEN);
    
    // Uygulama adı
    font_draw_string(x + 10, y + 15, app->name, GUI_COLOR_WHITE, 0);
    
    // İçerik alanı (beyaz arkaplan)
    vga_fill_rect(x, y + 40, width, height - 40, GUI_COLOR_WHITE);
    
    // Demo içerik
    char buffer[128];
    
    // Uygulama bilgileri
    snprintf(buffer, sizeof(buffer), "Paket: %s", app->package_name);
    font_draw_string(x + 20, y + 70, buffer, GUI_COLOR_BLACK, 0);
    
    snprintf(buffer, sizeof(buffer), "Versiyon: %s", app->version);
    font_draw_string(x + 20, y + 90, buffer, GUI_COLOR_BLACK, 0);
    
    snprintf(buffer, sizeof(buffer), "Geliştirici: %s", app->developer);
    font_draw_string(x + 20, y + 110, buffer, GUI_COLOR_BLACK, 0);
    
    // Android durum bilgisi
    font_draw_string(x + 20, y + 150, "Android Uyumluluk Katmanı:", GUI_COLOR_DARK_BLUE, 0);
    
    snprintf(buffer, sizeof(buffer), "SDK Sürümü: %d (Android 6.0)", android_vm.sdk_version);
    font_draw_string(x + 30, y + 170, buffer, GUI_COLOR_BLACK, 0);
    
    snprintf(buffer, sizeof(buffer), "Kullanılan Bellek: %d MB / %d MB", 
            android_vm.used_memory / 1024, android_vm.available_memory / 1024);
    font_draw_string(x + 30, y + 190, buffer, GUI_COLOR_BLACK, 0);
    
    // Demo UI bileşenleri
    // Butonlar
    vga_fill_rect(x + 30, y + 240, 200, 40, GUI_COLOR_LIGHT_BLUE);
    font_draw_string_center(x + 30 + 100, y + 240 + 20, "Demo Buton 1", GUI_COLOR_BLACK, 0);
    
    vga_fill_rect(x + 250, y + 240, 200, 40, GUI_COLOR_LIGHT_BLUE);
    font_draw_string_center(x + 250 + 100, y + 240 + 20, "Demo Buton 2", GUI_COLOR_BLACK, 0);
    
    // Basit liste görünümü
    vga_fill_rect(x + 30, y + 300, 420, 200, GUI_COLOR_LIGHT_GRAY);
    
    // Liste başlığı
    vga_fill_rect(x + 30, y + 300, 420, 30, GUI_COLOR_DARK_GRAY);
    font_draw_string(x + 40, y + 310, "Android Uygulama Öğeleri", GUI_COLOR_WHITE, 0);
    
    // Liste öğeleri
    const char* items[] = {
        "Öğe 1 - Android uyumluluk", 
        "Öğe 2 - APK entegrasyonu", 
        "Öğe 3 - Java API desteği", 
        "Öğe 4 - Native köprüsü", 
        "Öğe 5 - Dalvik VM"
    };
    
    for (int i = 0; i < 5; i++) {
        vga_fill_rect(x + 30, y + 330 + i * 30, 420, 30, (i % 2) ? GUI_COLOR_WHITE : GUI_COLOR_VERY_LIGHT_GRAY);
        font_draw_string(x + 40, y + 340 + i * 30, items[i], GUI_COLOR_BLACK, 0);
    }
    
    // Alt çubuk
    vga_fill_rect(x, y + height - 50, width, 50, GUI_COLOR_DARK_GREEN);
    
    // Geri butonu
    vga_fill_rect(x + 10, y + height - 40, 30, 30, GUI_COLOR_WHITE);
    font_draw_string_center(x + 10 + 15, y + height - 40 + 15, "<", GUI_COLOR_BLACK, 0);
    
    // Ana ekran butonu
    vga_fill_rect(x + 50, y + height - 40, 30, 30, GUI_COLOR_WHITE);
    font_draw_string_center(x + 50 + 15, y + height - 40 + 15, "O", GUI_COLOR_BLACK, 0);
    
    // Son uygulamalar butonu
    vga_fill_rect(x + 90, y + height - 40, 30, 30, GUI_COLOR_WHITE);
    font_draw_string_center(x + 90 + 15, y + height - 40 + 15, "[]", GUI_COLOR_BLACK, 0);
}

// Uygulama yöneticisi çizimi
static void app_manager_paint(gui_window_t* window) {
    if (!window) return;
    
    // Pencere içeriğini çiz
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    uint32_t width = window->client.width;
    
    // Başlık
    font_draw_string_center(x + width / 2, y + 30, "KALEM OS Uygulama Yöneticisi", GUI_COLOR_DARK_BLUE, 0xFF);
    
    // Açıklama
    font_draw_string_center(x + width / 2, y + 60, "Sisteminizdeki uygulamaları yönetin", GUI_COLOR_BLACK, 0xFF);
    
    // Butonlar
    gui_draw_button(window, 50, 100, 150, 30, "Kurulu Uygulamalar", GUI_COLOR_BLUE, GUI_COLOR_WHITE);
    gui_draw_button(window, 220, 100, 150, 30, "Uygulama Mağazası", GUI_COLOR_GREEN, GUI_COLOR_WHITE);
    gui_draw_button(window, 390, 100, 150, 30, "APK Yükle", GUI_COLOR_ORANGE, GUI_COLOR_WHITE);
    
    // Uygulamalar listesi
    vga_draw_rect(x + 50, y + 150, 500, 300, GUI_COLOR_DARK_GRAY);
    
    // Liste başlığı
    vga_fill_rect(x + 50, y + 150, 500, 30, GUI_COLOR_DARK_BLUE);
    font_draw_string(x + 60, y + 160, "Ad", GUI_COLOR_WHITE, 0xFF);
    font_draw_string(x + 200, y + 160, "Versiyon", GUI_COLOR_WHITE, 0xFF);
    font_draw_string(x + 280, y + 160, "Tür", GUI_COLOR_WHITE, 0xFF);
    font_draw_string(x + 350, y + 160, "Kaynak", GUI_COLOR_WHITE, 0xFF);
    font_draw_string(x + 450, y + 160, "Durum", GUI_COLOR_WHITE, 0xFF);
    
    // Uygulamaları listele (ilk 8 uygulama)
    uint32_t list_count = app_count > 8 ? 8 : app_count;
    
    for (uint32_t i = 0; i < list_count; i++) {
        // Satır arkaplanı
        vga_fill_rect(x + 50, y + 180 + i * 30, 500, 30, (i % 2) ? GUI_COLOR_LIGHT_GRAY : GUI_COLOR_WHITE);
        
        // Uygulama bilgileri
        font_draw_string(x + 60, y + 190 + i * 30, apps[i].name, GUI_COLOR_BLACK, 0xFF);
        font_draw_string(x + 200, y + 190 + i * 30, apps[i].version, GUI_COLOR_BLACK, 0xFF);
        font_draw_string(x + 280, y + 190 + i * 30, app_manager_get_category_name(apps[i].category), GUI_COLOR_BLACK, 0xFF);
        font_draw_string(x + 350, y + 190 + i * 30, app_manager_get_source_name(apps[i].source), GUI_COLOR_BLACK, 0xFF);
        
        // Durum (renkli gösterim)
        uint8_t status_color;
        switch(apps[i].status) {
            case APP_STATUS_INSTALLED:
                status_color = GUI_COLOR_DARK_GREEN;
                break;
            case APP_STATUS_INSTALLING:
            case APP_STATUS_UPDATING:
                status_color = GUI_COLOR_DARK_BLUE;
                break;
            case APP_STATUS_REMOVING:
                status_color = GUI_COLOR_DARK_RED;
                break;
            default:
                status_color = GUI_COLOR_DARK_GRAY;
        }
        
        font_draw_string(x + 450, y + 190 + i * 30, app_manager_get_status_name(apps[i].status), status_color, 0xFF);
    }
    
    // Bilgi ve durum metni
    char status_text[128];
    snprintf(status_text, sizeof(status_text), "Toplam Uygulama: %d | Kurulu: %d | Android Uyumlu: %d", 
             app_count, installed_app_count, 2); // Demo için sabit Android uygulaması sayısı
    
    font_draw_string(x + 50, y + 470, status_text, GUI_COLOR_DARK_GRAY, 0xFF);
}

// Uygulama mağazası çizimi
static void app_store_paint(gui_window_t* window) {
    if (!window) return;
    
    // Pencere içeriğini çiz
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    uint32_t width = window->client.width;
    
    // Başlık
    font_draw_string_center(x + width / 2, y + 30, "KALEM OS Uygulama Mağazası", GUI_COLOR_DARK_BLUE, 0xFF);
    
    // Açıklama
    font_draw_string_center(x + width / 2, y + 60, "Yeni uygulamalar keşfedin ve yükleyin", GUI_COLOR_BLACK, 0xFF);
    
    // Kategoriler
    gui_draw_button(window, 50, 100, 80, 30, "Tümü", GUI_COLOR_BLUE, GUI_COLOR_WHITE);
    gui_draw_button(window, 140, 100, 80, 30, "Ofis", GUI_COLOR_BLUE, GUI_COLOR_WHITE);
    gui_draw_button(window, 230, 100, 80, 30, "Medya", GUI_COLOR_BLUE, GUI_COLOR_WHITE);
    gui_draw_button(window, 320, 100, 80, 30, "İnternet", GUI_COLOR_BLUE, GUI_COLOR_WHITE);
    gui_draw_button(window, 410, 100, 80, 30, "Oyunlar", GUI_COLOR_BLUE, GUI_COLOR_WHITE);
    gui_draw_button(window, 500, 100, 80, 30, "Araçlar", GUI_COLOR_BLUE, GUI_COLOR_WHITE);
    
    // Arama kutusu
    vga_draw_rect(x + 50, y + 150, 400, 30, GUI_COLOR_DARK_GRAY);
    font_draw_string(x + 60, y + 160, "Uygulama ara...", GUI_COLOR_DARK_GRAY, 0xFF);
    gui_draw_button(window, 460, 150, 80, 30, "Ara", GUI_COLOR_GREEN, GUI_COLOR_WHITE);
    
    // Uygulamalar listesi (grid görünümü)
    // Önerilen uygulamalar başlığı
    font_draw_string(x + 50, y + 200, "Önerilen Uygulamalar", GUI_COLOR_DARK_BLUE, 0xFF);
    
    // Uygulama kartları
    const int card_width = 150;
    const int card_height = 180;
    const int cards_per_row = 3;
    
    for (uint32_t i = 0; i < 6; i++) { // İlk 6 uygulamayı göster
        if (i >= app_count) break;
        
        int row = i / cards_per_row;
        int col = i % cards_per_row;
        
        int card_x = x + 50 + col * (card_width + 20);
        int card_y = y + 230 + row * (card_height + 20);
        
        // Kart arkaplanı
        vga_fill_rect(card_x, card_y, card_width, card_height, GUI_COLOR_WHITE);
        vga_draw_rect(card_x, card_y, card_width, card_height, GUI_COLOR_DARK_GRAY);
        
        // Uygulama simgesi
        vga_fill_rect(card_x + 50, card_y + 20, 50, 50, apps[i].is_android ? GUI_COLOR_LIGHT_GREEN : GUI_COLOR_LIGHT_BLUE);
        
        // Uygulama adı
        font_draw_string_center(card_x + card_width/2, card_y + 90, apps[i].name, GUI_COLOR_BLACK, 0xFF);
        
        // Uygulama versiyonu
        font_draw_string_center(card_x + card_width/2, card_y + 110, apps[i].version, GUI_COLOR_DARK_GRAY, 0xFF);
        
        // Kategori
        font_draw_string_center(card_x + card_width/2, card_y + 130, app_manager_get_category_name(apps[i].category), GUI_COLOR_DARK_GRAY, 0xFF);
        
        // Kurulum butonu
        if (apps[i].status == APP_STATUS_INSTALLED) {
            vga_fill_rect(card_x + 25, card_y + card_height - 30, 100, 20, GUI_COLOR_DARK_GREEN);
            font_draw_string_center(card_x + 25 + 50, card_y + card_height - 20, "Kurulu", GUI_COLOR_WHITE, 0xFF);
        } else {
            vga_fill_rect(card_x + 25, card_y + card_height - 30, 100, 20, GUI_COLOR_BLUE);
            font_draw_string_center(card_x + 25 + 50, card_y + card_height - 20, "Kur", GUI_COLOR_WHITE, 0xFF);
        }
    }
}

// Uygulama yöneticisine ayrılmış ayarlar sayfası
void settings_paint_apps(gui_window_t* window) {
    if (!window) return;
    
    // Ayarlar penceresi içeriğini çiz
    uint32_t x = window->x + window->client.x;
    uint32_t y = window->y + window->client.y;
    
    // Başlık
    font_draw_string(x + 200, y + 30, "Uygulama Ayarları", GUI_COLOR_DARK_BLUE, 0xFF);
    
    // APK Uyumluluk Ayarları
    font_draw_string(x + 50, y + 80, "Android Uyumluluk Katmanı", GUI_COLOR_BLACK, 0xFF);
    
    // Durum ve versiyon
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Durum: %s", android_vm.initialized ? "Etkin" : "Devre Dışı");
    font_draw_string(x + 70, y + 110, buffer, GUI_COLOR_DARK_GRAY, 0xFF);
    
    snprintf(buffer, sizeof(buffer), "Android SDK Sürümü: %d (Android 6.0)", android_vm.sdk_version);
    font_draw_string(x + 70, y + 130, buffer, GUI_COLOR_DARK_GRAY, 0xFF);
    
    snprintf(buffer, sizeof(buffer), "Kullanılabilir Bellek: %d MB", android_vm.available_memory / 1024);
    font_draw_string(x + 70, y + 150, buffer, GUI_COLOR_DARK_GRAY, 0xFF);
    
    // Açma/kapama düğmesi
    gui_draw_switch(window, 400, 80, android_vm.initialized);
    
    // Ayar butonları
    gui_draw_button(window, 70, 180, 150, 30, "Uyumluluğu Test Et", GUI_COLOR_BLUE, GUI_COLOR_WHITE);
    gui_draw_button(window, 230, 180, 150, 30, "Kaynakları Yapılandır", GUI_COLOR_GREEN, GUI_COLOR_WHITE);
    gui_draw_button(window, 390, 180, 150, 30, "Sorun Giderme", GUI_COLOR_ORANGE, GUI_COLOR_WHITE);
    
    // Masaüstü kısayolu ayarları
    font_draw_string(x + 50, y + 230, "Masaüstü Kısayolu Davranışı", GUI_COLOR_BLACK, 0xFF);
    
    // Onay kutuları
    gui_draw_checkbox(window, 70, 260, 1, "Yeni kurulan uygulamalar için otomatik kısayol oluştur");
    gui_draw_checkbox(window, 70, 290, 0, "Kaldırılan uygulamaların kısayollarını otomatik temizle");
    gui_draw_checkbox(window, 70, 320, 1, "Uygulama başlatıcı simgeleri için animasyon göster");
    
    // Uygulama yükleme ayarları
    font_draw_string(x + 50, y + 370, "Uygulama Kurulum Ayarları", GUI_COLOR_BLACK, 0xFF);
    
    // Onay kutuları
    gui_draw_checkbox(window, 70, 400, 1, "Uygulama kaynaklarını doğrula");
    gui_draw_checkbox(window, 70, 430, 1, "Kurulum sonrası önbelleği temizle");
    gui_draw_checkbox(window, 70, 460, 0, "Otomatik güncellemeleri kontrol et");
} 