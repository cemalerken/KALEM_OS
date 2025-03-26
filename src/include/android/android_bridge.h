#ifndef ANDROID_BRIDGE_H
#define ANDROID_BRIDGE_H

#include <stdint.h>
#include "../gui.h"
#include "android_container.h"

// Android pencere yöneticisi ile KALEM OS GUI arasındaki bağlantı

// Android ekran modları
typedef enum {
    ANDROID_DISPLAY_MODE_WINDOWED,         // Pencere modu
    ANDROID_DISPLAY_MODE_FULLSCREEN,       // Tam ekran modu
    ANDROID_DISPLAY_MODE_SPLIT_SCREEN,     // Bölünmüş ekran modu
    ANDROID_DISPLAY_MODE_PICTURE_IN_PICTURE // Resim içinde resim modu
} android_display_mode_t;

// Android yüzeyi
typedef struct {
    uint32_t id;                   // Yüzey kimliği
    uint32_t width;                // Genişlik
    uint32_t height;               // Yükseklik
    uint32_t* buffer;              // Piksel tamponu
    uint8_t has_alpha;             // Alfa kanalı var mı?
    uint8_t dirty;                 // Değişiklik var mı?
    void* native_handle;           // Android tarafındaki yerel tanıtıcı
} android_surface_t;

// Android-KALEM OS köprü yapısı
typedef struct {
    android_container_t* container;     // Bağlı konteyner
    gui_window_t* window;              // KALEM OS penceresi
    android_surface_t* surface;        // Android yüzeyi
    android_display_mode_t display_mode; // Ekran modu
    
    // Giriş olayı tamponları
    void* input_events;                // Giriş olayları
    uint32_t input_event_count;        // Olay sayısı
    
    // Geri çağırma işlevleri
    void (*on_surface_created)(android_surface_t* surface);
    void (*on_surface_changed)(android_surface_t* surface);
    void (*on_surface_destroyed)(android_surface_t* surface);
    
    // Senkronizasyon
    uint8_t is_rendering;              // Yüzey şu anda yenileniyor mu?
    uint64_t last_frame_time;          // Son kare zamanı
    uint32_t fps;                      // Saniyedeki kare sayısı
} android_bridge_t;

// Köprü yönetim API
int bridge_initialize();
android_bridge_t* bridge_create(android_container_t* container);
int bridge_connect_to_window(android_bridge_t* bridge, gui_window_t* window);
int bridge_destroy(android_bridge_t* bridge);

// Yüzey yönetimi
android_surface_t* bridge_create_surface(android_bridge_t* bridge, uint32_t width, uint32_t height);
int bridge_resize_surface(android_surface_t* surface, uint32_t width, uint32_t height);
int bridge_destroy_surface(android_surface_t* surface);

// Yüzey erişimi ve senkronizasyonu
int bridge_lock_surface(android_surface_t* surface, void** buffer);
int bridge_unlock_surface(android_surface_t* surface);
int bridge_invalidate_surface(android_surface_t* surface);

// Pencere ve görüntüleme yönetimi
int bridge_set_display_mode(android_bridge_t* bridge, android_display_mode_t mode);
int bridge_get_display_mode(android_bridge_t* bridge, android_display_mode_t* mode);
int bridge_request_fullscreen(android_bridge_t* bridge);
int bridge_exit_fullscreen(android_bridge_t* bridge);

// Giriş olayı yönetimi
int bridge_inject_key_event(android_bridge_t* bridge, uint8_t key_code, uint8_t is_press);
int bridge_inject_mouse_event(android_bridge_t* bridge, uint32_t x, uint32_t y, uint8_t button, uint8_t is_press);
int bridge_inject_touch_event(android_bridge_t* bridge, uint32_t x, uint32_t y, uint8_t is_down);

// Binder IPC entegrasyonu
int bridge_init_binder();
int bridge_register_service(const char* name, void* service);
void* bridge_get_service(const char* name);
int bridge_send_transaction(void* service, uint32_t code, void* data, uint32_t size);

// Ses yönlendirme
int bridge_init_audio();
int bridge_route_android_audio(android_bridge_t* bridge, uint8_t enable);

#endif /* ANDROID_BRIDGE_H */ 