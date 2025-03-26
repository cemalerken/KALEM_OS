#ifndef KALEMOS_VGA_H
#define KALEMOS_VGA_H

#include <stdint.h>

// VGA grafik modları
#define VGA_MODE_320_200_256   0x13    // 320x200, 256 renk

// VGA port değerleri
#define VGA_AC_INDEX           0x3C0
#define VGA_AC_WRITE           0x3C0
#define VGA_AC_READ            0x3C1
#define VGA_MISC_WRITE         0x3C2
#define VGA_SEQ_INDEX          0x3C4
#define VGA_SEQ_DATA           0x3C5
#define VGA_DAC_READ_INDEX     0x3C7
#define VGA_DAC_WRITE_INDEX    0x3C8
#define VGA_DAC_DATA           0x3C9
#define VGA_MISC_READ          0x3CC
#define VGA_GC_INDEX           0x3CE
#define VGA_GC_DATA            0x3CF
#define VGA_CRTC_INDEX         0x3D4
#define VGA_CRTC_DATA          0x3D5
#define VGA_INSTAT_READ        0x3DA

// VGA renk paleti
#define VGA_PALETTE_SIZE       256

// Renk yapısı
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} vga_color_t;

// VGA sürücüsü başlatma
void vga_init();

// Grafik moduna geçiş
void vga_set_mode(uint8_t mode);

// Piksel çizme
void vga_draw_pixel(uint32_t x, uint32_t y, uint8_t color);

// Yatay çizgi
void vga_draw_hline(uint32_t x, uint32_t y, uint32_t width, uint8_t color);

// Dikey çizgi
void vga_draw_vline(uint32_t x, uint32_t y, uint32_t height, uint8_t color);

// Dikdörtgen
void vga_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color);

// Dolu dikdörtgen
void vga_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t color);

// Renk paleti ayarlama
void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

// Ekranı temizleme
void vga_clear_screen(uint8_t color);

// Ekranı güncelleme
void vga_update();

// Framebuffer başlangıç adresi
extern uint8_t* vga_framebuffer;

// Ekran özellikleri
extern uint32_t vga_width;
extern uint32_t vga_height;
extern uint8_t vga_bpp; // Bits per pixel

#endif // KALEMOS_VGA_H 