#ifndef ANDROID_SETTINGS_H
#define ANDROID_SETTINGS_H

/**
 * @file android_settings.h
 * @brief Android ayarları için kullanıcı arayüzü bileşeni
 * 
 * Bu modül, KALEM OS'ta çalışan Android altsisteminin yapılandırmasını
 * değiştirmek için kullanıcı arayüzü bileşenlerini sağlar.
 */

#include "gui.h"

/**
 * @brief Android ayarlar penceresini oluşturur
 * 
 * @return gui_window_t* Oluşturulan pencere işaretçisi veya NULL (başarısızlık durumunda)
 */
gui_window_t* android_settings_create();

/**
 * @brief Android ayarlar penceresini kapatır
 */
void android_settings_close();

/**
 * @brief Android ayarlar penceresini gösterir
 * 
 * Eğer pencere daha önce oluşturulmamışsa, önce oluşturur ve sonra gösterir.
 */
void android_settings_show();

/**
 * @brief Ana uygulama noktası (bağımsız çalıştırma için)
 * 
 * @param argc Komut satırı argüman sayısı
 * @param argv Komut satırı argümanları
 * @return int Dönüş kodu (0: başarılı)
 */
int android_settings_main(int argc, char* argv[]);

#endif /* ANDROID_SETTINGS_H */ 