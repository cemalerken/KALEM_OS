#ifndef KALEMOS_WIFI_H
#define KALEMOS_WIFI_H

#include <stdint.h>

// WiFi ağı yapısı
typedef struct {
    char ssid[32];         // Ağ adı
    uint8_t signal_strength; // Sinyal gücü (0-100)
    uint8_t security_type;   // Güvenlik türü
    uint8_t connected;       // Bağlantı durumu
} wifi_network_t;

// Güvenlik türleri
#define WIFI_SECURITY_NONE    0
#define WIFI_SECURITY_WEP     1
#define WIFI_SECURITY_WPA     2
#define WIFI_SECURITY_WPA2    3

// Durum kodları
#define WIFI_STATUS_OK        0
#define WIFI_STATUS_ERROR     1
#define WIFI_STATUS_TIMEOUT   2
#define WIFI_STATUS_NODEVICE  3

// Fonksiyonlar
void wifi_init();
int wifi_scan_networks(wifi_network_t* networks, int max_count);
int wifi_connect(const char* ssid, const char* password);
int wifi_disconnect();
int wifi_get_status();
const char* wifi_get_current_ssid();
uint8_t wifi_get_signal_strength();

// WiFi arayüzünü gösteren pencere
void wifi_show_manager();

#endif // KALEMOS_WIFI_H 