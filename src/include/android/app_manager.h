#ifndef ANDROID_APP_MANAGER_API_H
#define ANDROID_APP_MANAGER_API_H

/**
 * @file app_manager.h
 * @brief Android uygulama yöneticisi API'leri
 * 
 * Bu modül, KALEM OS'ta çalışan Android uygulamalarını yönetmek için
 * gerekli API'leri sağlar. Uygulama kurma, kaldırma, başlatma, durdurma ve 
 * detaylı bilgi sorgulama işlevleri içerir.
 */

#include "android.h"
#include <stdint.h>

/**
 * @brief Android uygulaması hakkında bilgiler
 */
typedef struct {
    /** Paket adı */
    char package_name[256];
    
    /** Uygulama sürümü */
    char version[32];
    
    /** Uygulama yükleme tarihi */
    char install_date[32];
    
    /** Son güncelleme tarihi */
    char last_update_date[32];
    
    /** Uygulama boyutu (KB) */
    uint32_t size_kb;
    
    /** Veri boyutu (KB) */
    uint32_t data_size_kb;
    
    /** Uygulama çalışıyor mu? */
    uint8_t is_running;
    
    /** Sistem uygulaması mı? */
    uint8_t is_system_app;
    
    /** İzin sayısı */
    uint32_t permission_count;
    
    /** Minimum SDK sürümü */
    uint32_t min_sdk_version;
    
    /** Hedef SDK sürümü */
    uint32_t target_sdk_version;
    
    /** APK dosya yolu */
    char apk_path[512];
    
    /** Veri yolu */
    char data_path[512];
    
    /** Uygulama ismi */
    char app_name[256];
    
    /** Geliştirici ismi */
    char developer_name[256];
} android_app_t;

/**
 * @brief Yüklü uygulamaları listeler
 * 
 * @param apps[out] Uygulama dizisi
 * @param max_count[in] Azami listelenecek uygulama sayısı
 * @param count[out] Mevcut uygulama sayısı
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_get_apps(
    android_app_t* apps, 
    uint32_t max_count, 
    uint32_t* count
);

/**
 * @brief Belirtilen paket adına sahip uygulamayı başlatır
 * 
 * @param package_name Paket adı
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_launch_app(const char* package_name);

/**
 * @brief Belirtilen paket adına sahip uygulamayı durdurur
 * 
 * @param package_name Paket adı
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_stop_app(const char* package_name);

/**
 * @brief APK dosyasını sisteme yükler
 * 
 * @param apk_path APK dosyasının yolu
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_install_app(const char* apk_path);

/**
 * @brief Belirtilen paket adına sahip uygulamayı sistemden kaldırır
 * 
 * @param package_name Paket adı
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_uninstall_app(const char* package_name);

/**
 * @brief Belirtilen paket adına sahip uygulamanın verilerini temizler
 * 
 * @param package_name Paket adı
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_clear_app_data(const char* package_name);

/**
 * @brief Belirtilen paket adına sahip uygulamayı yedekler
 * 
 * @param package_name Paket adı
 * @param backup_path Yedek dosyasının yolu
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_backup_app(
    const char* package_name, 
    const char* backup_path
);

/**
 * @brief Yedeklenmiş bir uygulamayı geri yükler
 * 
 * @param backup_path Yedek dosyasının yolu
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_restore_app(const char* backup_path);

/**
 * @brief Belirtilen paket adına sahip uygulamanın detaylarını alır
 * 
 * @param package_name Paket adı
 * @param app[out] Uygulama detayları
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_get_app_info(
    const char* package_name, 
    android_app_t* app
);

/**
 * @brief Belirtilen izinleri kontrol eder
 * 
 * @param package_name Paket adı
 * @param permission İzin adı
 * @param granted[out] İzin verilmiş mi?
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_check_permission(
    const char* package_name,
    const char* permission,
    uint8_t* granted
);

/**
 * @brief İzin durumunu değiştirir
 * 
 * @param package_name Paket adı
 * @param permission İzin adı
 * @param granted İzin durumu
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_set_permission(
    const char* package_name,
    const char* permission,
    uint8_t granted
);

/**
 * @brief Sistem servislerini yeniden başlatır
 * 
 * @return android_result_t Başarı kodu
 */
android_result_t android_app_manager_restart_services();

#endif /* ANDROID_APP_MANAGER_API_H */ 