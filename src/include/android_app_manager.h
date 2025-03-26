#ifndef ANDROID_APP_MANAGER_H
#define ANDROID_APP_MANAGER_H

/**
 * @file android_app_manager.h
 * @brief Android uygulama yöneticisi kullanıcı arayüzü
 * 
 * Bu modül, KALEM OS'ta çalışan Android uygulamalarını yönetmek için
 * kullanıcı arayüzü bileşenlerini sağlar.
 */

#include "gui.h"

/**
 * @brief Android uygulama yöneticisi penceresini oluşturur
 * 
 * @return gui_window_t* Oluşturulan pencere işaretçisi veya NULL (başarısızlık durumunda)
 */
gui_window_t* android_app_manager_create();

/**
 * @brief Android uygulama yöneticisi penceresini kapatır
 */
void android_app_manager_close();

/**
 * @brief Android uygulama yöneticisi penceresini gösterir
 * 
 * Eğer pencere daha önce oluşturulmamışsa, önce oluşturur ve sonra gösterir.
 */
void android_app_manager_show();

/**
 * @brief Ana uygulama noktası (bağımsız çalıştırma için)
 * 
 * @param argc Komut satırı argüman sayısı
 * @param argv Komut satırı argümanları
 * @return int Dönüş kodu (0: başarılı)
 */
int android_app_manager_main(int argc, char* argv[]);

#endif /* ANDROID_APP_MANAGER_H */ 