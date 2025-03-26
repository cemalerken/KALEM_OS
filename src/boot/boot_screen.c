#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "boot_screen.h"
#include "gui.h"
#include "logger.h"
#include "system.h"
#include "user_manager.h"
#include "login_screen.h"

// Bileşenler
static gui_window_t* boot_window = NULL;
static gui_image_t* logo_image = NULL;
static gui_progress_bar_t* progress_bar = NULL;
static gui_label_t* status_label = NULL;
static gui_timer_t* animation_timer = NULL;

// Durum takibi
static int current_progress = 0;
static int target_progress = 0;
static int boot_phase = 0;
static int boot_animation_frame = 0;

// Açılış aşamaları
static const char* boot_phases[] = {
    "Çekirdek yükleniyor...",
    "Donanım algılanıyor...",
    "Sürücüler başlatılıyor...",
    "Sistem hizmetleri başlatılıyor...",
    "Kullanıcı ortamı hazırlanıyor...",
    "KALEM OS başlatılıyor..."
};

// Kalem animasyonu
static const char* kalem_animation_frames[] = {
    "/usr/share/kalem/boot/kalem_anim_1.png",
    "/usr/share/kalem/boot/kalem_anim_2.png",
    "/usr/share/kalem/boot/kalem_anim_3.png",
    "/usr/share/kalem/boot/kalem_anim_4.png",
    "/usr/share/kalem/boot/kalem_anim_5.png",
    "/usr/share/kalem/boot/kalem_anim_6.png",
    "/usr/share/kalem/boot/kalem_anim_7.png",
    "/usr/share/kalem/boot/kalem_anim_8.png"
};

#define ANIMATION_FRAME_COUNT (sizeof(kalem_animation_frames) / sizeof(kalem_animation_frames[0]))

// Açılış bip sesi çal
static void play_startup_sound() {
    system_play_sound("/usr/share/kalem/sounds/startup.wav");
}

// Animasyon zamanlayıcı işleyicisi
static void on_animation_timer(gui_timer_t* timer, void* user_data) {
    // Kalem animasyonunu güncelle
    boot_animation_frame = (boot_animation_frame + 1) % ANIMATION_FRAME_COUNT;
    gui_image_set_source(logo_image, kalem_animation_frames[boot_animation_frame]);
    
    // İlerleme çubuğunu güncelle
    if (current_progress < target_progress) {
        current_progress++;
        gui_progress_bar_set_value(progress_bar, current_progress);
    }
    
    // Aşamayı kontrol et
    if (current_progress >= 100) {
        // Açılış tamamlandı, giriş ekranına geç
        gui_timer_stop(animation_timer);
        
        // Son bir kare göster ve biraz bekle
        boot_animation_frame = ANIMATION_FRAME_COUNT - 1;
        gui_image_set_source(logo_image, kalem_animation_frames[boot_animation_frame]);
        
        // Giriş ekranına geçiş zamanlayıcısı
        gui_timer_t* transition_timer = gui_create_timer(1000);
        gui_timer_set_callback(transition_timer, on_transition_timer, NULL);
        gui_timer_start(transition_timer);
    } else if (current_progress == target_progress && boot_phase < (sizeof(boot_phases) / sizeof(boot_phases[0]) - 1)) {
        // Sonraki aşamaya geç
        boot_phase++;
        gui_label_set_text(status_label, boot_phases[boot_phase]);
        
        // Hedefi güncelle
        target_progress = (boot_phase + 1) * 100 / (sizeof(boot_phases) / sizeof(boot_phases[0]));
    }
}

// Giriş ekranına geçiş zamanlayıcı işleyicisi
static void on_transition_timer(gui_timer_t* timer, void* user_data) {
    // Açılış ekranını kapat
    gui_window_hide(boot_window);
    gui_window_destroy(boot_window);
    
    // Zamanlayıcıyı yok et
    gui_timer_destroy(timer);
    
    // Kullanıcı yönetimini başlat
    user_manager_init();
    
    // Giriş ekranını göster
    login_screen_show();
}

// Açılış ekranını başlatır
int boot_screen_init() {
    // Pencere oluştur
    boot_window = gui_create_window("KALEM OS Başlatılıyor", 800, 600, GUI_WINDOW_FULLSCREEN);
    if (!boot_window) {
        log_error("Açılış ekranı penceresi oluşturulamadı");
        return BOOT_ERROR_GUI;
    }
    
    // Ana düzen
    gui_layout_t* main_layout = gui_create_layout(GUI_LAYOUT_VERTICAL);
    gui_layout_set_padding(main_layout, 40);
    gui_layout_set_spacing(main_layout, 30);
    gui_window_set_layout(boot_window, main_layout);
    
    // Logo/Animasyon alanı
    logo_image = gui_create_image(kalem_animation_frames[0]);
    gui_layout_add_child(main_layout, logo_image);
    
    // İlerleme çubuğu
    progress_bar = gui_create_progress_bar();
    gui_progress_bar_set_value(progress_bar, 0);
    gui_progress_bar_set_color(progress_bar, 0x3477eb); // Mavi
    gui_layout_add_child(main_layout, progress_bar);
    
    // Durum etiketi
    status_label = gui_create_label(boot_phases[0]);
    gui_label_set_font_size(status_label, 16);
    gui_label_set_alignment(status_label, GUI_ALIGN_CENTER);
    gui_layout_add_child(main_layout, status_label);
    
    // Başlangıç değerlerini ayarla
    current_progress = 0;
    target_progress = 100 / (sizeof(boot_phases) / sizeof(boot_phases[0]));
    boot_phase = 0;
    boot_animation_frame = 0;
    
    return BOOT_ERROR_NONE;
}

// Açılış ekranını gösterir
void boot_screen_show() {
    if (!boot_window) {
        if (boot_screen_init() != BOOT_ERROR_NONE) {
            // Hatada doğrudan giriş ekranına geç
            login_screen_show();
            return;
        }
    }
    
    // Pencereyi göster
    gui_window_show(boot_window);
    
    // Başlangıç sesi çal
    play_startup_sound();
    
    // Animasyon zamanlayıcısı
    animation_timer = gui_create_timer(100); // 100ms aralıklarla
    gui_timer_set_callback(animation_timer, on_animation_timer, NULL);
    gui_timer_start(animation_timer);
}

// Test için: hızlı açılış (doğrudan giriş ekranına)
void boot_screen_skip() {
    // Giriş ekranını başlat
    user_manager_init();
    login_screen_show();
} 