#ifndef _LOGIN_SCREEN_H
#define _LOGIN_SCREEN_H

#include "gui.h"
#include "user_manager.h"
#include "login_recovery.h"

/**
 * Maksimum giriş denemesi sayısı
 */
#define MAX_LOGIN_ATTEMPTS 5

/**
 * Giriş ekranı hata kodları
 */
typedef enum {
    LOGIN_ERROR_NONE = 0,         // Hata yok
    LOGIN_ERROR_INIT = -1,        // Başlatma hatası
    LOGIN_ERROR_GUI = -2,         // GUI hatası
    LOGIN_ERROR_USER_NOT_FOUND = -3, // Kullanıcı bulunamadı
    LOGIN_ERROR_INVALID_PASSWORD = -4, // Geçersiz parola
    LOGIN_ERROR_USER_DISABLED = -5,  // Kullanıcı devre dışı
    LOGIN_ERROR_CREATE_SESSION = -6, // Oturum oluşturma hatası
    LOGIN_ERROR_TOO_MANY_ATTEMPTS = -7, // Çok fazla giriş denemesi
    LOGIN_ERROR_INVALID_ARGS = -8,    // Geçersiz argümanlar
    LOGIN_ERROR_UNKNOWN = -9         // Bilinmeyen hata
} login_error_t;

/**
 * Giriş ekranı yapısı
 */
typedef struct login_screen {
    gui_window_t* window;
    gui_layout_t* main_layout;
    
    gui_label_t* message_label;
    gui_label_t* time_label;
    
    gui_list_t* user_list;
    gui_text_input_t* username_input;
    gui_password_input_t* password_input;
    
    gui_button_t* login_button;
    gui_button_t* guest_button;
    gui_button_t* power_button;
    gui_button_t* restart_button;
    
    gui_timer_t* clock_timer;
    
    // Yapılandırma
    char hostname[64];
    uint8_t guest_enabled;
    uint8_t auto_login_enabled;
    char auto_login_user[32];
    
    // Güvenlik
    uint8_t login_attempts;
} login_screen_t;

/**
 * Giriş ekranını başlatır
 * 
 * @return Hata kodu (LOGIN_ERROR_NONE başarı)
 */
int login_screen_init();

/**
 * Giriş ekranını gösterir
 */
void login_screen_show();

/**
 * Giriş ekranını temizler
 * 
 * @return Hata kodu (LOGIN_ERROR_NONE başarı)
 */
int login_screen_cleanup();

#endif /* _LOGIN_SCREEN_H */ 