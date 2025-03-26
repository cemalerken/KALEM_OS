#include "../include/multiboot.h"
#include "../include/memory.h"
#include "../include/vga.h"
#include "../include/gui.h"
#include "../include/launcher.h"
#include "../include/wifi.h"
#include "../include/browser.h"
#include "../include/noteplus.h"
#include "../include/archive.h"
#include "../include/app_manager.h"
#include <stdint.h>
#include <string.h>

// VGA renk kodları
enum vga_color {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GREY = 7,
    COLOR_DARK_GREY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_LIGHT_BROWN = 14,
    COLOR_WHITE = 15,
};

// Terminal yapılandırması
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

// Terminal değişkenleri
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

// Terminal işlevleri
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(COLOR_WHITE, COLOR_BLACK);
    terminal_buffer = VGA_MEMORY;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_set_color(uint8_t color) {
    terminal_color = color;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
        return;
    }

    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    terminal_buffer[index] = vga_entry(c, terminal_color);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_write_string(const char* data) {
    terminal_write(data, strlen(data));
}

void terminal_write_hex(uint32_t num) {
    char hex_chars[] = "0123456789ABCDEF";
    terminal_write_string("0x");
    
    // En yüksek değerli basamakları önce yaz
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (num >> (i * 4)) & 0xF;
        terminal_putchar(hex_chars[nibble]);
    }
}

void terminal_write_dec(uint32_t num) {
    // 32-bit sayı en fazla 10 basamak olabilir
    char buf[11];
    buf[10] = '\0';
    
    // Sayıyı tersten doldur
    int i = 9;
    do {
        buf[i--] = '0' + (num % 10);
        num /= 10;
    } while (num > 0 && i >= 0);
    
    // Sayıyı yaz
    terminal_write_string(&buf[i + 1]);
}

// Bellek haritasını işle
void process_memory_map(multiboot_info_t* mbi) {
    terminal_write_string("Bellek haritası işleniyor...\n");
    
    if (!(mbi->flags & MULTIBOOT_FLAG_MMAP)) {
        terminal_write_string("Bellek haritası bilgisi bulunamadı!\n");
        return;
    }
    
    multiboot_mmap_entry_t* mmap = (multiboot_mmap_entry_t*) mbi->mmap_addr;
    uint32_t end_addr = mbi->mmap_addr + mbi->mmap_length;
    
    terminal_write_string("Bellek bölgeleri:\n");
    
    uint32_t total_memory = 0;
    
    while ((uint32_t)mmap < end_addr) {
        terminal_write_string("Başlangıç: ");
        terminal_write_hex(mmap->addr_low);
        terminal_write_string(" Uzunluk: ");
        terminal_write_hex(mmap->len_low);
        terminal_write_string(" Tür: ");
        terminal_write_dec(mmap->type);
        terminal_write_string("\n");
        
        if (mmap->type == 1) {  // Kullanılabilir bellek
            total_memory += mmap->len_low;
        }
        
        // Sonraki girişe geç
        mmap = (multiboot_mmap_entry_t*) ((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }
    
    terminal_write_string("Toplam kullanılabilir bellek: ");
    terminal_write_dec(total_memory / 1024 / 1024);
    terminal_write_string(" MB\n");
    
    // Bellek yöneticisini başlat
    memory_init();
    terminal_write_string("Bellek yöneticisi başlatıldı.\n");
}

// Grafik sistemini başlat
void init_graphics() {
    vga_init();
    gui_init();
    launcher_init();
    wifi_init();
    browser_init();
    noteplus_init();
    archive_init();
}

// Kernel giriş noktası
void kernel_main(multiboot_info_t* mbi) {
    // Terminal başlat
    terminal_initialize();
    
    // Hoş geldin mesajı
    terminal_set_color(vga_entry_color(COLOR_LIGHT_GREEN, COLOR_BLACK));
    terminal_write_string("KALEM OS - Türkiye'nin İşletim Sistemi\n");
    terminal_write_string("Sürüm 0.1.0 - Erken Geliştirme Sürümü\n\n");
    terminal_set_color(vga_entry_color(COLOR_WHITE, COLOR_BLACK));
    
    // Multiboot bilgilerini kontrol et
    if (mbi) {
        terminal_write_string("Multiboot bilgileri algılandı.\n");
        
        // Bellek haritasını işle
        process_memory_map(mbi);
    } else {
        terminal_write_string("Hata: Multiboot bilgileri alınamadı!\n");
    }
    
    terminal_write_string("\nGrafik arayüzü başlatılıyor...\n");
    
    // Grafik sistemini başlat
    init_graphics();
    
    // Sonsuz döngü
    terminal_write_string("Kernel hazır, işlemler bekleniyor...\n");
    while (1) {
        // İşlemci beklemesi - gerçek sistemde burada işlem zamanlayıcı olacak
        __asm__ volatile("hlt");
    }
} 