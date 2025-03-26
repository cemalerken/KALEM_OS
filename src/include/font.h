#ifndef KALEMOS_FONT_H
#define KALEMOS_FONT_H

#include <stdint.h>

// Standart font boyutları
#define FONT_WIDTH  8
#define FONT_HEIGHT 16

// Font yapısı
typedef struct {
    const char* name;
    uint8_t width;
    uint8_t height;
    const uint8_t* data;
} font_t;

// Font işleme fonksiyonları
void font_init();
void font_draw_char(uint32_t x, uint32_t y, char c, uint8_t fg_color, uint8_t bg_color);
void font_draw_string(uint32_t x, uint32_t y, const char* str, uint8_t fg_color, uint8_t bg_color);
uint32_t font_get_string_width(const char* str);

// Standart font referansı
extern font_t* system_font;

#endif // KALEMOS_FONT_H 