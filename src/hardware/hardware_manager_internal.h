#ifndef HARDWARE_MANAGER_INTERNAL_H
#define HARDWARE_MANAGER_INTERNAL_H

#include "../include/hardware_manager.h"
#include "kernel_hal.h"

/**
 * @file hardware_manager_internal.h
 * @brief KALEM OS Donanım Yönetim Merkezi İç Yapıları
 * 
 * Bu dosya, donanım yöneticisi modülü içerisinde paylaşılan yapıları tanımlar.
 * Sadece implementasyon dosyaları tarafından dahil edilmeli, dışarıdan erişim için
 * hardware_manager.h kullanılmalıdır.
 */

/**
 * Donanım izleme yapısı
 */
typedef struct {
    uint32_t id;                 // İzleme kimliği
    uint32_t component_id;       // İzlenen bileşen kimliği (0: tüm bileşenler)
    uint32_t interval_ms;        // İzleme aralığı (ms)
    void* callback;              // İzleme geri çağırma fonksiyonu
    kernel_thread_t thread;      // İzleme thread'i (pthread_t yerine kernel_thread_t)
    bool running;                // İzleme çalışıyor mu?
} hw_monitor_t;

/**
 * İzleme geri çağırma fonksiyonu imzası
 */
typedef void (*hw_monitor_callback_t)(hw_component_t* component);

/**
 * PCI aygıt tanımlama yapısı
 */
typedef struct {
    uint16_t vendor_id;          // PCI üretici kimliği
    uint16_t device_id;          // PCI aygıt kimliği
    uint16_t class_id;           // PCI sınıf kimliği
    uint8_t revision;            // PCI revizyon
    uint8_t irq;                 // Kesme hattı
    uint32_t bar[6];             // Bellek ve I/O adresleri
} hw_pci_id_t;

/**
 * USB aygıt tanımlama yapısı
 */
typedef struct {
    uint16_t vendor_id;          // USB üretici kimliği
    uint16_t product_id;         // USB ürün kimliği
    uint8_t device_class;        // USB aygıt sınıfı
    uint8_t device_subclass;     // USB aygıt alt sınıfı
    uint8_t device_protocol;     // USB aygıt protokolü
    uint8_t configuration;       // Aktif yapılandırma
    uint8_t interface;           // Arabirim numarası
    uint8_t endpoint;            // Uç nokta sayısı
} hw_usb_id_t;

/**
 * ACPI aygıt tanımlama yapısı
 */
typedef struct {
    char hid[16];                // Donanım kimliği
    char uid[16];                // Benzersiz kimlik
    uint8_t adr;                 // Adres
    uint8_t status;              // Durum
    uint8_t flags;               // Bayraklar
} hw_acpi_id_t;

/**
 * Yardımcı fonksiyonlar
 */

/**
 * Bileşen ID'sine göre indisini bulur
 * 
 * @param component_id Bileşen ID'si
 * @return int Bulunan indis, veya -1 (bulunamadı)
 */
int hw_find_component_index(uint32_t component_id);

/**
 * Bileşen durumunu günceller
 * 
 * @param component Bileşen işaretçisi
 * @return int 0: başarılı, <0: hata
 */
int hw_update_component_status(hw_component_t* component);

/**
 * Günlük kaydı yapar
 * 
 * @param level Günlük seviyesi (0: hata, 1: uyarı, 2: bilgi, 3: hata ayıklama)
 * @param format Format dizesi
 * @param ... Değişken argümanlar
 */
void hw_log(uint32_t level, const char* format, ...);

/**
 * Donanım bileşeni için bir sürücü tespit eder
 * 
 * @param component Bileşen işaretçisi
 * @return int 0: başarılı, <0: hata
 */
int hw_detect_driver(hw_component_t* component);

/**
 * Donanım bileşenlerini tespit eder
 * 
 * @return int 0: başarılı, <0: hata
 */
int hw_detect_components(void);

/**
 * Yeni bir donanım bileşeni oluşturur
 * 
 * @param type Bileşen türü
 * @param name Bileşen adı
 * @param component_out Oluşturulan bileşen işaretçisi (isteğe bağlı)
 * @return int 0: başarılı, <0: hata
 */
int hw_create_component(hw_component_type_t type, const char* name, hw_component_t** component_out);

/**
 * Donanım bileşenini kaldırır
 * 
 * @param component_id Bileşen ID'si
 * @return int 0: başarılı, <0: hata
 */
int hw_remove_component(uint32_t component_id);

/**
 * İzleme thread'i ana fonksiyonu
 * 
 * @param arg İzleme yapısı işaretçisi
 * @return void* NULL
 */
void* hw_monitor_thread(void* arg);

#endif /* HARDWARE_MANAGER_INTERNAL_H */ 