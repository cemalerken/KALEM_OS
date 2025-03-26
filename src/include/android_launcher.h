#ifndef ANDROID_LAUNCHER_H
#define ANDROID_LAUNCHER_H

/**
 * @file android_launcher.h
 * @brief Android başlatıcı kullanıcı arayüzü
 * 
 * Bu modül, KALEM OS'ta çalışan Android uygulamalarını başlatmak için
 * kullanıcı arayüzü bileşenlerini sağlar.
 */

#include "gui.h"

/**
 * @brief Android başlatıcı penceresini oluşturur
 * 
 * @return gui_window_t* Oluşturulan pencere işaretçisi veya NULL (başarısızlık durumunda)
 */
gui_window_t* android_launcher_create();

/**
 * @brief Android başlatıcı penceresini kapatır
 */
void android_launcher_close();

/**
 * @brief Android başlatıcı penceresini gösterir
 * 
 * Eğer pencere daha önce oluşturulmamışsa, önce oluşturur ve sonra gösterir.
 */
void android_launcher_show();

/**
 * @brief Ana uygulama noktası (bağımsız çalıştırma için)
 * 
 * @param argc Komut satırı argüman sayısı
 * @param argv Komut satırı argümanları
 * @return int Dönüş kodu (0: başarılı)
 */
int android_launcher_main(int argc, char* argv[]);

#endif /* ANDROID_LAUNCHER_H */ 