#ifndef HARDWARE_MANAGER_KERNEL_H
#define HARDWARE_MANAGER_KERNEL_H

#include "hardware_manager.h"

/**
 * @file hardware_manager_kernel.h
 * @brief KALEM OS Hibrit Kernel Donanım Yönetim Merkezi API
 * 
 * Bu dosya, kernel seviyesinde donanım yönetimi için gelişmiş API'leri tanımlar.
 * Sadece kernel seviyesinde kullanılan fonksiyonları içerir.
 */

/* API Sabitleri */
#define HW_MANAGER_API_VERSION 1

/* Kesme numaraları */
#define PCI_HOTPLUG_IRQ   40
#define USB_HOTPLUG_IRQ   41
#define ACPI_HOTPLUG_IRQ  42

/**
 * Modül bilgileri yapısı
 */
typedef struct {
    char name[64];            /* Modül adı */
    char version[32];         /* Modül sürümü */
    char author[64];          /* Yazar bilgisi */
    char description[256];    /* Açıklama */
    uint32_t api_version;     /* Kullanılan API sürümü */
    uint32_t flags;           /* Modül bayrakları */
} hw_module_info_t;

/**
 * Modül işlemleri yapısı
 */
typedef struct {
    int (*init)(const hw_module_info_t* info, void** context);
    int (*cleanup)(void* context);
    int (*process_event)(void* context, uint32_t event_type, void* event_data);
    int (*command)(void* context, const char* cmd, void* params, void* result);
} hw_module_ops_t;

/**
 * AI veri toplayıcı yapısı
 */
typedef struct {
    union {
        struct {
            uint32_t frequency;    /* CPU frekansı (MHz) */
            uint8_t core_count;    /* Çekirdek sayısı */
            uint8_t thread_count;  /* Thread sayısı */
        } cpu;
        
        struct {
            uint8_t usage_percent; /* Bellek kullanım yüzdesi */
            uint64_t total_size;   /* Toplam boyut (byte) */
        } memory;
        
        struct {
            uint32_t read_speed;   /* Okuma hızı (MB/s) */
            uint32_t write_speed;  /* Yazma hızı (MB/s) */
            uint64_t used_space;   /* Kullanılan alan (byte) */
        } storage;
        
        struct {
            uint32_t tx_rate;      /* İletim hızı (KB/s) */
            uint32_t rx_rate;      /* Alım hızı (KB/s) */
            uint8_t signal_strength; /* Sinyal gücü (%) */
        } network;
    } specific;
    
    uint32_t component_id;         /* Bileşen kimliği */
    hw_component_type_t type;      /* Bileşen türü */
    hw_status_t status;            /* Durum */
    uint8_t temperature;           /* Sıcaklık (°C) */
    uint8_t utilization;           /* Kullanım oranı (%) */
    uint32_t power_usage;          /* Güç tüketimi (mW) */
    uint32_t error_count;          /* Hata sayısı */
} hw_ai_component_data_t;

/**
 * AI modülü için toplanan veriler
 */
typedef struct {
    uint64_t timestamp;                 /* Zaman damgası */
    uint32_t component_count;           /* Bileşen sayısı */
    hw_ai_component_data_t components[64]; /* Bileşen verileri */
} hw_ai_data_t;

/**
 * @brief Kesme olay işleyicisi
 * 
 * Bu fonksiyon, donanım hotplug olaylarını yakalar ve donanım
 * bileşenlerinin otomatik olarak algılanmasını sağlar.
 * 
 * @param irq Kesme numarası
 * @param context Kesme bağlam bilgisi
 */
void hw_manager_irq_handler(uint32_t irq, void* context);

/**
 * @brief Sistem olayı raporla
 * 
 * @param component_id Bileşen kimliği (0: tüm sistem)
 * @param event_type Olay türü
 * @param message Olay mesajı
 * @param severity Önem derecesi (0-4)
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_report_system_event(uint32_t component_id, uint32_t event_type, 
                                  const char* message, uint32_t severity);

/**
 * @brief AI modülü için donanım performans verilerini topla
 * 
 * @param data Veri yapısı
 * @param components Toplanacak maksimum bileşen sayısı
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_collect_ai_data(hw_ai_data_t* data, uint32_t components);

/**
 * @brief Donanım yönetim merkezine dinamik modül kaydet
 * 
 * @param module_info Modül bilgileri
 * @param ops Modül işlem fonksiyonları
 * @param module_context Modül bağlam işaretçisi (çıkış)
 * @return int 0: başarılı, <0: hata
 */
int hw_manager_register_module(const hw_module_info_t* module_info, 
                              hw_module_ops_t* ops, void** module_context);

#endif /* HARDWARE_MANAGER_KERNEL_H */ 