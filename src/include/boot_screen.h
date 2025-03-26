#ifndef _BOOT_SCREEN_H
#define _BOOT_SCREEN_H

/**
 * Açılış ekranı hata kodları
 */
typedef enum {
    BOOT_ERROR_NONE = 0,      // Hata yok
    BOOT_ERROR_INIT = -1,     // Başlatma hatası
    BOOT_ERROR_GUI = -2,      // GUI hatası
    BOOT_ERROR_RESOURCE = -3, // Kaynak hatası (örn. resim dosyası bulunamadı)
    BOOT_ERROR_UNKNOWN = -4   // Bilinmeyen hata
} boot_error_t;

/**
 * Açılış ekranını başlatır
 * 
 * @return Hata kodu (BOOT_ERROR_NONE başarı)
 */
int boot_screen_init();

/**
 * Açılış ekranını gösterir
 * 
 * Bu fonksiyon, açılış ekranını gösterir ve açılış animasyonunu başlatır.
 * Animasyon tamamlandığında giriş ekranına geçilir.
 */
void boot_screen_show();

/**
 * Açılış ekranını atlar
 * 
 * Test için kullanılır, doğrudan giriş ekranına geçer.
 */
void boot_screen_skip();

#endif /* _BOOT_SCREEN_H */ 