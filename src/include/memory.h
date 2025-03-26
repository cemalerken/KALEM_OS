#ifndef KALEMOS_MEMORY_H
#define KALEMOS_MEMORY_H

#include <stddef.h>
#include <stdint.h>

// Sayfa boyutu 4KB (4096 bayt)
#define PAGE_SIZE 4096

// Bellek bölgesi türleri (multiboot bilgisi)
#define MEMORY_AVAILABLE              1
#define MEMORY_RESERVED               2
#define MEMORY_ACPI_RECLAIMABLE       3
#define MEMORY_NVS                    4
#define MEMORY_BADRAM                 5

// Bellek bölgesi yapısı
typedef struct {
    uint64_t base_addr;    // Başlangıç adresi
    uint64_t length;       // Uzunluk
    uint32_t type;         // Tür
} memory_region_t;

// Fiziksel bellek yönetimi için bitmap yapısı
typedef struct {
    uint32_t* bitmap;      // Bitmap bellek adresi
    uint32_t size;         // Bitmap boyutu (32-bit değer sayısı)
    uint32_t total_pages;  // Toplam sayfa sayısı
    uint32_t free_pages;   // Boş sayfa sayısı
} phys_mem_manager_t;

// Bellek yöneticisini başlat
void init_memory(memory_region_t* regions, size_t count);

// Fiziksel sayfa tahsis et
void* alloc_phys_page(void);

// Fiziksel sayfa serbest bırak
void free_phys_page(void* page);

// Sanal bellek eşleştirme
void map_page(uint32_t* page_directory, void* phys, void* virt, uint32_t flags);

// Sanal bellek eşleştirme kaldır
void unmap_page(uint32_t* page_directory, void* virt);

// Çalışan sürecin sayfa dizini için bellek tahsis et
void* alloc_kheap(size_t size);

// Çalışan sürecin sayfa dizini için bellek serbest bırak
void free_kheap(void* ptr);

#endif // KALEMOS_MEMORY_H 