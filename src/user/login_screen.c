#include "../include/user_manager.h"
#include "../include/gui.h"
#include "../include/login_recovery.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Hata kodları
#define LOGIN_ERROR_NONE         0
#define LOGIN_ERROR_INIT        -1
#define LOGIN_ERROR_GUI         -2
#define LOGIN_ERROR_AUTH        -3
#define LOGIN_ERROR_SESSION     -4

// UI bileşenleri için tanımlamalar
typedef struct {
    // Ana pencere ve düzeni
    gui_window_t* window;
    gui_layout_t* main_layout;
    
    // Kullanıcı listesi
    gui_list_t* user_list;
    
    // Giriş kutuları
    gui_text_input_t* username_input;
    gui_password_input_t* password_input;
    
    // Butonlar
    gui_button_t* login_button;
    gui_button_t* guest_button;
    gui_button_t* power_button;
    gui_button_t* restart_button;
    
    // Mesaj etiketi
    gui_label_t* message_label;
    
    // Zaman etiketi
    gui_label_t* time_label;
    gui_timer_t* clock_timer;
    
    // Sistem bilgileri
    char hostname[64];
    uint8_t guest_enabled;
    uint8_t auto_login_enabled;
    char auto_login_user[32];
    
    // Kullanıcı listesi
    user_info_t* user_list_data;
    uint32_t user_count;
    
    // Geçerli seçilen kullanıcı
    int selected_user_index;
    
    // Oturum açma girişim sayacı
    int login_attempts;
} login_screen_t;

// Şifremi unuttum ekranı için bileşenler
typedef struct {
    gui_window_t* window;
    gui_layout_t* main_layout;
    gui_text_input_t* username_input;
    gui_text_input_t* email_input;
    gui_button_t* send_button;
    gui_button_t* cancel_button;
    gui_label_t* message_label;
} forgot_password_screen_t;

// Kullanıcı kayıt ekranı için bileşenler
typedef struct {
    gui_window_t* window;
    gui_layout_t* main_layout;
    gui_text_input_t* username_input;
    gui_text_input_t* password_input;
    gui_text_input_t* confirm_password_input;
    gui_text_input_t* email_input;
    gui_combobox_t* user_type_combobox;
    gui_button_t* register_button;
    gui_button_t* cancel_button;
    gui_label_t* message_label;
} register_screen_t;

// Global değişkenler
static login_screen_t* login_screen = NULL;
static uint8_t is_initialized = 0;

// Yeni global değişkenler
static forgot_password_screen_t* forgot_screen = NULL;
static register_screen_t* register_screen = NULL;

// İleri deklarasyonlar
static void on_login_button_clicked(gui_button_t* button, void* user_data);
static void on_guest_button_clicked(gui_button_t* button, void* user_data);
static void on_power_button_clicked(gui_button_t* button, void* user_data);
static void on_restart_button_clicked(gui_button_t* button, void* user_data);
static void on_user_selected(gui_list_t* list, int index, void* user_data);
static void on_clock_timer(gui_timer_t* timer, void* user_data);
static void update_user_list();
static void show_message(const char* message, int is_error);
static void attempt_auto_login();
static void create_forgot_password_screen();
static void create_register_screen();
static void on_forgot_password_button_clicked(gui_button_t* button, void* user_data);
static void on_register_button_clicked(gui_button_t* button, void* user_data);
static void on_send_code_button_clicked(gui_button_t* button, void* user_data);
static void on_reset_button_clicked(gui_button_t* button, void* user_data);
static void on_register_user_button_clicked(gui_button_t* button, void* user_data);
static void on_skip_email_changed(gui_checkbox_t* checkbox, int checked, void* user_data);
static void on_skip_email_warning_response(int response, void* user_data);

/**
 * Kullanıcı girişi yapar
 * 
 * @param username Kullanıcı adı
 * @param password Parola (otomatik giriş durumunda NULL olabilir)
 * @param is_auto_login Otomatik giriş mi?
 * @return Hata kodu (LOGIN_ERROR_NONE başarı)
 */
static int login_user(const char* username, const char* password, int is_auto_login) {
    if (!username || (!password && !is_auto_login)) {
        return LOGIN_ERROR_INVALID_ARGS;
    }
    
    // Kullanıcı bilgilerini doğrula
    int result;
    if (is_auto_login) {
        result = user_verify_auto_login(username);
    } else {
        result = user_verify_password(username, password);
    }
    
    if (result != USER_ERROR_NONE) {
        if (result == USER_ERROR_INVALID_PASSWORD) {
            login_screen->login_attempts++;
            
            // Belirli sayıda başarısız girişten sonra oturumu kilitle
            if (login_screen->login_attempts >= MAX_LOGIN_ATTEMPTS) {
                gui_label_set_text(login_screen->message_label, 
                                 "Çok fazla başarısız giriş denemesi. Lütfen daha sonra tekrar deneyin.");
                
                // Giriş düğmesini devre dışı bırak
                gui_widget_set_enabled(login_screen->login_button, 0);
                
                // 30 saniye sonra yeniden etkinleştir
                gui_timer_t* unlock_timer = gui_create_timer(30000);
                gui_timer_set_callback(unlock_timer, on_unlock_timer, NULL);
                gui_timer_start(unlock_timer);
                
                return LOGIN_ERROR_TOO_MANY_ATTEMPTS;
            }
            
            gui_label_set_text(login_screen->message_label, "Geçersiz kullanıcı adı veya parola.");
            return LOGIN_ERROR_INVALID_PASSWORD;
        } else if (result == USER_ERROR_USER_NOT_FOUND) {
            gui_label_set_text(login_screen->message_label, "Kullanıcı bulunamadı.");
            return LOGIN_ERROR_USER_NOT_FOUND;
        } else if (result == USER_ERROR_USER_DISABLED) {
            gui_label_set_text(login_screen->message_label, "Bu hesap devre dışı bırakılmış.");
            return LOGIN_ERROR_USER_DISABLED;
        } else {
            gui_label_set_text(login_screen->message_label, "Oturum açılırken bir hata oluştu.");
            return LOGIN_ERROR_UNKNOWN;
        }
    }
    
    // Oturum başlat
    user_info_t user_info;
    if (user_get_by_name(username, &user_info) != USER_ERROR_NONE) {
        gui_label_set_text(login_screen->message_label, "Kullanıcı bilgileri alınamadı.");
        return LOGIN_ERROR_USER_NOT_FOUND;
    }
    
    // Yeni oturum oluştur
    session_info_t session;
    memset(&session, 0, sizeof(session_info_t));
    
    session.user_id = user_info.user_id;
    session.login_time = time(NULL);
    strncpy(session.hostname, login_screen->hostname, sizeof(session.hostname) - 1);
    
    if (session_create(&session) != USER_ERROR_NONE) {
        gui_label_set_text(login_screen->message_label, "Oturum başlatılamadı.");
        return LOGIN_ERROR_CREATE_SESSION;
    }
    
    // Giriş ekranını gizle
    gui_window_hide(login_screen->window);
    
    // Masaüstü uygulamasını başlat
    system_launch_desktop(username, &session);
    
    return LOGIN_ERROR_NONE;
}

/**
 * Oturum kilidini açma zamanlayıcısı
 */
static void on_unlock_timer(gui_timer_t* timer, void* user_data) {
    // Giriş düğmesini yeniden etkinleştir
    gui_widget_set_enabled(login_screen->login_button, 1);
    
    // Mesajı güncelle
    gui_label_set_text(login_screen->message_label, "Şimdi tekrar deneyebilirsiniz.");
    
    // Oturum açma girişim sayacını sıfırla
    login_screen->login_attempts = 0;
    
    // Zamanlayıcıyı yok et
    gui_timer_destroy(timer);
}

/**
 * Giriş düğmesine tıklama işleyicisi
 */
static void on_login_button_clicked(gui_button_t* button, void* user_data) {
    login_screen_t* screen = (login_screen_t*)user_data;
    
    // Kullanıcı adı ve parolayı al
    const char* username = gui_text_input_get_text(screen->username_input);
    const char* password = gui_password_input_get_text(screen->password_input);
    
    if (!username || strlen(username) == 0) {
        gui_label_set_text(screen->message_label, "Lütfen kullanıcı adını girin.");
        return;
    }
    
    if (!password || strlen(password) == 0) {
        gui_label_set_text(screen->message_label, "Lütfen parolayı girin.");
        return;
    }
    
    // Kullanıcı girişi yap
    login_user(username, password, 0);
}

/**
 * Misafir girişi düğmesine tıklama işleyicisi
 */
static void on_guest_button_clicked(gui_button_t* button, void* user_data) {
    // Misafir girişi yap
    login_user("guest", "guest", 0);
}

/**
 * Giriş ekranını başlatır
 */
int login_screen_init() {
    if (is_initialized) {
        return LOGIN_ERROR_NONE;
    }
    
    // Kullanıcı yönetim sistemi başlatıldı mı kontrol et
    if (!user_manager_is_initialized()) {
        if (user_manager_init() != USER_ERROR_NONE) {
            return LOGIN_ERROR_INIT;
        }
    }
    
    // Giriş ekranı yapısını oluştur
    login_screen = (login_screen_t*)calloc(1, sizeof(login_screen_t));
    if (!login_screen) {
        return LOGIN_ERROR_INIT;
    }
    
    // Sistem bilgilerini al
    char hostname[64] = {0};
    system_get_hostname(hostname, sizeof(hostname));
    strncpy(login_screen->hostname, hostname, sizeof(login_screen->hostname) - 1);
    
    // Yapılandırmayı al
    user_config_t config;
    if (config_get(&config) == USER_ERROR_NONE) {
        login_screen->guest_enabled = config.guest_enabled;
        login_screen->auto_login_enabled = config.auto_login;
        strncpy(login_screen->auto_login_user, config.auto_login_user, sizeof(login_screen->auto_login_user) - 1);
    }
    
    // GUI bileşenlerini oluştur
    login_screen->window = gui_create_window("KALEM OS Login", 800, 600, 0);
    if (!login_screen->window) {
        free(login_screen);
        login_screen = NULL;
        return LOGIN_ERROR_GUI;
    }
    
    // Ana düzeni oluştur
    login_screen->main_layout = gui_create_layout(GUI_LAYOUT_VERTICAL);
    gui_window_set_layout(login_screen->window, login_screen->main_layout);
    
    // Üst bilgi çubuğu
    gui_layout_t* header_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_add_child(login_screen->main_layout, header_layout);
    
    // Zaman etiketi
    login_screen->time_label = gui_create_label("");
    gui_label_set_font_size(login_screen->time_label, 14);
    gui_layout_add_child(header_layout, login_screen->time_label);
    
    // Bilgisayar adı etiketi
    gui_label_t* hostname_label = gui_create_label(login_screen->hostname);
    gui_label_set_font_size(hostname_label, 14);
    gui_layout_add_child(header_layout, hostname_label);
    
    // İçerik alanı - orta kısım
    gui_layout_t* content_layout = gui_create_layout(GUI_LAYOUT_VERTICAL);
    gui_layout_set_spacing(content_layout, 20);
    gui_layout_set_padding(content_layout, 40);
    gui_layout_add_child(login_screen->main_layout, content_layout);
    gui_layout_set_expand(content_layout, 1);
    
    // KALEM OS logosu
    gui_image_t* logo = gui_create_image("/usr/share/kalem/images/logo.png");
    gui_layout_add_child(content_layout, logo);
    
    // Karşılama mesajı
    gui_label_t* welcome_label = gui_create_label("KALEM OS'a Hoş Geldiniz");
    gui_label_set_font_size(welcome_label, 24);
    gui_layout_add_child(content_layout, welcome_label);
    
    // Mesaj etiketi (hatalar ve diğer mesajlar için)
    login_screen->message_label = gui_create_label("");
    gui_label_set_font_size(login_screen->message_label, 14);
    gui_label_set_color(login_screen->message_label, 0xFF0000); // Kırmızı
    gui_layout_add_child(content_layout, login_screen->message_label);
    
    // Kullanıcı listesi
    login_screen->user_list = gui_create_list();
    gui_list_set_selection_callback(login_screen->user_list, on_user_selected, login_screen);
    gui_layout_add_child(content_layout, login_screen->user_list);
    
    // Giriş formu düzeni
    gui_layout_t* form_layout = gui_create_layout(GUI_LAYOUT_VERTICAL);
    gui_layout_set_spacing(form_layout, 10);
    gui_layout_add_child(content_layout, form_layout);
    
    // Kullanıcı adı giriş alanı
    gui_layout_t* username_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_add_child(form_layout, username_layout);
    
    gui_label_t* username_label = gui_create_label("Kullanıcı Adı:");
    gui_layout_add_child(username_layout, username_label);
    
    login_screen->username_input = gui_create_text_input();
    gui_layout_add_child(username_layout, login_screen->username_input);
    
    // Parola giriş alanı
    gui_layout_t* password_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_add_child(form_layout, password_layout);
    
    gui_label_t* password_label = gui_create_label("Parola:");
    gui_layout_add_child(password_layout, password_label);
    
    login_screen->password_input = gui_create_password_input();
    gui_layout_add_child(password_layout, login_screen->password_input);
    
    // Butonlar düzeni
    gui_layout_t* button_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_set_spacing(button_layout, 10);
    gui_layout_add_child(form_layout, button_layout);
    
    // Giriş butonu
    login_screen->login_button = gui_create_button("Giriş Yap");
    gui_button_set_callback(login_screen->login_button, on_login_button_clicked, login_screen);
    gui_layout_add_child(button_layout, login_screen->login_button);
    
    // Misafir butonu (yapılandırmaya bağlı olarak görünür)
    login_screen->guest_button = gui_create_button("Misafir Girişi");
    gui_button_set_callback(login_screen->guest_button, on_guest_button_clicked, login_screen);
    gui_layout_add_child(button_layout, login_screen->guest_button);
    gui_widget_set_visible(login_screen->guest_button, login_screen->guest_enabled);
    
    // Şifremi unuttum butonu
    gui_button_t* forgot_button = gui_create_button("Şifremi Unuttum");
    gui_button_set_callback(forgot_button, on_forgot_password_button_clicked, login_screen);
    gui_layout_add_child(button_layout, forgot_button);
    
    // Yeni kullanıcı düzeni
    gui_layout_t* register_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_set_spacing(register_layout, 5);
    gui_layout_add_child(form_layout, register_layout);
    
    // Hesabınız yok mu? etiketi
    gui_label_t* no_account_label = gui_create_label("Hesabınız yok mu?");
    gui_layout_add_child(register_layout, no_account_label);
    
    // Kayıt ol butonu
    gui_button_t* register_button = gui_create_button("Kayıt Ol");
    gui_button_set_callback(register_button, on_register_button_clicked, login_screen);
    gui_layout_add_child(register_layout, register_button);
    
    // Alt bilgi çubuğu
    gui_layout_t* footer_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_set_spacing(footer_layout, 10);
    gui_layout_add_child(login_screen->main_layout, footer_layout);
    
    // Güç düğmesi
    login_screen->power_button = gui_create_button("Kapat");
    gui_button_set_callback(login_screen->power_button, on_power_button_clicked, login_screen);
    gui_layout_add_child(footer_layout, login_screen->power_button);
    
    // Yeniden başlat düğmesi
    login_screen->restart_button = gui_create_button("Yeniden Başlat");
    gui_button_set_callback(login_screen->restart_button, on_restart_button_clicked, login_screen);
    gui_layout_add_child(footer_layout, login_screen->restart_button);
    
    // Kullanıcı listesini güncelle
    update_user_list();
    
    is_initialized = 1;
    // Başlangıçta zaman etiketini güncelle
    on_clock_timer(login_screen->clock_timer, login_screen);
    
    // Pencereyi göster
    gui_window_show(login_screen->window);
    
    is_initialized = 1;
    
    // Otomatik giriş yapılandırıldıysa, giriş yapmayı dene
    if (login_screen->auto_login_enabled && login_screen->auto_login_user[0] != '\0') {
        attempt_auto_login();
    }
    
    return LOGIN_ERROR_NONE;
}

/**
 * Giriş ekranını temizler
 */
int login_screen_cleanup() {
    if (!is_initialized || !login_screen) {
        return LOGIN_ERROR_NONE;
    }
    
    // Zamanlatıcıyı durdur
    if (login_screen->clock_timer) {
        gui_timer_stop(login_screen->clock_timer);
        gui_timer_destroy(login_screen->clock_timer);
    }
    
    // Pencereyi kapat
    if (login_screen->window) {
        gui_window_close(login_screen->window);
        gui_window_destroy(login_screen->window);
    }
    
    // Kullanıcı listesi verisini temizle
    if (login_screen->user_list_data) {
        free(login_screen->user_list_data);
    }
    
    // Yapıyı temizle
    free(login_screen);
    login_screen = NULL;
    
    is_initialized = 0;
    return LOGIN_ERROR_NONE;
}

/**
 * Giriş butonu tıklama işleyicisi
 */
static void on_login_button_clicked(gui_button_t* button, void* user_data) {
    login_screen_t* screen = (login_screen_t*)user_data;
    
    // Kullanıcı adı ve parolayı al
    const char* username = gui_text_input_get_text(screen->username_input);
    const char* password = gui_password_input_get_text(screen->password_input);
    
    // Boş alanları kontrol et
    if (!username || strlen(username) == 0) {
        show_message("Lütfen kullanıcı adınızı girin.", 1);
        return;
    }
    
    if (!password || strlen(password) == 0) {
        show_message("Lütfen parolanızı girin.", 1);
        return;
    }
    
    // Giriş yap
    uint32_t session_id = 0;
    int result = session_login(username, password, &session_id);
    
    if (result == USER_ERROR_NONE) {
        // Başarılı giriş
        show_message("Giriş başarılı, oturum başlatılıyor...", 0);
        
        // Kısa bir gecikme ile oturum açma ekranını kapat ve masaüstüne geç
        gui_timer_t* delay_timer = gui_create_timer(1000);
        gui_timer_set_callback(delay_timer, 
            (void (*)(gui_timer_t*, void*))login_screen_cleanup, NULL);
        gui_timer_set_single_shot(delay_timer, 1);
        gui_timer_start(delay_timer);
        
        // Masaüstü uygulamasını başlat
        // Not: Gerçek uygulamada burada masaüstü oturumu başlatılır
        // system("desktop_session");
    } else {
        // Başarısız giriş
        screen->login_attempts++;
        
        const char* error_message = "Kullanıcı adı veya parola hatalı.";
        
        if (result == USER_ERROR_USER_NOT_FOUND) {
            error_message = "Kullanıcı bulunamadı.";
        } else if (result == USER_ERROR_AUTH_FAILED) {
            error_message = "Parola hatalı.";
        } else if (result == USER_ERROR_SESSION) {
            error_message = "Oturum oluşturulamadı.";
        }
        
        // Çok fazla başarısız giriş denemesi varsa, ek önlemler al
        if (screen->login_attempts >= 5) {
            error_message = "Çok fazla başarısız giriş denemesi. Lütfen bir süre bekleyin.";
            
            // Giriş butonunu geçici olarak devre dışı bırak
            gui_widget_set_enabled(screen->login_button, 0);
            
            // 30 saniye sonra tekrar etkinleştir
            gui_timer_t* delay_timer = gui_create_timer(30000);
            gui_timer_set_callback(delay_timer, 
                (void (*)(gui_timer_t*, void*))gui_widget_set_enabled, screen->login_button);
            gui_timer_set_single_shot(delay_timer, 1);
            gui_timer_start(delay_timer);
            
            screen->login_attempts = 0;
        }
        
        show_message(error_message, 1);
        
        // Parolayı temizle
        gui_password_input_clear(screen->password_input);
    }
    
    // Başarılı oturum açma durumunda giriş ve şifremi unuttum ekranlarını kapat
    if (result == USER_ERROR_NONE) {
        if (forgot_screen && forgot_screen->window) {
            gui_window_hide(forgot_screen->window);
        }
        
        if (register_screen && register_screen->window) {
            gui_window_hide(register_screen->window);
        }
    }
}

/**
 * Misafir butonu tıklama işleyicisi
 */
static void on_guest_button_clicked(gui_button_t* button, void* user_data) {
    login_screen_t* screen = (login_screen_t*)user_data;
    
    // Misafir oturumu açma izni kontrolü
    if (!screen->guest_enabled) {
        show_message("Misafir girişi devre dışı bırakılmış.", 1);
        return;
    }
    
    // Misafir oturumu açma desteği için session_login'e "guest" kullanıcısı gönder
    uint32_t session_id = 0;
    int result = session_login("guest", "", &session_id);
    
    if (result == USER_ERROR_NONE) {
        // Başarılı giriş
        show_message("Misafir oturumu başlatılıyor...", 0);
        
        // Kısa bir gecikme ile oturum açma ekranını kapat ve misafir masaüstüne geç
        gui_timer_t* delay_timer = gui_create_timer(1000);
        gui_timer_set_callback(delay_timer, 
            (void (*)(gui_timer_t*, void*))login_screen_cleanup, NULL);
        gui_timer_set_single_shot(delay_timer, 1);
        gui_timer_start(delay_timer);
        
        // Misafir masaüstü oturumunu başlat
        // Not: Gerçek uygulamada burada misafir oturumu başlatılır
        // system("desktop_session --guest");
    } else {
        // Başarısız giriş
        show_message("Misafir oturumu başlatılamadı.", 1);
    }
}

/**
 * Güç butonu tıklama işleyicisi
 */
static void on_power_button_clicked(gui_button_t* button, void* user_data) {
    // Onay penceresi göster
    gui_dialog_t* dialog = gui_create_dialog(
        "Kapatma", 
        "Bilgisayarınızı kapatmak istediğinizden emin misiniz?",
        GUI_DIALOG_YES_NO
    );
    
    int result = gui_dialog_show(dialog);
    gui_dialog_destroy(dialog);
    
    if (result == GUI_DIALOG_RESULT_YES) {
        // Sistemi kapat
        // Not: Gerçek uygulamada güvenli kapatma işlemi yapılır
        // system("poweroff");
    }
}

/**
 * Yeniden başlatma butonu tıklama işleyicisi
 */
static void on_restart_button_clicked(gui_button_t* button, void* user_data) {
    // Onay penceresi göster
    gui_dialog_t* dialog = gui_create_dialog(
        "Yeniden Başlatma", 
        "Bilgisayarınızı yeniden başlatmak istediğinizden emin misiniz?",
        GUI_DIALOG_YES_NO
    );
    
    int result = gui_dialog_show(dialog);
    gui_dialog_destroy(dialog);
    
    if (result == GUI_DIALOG_RESULT_YES) {
        // Sistemi yeniden başlat
        // Not: Gerçek uygulamada güvenli yeniden başlatma işlemi yapılır
        // system("reboot");
    }
}

/**
 * Kullanıcı seçimi işleyicisi
 */
static void on_user_selected(gui_list_t* list, int index, void* user_data) {
    login_screen_t* screen = (login_screen_t*)user_data;
    
    if (index < 0 || index >= screen->user_count) {
        return;
    }
    
    // Seçilen kullanıcıyı güncelle
    screen->selected_user_index = index;
    
    // Kullanıcı adını giriş kutusuna kopyala
    gui_text_input_set_text(screen->username_input, screen->user_list_data[index].username);
    
    // Parolayı temizle
    gui_password_input_clear(screen->password_input);
    
    // Mesajı temizle
    show_message("", 0);
    
    // Parola alanına odaklan
    gui_widget_focus(screen->password_input);
}

/**
 * Saat zamanlatıcısı işleyicisi
 */
static void on_clock_timer(gui_timer_t* timer, void* user_data) {
    login_screen_t* screen = (login_screen_t*)user_data;
    
    // Geçerli zamanı al
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    
    // Zaman dizesini oluştur
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%d %B %Y %H:%M:%S", tm_info);
    
    // Etiketi güncelle
    gui_label_set_text(screen->time_label, time_str);
}

/**
 * Kullanıcı listesini günceller
 */
static void update_user_list() {
    if (!login_screen) {
        return;
    }
    
    // Eski liste verisini temizle
    if (login_screen->user_list_data) {
        free(login_screen->user_list_data);
        login_screen->user_list_data = NULL;
        login_screen->user_count = 0;
    }
    
    // Kullanıcı listesini al
    // Varsayılan olarak 64 kullanıcıya kadar destek verelim
    login_screen->user_list_data = (user_info_t*)calloc(64, sizeof(user_info_t));
    if (!login_screen->user_list_data) {
        return;
    }
    
    // Kullanıcı listesini al
    uint32_t count = 0;
    int result = user_list_all(login_screen->user_list_data, 64, &count);
    if (result != USER_ERROR_NONE) {
        free(login_screen->user_list_data);
        login_screen->user_list_data = NULL;
        return;
    }
    
    login_screen->user_count = count;
    
    // Kullanıcı listesini güncelle
    gui_list_clear(login_screen->user_list);
    
    for (uint32_t i = 0; i < count; i++) {
        // Misafir kullanıcıyı gösterme
        if (strcmp(login_screen->user_list_data[i].username, "guest") == 0) {
            continue;
        }
        
        // Devre dışı kullanıcıları gösterme
        if (login_screen->user_list_data[i].account_disabled) {
            continue;
        }
        
        // Kullanıcıyı listeye ekle
        gui_list_add_item(login_screen->user_list, login_screen->user_list_data[i].username);
    }
    
    // Otomatik giriş kullanıcısını seç (varsa)
    if (login_screen->auto_login_enabled && login_screen->auto_login_user[0] != '\0') {
        for (uint32_t i = 0; i < count; i++) {
            if (strcmp(login_screen->user_list_data[i].username, login_screen->auto_login_user) == 0) {
                gui_list_set_selected(login_screen->user_list, i);
                on_user_selected(login_screen->user_list, i, login_screen);
                break;
            }
        }
    }
}

/**
 * Mesaj gösterir
 */
static void show_message(const char* message, int is_error) {
    if (!login_screen) {
        return;
    }
    
    gui_label_set_text(login_screen->message_label, message);
    gui_label_set_color(login_screen->message_label, is_error ? 0xFF0000 : 0x008000);
}

/**
 * Otomatik giriş denemesi
 */
static int attempt_auto_login() {
    if (!login_screen || !login_screen->auto_login_enabled || strlen(login_screen->auto_login_user) == 0) {
        return -1;
    }
    
    // Kullanıcıyı bul
    user_info_t user_info;
    int result = user_get_by_name(login_screen->auto_login_user, &user_info);
    if (result != USER_ERROR_NONE) {
        return -1;
    }
    
    if (!user_info.enabled) {
        return -1;
    }
    
    // Otomatik giriş bilgilerini kontrol et
    int valid = user_verify_auto_login(login_screen->auto_login_user);
    if (!valid) {
        return -1;
    }
    
    // Otomatik giriş için 3 saniye bekle (kullanıcının iptal etmesine izin verir)
    gui_label_set_text(login_screen->message_label, "Otomatik giriş yapılıyor...");
    
    gui_timer_t* auto_login_timer = gui_create_timer(3000);
    gui_timer_set_callback(auto_login_timer, on_auto_login_timer, &user_info);
    gui_timer_start(auto_login_timer);
    
    return 0;
}

/**
 * Otomatik giriş zamanlayıcısı
 */
static void on_auto_login_timer(gui_timer_t* timer, void* user_data) {
    user_info_t* user_info = (user_info_t*)user_data;
    
    // Otomatik giriş için oturum başlat
    login_user(user_info->username, NULL, 1);
    
    // Zamanlayıcıyı yok et
    gui_timer_destroy(timer);
}

/**
 * Giriş ekranını gösterir
 */
void login_screen_show() {
    if (!is_initialized) {
        if (login_screen_init() != LOGIN_ERROR_NONE) {
            return;
        }
    }
    
    // Kullanıcı listesini güncelle
    update_user_list();
    
    // Şifremi unuttum ve kayıt ekranlarını oluştur (gösterme)
    if (!forgot_screen) {
        forgot_screen = create_forgot_password_screen();
    }
    
    if (!register_screen) {
        register_screen = create_register_screen();
    }
    
    // Giriş ekranını göster
    gui_window_show(login_screen->window);
    
    // Zaman ve tarih etiketi güncelle
    on_clock_timer(login_screen->clock_timer, login_screen);
    
    // Misafir butonu görünürlüğünü güncelle
    gui_widget_set_visible(login_screen->guest_button, login_screen->guest_enabled);
    
    // Oturum açma girişim sayacını sıfırla
    login_screen->login_attempts = 0;
    
    // Otomatik giriş kontrolü
    if (login_screen->auto_login_enabled && strlen(login_screen->auto_login_user) > 0) {
        attempt_auto_login();
    }
}

/**
 * Şifremi unuttum ekranını oluşturur
 */
static void create_forgot_password_screen() {
    if (forgot_screen) {
        return; // Zaten oluşturulmuş
    }
    
    // Yapıyı oluştur
    forgot_screen = (forgot_password_screen_t*)calloc(1, sizeof(forgot_password_screen_t));
    if (!forgot_screen) {
        return;
    }
    
    // Pencereyi oluştur
    forgot_screen->window = gui_create_window("Şifremi Unuttum", 450, 350, 0);
    if (!forgot_screen->window) {
        free(forgot_screen);
        forgot_screen = NULL;
        return;
    }
    
    // Ana düzeni oluştur
    forgot_screen->main_layout = gui_create_layout(GUI_LAYOUT_VERTICAL);
    gui_layout_set_spacing(forgot_screen->main_layout, 20);
    gui_layout_set_padding(forgot_screen->main_layout, 30);
    gui_window_set_layout(forgot_screen->window, forgot_screen->main_layout);
    
    // Başlık
    gui_label_t* title_label = gui_create_label("Şifremi Unuttum");
    gui_label_set_font_size(title_label, 20);
    gui_layout_add_child(forgot_screen->main_layout, title_label);
    
    // Açıklama
    gui_label_t* desc_label = gui_create_label("Lütfen kullanıcı adınızı girin. Kayıtlı e-posta adresinize bir kurtarma kodu göndereceğiz.");
    gui_label_set_wrap(desc_label, 1);
    gui_layout_add_child(forgot_screen->main_layout, desc_label);
    
    // Mesaj etiketi
    forgot_screen->message_label = gui_create_label("");
    gui_label_set_font_size(forgot_screen->message_label, 14);
    gui_label_set_color(forgot_screen->message_label, 0xFF0000); // Kırmızı
    gui_layout_add_child(forgot_screen->main_layout, forgot_screen->message_label);
    
    // Kullanıcı adı alanı
    gui_layout_t* username_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_add_child(forgot_screen->main_layout, username_layout);
    
    gui_label_t* username_label = gui_create_label("Kullanıcı Adı:");
    gui_layout_add_child(username_layout, username_label);
    
    forgot_screen->username_input = gui_create_text_input();
    gui_layout_add_child(username_layout, forgot_screen->username_input);
    
    // E-posta alanı
    gui_layout_t* email_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_add_child(forgot_screen->main_layout, email_layout);
    
    gui_label_t* email_label = gui_create_label("E-posta:");
    gui_layout_add_child(email_layout, email_label);
    
    forgot_screen->email_input = gui_create_text_input();
    gui_layout_add_child(email_layout, forgot_screen->email_input);
    
    // Butonlar düzeni
    gui_layout_t* button_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_set_spacing(button_layout, 10);
    gui_layout_add_child(forgot_screen->main_layout, button_layout);
    
    // İptal butonu
    forgot_screen->cancel_button = gui_create_button("İptal");
    gui_button_set_callback(forgot_screen->cancel_button, on_forgot_cancel_clicked, forgot_screen);
    gui_layout_add_child(button_layout, forgot_screen->cancel_button);
    
    // Gönder butonu
    forgot_screen->send_button = gui_create_button("Kurtarma Kodu Gönder");
    gui_button_set_callback(forgot_screen->send_button, on_forgot_send_clicked, forgot_screen);
    gui_layout_add_child(button_layout, forgot_screen->send_button);
    
    // Başlangıçta pencere gizli
    gui_window_hide(forgot_screen->window);
}

/**
 * Kayıt ekranını oluşturur
 */
static void create_register_screen() {
    if (register_screen) {
        return; // Zaten oluşturulmuş
    }
    
    // Yapıyı oluştur
    register_screen = (register_screen_t*)calloc(1, sizeof(register_screen_t));
    if (!register_screen) {
        return;
    }
    
    // Pencereyi oluştur
    register_screen->window = gui_create_window("Yeni Kullanıcı Kaydı", 500, 400, 0);
    if (!register_screen->window) {
        free(register_screen);
        register_screen = NULL;
        return;
    }
    
    // Ana düzeni oluştur
    register_screen->main_layout = gui_create_layout(GUI_LAYOUT_VERTICAL);
    gui_layout_set_spacing(register_screen->main_layout, 15);
    gui_layout_set_padding(register_screen->main_layout, 30);
    gui_window_set_layout(register_screen->window, register_screen->main_layout);
    
    // Başlık
    gui_label_t* title_label = gui_create_label("Yeni Kullanıcı Kaydı");
    gui_label_set_font_size(title_label, 20);
    gui_layout_add_child(register_screen->main_layout, title_label);
    
    // Açıklama
    gui_label_t* desc_label = gui_create_label("Lütfen yeni kullanıcı hesabınız için aşağıdaki bilgileri doldurun.");
    gui_label_set_wrap(desc_label, 1);
    gui_layout_add_child(register_screen->main_layout, desc_label);
    
    // Mesaj etiketi
    register_screen->message_label = gui_create_label("");
    gui_label_set_font_size(register_screen->message_label, 14);
    gui_label_set_color(register_screen->message_label, 0xFF0000); // Kırmızı
    gui_layout_add_child(register_screen->main_layout, register_screen->message_label);
    
    // Kullanıcı adı alanı
    gui_layout_t* username_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_add_child(register_screen->main_layout, username_layout);
    
    gui_label_t* username_label = gui_create_label("Kullanıcı Adı:");
    gui_layout_set_width(username_label, 150);
    gui_layout_add_child(username_layout, username_label);
    
    register_screen->username_input = gui_create_text_input();
    gui_layout_add_child(username_layout, register_screen->username_input);
    
    // Parola alanı
    gui_layout_t* password_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_add_child(register_screen->main_layout, password_layout);
    
    gui_label_t* password_label = gui_create_label("Parola:");
    gui_layout_set_width(password_label, 150);
    gui_layout_add_child(password_layout, password_label);
    
    register_screen->password_input = gui_create_password_input();
    gui_layout_add_child(password_layout, register_screen->password_input);
    
    // Parola onay alanı
    gui_layout_t* confirm_password_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_add_child(register_screen->main_layout, confirm_password_layout);
    
    gui_label_t* confirm_password_label = gui_create_label("Parolayı Onayla:");
    gui_layout_set_width(confirm_password_label, 150);
    gui_layout_add_child(confirm_password_layout, confirm_password_label);
    
    register_screen->confirm_password_input = gui_create_password_input();
    gui_layout_add_child(confirm_password_layout, register_screen->confirm_password_input);
    
    // E-posta alanı
    gui_layout_t* email_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_add_child(register_screen->main_layout, email_layout);
    
    gui_label_t* email_label = gui_create_label("Kurtarma E-postası:");
    gui_layout_set_width(email_label, 150);
    gui_layout_add_child(email_layout, email_label);
    
    register_screen->email_input = gui_create_text_input();
    gui_layout_add_child(email_layout, register_screen->email_input);
    
    // Kullanıcı tipi alanı
    gui_layout_t* type_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_add_child(register_screen->main_layout, type_layout);
    
    gui_label_t* type_label = gui_create_label("Kullanıcı Tipi:");
    gui_layout_add_child(type_layout, type_label);
    
    register_screen->user_type_combobox = gui_create_combobox();
    gui_combobox_add_item(register_screen->user_type_combobox, "Standart Kullanıcı");
    gui_combobox_add_item(register_screen->user_type_combobox, "Yönetici");
    gui_combobox_set_selected_index(register_screen->user_type_combobox, 0); // Varsayılan olarak standart kullanıcı
    gui_layout_add_child(type_layout, register_screen->user_type_combobox);
    
    // Butonlar
    gui_layout_t* button_layout = gui_create_layout(GUI_LAYOUT_HORIZONTAL);
    gui_layout_set_spacing(button_layout, 10);
    gui_layout_add_child(register_screen->main_layout, button_layout);
    
    register_screen->register_button = gui_create_button("Kayıt Ol");
    gui_button_set_callback(register_screen->register_button, on_register_submit_clicked, register_screen);
    gui_layout_add_child(button_layout, register_screen->register_button);
    
    register_screen->cancel_button = gui_create_button("İptal");
    gui_button_set_callback(register_screen->cancel_button, on_register_cancel_clicked, register_screen);
    gui_layout_add_child(button_layout, register_screen->cancel_button);
    
    // Başlangıçta pencere gizli
    gui_window_hide(register_screen->window);
}

/**
 * Şifremi unuttum butonuna tıklandığında
 */
static void on_forgot_password_button_clicked(gui_button_t* button, void* user_data) {
    login_screen_t* screen = (login_screen_t*)user_data;
    
    // Şifremi unuttum ekranı yoksa oluştur
    if (!forgot_screen) {
        forgot_screen = create_forgot_password_screen();
        if (!forgot_screen) {
            gui_label_set_text(screen->message_label, "Hata: Şifre kurtarma ekranı oluşturulamadı");
            return;
        }
    }
    
    // Seçili kullanıcı varsa kullanıcı adını doldur
    const char* selected_user = gui_list_get_selected_item(screen->user_list);
    if (selected_user) {
        gui_text_input_set_text(forgot_screen->username_input, selected_user);
    } else {
        // Giriş alanındaki kullanıcı adını kullan
        const char* username = gui_text_input_get_text(screen->username_input);
        if (username && strlen(username) > 0) {
            gui_text_input_set_text(forgot_screen->username_input, username);
        }
    }
    
    // Ekranı göster
    gui_window_show(forgot_screen->window);
}

/**
 * Kayıt ol butonuna tıklandığında
 */
static void on_register_button_clicked(gui_button_t* button, void* user_data) {
    // Kayıt ekranı yoksa oluştur
    if (!register_screen) {
        register_screen = create_register_screen();
        if (!register_screen) {
            login_screen_t* screen = (login_screen_t*)user_data;
            gui_label_set_text(screen->message_label, "Hata: Kayıt ekranı oluşturulamadı");
            return;
        }
    }
    
    // Ekranı göster
    gui_window_show(register_screen->window);
}

/**
 * Şifremi unuttum ekranında iptal butonuna tıklandığında
 */
static void on_forgot_cancel_clicked(gui_button_t* button, void* user_data) {
    forgot_password_screen_t* screen = (forgot_password_screen_t*)user_data;
    gui_window_hide(screen->window);
}

/**
 * Şifremi unuttum ekranında gönder butonuna tıklandığında
 */
static void on_forgot_send_clicked(gui_button_t* button, void* user_data) {
    forgot_password_screen_t* screen = (forgot_password_screen_t*)user_data;
    
    // Giriş alanlarını doğrula
    const char* username = gui_text_input_get_text(screen->username_input);
    const char* email = gui_text_input_get_text(screen->email_input);
    
    if (!username || strlen(username) == 0) {
        gui_label_set_text(screen->message_label, "Lütfen kullanıcı adını girin");
        return;
    }
    
    if (!email || strlen(email) == 0) {
        gui_label_set_text(screen->message_label, "Lütfen e-posta adresini girin");
        return;
    }
    
    // Kullanıcıyı bul
    user_info_t user_info;
    int user_exists = 0;
    int user_count = 0;
    user_list_all(&user_count, NULL);
    
    if (user_count > 0) {
        user_info_t* users = (user_info_t*)malloc(sizeof(user_info_t) * user_count);
        if (users) {
            user_list_all(&user_count, users);
            
            for (int i = 0; i < user_count; i++) {
                if (strcmp(users[i].username, username) == 0) {
                    user_info = users[i];
                    user_exists = 1;
                    break;
                }
            }
            
            free(users);
        }
    }
    
    if (!user_exists) {
        gui_label_set_text(screen->message_label, "Kullanıcı bulunamadı");
        return;
    }
    
    // Kurtarma koduyla parola sıfırlama işlemini başlat
    int result = recovery_send_code(username, email);
    if (result == RECOVERY_ERROR_NONE) {
        gui_label_set_text(screen->message_label, "Kurtarma kodu gönderildi");
        // 3 saniye sonra pencereyi kapat
        gui_timer_t* close_timer = gui_create_timer(3000);
        gui_timer_set_callback(close_timer, on_forgot_close_timer, screen);
        gui_timer_start(close_timer);
    } else if (result == RECOVERY_ERROR_EMAIL_MISMATCH) {
        gui_label_set_text(screen->message_label, "E-posta adresi kullanıcı hesabıyla eşleşmiyor");
    } else {
        gui_label_set_text(screen->message_label, "Kurtarma kodu gönderilemedi");
    }
}

/**
 * Şifremi unuttum ekranını kapatmak için zamanlayıcı
 */
static void on_forgot_close_timer(gui_timer_t* timer, void* user_data) {
    forgot_password_screen_t* screen = (forgot_password_screen_t*)user_data;
    gui_window_hide(screen->window);
    gui_timer_destroy(timer);
}

/**
 * Kayıt ekranında iptal butonuna tıklandığında
 */
static void on_register_cancel_clicked(gui_button_t* button, void* user_data) {
    register_screen_t* screen = (register_screen_t*)user_data;
    gui_window_hide(screen->window);
}

/**
 * Kayıt ekranında hesap oluştur butonuna tıklandığında
 */
static void on_register_submit_clicked(gui_button_t* button, void* user_data) {
    register_screen_t* screen = (register_screen_t*)user_data;
    
    // Giriş alanlarını doğrula
    const char* username = gui_text_input_get_text(screen->username_input);
    const char* password = gui_text_input_get_text(screen->password_input);
    const char* confirm = gui_text_input_get_text(screen->confirm_password_input);
    const char* email = gui_text_input_get_text(screen->email_input);
    int type_index = gui_combobox_get_selected_index(screen->user_type_combobox);
    
    if (!username || strlen(username) == 0) {
        gui_label_set_text(screen->message_label, "Lütfen kullanıcı adını girin");
        return;
    }
    
    if (!password || strlen(password) == 0) {
        gui_label_set_text(screen->message_label, "Lütfen parolayı girin");
        return;
    }
    
    if (!confirm || strlen(confirm) == 0) {
        gui_label_set_text(screen->message_label, "Lütfen parolayı onaylayın");
        return;
    }
    
    if (strcmp(password, confirm) != 0) {
        gui_label_set_text(screen->message_label, "Parolalar eşleşmiyor");
        return;
    }
    
    if (!email || strlen(email) == 0) {
        gui_label_set_text(screen->message_label, "Lütfen e-posta adresini girin");
        return;
    }
    
    // Kullanıcı tipini belirle
    user_type_t user_type = USER_TYPE_STANDARD;
    if (type_index == 1) { // Yönetici seçildi
        user_type = USER_TYPE_ADMIN;
    }
    
    // Yeni kullanıcı oluştur
    int result = user_create(username, password, user_type, email);
    if (result == USER_ERROR_NONE) {
        gui_label_set_text(screen->message_label, "Kullanıcı başarıyla oluşturuldu");
        
        // Kullanıcı listesini güncelle
        update_user_list();
        
        // 3 saniye sonra pencereyi kapat
        gui_timer_t* close_timer = gui_create_timer(3000);
        gui_timer_set_callback(close_timer, on_register_close_timer, screen);
        gui_timer_start(close_timer);
    } else if (result == USER_ERROR_USERNAME_EXISTS) {
        gui_label_set_text(screen->message_label, "Bu kullanıcı adı zaten kullanılıyor");
    } else if (result == USER_ERROR_INVALID_USERNAME) {
        gui_label_set_text(screen->message_label, "Geçersiz kullanıcı adı (3-20 karakter, alfanümerik)");
    } else if (result == USER_ERROR_INVALID_PASSWORD) {
        gui_label_set_text(screen->message_label, "Geçersiz parola (en az 8 karakter, 1 büyük, 1 küçük, 1 sayı)");
    } else {
        gui_label_set_text(screen->message_label, "Kullanıcı oluşturulamadı");
    }
}

/**
 * Kayıt ekranını kapatmak için zamanlayıcı
 */
static void on_register_close_timer(gui_timer_t* timer, void* user_data) {
    register_screen_t* screen = (register_screen_t*)user_data;
    gui_window_hide(screen->window);
    gui_timer_destroy(timer);
}

/**
 * Ana fonksiyon - kullanıcı giriş ekranını çalıştırır
 */
int main(int argc, char** argv) {
    // Kullanıcı yönetim sistemini başlat
    if (user_manager_init() != USER_ERROR_NONE) {
        fprintf(stderr, "Kullanıcı yönetim sistemi başlatılamadı\n");
        return 1;
    }
    
    // GUI sistemini başlat
    if (gui_init() != 0) {
        fprintf(stderr, "GUI sistemi başlatılamadı\n");
        user_manager_cleanup();
        return 1;
    }
    
    // Giriş ekranını başlat
    if (login_screen_init() != LOGIN_ERROR_NONE) {
        fprintf(stderr, "Giriş ekranı başlatılamadı\n");
        gui_cleanup();
        user_manager_cleanup();
        return 1;
    }
    
    // Ana döngüyü çalıştır
    gui_main_loop();
    
    // Temizlik
    login_screen_cleanup();
    gui_cleanup();
    user_manager_cleanup();
    
    return 0;
} 