#include "../include/boot_manager.h"
#include "../include/gui.h"
#include "../include/user_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Hata kodları
#define BOOT_ERROR_NONE              0
#define BOOT_ERROR_INIT_FAILED      -1
#define BOOT_ERROR_NOT_INITIALIZED  -2
#define BOOT_ERROR_INVALID_ARG      -3
#define BOOT_ERROR_DISPLAY          -4
#define BOOT_ERROR_FILE_IO          -5
#define BOOT_ERROR_LOGIN_FAILED     -6
#define BOOT_ERROR_CONFIG           -7
#define BOOT_ERROR_INTERNAL         -99

// Boot yapılandırma dosyası
#define BOOT_CONFIG_PATH "/etc/kalem/boot_config.dat"

// Başlatılma durumu
static uint8_t is_initialized = 0;
static boot_mode_t current_mode = BOOT_MODE_NORMAL;
static boot_config_t boot_config = {0};
static boot_progress_t boot_progress = {0};

// Grafik nesneleri
static gui_window_t* boot_window = NULL;
static gui_image_t* logo_image = NULL;
static gui_progress_bar_t* progress_bar = NULL;
static gui_label_t* status_label = NULL;
static gui_animation_t* pencil_animation = NULL;

// Animasyon kareleri
#define MAX_ANIMATION_FRAMES 24
static gui_image_t* pencil_frames[MAX_ANIMATION_FRAMES] = {NULL};
static uint32_t current_frame = 0;
static uint32_t frame_count = 0;

// Sistem açılış zamanı
static uint64_t system_boot_time = 0;

// İleri bildirimler
static int load_config();
static int save_config();
static int create_boot_screen();
static int update_boot_screen();
static int load_animation_frames();
static int animate_frame(void* data);
static uint64_t get_current_time_ms();

/**
 * Açılış yöneticisini başlatır
 */
int boot_manager_init(boot_mode_t mode) {
    if (is_initialized) {
        return BOOT_ERROR_NONE; // Zaten başlatılmış
    }
    
    printf("KALEM OS Boot Yöneticisi başlatılıyor...\n");
    
    // Sistem açılış zamanını kaydet
    system_boot_time = get_current_time_ms();
    
    // Açılış modunu ayarla
    current_mode = mode;
    
    // İlerleme bilgisini sıfırla
    memset(&boot_progress, 0, sizeof(boot_progress));
    boot_progress.state = BOOT_STATE_INIT;
    boot_progress.progress = 0;
    boot_progress.total_steps = 6; // Toplam açılış adımı sayısı
    strcpy(boot_progress.status_message, "KALEM OS başlatılıyor...");
    
    // Yapılandırmayı yükle
    if (load_config() != BOOT_ERROR_NONE) {
        // Yapılandırma yüklenemedi, varsayılanları kullan
        boot_config.show_logo = 1;
        boot_config.show_progress = 1;
        boot_config.show_text = 1;
        boot_config.show_animation = 1;
        boot_config.animation = BOOT_ANIM_PENCIL;
        boot_config.style = BOOT_STYLE_DARK;
        strcpy(boot_config.logo_path, "/usr/share/kalem/images/logo.png");
        strcpy(boot_config.background_path, "/usr/share/kalem/images/boot_bg.png");
        boot_config.timeout = 500; // 500ms
        boot_config.auto_login = 0;
        strcpy(boot_config.auto_login_user, "");
        boot_config.safe_mode_enabled = 0;
        boot_config.show_boot_messages = 1;
        
        // Değişiklikleri kaydet
        save_config();
    }
    
    // Güvenli mod kontrolü
    if (mode == BOOT_MODE_SAFE) {
        boot_config.safe_mode_enabled = 1;
        printf("KALEM OS güvenli modda başlatılıyor...\n");
    }
    
    // Kullanıcı yönetimini başlat
    user_manager_init();
    
    is_initialized = 1;
    return BOOT_ERROR_NONE;
}

/**
 * Açılış yöneticisini kapatır
 */
int boot_manager_cleanup() {
    if (!is_initialized) {
        return BOOT_ERROR_NOT_INITIALIZED;
    }
    
    // Açılış ekranını kapat
    boot_screen_stop();
    
    // Kullanıcı yönetimini kapat
    user_manager_cleanup();
    
    is_initialized = 0;
    return BOOT_ERROR_NONE;
}

/**
 * Açılış ekranını başlatır
 */
int boot_screen_start() {
    if (!is_initialized) {
        return BOOT_ERROR_NOT_INITIALIZED;
    }
    
    // GUI başlat
    if (gui_init() != 0) {
        printf("Grafik arayüzü başlatılamadı!\n");
        return BOOT_ERROR_DISPLAY;
    }
    
    // Açılış ekranı penceresini oluştur
    if (create_boot_screen() != BOOT_ERROR_NONE) {
        printf("Açılış ekranı oluşturulamadı!\n");
        gui_cleanup();
        return BOOT_ERROR_DISPLAY;
    }
    
    // Animasyon karelerini yükle
    if (boot_config.show_animation) {
        if (load_animation_frames() != BOOT_ERROR_NONE) {
            printf("Animasyon kareleri yüklenemedi!\n");
            boot_config.show_animation = 0;
        } else {
            // Animasyon zamanlayıcısını başlat
            gui_set_timer(boot_window, 40, animate_frame, NULL);
        }
    }
    
    // Pencereyi göster
    gui_window_show(boot_window);
    
    // Pencereyi güncelle
    update_boot_screen();
    
    return BOOT_ERROR_NONE;
}

/**
 * Açılış ekranını kapatır
 */
int boot_screen_stop() {
    if (!is_initialized) {
        return BOOT_ERROR_NOT_INITIALIZED;
    }
    
    if (boot_window) {
        // Animasyon karelerini temizle
        for (uint32_t i = 0; i < frame_count; i++) {
            if (pencil_frames[i]) {
                gui_image_destroy(pencil_frames[i]);
                pencil_frames[i] = NULL;
            }
        }
        
        // Animasyonu durdur
        if (pencil_animation) {
            gui_animation_destroy(pencil_animation);
            pencil_animation = NULL;
        }
        
        // Pencereyi kapat
        gui_window_destroy(boot_window);
        boot_window = NULL;
        logo_image = NULL;
        progress_bar = NULL;
        status_label = NULL;
        
        // GUI temizle
        gui_cleanup();
    }
    
    return BOOT_ERROR_NONE;
}

/**
 * Açılış ilerlemesini günceller
 */
int boot_update_progress(boot_state_t state, uint8_t progress, const char* message) {
    if (!is_initialized) {
        return BOOT_ERROR_NOT_INITIALIZED;
    }
    
    // İlerleme bilgisini güncelle
    boot_progress.state = state;
    boot_progress.progress = progress;
    boot_progress.current_step = (uint32_t)state;
    
    if (message) {
        strncpy(boot_progress.status_message, message, sizeof(boot_progress.status_message) - 1);
        boot_progress.status_message[sizeof(boot_progress.status_message) - 1] = '\0';
    }
    
    // Açılış durumuna göre mesajları yazdır
    if (boot_config.show_boot_messages) {
        printf("[%d%%] %s\n", progress, boot_progress.status_message);
    }
    
    // Açılış ekranını güncelle
    if (boot_window) {
        update_boot_screen();
    }
    
    // Açılış tamamlandı mı?
    if (state == BOOT_STATE_COMPLETE && progress >= 100) {
        // Otomatik oturum açma veya giriş ekranına geçiş
        if (boot_config.auto_login && strlen(boot_config.auto_login_user) > 0) {
            // Otomatik oturum açmayı dene
            boot_auto_login();
        } else {
            // Belli bir süre sonra oturum açma ekranını göster
            uint32_t delay = boot_config.timeout;
            if (delay < 100) delay = 100; // En az 100ms bekle
            
            // Delay sonrası giriş ekranı
            gui_set_timeout(boot_window, delay, boot_show_login_screen, NULL);
        }
    }
    
    return BOOT_ERROR_NONE;
}

/**
 * Açılış hatasını raporlar
 */
int boot_report_error(const char* error_message) {
    if (!is_initialized) {
        return BOOT_ERROR_NOT_INITIALIZED;
    }
    
    // Hata durumunu ayarla
    boot_progress.state = BOOT_STATE_ERROR;
    boot_progress.error_occurred = 1;
    
    if (error_message) {
        strncpy(boot_progress.error_message, error_message, sizeof(boot_progress.error_message) - 1);
        boot_progress.error_message[sizeof(boot_progress.error_message) - 1] = '\0';
    }
    
    // Hata mesajını görüntüle
    printf("BOOT HATASI: %s\n", boot_progress.error_message);
    
    // Açılış ekranını güncelle
    if (boot_window) {
        update_boot_screen();
    }
    
    return BOOT_ERROR_NONE;
}

/**
 * Açılış ilerlemesini alır
 */
int boot_get_progress(boot_progress_t* progress) {
    if (!is_initialized) {
        return BOOT_ERROR_NOT_INITIALIZED;
    }
    
    if (!progress) {
        return BOOT_ERROR_INVALID_ARG;
    }
    
    // İlerleme bilgisini kopyala
    memcpy(progress, &boot_progress, sizeof(boot_progress_t));
    
    return BOOT_ERROR_NONE;
}

/**
 * Açılış yapılandırmasını alır
 */
int boot_get_config(boot_config_t* config) {
    if (!is_initialized) {
        return BOOT_ERROR_NOT_INITIALIZED;
    }
    
    if (!config) {
        return BOOT_ERROR_INVALID_ARG;
    }
    
    // Yapılandırmayı kopyala
    memcpy(config, &boot_config, sizeof(boot_config_t));
    
    return BOOT_ERROR_NONE;
}

/**
 * Açılış yapılandırmasını ayarlar
 */
int boot_set_config(const boot_config_t* config) {
    if (!is_initialized) {
        return BOOT_ERROR_NOT_INITIALIZED;
    }
    
    if (!config) {
        return BOOT_ERROR_INVALID_ARG;
    }
    
    // Yapılandırmayı kopyala
    memcpy(&boot_config, config, sizeof(boot_config_t));
    
    // Değişiklikleri kaydet
    return save_config();
}

/**
 * Oturum açma ekranına geçiş yapar
 */
int boot_show_login_screen() {
    if (!is_initialized) {
        return BOOT_ERROR_NOT_INITIALIZED;
    }
    
    printf("Oturum açma ekranına geçiliyor...\n");
    
    // Açılış ekranını kapat
    boot_screen_stop();
    
    // Giriş ekranını göster
    // Bu fonksiyon çağrılacağı için login_screen.c modülünde tanımlıdır
    extern int login_screen_show();
    return login_screen_show();
}

/**
 * Otomatik oturum açma ayarlarını kontrol eder ve uygular
 */
int boot_auto_login() {
    if (!is_initialized) {
        return BOOT_ERROR_NOT_INITIALIZED;
    }
    
    // Otomatik oturum açma etkin mi?
    if (!boot_config.auto_login || strlen(boot_config.auto_login_user) == 0) {
        return BOOT_ERROR_CONFIG;
    }
    
    printf("Otomatik oturum açma: %s\n", boot_config.auto_login_user);
    
    // Açılış ekranını kapat
    boot_screen_stop();
    
    // Kullanıcı bilgilerini sorgula
    user_info_t user_info;
    int result = user_get_info_by_name(boot_config.auto_login_user, &user_info);
    
    if (result != 0) {
        printf("Otomatik oturum açma kullanıcısı bulunamadı: %s\n", boot_config.auto_login_user);
        
        // Giriş ekranını göster
        boot_show_login_screen();
        return BOOT_ERROR_LOGIN_FAILED;
    }
    
    // Hesap devre dışı mı?
    if (user_info.account_disabled) {
        printf("Otomatik oturum açma kullanıcısı devre dışı: %s\n", boot_config.auto_login_user);
        
        // Giriş ekranını göster
        boot_show_login_screen();
        return BOOT_ERROR_LOGIN_FAILED;
    }
    
    // Otomatik oturum açma için özel bir oturum açma fonksiyonu kullan
    // Parola kontrolü olmadan doğrudan giriş yap
    uint32_t session_id;
    extern int session_auto_login(const char* username, uint32_t* session_id_out);
    result = session_auto_login(boot_config.auto_login_user, &session_id);
    
    if (result != 0) {
        printf("Otomatik oturum açma başarısız: %s\n", boot_config.auto_login_user);
        
        // Giriş ekranını göster
        boot_show_login_screen();
        return BOOT_ERROR_LOGIN_FAILED;
    }
    
    printf("Otomatik oturum açma başarılı: %s (Oturum ID: %u)\n", 
           boot_config.auto_login_user, session_id);
    
    // Masaüstünü başlat
    extern int desktop_start(uint32_t session_id);
    return desktop_start(session_id);
}

/**
 * Sistem açılış zamanını milisaniye cinsinden döndürür
 */
uint64_t boot_get_uptime_ms() {
    uint64_t current_time = get_current_time_ms();
    return current_time - system_boot_time;
}

/**
 * Yapılandırmayı yükler
 */
static int load_config() {
    FILE* file = fopen(BOOT_CONFIG_PATH, "rb");
    if (!file) {
        printf("Boot yapılandırma dosyası bulunamadı: %s\n", BOOT_CONFIG_PATH);
        return BOOT_ERROR_FILE_IO;
    }
    
    size_t read_size = fread(&boot_config, sizeof(boot_config_t), 1, file);
    fclose(file);
    
    if (read_size != 1) {
        printf("Boot yapılandırması okunamadı!\n");
        return BOOT_ERROR_FILE_IO;
    }
    
    return BOOT_ERROR_NONE;
}

/**
 * Yapılandırmayı kaydeder
 */
static int save_config() {
    FILE* file = fopen(BOOT_CONFIG_PATH, "wb");
    if (!file) {
        printf("Boot yapılandırma dosyası oluşturulamadı: %s\n", BOOT_CONFIG_PATH);
        return BOOT_ERROR_FILE_IO;
    }
    
    size_t write_size = fwrite(&boot_config, sizeof(boot_config_t), 1, file);
    fclose(file);
    
    if (write_size != 1) {
        printf("Boot yapılandırması yazılamadı!\n");
        return BOOT_ERROR_FILE_IO;
    }
    
    return BOOT_ERROR_NONE;
}

/**
 * Açılış ekranı penceresini oluşturur
 */
static int create_boot_screen() {
    // Tam ekran pencere oluştur
    gui_rect_t screen_rect = gui_get_screen_rect();
    boot_window = gui_window_create("KALEM OS Boot", screen_rect, NULL);
    
    if (!boot_window) {
        return BOOT_ERROR_DISPLAY;
    }
    
    // Pencereyi tam ekran yap
    gui_window_set_fullscreen(boot_window, 1);
    
    // Arka plan rengini ayarla (karanlık tema)
    gui_color_t bg_color = {0, 0, 0, 255}; // Siyah
    if (boot_config.style == BOOT_STYLE_LIGHT) {
        bg_color.r = 255;
        bg_color.g = 255;
        bg_color.b = 255;
    } else if (boot_config.style == BOOT_STYLE_MODERN) {
        bg_color.r = 16;
        bg_color.g = 16;
        bg_color.b = 32;
    }
    
    gui_window_set_background(boot_window, bg_color);
    
    // Arka plan resmi varsa yükle
    if (boot_config.show_logo && strlen(boot_config.background_path) > 0) {
        gui_image_t* bg_image = gui_image_load_from_file(boot_config.background_path);
        if (bg_image) {
            gui_rect_t bg_rect = {0, 0, screen_rect.width, screen_rect.height};
            gui_window_set_background_image(boot_window, bg_image, bg_rect);
        }
    }
    
    // KALEM OS logosu
    if (boot_config.show_logo) {
        logo_image = gui_image_load_from_file(boot_config.logo_path);
        if (logo_image) {
            // Logo boyutunu al
            int logo_width, logo_height;
            gui_image_get_size(logo_image, &logo_width, &logo_height);
            
            // Merkeze yerleştir
            int x = (screen_rect.width - logo_width) / 2;
            int y = (screen_rect.height - logo_height) / 2 - 50; // Biraz yukarıda
            
            gui_rect_t logo_rect = {x, y, logo_width, logo_height};
            gui_window_add_image(boot_window, logo_image, logo_rect);
        }
    }
    
    // İlerleme çubuğu
    if (boot_config.show_progress) {
        int bar_width = 400;
        int bar_height = 6;
        int x = (screen_rect.width - bar_width) / 2;
        int y = (screen_rect.height - bar_height) / 2 + 80; // Biraz aşağıda
        
        gui_rect_t bar_rect = {x, y, bar_width, bar_height};
        
        // İlerleme çubuğu renkleri
        gui_color_t fg_color = {0, 160, 255, 255}; // Mavi
        gui_color_t bg_color = {60, 60, 60, 200};  // Yarı-saydam gri
        
        progress_bar = gui_progress_bar_create(bar_rect, fg_color, bg_color);
        gui_progress_bar_set_value(progress_bar, 0);
        gui_window_add_progress_bar(boot_window, progress_bar);
    }
    
    // Durum etiketi
    if (boot_config.show_text) {
        int label_width = 400;
        int label_height = 24;
        int x = (screen_rect.width - label_width) / 2;
        int y = (screen_rect.height - label_height) / 2 + 100; // İlerleme çubuğunun altında
        
        gui_rect_t label_rect = {x, y, label_width, label_height};
        
        // Etiket rengi
        gui_color_t text_color = {200, 200, 200, 255}; // Açık gri
        if (boot_config.style == BOOT_STYLE_LIGHT) {
            text_color.r = 50;
            text_color.g = 50;
            text_color.b = 50;
        }
        
        status_label = gui_label_create(label_rect, boot_progress.status_message, text_color);
        gui_label_set_alignment(status_label, GUI_ALIGN_CENTER);
        gui_window_add_label(boot_window, status_label);
    }
    
    return BOOT_ERROR_NONE;
}

/**
 * Açılış ekranını günceller
 */
static int update_boot_screen() {
    if (!boot_window) {
        return BOOT_ERROR_DISPLAY;
    }
    
    // İlerleme çubuğunu güncelle
    if (progress_bar) {
        gui_progress_bar_set_value(progress_bar, boot_progress.progress);
    }
    
    // Durum etiketini güncelle
    if (status_label) {
        char status_text[150];
        sprintf(status_text, "Yükleniyor: %%%d - %s", 
                boot_progress.progress, boot_progress.status_message);
        gui_label_set_text(status_label, status_text);
        
        // Hata durumunda kırmızı renk kullan
        if (boot_progress.error_occurred) {
            gui_color_t error_color = {255, 50, 50, 255}; // Kırmızı
            gui_label_set_color(status_label, error_color);
        }
    }
    
    // Pencereyi yeniden çiz
    gui_window_update(boot_window);
    
    return BOOT_ERROR_NONE;
}

/**
 * Animasyon karelerini yükler
 */
static int load_animation_frames() {
    if (boot_config.animation != BOOT_ANIM_PENCIL) {
        return BOOT_ERROR_NONE; // Sadece kalem animasyonu destekleniyor
    }
    
    // Animasyon klasörü
    char path[300];
    sprintf(path, "/usr/share/kalem/animations/pencil");
    
    // Animasyon karelerini yükle
    frame_count = 0;
    for (uint32_t i = 0; i < MAX_ANIMATION_FRAMES; i++) {
        char frame_path[512];
        sprintf(frame_path, "%s/frame%02u.png", path, i);
        
        pencil_frames[i] = gui_image_load_from_file(frame_path);
        if (!pencil_frames[i]) {
            break; // Daha fazla kare yok
        }
        
        frame_count++;
    }
    
    if (frame_count == 0) {
        printf("Animasyon kareleri bulunamadı!\n");
        return BOOT_ERROR_FILE_IO;
    }
    
    printf("Toplam %u animasyon karesi yüklendi.\n", frame_count);
    
    // Pencil animasyon konumunu ayarla
    gui_rect_t screen_rect = gui_get_screen_rect();
    
    // İlk kareyi kullanarak boyutu al
    int anim_width, anim_height;
    gui_image_get_size(pencil_frames[0], &anim_width, &anim_height);
    
    // Animasyonu progress bar üzerinde hareket ettir
    if (progress_bar) {
        gui_rect_t bar_rect;
        gui_progress_bar_get_rect(progress_bar, &bar_rect);
        
        int x = bar_rect.x;
        int y = bar_rect.y - anim_height - 5; // Çubuğun biraz üzerinde
        
        gui_rect_t anim_rect = {x, y, anim_width, anim_height};
        pencil_animation = gui_animation_create(anim_rect);
        gui_window_add_animation(boot_window, pencil_animation);
    }
    
    return BOOT_ERROR_NONE;
}

/**
 * Animasyon karesini günceller
 */
static int animate_frame(void* data) {
    if (!pencil_animation || frame_count == 0) {
        return 0;
    }
    
    // Sonraki kareye geç
    current_frame = (current_frame + 1) % frame_count;
    
    // Kareyi ayarla
    gui_animation_set_frame(pencil_animation, pencil_frames[current_frame]);
    
    // Animasyonu progress bar ile senkronize et
    if (progress_bar) {
        gui_rect_t bar_rect;
        gui_progress_bar_get_rect(progress_bar, &bar_rect);
        
        // Progress değerine göre x konumunu hesapla
        float progress_pct = boot_progress.progress / 100.0f;
        int anim_width, anim_height;
        gui_animation_get_size(pencil_animation, &anim_width, &anim_height);
        
        // Pencil'i progress değeriyle hizala
        int max_travel = bar_rect.width - anim_width;
        int x = bar_rect.x + (int)(progress_pct * max_travel);
        int y = bar_rect.y - anim_height - 5;
        
        gui_rect_t anim_rect = {x, y, anim_width, anim_height};
        gui_animation_set_position(pencil_animation, anim_rect);
    }
    
    // Pencereyi güncelle
    gui_window_update(boot_window);
    
    return 1; // Devam et
}

/**
 * Geçerli zamanı milisaniye cinsinden döndürür
 */
static uint64_t get_current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
} 