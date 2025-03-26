#ifndef __LAUNCHER_H__
#define __LAUNCHER_H__

#include "gui.h"
#include <stdint.h>

// Uygulama simgesi yapısı
typedef struct {
    char name[32];         // Uygulama adı
    uint32_t x;            // X konumu
    uint32_t y;            // Y konumu
    uint8_t color;         // Simge rengi
    void (*on_click)();    // Tıklama olayı işleyicisi
} app_icon_t;

// Launcher başlat
void launcher_init();

// Masaüstü çiz
void launcher_draw_desktop();

// Simgeleri çiz
void launcher_draw_icons();

// Simge ekle
void launcher_add_icon(const char* name, uint32_t x, uint32_t y, uint8_t color, void (*on_click)());

// Uygulama işlevleri
void app_terminal();
void app_settings();
void app_notepad();
void app_calculator();
void app_about();
void app_wifi();
void app_browser();
void app_noteplus();
void app_archive();
void app_store();

#endif /* __LAUNCHER_H__ */ 