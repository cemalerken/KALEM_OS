#include "../include/font.h"
#include "../include/vga.h"
#include <stddef.h>

// 8x16 VGA standardı font verileri (ilk 128 ASCII karakteri)
static const uint8_t vga_font_data[128 * 16] = {
    /* 0x00 - Null */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x01 - Gülümseyen yüz */ 0x00, 0x00, 0x3C, 0x42, 0x81, 0xA5, 0x81, 0xA5, 0x99, 0x81, 0x81, 0x42, 0x3C, 0x00, 0x00, 0x00,
    /* 0x02 - Gülen yüz */ 0x00, 0x00, 0x3C, 0x42, 0xA5, 0x81, 0x81, 0xBD, 0x99, 0x81, 0x81, 0x42, 0x3C, 0x00, 0x00, 0x00,
    /* Yer tasarrufu için diğer karakterler kısaltıldı. Gerçek uygulamada 128 karakter için tüm bitmap verileri eklenmelidir... */
    
    /* 0x41 - A */ 0x00, 0x00, 0x18, 0x24, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00,
    /* 0x42 - B */ 0x00, 0x00, 0x7C, 0x42, 0x42, 0x42, 0x7C, 0x42, 0x42, 0x42, 0x42, 0x7C, 0x00, 0x00, 0x00, 0x00,
    /* 0x43 - C */ 0x00, 0x00, 0x3C, 0x42, 0x42, 0x40, 0x40, 0x40, 0x40, 0x42, 0x42, 0x3C, 0x00, 0x00, 0x00, 0x00,
    
    /* Kalan karakterler burada olmalı */
};

// Standart sistem fontu
static font_t vga_font = {
    .name = "VGA 8x16",
    .width = 8,
    .height = 16,
    .data = vga_font_data
};

// Global sistem fontu
font_t* system_font = &vga_font;

// Font başlatma
void font_init() {
    // Ek başlatma adımları gerekirse burada yapılır
}

// Font karakter çizme
void font_draw_char(uint32_t x, uint32_t y, char c, uint8_t fg_color, uint8_t bg_color) {
    if (c < 0 || c >= 128) {
        c = '?'; // Desteklenmeyen karakter
    }
    
    const uint8_t* char_data = &system_font->data[c * system_font->height];
    
    for (uint8_t j = 0; j < system_font->height; j++) {
        uint8_t row = char_data[j];
        
        for (uint8_t i = 0; i < system_font->width; i++) {
            if (row & (1 << (7 - i))) {
                vga_draw_pixel(x + i, y + j, fg_color);
            } else if (bg_color != 0xFF) { // 0xFF arkaplan rengi çizme anlamına gelir
                vga_draw_pixel(x + i, y + j, bg_color);
            }
        }
    }
}

// Font dizgi çizme
void font_draw_string(uint32_t x, uint32_t y, const char* str, uint8_t fg_color, uint8_t bg_color) {
    uint32_t offset_x = 0;
    
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            offset_x = 0;
            y += system_font->height;
            continue;
        }
        
        font_draw_char(x + offset_x, y, str[i], fg_color, bg_color);
        offset_x += system_font->width;
    }
}

// Dizgi genişliği hesaplama
uint32_t font_get_string_width(const char* str) {
    uint32_t max_width = 0;
    uint32_t current_width = 0;
    
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            if (current_width > max_width) {
                max_width = current_width;
            }
            current_width = 0;
            continue;
        }
        
        current_width += system_font->width;
    }
    
    if (current_width > max_width) {
        max_width = current_width;
    }
    
    return max_width;
} 