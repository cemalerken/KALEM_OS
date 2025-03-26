#include "../include/vga.h"
#include <stdint.h>

// VGA değişkenleri
uint8_t* vga_framebuffer = (uint8_t*)0xA0000;
uint32_t vga_width = 320;
uint32_t vga_height = 200;
uint8_t vga_bpp = 8;

// Port I/O işlemleri
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// VGA kontrol rejistresi
static void vga_write_registers(uint8_t* registers) {
    // MISC
    outb(VGA_MISC_WRITE, *registers++);
    
    // SEQ
    for (uint8_t i = 0; i < 5; i++) {
        outb(VGA_SEQ_INDEX, i);
        outb(VGA_SEQ_DATA, *registers++);
    }
    
    // CRTC
    outb(VGA_CRTC_INDEX, 0x03);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);
    
    registers[0x03] = registers[0x03] | 0x80;
    registers[0x11] = registers[0x11] & ~0x80;
    
    for (uint8_t i = 0; i < 25; i++) {
        outb(VGA_CRTC_INDEX, i);
        outb(VGA_CRTC_DATA, *registers++);
    }
    
    // GC
    for (uint8_t i = 0; i < 9; i++) {
        outb(VGA_GC_INDEX, i);
        outb(VGA_GC_DATA, *registers++);
    }
    
    // AC
    for (uint8_t i = 0; i < 21; i++) {
        inb(VGA_INSTAT_READ);
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, *registers++);
    }
    
    inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20);
}

// 320x200 256 renk modu (Mode 13h) rejistresi
static uint8_t mode_13h_registers[] = {
    /* MISC */
    0x63,
    /* SEQ */
    0x03, 0x01, 0x0F, 0x00, 0x0E,
    /* CRTC */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
    0xFF,
    /* GC */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
    0xFF,
    /* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00, 0x00
};

// Standart 256 renk VGA paleti
static vga_color_t default_palette[VGA_PALETTE_SIZE] = {
    {0x00, 0x00, 0x00}, {0x00, 0x00, 0x2A}, {0x00, 0x2A, 0x00}, {0x00, 0x2A, 0x2A},
    {0x2A, 0x00, 0x00}, {0x2A, 0x00, 0x2A}, {0x2A, 0x15, 0x00}, {0x2A, 0x2A, 0x2A},
    {0x15, 0x15, 0x15}, {0x15, 0x15, 0x3F}, {0x15, 0x3F, 0x15}, {0x15, 0x3F, 0x3F},
    {0x3F, 0x15, 0x15}, {0x3F, 0x15, 0x3F}, {0x3F, 0x3F, 0x15}, {0x3F, 0x3F, 0x3F},
    // ... Diğer renkler (düzenleme basitliği için kısaltıldı)
};

// VGA başlatma
void vga_init() {
    // Grafik modunu ayarla
    vga_set_mode(VGA_MODE_320_200_256);
    
    // Standart paleti ayarla
    for (int i = 0; i < 16; i++) {
        vga_set_palette(i, default_palette[i].r, default_palette[i].g, default_palette[i].b);
    }
    
    // Ekranı temizle
    vga_clear_screen(0);
}

// Grafik modunu ayarla
void vga_set_mode(uint8_t mode) {
    switch (mode) {
        case VGA_MODE_320_200_256:
            vga_write_registers(mode_13h_registers);
            vga_width = 320;
            vga_height = 200;
            vga_bpp = 8;
            break;
        default:
            // Desteklenmeyen mod, varsayılan olarak Mode 13h kullan
            vga_write_registers(mode_13h_registers);
            vga_width = 320;
            vga_height = 200;
            vga_bpp = 8;
            break;
    }
}

// Piksel çizme
void vga_draw_pixel(uint32_t x, uint32_t y, uint8_t color) {
    if (x < vga_width && y < vga_height) {
        uint32_t offset = y * vga_width + x;
        vga_framebuffer[offset] = color;
    }
}

// Yatay çizgi
void vga_draw_hline(uint32_t x, uint32_t y, uint32_t width, uint8_t color) {
    for (uint32_t i = 0; i < width; i++) {
        vga_draw_pixel(x + i, y, color);
    }
}

// Dikey çizgi
void vga_draw_vline(uint32_t x, uint32_t y, uint32_t height, uint8_t color) {
    for (uint32_t i = 0; i < height; i++) {
        vga_draw_pixel(x, y + i, color);
    }
}

// Dikdörtgen
void vga_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color) {
    vga_draw_hline(x, y, width, color);
    vga_draw_hline(x, y + height - 1, width, color);
    vga_draw_vline(x, y, height, color);
    vga_draw_vline(x + width - 1, y, height, color);
}

// Dolu dikdörtgen
void vga_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color) {
    for (uint32_t j = 0; j < height; j++) {
        vga_draw_hline(x, y + j, width, color);
    }
}

// Renk paleti ayarlama
void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(VGA_DAC_WRITE_INDEX, index);
    outb(VGA_DAC_DATA, r >> 2); // VGA paleti 6-bit'lik değerlere ihtiyaç duyar
    outb(VGA_DAC_DATA, g >> 2);
    outb(VGA_DAC_DATA, b >> 2);
}

// Ekranı temizleme
void vga_clear_screen(uint8_t color) {
    for (uint32_t i = 0; i < vga_width * vga_height; i++) {
        vga_framebuffer[i] = color;
    }
}

// Ekranı güncelleme (bu sürücü doğrudan framebuffer'a yazdığı için gerekli değil)
void vga_update() {
    // Doğrudan belleğe yazıldığı için ek işlem gerekmez
} 