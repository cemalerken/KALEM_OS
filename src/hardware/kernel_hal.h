#ifndef KERNEL_HAL_H
#define KERNEL_HAL_H

/**
 * @file kernel_hal.h
 * @brief KALEM OS Hibrit Kernel Donanım Soyutlama Katmanı (HAL)
 * 
 * Bu dosya, kernel seviyesinde donanım erişimi için soyutlama katmanını tanımlar.
 * Pthread benzeri işlevsellikleri kernel seviyesinde sağlar ve düşük seviye donanım
 * erişimi için gerekli yapıları içerir.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Temel kernel veri tipleri */
typedef int kernel_status_t;
typedef uint32_t device_id_t;
typedef uint64_t physical_addr_t;
typedef void* virtual_addr_t;

/* Kernel mutex yapısı */
typedef struct {
    volatile int lock;
    uint32_t owner;
    uint32_t recursion_count;
} kernel_mutex_t;

/* Kernel thread yapısı */
typedef struct {
    uint32_t id;
    void* stack;
    size_t stack_size;
    int priority;
    volatile int state;
    void* (*entry)(void*);
    void* arg;
    void* private_data;
} kernel_thread_t;

/* Kernel zamanlayıcı yapısı */
typedef struct {
    int64_t tv_sec;  /* saniye */
    int64_t tv_nsec; /* nanosaniye */
} kernel_timespec_t;

/* Kernel mutex sabitleri */
#define KERNEL_MUTEX_INITIALIZER {0, 0, 0}

/* Thread durumları */
#define THREAD_STATE_READY     0
#define THREAD_STATE_RUNNING   1
#define THREAD_STATE_BLOCKED   2
#define THREAD_STATE_SLEEPING  3
#define THREAD_STATE_ZOMBIE    4

/* Kernel Mutex API */
kernel_status_t kernel_mutex_init(kernel_mutex_t* mutex);
kernel_status_t kernel_mutex_lock(kernel_mutex_t* mutex);
kernel_status_t kernel_mutex_unlock(kernel_mutex_t* mutex);
kernel_status_t kernel_mutex_destroy(kernel_mutex_t* mutex);

/* Kernel Thread API */
kernel_status_t kernel_thread_create(kernel_thread_t* thread, void* (*entry)(void*), void* arg, int priority);
kernel_status_t kernel_thread_join(kernel_thread_t* thread, void** retval);
kernel_status_t kernel_thread_sleep(const kernel_timespec_t* duration);
kernel_status_t kernel_thread_yield(void);
uint32_t kernel_thread_get_current_id(void);
kernel_thread_t* kernel_thread_get_current(void);

/* Hafıza Yönetimi API */
void* kernel_alloc(size_t size);
void* kernel_calloc(size_t count, size_t size);
void* kernel_realloc(void* ptr, size_t size);
void kernel_free(void* ptr);

/* Direkt Hafıza Erişim (DMA) API */
kernel_status_t kernel_dma_allocate(size_t size, physical_addr_t* phys_addr, virtual_addr_t* virt_addr);
kernel_status_t kernel_dma_free(virtual_addr_t virt_addr, size_t size);
kernel_status_t kernel_dma_map(physical_addr_t phys_addr, size_t size, virtual_addr_t* virt_addr);
kernel_status_t kernel_dma_unmap(virtual_addr_t virt_addr, size_t size);
kernel_status_t kernel_dma_sync(virtual_addr_t virt_addr, size_t size, int direction);

/* Doğrudan Port Erişimi API */
uint8_t kernel_inb(uint16_t port);
uint16_t kernel_inw(uint16_t port);
uint32_t kernel_inl(uint16_t port);
void kernel_outb(uint16_t port, uint8_t data);
void kernel_outw(uint16_t port, uint16_t data);
void kernel_outl(uint16_t port, uint32_t data);

/* PCI Konfigurasyon Alanı Erişimi */
uint8_t kernel_pci_read_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint16_t kernel_pci_read_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint32_t kernel_pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void kernel_pci_write_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t data);
void kernel_pci_write_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t data);
void kernel_pci_write_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t data);

/* ACPI Erişimi API */
kernel_status_t kernel_acpi_get_table(const char* signature, void** table, size_t* size);
kernel_status_t kernel_acpi_eval_object(const char* path, const char* method, void* args, size_t args_size, void* result, size_t* result_size);

/* Kesme (Interrupt) Yönetimi API */
typedef void (*kernel_irq_handler_t)(uint32_t irq, void* context);
kernel_status_t kernel_register_irq_handler(uint32_t irq, kernel_irq_handler_t handler, void* context);
kernel_status_t kernel_unregister_irq_handler(uint32_t irq);
kernel_status_t kernel_enable_irq(uint32_t irq);
kernel_status_t kernel_disable_irq(uint32_t irq);

/* Zaman Ölçüm API */
uint64_t kernel_get_time_ms(void);
kernel_status_t kernel_get_timespec(kernel_timespec_t* ts);

#endif /* KERNEL_HAL_H */ 