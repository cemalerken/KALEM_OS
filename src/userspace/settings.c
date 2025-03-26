#include "../include/settings.h"
#include "../include/gui.h"
#include "../include/font.h"
#include "../include/vga.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Renk tanımlamaları
#define COLOR_BLACK          GUI_COLOR_BLACK
#define COLOR_BLUE           GUI_COLOR_BLUE
#define COLOR_GREEN          GUI_COLOR_GREEN
#define COLOR_CYAN           GUI_COLOR_CYAN
#define COLOR_RED            GUI_COLOR_RED
#define COLOR_MAGENTA        GUI_COLOR_MAGENTA
#define COLOR_BROWN          GUI_COLOR_BROWN
#define COLOR_LIGHT_GRAY     GUI_COLOR_LIGHT_GRAY
#define COLOR_DARK_GRAY      GUI_COLOR_DARK_GRAY
#define COLOR_LIGHT_BLUE     GUI_COLOR_LIGHT_BLUE
#define COLOR_LIGHT_GREEN    GUI_COLOR_LIGHT_GREEN
#define COLOR_LIGHT_CYAN     GUI_COLOR_LIGHT_CYAN
#define COLOR_LIGHT_RED      GUI_COLOR_LIGHT_RED
#define COLOR_LIGHT_MAGENTA  GUI_COLOR_LIGHT_MAGENTA
#define COLOR_YELLOW         GUI_COLOR_YELLOW
#define COLOR_WHITE          GUI_COLOR_WHITE
#define COLOR_DARK_GREEN     GUI_COLOR_GREEN
#define COLOR_DARK_RED       GUI_COLOR_RED
#define COLOR_ORANGE         GUI_COLOR_BROWN
#define COLOR_PURPLE         GUI_COLOR_MAGENTA

// Aktif ayarlar penceresi
static settings_window_t* active_settings = NULL;

// Kategori isimleri - USB Aygıtları kategorisi eklenmiş
static const char* settings_category_names[] = {
    "Sistem",
    "Ekran",
    "Ses",
    "Ağ",
    "Donanım",
    "Tarih/Saat",
    "Dil",
    "Depolama",
    "USB Aygıtları",
    "İşlemci",   // Yeni kategori
    "Batarya",    // Yeni kategori
    "Uygulamalar" // Yeni kategori
};

// Kategoriler - USB_DEVICES kategorisi eklenmiş
typedef enum {
    SETTINGS_CAT_SYSTEM,
    SETTINGS_CAT_DISPLAY,
    SETTINGS_CAT_SOUND,
    SETTINGS_CAT_NETWORK,
    SETTINGS_CAT_HARDWARE,
    SETTINGS_CAT_DATETIME,
    SETTINGS_CAT_LANGUAGE,
    SETTINGS_CAT_STORAGE,
    SETTINGS_CAT_USB_DEVICES,
    SETTINGS_CAT_CPU,     // Yeni kategori
    SETTINGS_CAT_BATTERY, // Yeni kategori
    SETTINGS_CAT_APPLICATIONS, // Yeni kategori
    SETTINGS_CAT_COUNT
} settings_category_t;

// Ekran sürücü yapısı
typedef struct {
    char name[64];           // Sürücü adı
    char vendor[32];         // Üretici
    char device_id[16];      // Cihaz kimliği
    char version[16];        // Sürücü sürümü
    uint8_t autodetect;      // Otomatik tanıma desteği
    uint8_t acceleration;    // Donanım hızlandırma desteği (0-10)
    uint8_t compatibility;   // Uyumluluk seviyesi (0-10)
    uint8_t installed;       // Kurulu mu?
} display_driver_t;

// Ses sürücü yapısı
typedef struct {
    char name[64];           // Sürücü adı
    char vendor[32];         // Üretici
    char device_id[16];      // Cihaz kimliği
    char version[16];        // Sürücü sürümü
    uint8_t autodetect;      // Otomatik tanıma desteği
    uint8_t channels;        // Kanal sayısı
    uint8_t hd_audio;        // HD Audio desteği (0/1)
    uint8_t installed;       // Kurulu mu?
} sound_driver_t;

// Genişletilmiş ağ adaptörü yapısı (geriye uyumluluk sağlayan takma isimler)
typedef struct {
    char name[64];               // Cihaz adı
    char mac[18];                // MAC adresi
    char mac_address[18];        // MAC adresi (takma isim)
    uint8_t type;                // 0: Ethernet, 1: WiFi
    uint8_t connected;           // Bağlı mı?
    uint8_t is_wireless;         // Kablosuz mu? (takma isim)
    uint8_t is_active;           // Aktif mi? (takma isim)
} extended_network_adapter_t;

// Genişletilmiş çözünürlük yapısı (geriye uyumluluk için)
typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t bpp;                 // Piksel başına bit
    char name[32];               // Çözünürlük adı (örn. "1024x768")
    char description[32];        // Çözünürlük açıklaması (takma isim)
} extended_display_resolution_t;

// Genişletilmiş sürücü yönetici yapısı (ek alanlar ve takma isimler)
typedef struct {
    // Mevcut ayarlar yapısı içeriği
    display_driver_t* current_display_driver;  // Mevcut ekran sürücüsü
    display_driver_t* display_driver;          // Ekran sürücüsü (takma isim)
    sound_driver_t* current_sound_driver;      // Mevcut ses sürücüsü
    sound_driver_t* sound_driver;              // Ses sürücüsü (takma isim)
    uint8_t driver_scan_progress;              // Sürücü tarama ilerleme durumu
    uint8_t scan_progress;                     // Tarama ilerleme durumu (takma isim)
    uint8_t driver_install_progress;           // Sürücü kurulum ilerleme durumu
    uint8_t install_progress;                  // Kurulum ilerleme durumu (takma isim)
    uint8_t is_installing_driver;              // Sürücü kuruluyor mu?
    uint8_t is_installing;                     // Kuruluyor mu? (takma isim)
    uint8_t is_scanning;                       // Taranıyor mu?
    char current_operation[128];               // Mevcut işlem
} extended_driver_manager_t;

// Sürücü yöneticisi
static extended_driver_manager_t driver_manager = {0};

// Demo ekran çözünürlükleri - genişletilmiş yapıyı kullanacak şekilde güncellendi
static extended_display_resolution_t demo_resolutions[] = {
    {640, 480, 16, "640x480 (16-bit)", "640x480 (16-bit)"},
    {800, 600, 16, "800x600 (16-bit)", "800x600 (16-bit)"},
    {1024, 768, 16, "1024x768 (16-bit)", "1024x768 (16-bit)"},
    {1280, 720, 16, "1280x720 (16-bit)", "1280x720 (16-bit)"},
    {1366, 768, 16, "1366x768 (16-bit)", "1366x768 (16-bit)"},
    {1600, 900, 16, "1600x900 (16-bit)", "1600x900 (16-bit)"},
    {1920, 1080, 16, "1920x1080 (16-bit)", "1920x1080 (16-bit)"}
};

// Demo ağ adaptörleri - genişletilmiş yapıyı kullanacak şekilde güncellendi
static extended_network_adapter_t demo_adapters[] = {
    {"Realtek RTL8111/8168 Ethernet", "00:1A:2B:3C:4D:5E", "00:1A:2B:3C:4D:5E", 0, 1, 0, 1},
    {"Intel Wireless-AC 8265", "A1:B2:C3:D4:E5:F6", "A1:B2:C3:D4:E5:F6", 1, 1, 1, 1}
};

// Disk türleri
typedef enum {
    DISK_TYPE_HDD,       // Sabit disk
    DISK_TYPE_SSD,       // SSD
    DISK_TYPE_NVME,      // NVMe
    DISK_TYPE_USB,       // USB depolama
    DISK_TYPE_CDROM,     // CD/DVD
    DISK_TYPE_UNKNOWN    // Bilinmeyen
} disk_type_t;

// Dosya sistemi türleri
typedef enum {
    FILE_SYSTEM_FAT16,
    FILE_SYSTEM_FAT32,
    FILE_SYSTEM_NTFS,
    FILE_SYSTEM_EXT2,
    FILE_SYSTEM_EXT3,
    FILE_SYSTEM_EXT4,
    FILE_SYSTEM_BTRFS,
    FILE_SYSTEM_KALEMOSFS, // KALEM OS'un kendi dosya sistemi
    FILE_SYSTEM_NONE,   // Formatsız
    FILE_SYSTEM_UNKNOWN, // Bilinmeyen
    FILE_SYSTEM_KALEMOS // Yeni dosya sistemi
} file_system_t;

// Disk bölümü yapısı
typedef struct {
    char name[32];                // Bölüm adı (örn: sda1)
    uint64_t start_sector;        // Başlangıç sektörü
    uint64_t size_sectors;        // Bölüm boyutu (sektör cinsinden)
    uint64_t size_bytes;          // Bölüm boyutu (bayt cinsinden)
    file_system_t file_system;    // Dosya sistemi türü
    char label[64];               // Bölüm etiketi
    char mount_point[128];        // Bağlama noktası
    uint8_t is_bootable;          // Önyüklenebilir mi?
    uint8_t is_mounted;           // Bağlı mı?
} disk_partition_t;

// Disk yapısı
typedef struct {
    char name[32];                // Disk adı (örn: sda)
    char model[64];               // Disk modeli
    char serial[32];              // Seri numarası
    disk_type_t type;             // Disk türü
    uint64_t size_bytes;          // Toplam boyut (bayt)
    uint64_t size_sectors;        // Toplam boyut (sektör)
    uint32_t sector_size;         // Sektör boyutu
    uint32_t partition_count;     // Bölüm sayısı
    disk_partition_t* partitions; // Bölümler
    uint8_t is_removable;         // Çıkarılabilir mi?
} disk_t;

// Dosya sistemleri hakkında bilgiler
typedef struct {
    file_system_t type;           // Dosya sistemi türü
    char name[32];                // İsim
    char description[128];        // Açıklama
    uint64_t max_volume_size;     // Maksimum birim boyutu
    uint64_t max_file_size;       // Maksimum dosya boyutu
    uint8_t supports_permissions; // İzinleri destekliyor mu?
    uint8_t supports_journaling;  // Günlüklemeyi destekliyor mu?
    uint8_t supports_compression; // Sıkıştırmayı destekliyor mu?
    uint8_t supports_encryption;  // Şifrelemeyi destekliyor mu?
} file_system_info_t;

// Demo diskler
static disk_t demo_disks[] = {
    {
        .name = "sda",
        .model = "KALEM OS Virtual HDD",
        .serial = "KALEMOS00001",
        .type = DISK_TYPE_HDD,
        .size_bytes = 128ULL * 1024 * 1024 * 1024, // 128 GB
        .size_sectors = 250069680,
        .sector_size = 512,
        .partition_count = 2,
        .is_removable = 0
    },
    {
        .name = "sdb",
        .model = "KALEM OS Virtual SSD",
        .serial = "KALEMOS00002",
        .type = DISK_TYPE_SSD,
        .size_bytes = 256ULL * 1024 * 1024 * 1024, // 256 GB
        .size_sectors = 500139008,
        .sector_size = 512,
        .partition_count = 3,
        .is_removable = 0
    },
    {
        .name = "sdc",
        .model = "KALEM OS USB Flash",
        .serial = "KALEMOS00003",
        .type = DISK_TYPE_USB,
        .size_bytes = 16ULL * 1024 * 1024 * 1024, // 16 GB
        .size_sectors = 31258624,
        .sector_size = 512,
        .partition_count = 1,
        .is_removable = 1
    }
};

// Demo bölümler
static disk_partition_t demo_partitions_sda[] = {
    {
        .name = "sda1",
        .start_sector = 2048,
        .size_sectors = 2097152, // 1 GB
        .size_bytes = 1ULL * 1024 * 1024 * 1024,
        .file_system = FILE_SYSTEM_KALEMOS,
        .label = "KALEM-BOOT",
        .mount_point = "/boot",
        .is_bootable = 1,
        .is_mounted = 1
    },
    {
        .name = "sda2",
        .start_sector = 2099200,
        .size_sectors = 247970480, // 127 GB
        .size_bytes = 127ULL * 1024 * 1024 * 1024,
        .file_system = FILE_SYSTEM_KALEMOS,
        .label = "KALEM-ROOT",
        .mount_point = "/",
        .is_bootable = 0,
        .is_mounted = 1
    }
};

// Demo bölümler SSD
static disk_partition_t demo_partitions_sdb[] = {
    {
        .name = "sdb1",
        .start_sector = 2048,
        .size_sectors = 2097152, // 1 GB
        .size_bytes = 1ULL * 1024 * 1024 * 1024,
        .file_system = FILE_SYSTEM_EXT4,
        .label = "EXT4-BOOT",
        .mount_point = "/mnt/ext4boot",
        .is_bootable = 0,
        .is_mounted = 0
    },
    {
        .name = "sdb2",
        .start_sector = 2099200,
        .size_sectors = 419430400, // 200 GB
        .size_bytes = 200ULL * 1024 * 1024 * 1024,
        .file_system = FILE_SYSTEM_NTFS,
        .label = "NTFS-DATA",
        .mount_point = "/mnt/ntfs",
        .is_bootable = 0,
        .is_mounted = 1
    },
    {
        .name = "sdb3",
        .start_sector = 421529600,
        .size_sectors = 78609408, // 55 GB
        .size_bytes = 55ULL * 1024 * 1024 * 1024,
        .file_system = FILE_SYSTEM_FAT32,
        .label = "FAT32-SHARE",
        .mount_point = "/mnt/share",
        .is_bootable = 0,
        .is_mounted = 1
    }
};

// Demo bölümler USB
static disk_partition_t demo_partitions_sdc[] = {
    {
        .name = "sdc1",
        .start_sector = 2048,
        .size_sectors = 31256576, // 16 GB
        .size_bytes = 16ULL * 1024 * 1024 * 1024,
        .file_system = FILE_SYSTEM_FAT32,
        .label = "USB-FLASH",
        .mount_point = "/mnt/usb",
        .is_bootable = 0,
        .is_mounted = 1
    }
};

// Dosya sistemi bilgileri
static file_system_info_t file_system_info[] = {
    {
        .type = FILE_SYSTEM_FAT16,
        .name = "FAT16",
        .description = "File Allocation Table 16-bit",
        .max_volume_size = 4ULL * 1024 * 1024 * 1024, // 4 GB
        .max_file_size = 2ULL * 1024 * 1024 * 1024,  // 2 GB
        .supports_permissions = 0,
        .supports_journaling = 0,
        .supports_compression = 0,
        .supports_encryption = 0
    },
    {
        .type = FILE_SYSTEM_FAT32,
        .name = "FAT32",
        .description = "File Allocation Table 32-bit",
        .max_volume_size = 32ULL * 1024 * 1024 * 1024, // 32 GB
        .max_file_size = 4ULL * 1024 * 1024 * 1024,   // 4 GB
        .supports_permissions = 0,
        .supports_journaling = 0,
        .supports_compression = 0,
        .supports_encryption = 0
    },
    {
        .type = FILE_SYSTEM_NTFS,
        .name = "NTFS",
        .description = "New Technology File System",
        .max_volume_size = 256ULL * 1024 * 1024 * 1024 * 1024, // 256 TB
        .max_file_size = 16ULL * 1024 * 1024 * 1024 * 1024,   // 16 TB
        .supports_permissions = 1,
        .supports_journaling = 1,
        .supports_compression = 1,
        .supports_encryption = 1
    },
    {
        .type = FILE_SYSTEM_EXT2,
        .name = "ext2",
        .description = "Extended Filesystem 2",
        .max_volume_size = 32ULL * 1024 * 1024 * 1024 * 1024, // 32 TB
        .max_file_size = 2ULL * 1024 * 1024 * 1024 * 1024,   // 2 TB
        .supports_permissions = 1,
        .supports_journaling = 0,
        .supports_compression = 0,
        .supports_encryption = 0
    },
    {
        .type = FILE_SYSTEM_EXT3,
        .name = "ext3",
        .description = "Extended Filesystem 3",
        .max_volume_size = 32ULL * 1024 * 1024 * 1024 * 1024, // 32 TB
        .max_file_size = 2ULL * 1024 * 1024 * 1024 * 1024,   // 2 TB
        .supports_permissions = 1,
        .supports_journaling = 1,
        .supports_compression = 0,
        .supports_encryption = 0
    },
    {
        .type = FILE_SYSTEM_EXT4,
        .name = "ext4",
        .description = "Extended Filesystem 4",
        .max_volume_size = 1024ULL * 1024 * 1024 * 1024 * 1024, // 1 EB
        .max_file_size = 16ULL * 1024 * 1024 * 1024 * 1024,    // 16 TB
        .supports_permissions = 1,
        .supports_journaling = 1,
        .supports_compression = 1,
        .supports_encryption = 1
    },
    {
        .type = FILE_SYSTEM_BTRFS,
        .name = "btrfs",
        .description = "B-Tree Filesystem",
        .max_volume_size = 16ULL * 1024 * 1024 * 1024 * 1024 * 1024, // 16 EB
        .max_file_size = 8ULL * 1024 * 1024 * 1024 * 1024 * 1024,   // 8 EB
        .supports_permissions = 1,
        .supports_journaling = 1,
        .supports_compression = 1,
        .supports_encryption = 1
    },
    {
        .type = FILE_SYSTEM_KALEMOSFS,
        .name = "KALEMOSFS",
        .description = "KALEM OS Filesystem",
        .max_volume_size = 64ULL * 1024 * 1024 * 1024 * 1024, // 64 TB
        .max_file_size = 4ULL * 1024 * 1024 * 1024 * 1024,   // 4 TB
        .supports_permissions = 1,
        .supports_journaling = 1,
        .supports_compression = 1,
        .supports_encryption = 1
    },
    {
        .type = FILE_SYSTEM_NONE,
        .name = "Formatsız",
        .description = "Formatız",
        .max_volume_size = 0,
        .max_file_size = 0,
        .supports_permissions = 0,
        .supports_journaling = 0,
        .supports_compression = 0,
        .supports_encryption = 0
    },
    {
        .type = FILE_SYSTEM_UNKNOWN,
        .name = "Bilinmeyen",
        .description = "Bilinmeyen",
        .max_volume_size = 0,
        .max_file_size = 0,
        .supports_permissions = 0,
        .supports_journaling = 0,
        .supports_compression = 0,
        .supports_encryption = 0
    },
    {
        .type = FILE_SYSTEM_KALEMOS,
        .name = "KALEMOSFS",
        .description = "KALEM OS Filesystem",
        .max_volume_size = 64ULL * 1024 * 1024 * 1024 * 1024, // 64 TB
        .max_file_size = 4ULL * 1024 * 1024 * 1024 * 1024,   // 4 TB
        .supports_permissions = 1,
        .supports_journaling = 1,
        .supports_compression = 1,
        .supports_encryption = 1
    }
};

// Disk Yönetimi için statik değişkenler
static struct {
    disk_t* disks;
    uint32_t disk_count;
    uint32_t selected_disk;
    uint32_t selected_partition;
    file_system_t selected_filesystem;
    uint8_t is_formatting;
    uint8_t format_progress;
} storage_manager = {0};

// Popüler ekran kartı sürücü planları
static display_driver_t display_drivers[] = {
    {"KALEM OS Intel HD Graphics Sürücüsü", "Intel", "8086:0102", "1.2.3", 1, 8, 9, 1},
    {"KALEM OS NVIDIA GeForce Sürücüsü", "NVIDIA", "10DE:1180", "2.1.0", 1, 9, 8, 0},
    {"KALEM OS AMD Radeon Sürücüsü", "AMD", "1002:6798", "1.9.2", 1, 9, 8, 0},
    {"KALEM OS VirtualBox Grafik Sürücüsü", "Oracle", "80EE:BEEF", "1.0.5", 1, 6, 10, 0},
    {"KALEM OS VESA Temel Görüntü Sürücüsü", "VESA", "0000:0000", "1.0.0", 1, 3, 10, 1},
    {"KALEM OS VMware SVGA Sürücüsü", "VMware", "15AD:0405", "1.1.0", 1, 7, 9, 0},
    {"KALEM OS Temel Framebuffer Sürücüsü", "Generic", "0000:0001", "1.0.0", 1, 2, 10, 1},
};

// Popüler ses kartı sürücü planları
static sound_driver_t sound_drivers[] = {
    {"KALEM OS Realtek HD Audio Sürücüsü", "Realtek", "10EC:0880", "1.3.4", 1, 8, 1, 1},
    {"KALEM OS Intel HDA Sürücüsü", "Intel", "8086:2668", "1.2.1", 1, 6, 1, 0},
    {"KALEM OS Creative SoundBlaster Sürücüsü", "Creative", "1102:0007", "2.0.0", 1, 8, 1, 0},
    {"KALEM OS VirtualBox Audio Sürücüsü", "Oracle", "80EE:BEEF", "1.0.3", 1, 2, 0, 0},
    {"KALEM OS VMware HD Audio Sürücüsü", "VMware", "15AD:1977", "1.1.0", 1, 2, 1, 0},
    {"KALEM OS C-Media Audio Sürücüsü", "C-Media", "13F6:0111", "1.5.2", 1, 6, 1, 0},
    {"KALEM OS Temel PC Beeper Sürücüsü", "Generic", "0000:0001", "1.0.0", 1, 1, 0, 1},
};

// Statik değişkenler için genişletme
static struct {
    // Mevcut ayarlar yapısı içeriği
    display_driver_t* current_display_driver;
    sound_driver_t* current_sound_driver;
    uint8_t driver_scan_progress;
    uint8_t driver_install_progress;
    uint8_t is_installing_driver;
} driver_manager = {0};

// Ayarlar uygulamasını başlat - Depolama desteği için genişletilmiş
void settings_init() {
    // Ayarlar yapısını oluştur
    if (active_settings == NULL) {
        active_settings = (settings_window_t*)malloc(sizeof(settings_window_t));
        if (!active_settings) return;
        memset(active_settings, 0, sizeof(settings_window_t));
        
        // Varsayılan kategori
        active_settings->current_category = SETTINGS_CAT_SYSTEM;
        
        // Demo ekran kartı bilgilerini ayarla
        strcpy(active_settings->display.name, "Intel HD Graphics 630");
        active_settings->display.vram_size = 128 * 1024 * 1024; // 128 MB
        active_settings->display.resolution_count = sizeof(demo_resolutions) / sizeof(extended_display_resolution_t);
        active_settings->display.resolutions = (display_resolution_t*)demo_resolutions;
        active_settings->display.current_resolution = 2; // 1024x768
        
        // Demo ses kartı bilgilerini ayarla
        strcpy(active_settings->sound.name, "Realtek ALC1220");
        active_settings->sound.channels = 2;
        active_settings->sound.volume = 80;
        active_settings->sound.muted = 0;
        
        // Ağ adaptörleri
        active_settings->network_adapter_count = sizeof(demo_adapters) / sizeof(extended_network_adapter_t);
        active_settings->network_adapters = (network_adapter_t*)demo_adapters;
        
        // Sistem ayarları
        strcpy(active_settings->system.hostname, "KALEM-OS");
        strcpy(active_settings->system.username, "user");
        active_settings->system.theme = 0;
        active_settings->system.autostart_gui = 1;
        active_settings->system.screen_saver_timeout = 10;
        active_settings->system.power_save_mode = 1;
        
        // Tarih/saat ayarları
        active_settings->datetime.year = 2023;
        active_settings->datetime.month = 3;
        active_settings->datetime.day = 24;
        active_settings->datetime.hour = 12;
        active_settings->datetime.minute = 30;
        active_settings->datetime.second = 0;
        active_settings->datetime.timezone = 3; // UTC+3
        
        // Dil ayarları
        active_settings->language = 0; // Türkçe
    }
    
    // Donanım bilgilerini ayarla
    settings_detect_hardware();
    
    // Disk bilgilerini ayarla
    settings_detect_storage();
    
    // USB aygıtlarını başlat
    settings_usb_monitor_init();
    
    // CPU yöneticisini başlat
    settings_cpu_manager_init();
    
    // Batarya yöneticisini başlat
    settings_battery_manager_init();
}

// Ayarlar penceresini göster
void settings_show() {
    // Uygulama başlatılmadıysa başlat
    if (active_settings == NULL) {
        settings_init();
    }
    
    // Pencere zaten açıksa odakla
    if (active_settings->window) {
        gui_window_bring_to_front(active_settings->window);
        return;
    }
    
    // Pencereyi oluştur
    active_settings->window = gui_window_create("Sistem Ayarları", 100, 50, 600, 450, GUI_WINDOW_STYLE_NORMAL);
    if (!active_settings->window) return;
    
    // Pencere kapatıldığında referansı temizleyecek kapanış fonksiyonu
    active_settings->window->on_close = window_close_handler;
    
    // Çizim fonksiyonunu ayarla
    active_settings->window->on_paint = settings_paint;
    
    // Pencereyi göster
    gui_window_show(active_settings->window);
}

// Kategori değiştir
void settings_change_category(settings_window_t* settings, settings_category_t category) {
    if (!settings || category >= SETTINGS_CAT_COUNT) return;
    
    settings->current_category = category;
    
    // Pencereyi yeniden çiz
    if (settings->window) {
        gui_window_redraw(settings->window);
    }
}

// Donanım algılama işlemleri
void settings_detect_hardware() {
    // Bu fonksiyon tüm donanım algılama fonksiyonlarını çağırır
    settings_detect_display();
    settings_detect_sound();
    settings_detect_network();
    
    // Eksik yapı tanımları için gerekli düzenlemeler
    // Driver_manager yapısını başlat
    memset(&driver_manager, 0, sizeof(extended_driver_manager_t));
    
    // Sürücüleri oluştur ve bağla
    display_driver_t* display_driver = (display_driver_t*)malloc(sizeof(display_driver_t));
    if (display_driver) {
        strcpy(display_driver->name, "Intel HD Graphics 630 Driver");
        strcpy(display_driver->vendor, "Intel Corporation");
        strcpy(display_driver->device_id, "8086:5912");
        strcpy(display_driver->version, "31.0.101.2111");
        display_driver->autodetect = 1;
        display_driver->acceleration = 8;
        display_driver->compatibility = 9;
        display_driver->installed = 1;
        
        driver_manager.current_display_driver = display_driver;
        driver_manager.display_driver = display_driver;  // Takma isim için aynı değeri atıyoruz
    }
    
    sound_driver_t* sound_driver = (sound_driver_t*)malloc(sizeof(sound_driver_t));
    if (sound_driver) {
        strcpy(sound_driver->name, "Realtek High Definition Audio");
        strcpy(sound_driver->vendor, "Realtek Semiconductor");
        strcpy(sound_driver->device_id, "10EC:0892");
        strcpy(sound_driver->version, "6.0.9240.1");
        sound_driver->autodetect = 1;
        sound_driver->channels = 5;
        sound_driver->hd_audio = 1;
        sound_driver->installed = 1;
        
        driver_manager.current_sound_driver = sound_driver;
        driver_manager.sound_driver = sound_driver;  // Takma isim için aynı değeri atıyoruz
    }
    
    // İlerleme durumlarını sıfırla
    driver_manager.driver_scan_progress = 0;
    driver_manager.scan_progress = 0;
    driver_manager.driver_install_progress = 0;
    driver_manager.install_progress = 0;
    driver_manager.is_installing_driver = 0;
    driver_manager.is_installing = 0;
    driver_manager.is_scanning = 0;
    strcpy(driver_manager.current_operation, "Hazır");
}

void settings_detect_display() {
    // Demo ekran kartı bilgilerini ayarla (zaten doldurulmuş)
    if (active_settings) {
        // Tip dönüşümü ile extended_display_resolution_t dizisini display_resolution_t dizisine çeviriyoruz
        active_settings->display.resolutions = (display_resolution_t*)demo_resolutions;
    }
}

void settings_detect_sound() {
    // Demo ses kartı bilgilerini doldur (zaten doldurulmuş)
}

void settings_detect_network() {
    // Demo ağ adaptörleri bilgilerini ayarla
    if (active_settings) {
        // Tip dönüşümüyle extended_network_adapter_t dizisini network_adapter_t dizisine çeviriyoruz
        active_settings->network_adapters = (network_adapter_t*)demo_adapters;
        
        // Demo adaptörlerinde is_wireless ve is_active alanlarını initial olarak ayarla
        for (uint32_t i = 0; i < active_settings->network_adapter_count; i++) {
            extended_network_adapter_t* adapter = (extended_network_adapter_t*)&active_settings->network_adapters[i];
            
            // Kablosuz adaptör ise (tip == 1), is_wireless'ı true yap
            adapter->is_wireless = (adapter->type == 1);
            
            // Bağlı ise (connected == 1), is_active'i true yap
            adapter->is_active = adapter->connected;
            
            // mac_address alanını mac alanının kopyası olarak ayarla
            strcpy(adapter->mac_address, adapter->mac);
        }
    }
}

// Çözünürlük değiştir
int settings_change_resolution(uint32_t width, uint32_t height, uint8_t bpp) {
    if (!active_settings) return -1;
    
    // İstenen çözünürlüğü ara
    for (uint32_t i = 0; i < active_settings->display.resolution_count; i++) {
        extended_display_resolution_t* res = &active_settings->display.resolutions[i];
        
        if (res->width == width && res->height == height && res->bpp == bpp) {
            // Çözünürlüğü ayarla
            active_settings->display.current_resolution = i;
            return 0;
        }
    }
    
    return -1; // Çözünürlük bulunamadı
}

// Ses ayarları
void settings_set_volume(uint8_t volume) {
    if (!active_settings) return;
    
    // Ses seviyesini sınırla (0-100)
    if (volume > 100) volume = 100;
    
    active_settings->sound.volume = volume;
}

void settings_toggle_mute() {
    if (!active_settings) return;
    
    active_settings->sound.muted = !active_settings->sound.muted;
}

// Saat ayarları
void settings_set_datetime(datetime_t* datetime) {
    if (!active_settings || !datetime) return;
    
    // Temel doğrulama
    if (datetime->month < 1 || datetime->month > 12) return;
    if (datetime->day < 1 || datetime->day > 31) return;
    if (datetime->hour > 23) return;
    if (datetime->minute > 59) return;
    if (datetime->second > 59) return;
    
    // Tarih/saati kopyala
    memcpy(&active_settings->datetime, datetime, sizeof(datetime_t));
}

// Ayarlar penceresini çiz
void settings_paint(gui_window_t* window) {
    if (!window || !active_settings) return;
    
    // Pencere içeriğini temizle
    gui_window_clear(window, COLOR_WHITE);
    
    // Kategori listesini çiz
    settings_paint_categories(window);
    
    // Seçilen kategoriye göre ilgili içeriği çiz
    switch (active_settings->current_category) {
        case SETTINGS_CAT_SYSTEM:
            settings_paint_system(window);
            break;
        case SETTINGS_CAT_DISPLAY:
            settings_paint_display(window);
            break;
        case SETTINGS_CAT_SOUND:
            settings_paint_sound(window);
            break;
        case SETTINGS_CAT_NETWORK:
            settings_paint_network(window);
            break;
        case SETTINGS_CAT_HARDWARE:
            settings_paint_hardware(window);
            break;
        case SETTINGS_CAT_DATETIME:
            settings_paint_datetime(window);
            break;
        case SETTINGS_CAT_LANGUAGE:
            settings_paint_language(window);
            break;
        case SETTINGS_CAT_STORAGE:
            settings_paint_storage(window);
            break;
        case SETTINGS_CAT_USB_DEVICES:
            settings_paint_usb_devices(window);
            break;
        case SETTINGS_CAT_CPU:
            settings_paint_cpu(window);
            break;
        case SETTINGS_CAT_BATTERY:
            settings_paint_battery(window);
            break;
        case SETTINGS_CAT_APPLICATIONS:
            // Uygulamalar kategorisi eklenecek
            break;
        default:
            break;
    }
}

// Kategori listesini çiz
void settings_paint_categories(gui_window_t* window) {
    if (!window || !active_settings) return;
    
    // Çizgi ile ayrılmış kategori paneli
    gui_draw_rect(window, 170, 0, 1, window->client.height, COLOR_DARK_GRAY);
    
    // Kategori listesi
    int y = 30;
    for (int i = 0; i < SETTINGS_CAT_COUNT; i++) {
        int bg_color = (i == active_settings->current_category) ? COLOR_LIGHT_BLUE : COLOR_WHITE;
        int text_color = (i == active_settings->current_category) ? COLOR_WHITE : COLOR_BLACK;
        
        // Kategori arka planı
        gui_draw_rect(window, 0, y, 170, 30, bg_color);
        
        // Kategori adı
        gui_draw_text(window, settings_category_names[i], 20, y + 8, text_color);
        
        y += 30;
    }
}

// Sistem ayarları
void settings_paint_system(gui_window_t* window) {
    if (!window || !active_settings) return;
    
    // Başlık
    gui_draw_text(window, "Sistem Ayarları", 180, 50, COLOR_BLACK);
    
    // Bilgisayar adı
    gui_draw_text(window, "Bilgisayar adı:", 180, 90, COLOR_DARK_GRAY);
    gui_draw_rect(window, 300, 85, 250, 25, COLOR_LIGHT_GRAY);
    gui_draw_text(window, active_settings->system.hostname, 310, 90, COLOR_BLACK);
    
    // Kullanıcı adı
    gui_draw_text(window, "Kullanıcı adı:", 180, 125, COLOR_DARK_GRAY);
    gui_draw_rect(window, 300, 120, 250, 25, COLOR_LIGHT_GRAY);
    gui_draw_text(window, active_settings->system.username, 310, 125, COLOR_BLACK);
    
    // GUI otomatik başlat
    gui_draw_text(window, "GUI otomatik başlat:", 180, 160, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 300, 160, active_settings->system.autostart_gui);
    
    // Tema
    gui_draw_text(window, "Tema:", 180, 195, COLOR_DARK_GRAY);
    const char* theme_names[] = {"Standart", "Koyu", "Açık", "Yüksek Kontrast"};
    gui_draw_dropdown(window, 300, 190, 250, 30, theme_names, 4, active_settings->system.theme);
    
    // Ekran koruyucu
    gui_draw_text(window, "Ekran koruyucu zaman aşımı:", 180, 230, COLOR_DARK_GRAY);
    char timeout_text[32];
    sprintf(timeout_text, "%d dakika", active_settings->system.screen_saver_timeout);
    gui_draw_rect(window, 380, 225, 80, 25, COLOR_LIGHT_GRAY);
    gui_draw_text(window, timeout_text, 390, 230, COLOR_BLACK);
    
    // Güç tasarruf modu
    gui_draw_text(window, "Güç tasarruf modu:", 180, 265, COLOR_DARK_GRAY);
    const char* power_modes[] = {"Kapalı", "Düşük", "Orta", "Yüksek"};
    gui_draw_dropdown(window, 300, 260, 250, 30, power_modes, 4, active_settings->system.power_save_mode);
    
    // Sistem bilgileri
    gui_draw_text(window, "Sistem Bilgileri", 180, 310, COLOR_BLACK);
    gui_draw_text(window, "İşletim Sistemi: KALEM OS 1.0", 200, 340, COLOR_DARK_GRAY);
    gui_draw_text(window, "Çekirdek Sürümü: 0.1.0", 200, 370, COLOR_DARK_GRAY);
    gui_draw_text(window, "İşlemci: Demo CPU @ 2.4 GHz", 200, 400, COLOR_DARK_GRAY);
}

// Ekran ayarları
void settings_paint_display(gui_window_t* window) {
    int x = 180;
    int y = 80;
    
    // Başlık
    gui_draw_text(window, "Ekran Ayarları", x, y, COLOR_BLUE);
    
    y += 30;
    
    // Mevcut ekran kartı
    gui_draw_text(window, "Ekran Kartı:", x, y, COLOR_DARK_GRAY);
    gui_draw_text(window, active_settings->display.name, x + 120, y, COLOR_BLACK);
    
    y += 20;
    
    // VRAM boyutu
    char vram_text[32];
    sprintf(vram_text, "VRAM: %d MB", active_settings->display.vram_size / (1024 * 1024));
    gui_draw_text(window, vram_text, x, y, COLOR_DARK_GRAY);
    
    y += 30;
    
    // Çözünürlük ayarları başlığı
    gui_draw_text(window, "Çözünürlük Ayarları:", x, y, COLOR_DARK_GRAY);
    
    y += 30;
    
    // Mevcut çözünürlüğü göster
    extended_display_resolution_t* current_res = (extended_display_resolution_t*)&active_settings->display.resolutions[active_settings->display.current_resolution];
    gui_draw_text(window, "Mevcut Çözünürlük:", x, y, COLOR_DARK_GRAY);
    gui_draw_text(window, current_res->description, x + 150, y, COLOR_BLACK);
    
    y += 30;
    
    // Diğer çözünürlük seçenekleri
    gui_draw_text(window, "Kullanılabilir Çözünürlükler:", x, y, COLOR_DARK_GRAY);
    
    y += 30;
    
    for (uint32_t i = 0; i < active_settings->display.resolution_count; i++) {
        extended_display_resolution_t* res = (extended_display_resolution_t*)&active_settings->display.resolutions[i];
        
        // Seçili mi?
        int bg_color = (i == active_settings->display.current_resolution) ? COLOR_LIGHT_BLUE : COLOR_WHITE;
        
        // Çözünürlük butonunu çiz
        gui_draw_rect(window, 180, y, 300, 30, bg_color);
        gui_draw_text(window, res->description, 195, y + 8, COLOR_BLACK);
        
        y += 40;
    }
}

// Ses ayarları
void settings_paint_sound(gui_window_t* window) {
    if (!window || !active_settings) return;
    
    // Başlık
    gui_draw_text(window, "Ses Ayarları", 180, 50, COLOR_BLACK);
    
    // Ses kartı bilgileri
    gui_draw_text(window, "Ses Kartı:", 180, 90, COLOR_DARK_GRAY);
    gui_draw_text(window, active_settings->sound.name, 280, 90, COLOR_BLACK);
    
    // Ses kanalları
    gui_draw_text(window, "Ses Kanalları:", 180, 125, COLOR_DARK_GRAY);
    char channels_text[32];
    sprintf(channels_text, "%d (Stereo)", active_settings->sound.channels);
    gui_draw_text(window, channels_text, 280, 125, COLOR_BLACK);
    
    // Ses seviyesi
    gui_draw_text(window, "Ses Seviyesi:", 180, 160, COLOR_DARK_GRAY);
    
    // Ses çubuğu
    int volume_bar_width = 300;
    int filled_width = (active_settings->sound.volume * volume_bar_width) / 100;
    
    // Arka plan
    gui_draw_rect(window, 280, 160, volume_bar_width, 20, COLOR_LIGHT_GRAY);
    
    // Doldurulmuş kısım
    gui_draw_rect(window, 280, 160, filled_width, 20, COLOR_BLUE);
    
    // Değer
    char volume_text[16];
    sprintf(volume_text, "%d%%", active_settings->sound.volume);
    gui_draw_text(window, volume_text, 590, 160, COLOR_BLACK);
    
    // Sessiz
    gui_draw_text(window, "Sessiz:", 180, 195, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 280, 195, active_settings->sound.muted);
    
    // Test butonu
    gui_draw_button(window, 180, 240, 150, 30, "Ses Testi", COLOR_BLUE, COLOR_WHITE);
}

// Ağ ayarları
void settings_paint_network(gui_window_t* window) {
    int x = 180;
    int y = 80;
    
    // Başlık
    gui_draw_text(window, "Ağ Ayarları", x, y, COLOR_BLUE);
    
    y += 40;
    
    // Bulunan adaptörleri listele
    for (uint32_t i = 0; i < active_settings->network_adapter_count; i++) {
        extended_network_adapter_t* adapter = (extended_network_adapter_t*)&active_settings->network_adapters[i];
        
        // Adaptör adı
        gui_draw_text(window, adapter->name, 180, y, COLOR_BLUE);
        
        // MAC adresi
        gui_draw_text(window, "MAC:", 180, y + 25, COLOR_DARK_GRAY);
        gui_draw_text(window, adapter->mac_address, 220, y + 25, COLOR_BLACK);
        
        // Kablosuz mu?
        if (adapter->is_wireless) {
            gui_draw_text(window, "Kablosuz", 400, y, COLOR_BLUE);
        } else {
            gui_draw_text(window, "Kablolu", 400, y, COLOR_DARK_GREEN);
        }
        
        // Aktif mi?
        if (adapter->is_active) {
            gui_draw_text(window, "Aktif", 470, y, COLOR_DARK_GREEN);
        } else {
            gui_draw_text(window, "Devre Dışı", 470, y, COLOR_DARK_RED);
        }
        
        // Bir sonraki adaptör için satır atla
        y += 60;
    }
    
    y += 20;
    
    // Ağ bağlantı yapılandırma butonu
    gui_draw_button(window, 180, 320, 210, 30, "Bağlantı Yapılandır", COLOR_BLUE, COLOR_WHITE);
}

// Donanım ayarlarını çiz - Yapay zeka özellikleri geliştirilmiş
void settings_paint_hardware(gui_window_t* window) {
    int x = 180;
    int y = 80;
    
    // Başlık
    gui_draw_text(window, "Donanım Ayarları", x, y, COLOR_BLUE);
    
    // Genel donanım algılama butonu
    gui_draw_button(window, 300, 80, 180, 30, "Donanımları Algıla", COLOR_BLUE, COLOR_WHITE);
    
    y += 50;
    
    // Ekran kartı
    gui_draw_text(window, "Ekran Kartı:", 180, 130, COLOR_DARK_GRAY);
    
    if (driver_manager.display_driver) {
        char gpu_text[256];
        snprintf(gpu_text, sizeof(gpu_text), "%s (%s)", 
                driver_manager.display_driver->name,
                driver_manager.display_driver->installed ? "Kurulu" : "Kurulu Değil");
        
        gui_draw_text(window, gpu_text, 280, 130, 
                     driver_manager.display_driver->installed ? COLOR_DARK_GREEN : COLOR_DARK_RED);
        
        // Detaylar
        gui_draw_text(window, driver_manager.display_driver->vendor, 280, 150, COLOR_DARK_GRAY);
        
        char version_text[64];
        snprintf(version_text, sizeof(version_text), "Sürüm: %s", driver_manager.display_driver->version);
        gui_draw_text(window, version_text, 280, 170, COLOR_DARK_GRAY);
        
        // Sürücü özellikleri
        int feature_y = 190;
        
        if (driver_manager.display_driver->autodetect) {
            gui_draw_text(window, "✓ Otomatik Algılama", 280, feature_y, COLOR_DARK_GREEN);
            feature_y += 20;
        }
        
        if (driver_manager.display_driver->acceleration) {
            gui_draw_text(window, "✓ Donanım Hızlandırma", 280, feature_y, COLOR_DARK_GREEN);
            feature_y += 20;
        }
        
        // Butonlar
        if (!driver_manager.display_driver->installed) {
            gui_draw_button(window, 500, 130, 130, 30, "Sürücüyü Kur", COLOR_BLUE, COLOR_WHITE);
        } else {
            gui_draw_button(window, 500, 130, 130, 30, "Sürücüyü Güncelle", COLOR_GREEN, COLOR_WHITE);
        }
        
        gui_draw_button(window, 500, 170, 130, 30, "Sürücü Ara", COLOR_BLUE, COLOR_WHITE);
    } else {
        gui_draw_text(window, "Desteklenen sürücü bulunamadı.", 280, 130, COLOR_DARK_RED);
    }
    
    y = 230;
    
    // Ses kartı
    gui_draw_text(window, "Ses Kartı:", 180, 230, COLOR_DARK_GRAY);
    
    if (driver_manager.sound_driver) {
        char audio_text[256];
        snprintf(audio_text, sizeof(audio_text), "%s (%s)", 
                driver_manager.sound_driver->name,
                driver_manager.sound_driver->installed ? "Kurulu" : "Kurulu Değil");
        
        gui_draw_text(window, audio_text, 280, 230, 
                     driver_manager.sound_driver->installed ? COLOR_DARK_GREEN : COLOR_DARK_RED);
        
        // Detaylar
        gui_draw_text(window, driver_manager.sound_driver->vendor, 280, 250, COLOR_DARK_GRAY);
        
        char version_text[64];
        snprintf(version_text, sizeof(version_text), "Sürüm: %s", driver_manager.sound_driver->version);
        gui_draw_text(window, version_text, 280, 270, COLOR_DARK_GRAY);
        
        // Butonlar
        if (!driver_manager.sound_driver->installed) {
            gui_draw_button(window, 500, 230, 130, 30, "Sürücüyü Kur", COLOR_BLUE, COLOR_WHITE);
        } else {
            gui_draw_button(window, 500, 230, 130, 30, "Sürücüyü Güncelle", COLOR_GREEN, COLOR_WHITE);
        }
        
        gui_draw_button(window, 500, 270, 130, 30, "Sürücü Ara", COLOR_BLUE, COLOR_WHITE);
    } else {
        gui_draw_text(window, "Desteklenen sürücü bulunamadı.", 280, 230, COLOR_DARK_RED);
    }
    
    // Şu anki tanıma durumu
    char status_text[64];
    if (driver_manager.is_scanning) {
        sprintf(status_text, "Donanım Taranıyor: %%%d", driver_manager.scan_progress);
        gui_draw_text(window, status_text, 320, 320, COLOR_BLUE);
        
        // İlerleme çubuğu
        int progress_width = 200;
        int filled_width = (driver_manager.scan_progress * progress_width) / 100;
        
        gui_draw_rect(window, 320, 340, progress_width, 15, COLOR_LIGHT_GRAY);
        gui_draw_rect(window, 320, 340, filled_width, 15, COLOR_BLUE);
    } 
    else if (driver_manager.is_installing) {
        sprintf(status_text, "Sürücü Yükleniyor: %%%d", driver_manager.install_progress);
        gui_draw_text(window, status_text, 320, 320, COLOR_GREEN);
        
        // İlerleme çubuğu
        int progress_width = 200;
        int filled_width = (driver_manager.install_progress * progress_width) / 100;
        
        gui_draw_rect(window, 320, 340, progress_width, 15, COLOR_LIGHT_GRAY);
        gui_draw_rect(window, 320, 340, filled_width, 15, COLOR_GREEN);
        
        // Şu anki işlem
        gui_draw_text(window, driver_manager.current_operation, 320, 360, COLOR_DARK_GRAY);
    } 
    else {
        gui_draw_text(window, "Sistem Hazır", 320, 320, COLOR_DARK_GREEN);
    }
    
    // Yapay zeka kullanarak donanımları algılama butonu
    gui_draw_button(window, 180, 350, 220, 30, "Yapay Zeka ile Tüm Donanımları Algıla", COLOR_DARK_GREEN, COLOR_WHITE);
    
    // Donanım uyumluluk raporu oluştur
    gui_draw_button(window, 410, 350, 220, 30, "Donanım Uyumluluk Raporu Oluştur", COLOR_BLUE, COLOR_WHITE);
}

// Tarih ve saat ayarları
void settings_paint_datetime(gui_window_t* window) {
    if (!window || !active_settings) return;
    
    // Başlık
    gui_draw_text(window, "Tarih ve Saat Ayarları", 180, 50, COLOR_BLACK);
    
    // Tarih girdileri
    gui_draw_text(window, "Tarih:", 180, 90, COLOR_DARK_GRAY);
    
    char day_text[8];
    char month_text[8];
    char year_text[8];
    sprintf(day_text, "%02d", active_settings->datetime.day);
    sprintf(month_text, "%02d", active_settings->datetime.month);
    sprintf(year_text, "%04d", active_settings->datetime.year);
    
    // Gün
    gui_draw_rect(window, 240, 85, 40, 25, COLOR_LIGHT_GRAY);
    gui_draw_text(window, day_text, 250, 90, COLOR_BLACK);
    
    gui_draw_text(window, "/", 285, 90, COLOR_BLACK);
    
    // Ay
    gui_draw_rect(window, 295, 85, 40, 25, COLOR_LIGHT_GRAY);
    gui_draw_text(window, month_text, 305, 90, COLOR_BLACK);
    
    gui_draw_text(window, "/", 340, 90, COLOR_BLACK);
    
    // Yıl
    gui_draw_rect(window, 350, 85, 60, 25, COLOR_LIGHT_GRAY);
    gui_draw_text(window, year_text, 360, 90, COLOR_BLACK);
    
    // Saat girdileri
    gui_draw_text(window, "Saat:", 180, 130, COLOR_DARK_GRAY);
    
    char hour_text[8];
    char minute_text[8];
    char second_text[8];
    sprintf(hour_text, "%02d", active_settings->datetime.hour);
    sprintf(minute_text, "%02d", active_settings->datetime.minute);
    sprintf(second_text, "%02d", active_settings->datetime.second);
    
    // Saat
    gui_draw_rect(window, 240, 125, 40, 25, COLOR_LIGHT_GRAY);
    gui_draw_text(window, hour_text, 250, 130, COLOR_BLACK);
    
    gui_draw_text(window, ":", 285, 130, COLOR_BLACK);
    
    // Dakika
    gui_draw_rect(window, 295, 125, 40, 25, COLOR_LIGHT_GRAY);
    gui_draw_text(window, minute_text, 305, 130, COLOR_BLACK);
    
    gui_draw_text(window, ":", 340, 130, COLOR_BLACK);
    
    // Saniye
    gui_draw_rect(window, 350, 125, 40, 25, COLOR_LIGHT_GRAY);
    gui_draw_text(window, second_text, 360, 130, COLOR_BLACK);
    
    // Zaman dilimi
    gui_draw_text(window, "Zaman Dilimi:", 180, 170, COLOR_DARK_GRAY);
    
    const char* timezone_names[] = {
        "UTC-12:00", "UTC-11:00", "UTC-10:00", "UTC-09:00", "UTC-08:00", "UTC-07:00",
        "UTC-06:00", "UTC-05:00", "UTC-04:00", "UTC-03:00", "UTC-02:00", "UTC-01:00",
        "UTC+00:00", "UTC+01:00", "UTC+02:00", "UTC+03:00 (Türkiye)", "UTC+04:00", "UTC+05:00",
        "UTC+06:00", "UTC+07:00", "UTC+08:00", "UTC+09:00", "UTC+10:00", "UTC+11:00", "UTC+12:00"
    };
    
    gui_draw_dropdown(window, 280, 165, 250, 30, timezone_names, 25, active_settings->datetime.timezone + 12);
    
    // Otomatik saat ayarı
    gui_draw_text(window, "Otomatik saat ayarla:", 180, 220, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 320, 220, 1);
    
    // Format
    gui_draw_text(window, "Tarih Formatı:", 180, 260, COLOR_DARK_GRAY);
    const char* date_formats[] = {"DD/MM/YYYY", "MM/DD/YYYY", "YYYY-MM-DD"};
    gui_draw_dropdown(window, 280, 255, 150, 30, date_formats, 3, 0);
    
    gui_draw_text(window, "Saat Formatı:", 180, 300, COLOR_DARK_GRAY);
    const char* time_formats[] = {"24 Saat", "12 Saat (AM/PM)"};
    gui_draw_dropdown(window, 280, 295, 150, 30, time_formats, 2, 0);
    
    // Uygula butonu
    gui_draw_button(window, 180, 350, 150, 30, "Uygula", COLOR_BLUE, COLOR_WHITE);
}

// Dil ayarları
void settings_paint_language(gui_window_t* window) {
    if (!window || !active_settings) return;
    
    // Başlık
    gui_draw_text(window, "Dil Ayarları", 180, 50, COLOR_BLACK);
    
    // Dil seçimi
    gui_draw_text(window, "Sistem Dili:", 180, 90, COLOR_DARK_GRAY);
    
    // Dilleri listele
    const char* languages[] = {
        "Türkçe",
        "English (İngilizce)",
        "Deutsch (Almanca)",
        "Français (Fransızca)",
        "Español (İspanyolca)",
        "Italiano (İtalyanca)",
        "Русский (Rusça)",
        "日本語 (Japonca)",
        "中文 (Çince)"
    };
    
    int y = 130;
    for (int i = 0; i < 9; i++) {
        // Seçili mi?
        int bg_color = (i == active_settings->language) ? COLOR_LIGHT_BLUE : COLOR_LIGHT_GRAY;
        
        // Dil butonunu çiz
        gui_draw_rect(window, 180, y, 300, 30, bg_color);
        gui_draw_text(window, languages[i], 195, y + 8, COLOR_BLACK);
        
        y += 40;
    }
    
    // Uygula butonu
    gui_draw_button(window, 180, y + 20, 150, 30, "Uygula", COLOR_BLUE, COLOR_WHITE);
    gui_draw_text(window, "* Değişiklikler yeniden başlatma gerektirebilir", 180, y + 60, COLOR_DARK_RED);
}

// Uygulama başlangıç fonksiyonu
void app_settings() {
    settings_show();
}

// Donanımı tanımlamayı simüle eden fonksiyon
int settings_hardware_fingerprint(const char* vendor_id, const char* device_id) {
    // Bu fonksiyon gerçek bir sistemde PCI/USB ID'leri ile donanımı tanımlar
    // Şu an için bir simülasyon amacıyla sabit bir değer döndürüyoruz
    
    // Demo olarak bazı cihaz ID'lerini tanıyoruz
    if (strcmp(vendor_id, "8086") == 0) {
        return 90; // Intel cihazları
    } else if (strcmp(vendor_id, "10DE") == 0) {
        return 85; // NVIDIA cihazları
    } else if (strcmp(vendor_id, "1002") == 0) {
        return 88; // AMD cihazları
    } else if (strcmp(vendor_id, "10EC") == 0) {
        return 92; // Realtek cihazları
    } else if (strcmp(vendor_id, "80EE") == 0) {
        return 95; // VirtualBox cihazları
    }
    
    return 50; // Tanınmayan cihazlar için %50 eşleşme
}

// Ekran sürücüsünü otomatik tanı
display_driver_t* settings_detect_display_driver() {
    display_driver_t* best_match = NULL;
    int best_score = 0;
    
    // Demo: Intel HD Graphics cihazını tanıma
    const char* demo_vendor = "8086"; // Intel
    const char* demo_device = "0102"; // HD Graphics
    
    // Tüm ekran sürücülerini kontrol et
    for (size_t i = 0; i < sizeof(display_drivers) / sizeof(display_driver_t); i++) {
        display_driver_t* driver = &display_drivers[i];
        
        // Vendor ID'yi ayıkla
        char vendor_id[5] = {0};
        strncpy(vendor_id, driver->device_id, 4);
        
        // Cihaz ID'yi ayıkla
        char device_id[5] = {0};
        strncpy(device_id, driver->device_id + 5, 4);
        
        // Donanım parmak izi ile eşleşme skoru hesapla
        int match_score = settings_hardware_fingerprint(vendor_id, device_id);
        
        // Tam eşleşme varsa hemen döndür
        if (strcmp(demo_vendor, vendor_id) == 0 && 
            strcmp(demo_device, device_id) == 0) {
            return driver;
        }
        
        // En iyi eşleşmeyi tut
        if (match_score > best_score) {
            best_score = match_score;
            best_match = driver;
        }
    }
    
    // En iyi eşleşen sürücüyü veya hiçbiri bulunamazsa VESA sürücüsünü döndür
    if (best_score < 40) {
        // VESA sürücüsünü bul
        for (size_t i = 0; i < sizeof(display_drivers) / sizeof(display_driver_t); i++) {
            if (strcmp(display_drivers[i].name, "KALEM OS VESA Temel Görüntü Sürücüsü") == 0) {
                return &display_drivers[i];
            }
        }
    }
    
    return best_match;
}

// Ses sürücüsünü otomatik tanı
sound_driver_t* settings_detect_sound_driver() {
    sound_driver_t* best_match = NULL;
    int best_score = 0;
    
    // Demo: Realtek HD Audio cihazını tanıma
    const char* demo_vendor = "10EC"; // Realtek
    const char* demo_device = "0880"; // HD Audio
    
    // Tüm ses sürücülerini kontrol et
    for (size_t i = 0; i < sizeof(sound_drivers) / sizeof(sound_driver_t); i++) {
        sound_driver_t* driver = &sound_drivers[i];
        
        // Vendor ID'yi ayıkla
        char vendor_id[5] = {0};
        strncpy(vendor_id, driver->device_id, 4);
        
        // Cihaz ID'yi ayıkla
        char device_id[5] = {0};
        strncpy(device_id, driver->device_id + 5, 4);
        
        // Donanım parmak izi ile eşleşme skoru hesapla
        int match_score = settings_hardware_fingerprint(vendor_id, device_id);
        
        // Tam eşleşme varsa hemen döndür
        if (strcmp(demo_vendor, vendor_id) == 0 && 
            strcmp(demo_device, device_id) == 0) {
            return driver;
        }
        
        // En iyi eşleşmeyi tut
        if (match_score > best_score) {
            best_score = match_score;
            best_match = driver;
        }
    }
    
    // En iyi eşleşen sürücüyü veya hiçbiri bulunamazsa temel beeper sürücüsünü döndür
    if (best_score < 40) {
        // Temel beeper sürücüsünü bul
        for (size_t i = 0; i < sizeof(sound_drivers) / sizeof(sound_driver_t); i++) {
            if (strcmp(sound_drivers[i].name, "KALEM OS Temel PC Beeper Sürücüsü") == 0) {
                return &sound_drivers[i];
            }
        }
    }
    
    return best_match;
}

// Ekran sürücüsünü kur
int settings_install_display_driver(display_driver_t* driver) {
    if (!driver) return -1;
    
    // Sürücüyü kurulu olarak işaretle
    driver->installed = 1;
    
    // İlgili donanımın sürücüsünü güncelle
    driver_manager.current_display_driver = driver;
    
    // Gerçek bir kurulum için burada dosya kopyalamaları ve yapılandırmalar yapılır
    // Demo sürümünde sadece kurulmuş olarak işaretliyoruz
    
    return 0;
}

// Ses sürücüsünü kur
int settings_install_sound_driver(sound_driver_t* driver) {
    if (!driver) return -1;
    
    // Sürücüyü kurulu olarak işaretle
    driver->installed = 1;
    
    // İlgili donanımın sürücüsünü güncelle
    driver_manager.current_sound_driver = driver;
    
    // Gerçek bir kurulum için burada dosya kopyalamaları ve yapılandırmalar yapılır
    // Demo sürümünde sadece kurulmuş olarak işaretliyoruz
    
    return 0;
}

// Tüm sürücüleri otomatik tanı ve kur
void settings_autodetect_and_install_drivers() {
    // İlerleme durumunu başlat
    driver_manager.driver_scan_progress = 0;
    driver_manager.is_installing_driver = 1;
    
    // Ekran sürücüsünü tanı
    display_driver_t* display_driver = settings_detect_display_driver();
    driver_manager.driver_scan_progress = 50;
    
    // Ses sürücüsünü tanı
    sound_driver_t* sound_driver = settings_detect_sound_driver();
    driver_manager.driver_scan_progress = 100;
    
    // Kuruluma başla
    driver_manager.driver_install_progress = 0;
    
    // Ekran sürücüsünü kur
    if (display_driver) {
        settings_install_display_driver(display_driver);
    }
    driver_manager.driver_install_progress = 50;
    
    // Ses sürücüsünü kur
    if (sound_driver) {
        settings_install_sound_driver(sound_driver);
    }
    driver_manager.driver_install_progress = 100;
    
    driver_manager.is_installing_driver = 0;
}

// Sürücü kurulumu
int settings_install_driver(const char* device_name) {
    if (!active_settings) return -1;
    
    // Ekran kartı mı?
    if (strcmp(device_name, active_settings->display.name) == 0) {
        if (driver_manager.current_display_driver) {
            return settings_install_display_driver(driver_manager.current_display_driver);
        }
    }
    // Ses kartı mı?
    else if (strcmp(device_name, active_settings->sound.name) == 0) {
        if (driver_manager.current_sound_driver) {
            return settings_install_sound_driver(driver_manager.current_sound_driver);
        }
    }
    
    // Diğer donanımlar için genel driver kurulumu
    return 0;
}

// Tüm sürücüleri güncelle
int settings_update_drivers() {
    // Ekran sürücüsünü güncelle
    if (driver_manager.current_display_driver) {
        settings_install_display_driver(driver_manager.current_display_driver);
    }
    
    // Ses sürücüsünü güncelle
    if (driver_manager.current_sound_driver) {
        settings_install_sound_driver(driver_manager.current_sound_driver);
    }
    
    return 0;
}

// Disk sürücülerini algıla ve başlat
void settings_detect_storage() {
    // Bu demo sürümünde sabit diskleri yerleştir
    
    // Demo olarak 3 disk gösterelim
    storage_manager.disk_count = 3;
    storage_manager.disks = malloc(sizeof(disk_t) * storage_manager.disk_count);
    
    if (!storage_manager.disks) {
        return;
    }
    
    // Diskleri kopyala
    memcpy(storage_manager.disks, demo_disks, sizeof(disk_t) * storage_manager.disk_count);
    
    // Bölümleri ayarla
    storage_manager.disks[0].partitions = malloc(sizeof(disk_partition_t) * 2);
    if (storage_manager.disks[0].partitions) {
        memcpy(storage_manager.disks[0].partitions, demo_partitions_sda, sizeof(disk_partition_t) * 2);
    }
    
    storage_manager.disks[1].partitions = malloc(sizeof(disk_partition_t) * 3);
    if (storage_manager.disks[1].partitions) {
        memcpy(storage_manager.disks[1].partitions, demo_partitions_sdb, sizeof(disk_partition_t) * 3);
    }
    
    storage_manager.disks[2].partitions = malloc(sizeof(disk_partition_t) * 1);
    if (storage_manager.disks[2].partitions) {
        memcpy(storage_manager.disks[2].partitions, demo_partitions_sdc, sizeof(disk_partition_t) * 1);
    }
    
    // İlk disk seçildi
    storage_manager.selected_disk = 0;
    storage_manager.selected_partition = 0;
    storage_manager.selected_filesystem = FILE_SYSTEM_KALEMOS;
}

// Disk ismi ile dosya sistemi adını al
const char* settings_get_filesystem_name(file_system_t fs) {
    for (int i = 0; i < sizeof(file_system_info) / sizeof(file_system_info_t); i++) {
        if (file_system_info[i].type == fs) {
            return file_system_info[i].name;
        }
    }
    
    if (fs == FILE_SYSTEM_NONE) {
        return "Formatsız";
    } else if (fs == FILE_SYSTEM_UNKNOWN) {
        return "Bilinmeyen";
    } else if (fs == FILE_SYSTEM_KALEMOSFS) {
        return "KALEMOSFS";
    }
    
    return "?";
}

// Disk tipini metin olarak al
const char* settings_get_disk_type_name(disk_type_t type) {
    switch (type) {
        case DISK_TYPE_HDD:   return "Sabit Disk (HDD)";
        case DISK_TYPE_SSD:   return "SSD";
        case DISK_TYPE_NVME:  return "NVMe";
        case DISK_TYPE_USB:   return "USB Depolama";
        case DISK_TYPE_CDROM: return "CD/DVD";
        default:              return "Bilinmeyen";
    }
}

// Boyutu okunabilir formatta al
void settings_format_size(uint64_t size_bytes, char* buffer, size_t buffer_size) {
    if (size_bytes < 1024) {
        snprintf(buffer, buffer_size, "%llu B", size_bytes);
    } else if (size_bytes < 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.2f KB", (float)size_bytes / 1024);
    } else if (size_bytes < 1024 * 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.2f MB", (float)size_bytes / (1024 * 1024));
    } else if (size_bytes < 1024ULL * 1024 * 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.2f GB", (float)size_bytes / (1024 * 1024 * 1024));
    } else {
        snprintf(buffer, buffer_size, "%.2f TB", (float)size_bytes / (1024ULL * 1024 * 1024 * 1024));
    }
}

// Disk bağlama
int settings_mount_partition(disk_partition_t* partition, const char* mount_point) {
    if (!partition) return -1;
    
    // Demo olarak başarılı kabul edelim
    partition->is_mounted = 1;
    strncpy(partition->mount_point, mount_point, sizeof(partition->mount_point) - 1);
    partition->mount_point[sizeof(partition->mount_point) - 1] = '\0';
    
    return 0;
}

// Disk bağlantısını kesme
int settings_unmount_partition(disk_partition_t* partition) {
    if (!partition) return -1;
    
    // Demo olarak başarılı kabul edelim
    partition->is_mounted = 0;
    partition->mount_point[0] = '\0';
    
    return 0;
}

// Disk formatlama
int settings_format_partition(disk_partition_t* partition, file_system_t fs) {
    if (!partition) return -1;
    
    // Formatlama işlemi
    storage_manager.is_formatting = 1;
    storage_manager.format_progress = 0;
    
    // Simülasyon
    partition->file_system = fs;
    
    // Demo olarak başarılı kabul edelim
    return 0;
}

// Disk bölümleme
int settings_create_partition(disk_t* disk, uint64_t start_sector, uint64_t size_sectors) {
    if (!disk) return -1;
    
    // Demo olarak başarılı kabul edelim
    return 0;
}

// Depolama ayarları
void settings_paint_storage(gui_window_t* window) {
    if (!window || !active_settings) return;
    
    // Başlık
    gui_draw_text(window, "Disk ve Depolama Yönetimi", 180, 50, COLOR_BLACK);
    
    // Diskler
    gui_draw_text(window, "Diskler:", 180, 90, COLOR_DARK_GRAY);
    
    if (storage_manager.disk_count == 0 || !storage_manager.disks) {
        gui_draw_text(window, "Disk bulunamadı.", 200, 120, COLOR_DARK_RED);
        return;
    }
    
    // Disk listesi
    int y = 120;
    for (uint32_t i = 0; i < storage_manager.disk_count; i++) {
        disk_t* disk = &storage_manager.disks[i];
        
        // Disk seçili mi?
        int bg_color = (i == storage_manager.selected_disk) ? COLOR_LIGHT_BLUE : COLOR_LIGHT_GRAY;
        
        // Disk bilgisi
        char size_text[32];
        settings_format_size(disk->size_bytes, size_text, sizeof(size_text));
        
        char disk_text[128];
        snprintf(disk_text, sizeof(disk_text), "%s: %s - %s (%s)", 
                disk->name, disk->model, size_text, settings_get_disk_type_name(disk->type));
        
        // Disk butonunu çiz
        gui_draw_rect(window, 180, y, 400, 30, bg_color);
        gui_draw_text(window, disk_text, 190, y + 8, COLOR_BLACK);
        
        y += 40;
    }
    
    // Seçili disk bilgileri
    if (storage_manager.selected_disk < storage_manager.disk_count) {
        disk_t* disk = &storage_manager.disks[storage_manager.selected_disk];
        
        y += 20;
        gui_draw_text(window, "Seçili Disk Bilgileri:", 180, y, COLOR_BLACK);
        y += 30;
        
        char model_text[128];
        snprintf(model_text, sizeof(model_text), "Model: %s", disk->model);
        gui_draw_text(window, model_text, 200, y, COLOR_DARK_GRAY);
        y += 25;
        
        char serial_text[128];
        snprintf(serial_text, sizeof(serial_text), "Seri No: %s", disk->serial);
        gui_draw_text(window, serial_text, 200, y, COLOR_DARK_GRAY);
        y += 25;
        
        char size_text[32];
        settings_format_size(disk->size_bytes, size_text, sizeof(size_text));
        char size_info[128];
        snprintf(size_info, sizeof(size_info), "Toplam Boyut: %s (%llu sektör, %u bayt/sektör)", 
                size_text, disk->size_sectors, disk->sector_size);
        gui_draw_text(window, size_info, 200, y, COLOR_DARK_GRAY);
        y += 25;
        
        char removable_text[128];
        snprintf(removable_text, sizeof(removable_text), "Çıkarılabilir: %s", 
                disk->is_removable ? "Evet" : "Hayır");
        gui_draw_text(window, removable_text, 200, y, COLOR_DARK_GRAY);
        y += 40;
        
        // Bölüm başlığı
        gui_draw_text(window, "Disk Bölümleri:", 180, y, COLOR_BLACK);
        y += 30;
        
        // Bölüm listesi
        if (disk->partition_count == 0 || !disk->partitions) {
            gui_draw_text(window, "Bölüm bulunamadı.", 200, y, COLOR_DARK_RED);
        } else {
            for (uint32_t i = 0; i < disk->partition_count; i++) {
                disk_partition_t* part = &disk->partitions[i];
                
                // Bölüm seçili mi?
                int bg_color = (i == storage_manager.selected_partition) ? COLOR_LIGHT_BLUE : COLOR_LIGHT_GRAY;
                
                // Bölüm bilgisi
                char part_size[32];
                settings_format_size(part->size_bytes, part_size, sizeof(part_size));
                
                char part_text[256];
                snprintf(part_text, sizeof(part_text), "%s: %s (%s) - %s%s%s", 
                        part->name, part->label, part_size, 
                        settings_get_filesystem_name(part->file_system),
                        part->is_mounted ? " - Bağlı: " : "",
                        part->is_mounted ? part->mount_point : "");
                
                // Bölüm butonunu çiz
                gui_draw_rect(window, 200, y, 380, 30, bg_color);
                gui_draw_text(window, part_text, 210, y + 8, COLOR_BLACK);
                
                y += 40;
            }
            
            y += 20;
            
            // İşlem butonları
            gui_draw_button(window, 200, y, 100, 30, "Bağla", COLOR_BLUE, COLOR_WHITE);
            gui_draw_button(window, 310, y, 100, 30, "Ayır", COLOR_DARK_RED, COLOR_WHITE);
            gui_draw_button(window, 420, y, 100, 30, "Formatla", COLOR_DARK_RED, COLOR_WHITE);
            
            // Formatlama işlemi devam ediyorsa ilerleme çubuğunu göster
            if (storage_manager.is_formatting) {
                y += 50;
                gui_draw_text(window, "Formatlama İşlemi Devam Ediyor...", 200, y, COLOR_BLUE);
                
                // İlerleme çubuğu
                y += 25;
                int progress_width = 380;
                int filled_width = (storage_manager.format_progress * progress_width) / 100;
                
                // Arka plan
                gui_draw_rect(window, 200, y, progress_width, 20, COLOR_LIGHT_GRAY);
                
                // Doldurulmuş kısım
                gui_draw_rect(window, 200, y, filled_width, 20, COLOR_BLUE);
                
                // Yüzde
                char percent_text[8];
                sprintf(percent_text, "%%%d", storage_manager.format_progress);
                gui_draw_text(window, percent_text, 590, y, COLOR_BLACK);
            }
        }
    }
}

// USB aygıt sınıfları
typedef enum {
    USB_CLASS_UNKNOWN = 0,
    USB_CLASS_AUDIO = 1,
    USB_CLASS_COMM = 2,
    USB_CLASS_HID = 3,
    USB_CLASS_PHYSICAL = 5,
    USB_CLASS_IMAGE = 6,
    USB_CLASS_PRINTER = 7,
    USB_CLASS_MASS_STORAGE = 8,
    USB_CLASS_HUB = 9,
    USB_CLASS_CDC_DATA = 10,
    USB_CLASS_SMART_CARD = 11,
    USB_CLASS_VIDEO = 14,
    USB_CLASS_WIRELESS = 224,
    USB_CLASS_VENDOR_SPECIFIC = 255
} usb_class_t;

// USB aygıt yapısı
typedef struct {
    char name[64];               // Cihaz adı
    char manufacturer[64];       // Üretici adı
    char product[64];            // Ürün adı
    char serial[32];             // Seri numarası
    char vendor_id[8];           // Üretici ID (örn: 0x8086)
    char product_id[8];          // Ürün ID (örn: 0x1234)
    usb_class_t device_class;    // Cihaz sınıfı
    uint8_t is_attached;         // Takılı mı?
    uint8_t is_configured;       // Yapılandırılmış mı?
    uint8_t has_driver;          // Sürücü var mı?
    uint8_t port_number;         // Port numarası
    uint8_t is_root_hub;         // Kök hub mu?
} usb_device_t;

// USB sürücü yapısı
typedef struct {
    char name[64];               // Sürücü adı
    usb_class_t device_class;    // Desteklenen cihaz sınıfı
    char vendor_ids[10][8];      // Desteklenen üretici ID'leri
    char product_ids[10][8];     // Desteklenen ürün ID'leri
    uint8_t id_count;            // ID eşleşme sayısı
    uint8_t is_installed;        // Kurulu mu?
} usb_driver_t;

// Otomatik tanıma hata yapısı
typedef struct {
    int error_code;              // Hata kodu
    char device_id[32];          // Cihaz ID
    char error_message[128];     // Hata mesajı
    char solution[256];          // Çözüm önerisi
    uint8_t is_resolved;         // Çözüldü mü?
} auto_detection_issue_t;

// USB aygıt izleme yapısı
typedef struct {
    usb_device_t* devices;       // USB aygıtları
    uint32_t device_count;       // Aygıt sayısı
    uint32_t selected_device;    // Seçili aygıt
    usb_driver_t* drivers;       // USB sürücüleri
    uint32_t driver_count;       // Sürücü sayısı
    auto_detection_issue_t* issues; // Algılanan sorunlar
    uint32_t issue_count;        // Sorun sayısı
    uint8_t is_scanning;         // Tarama yapılıyor mu?
    uint8_t scan_progress;       // Tarama ilerleme durumu
    uint8_t auto_fix_enabled;    // Otomatik düzeltme aktif mi?
    uint8_t ai_assist_enabled;   // Yapay zeka asistanı aktif mi?
} usb_monitor_t;

// Demo USB cihazları
static usb_device_t demo_usb_devices[] = {
    {
        .name = "Kingston DataTraveler",
        .manufacturer = "Kingston",
        .product = "DataTraveler 3.0",
        .serial = "KT00001",
        .vendor_id = "0x0951",
        .product_id = "0x1666",
        .device_class = USB_CLASS_MASS_STORAGE,
        .is_attached = 1,
        .is_configured = 1,
        .has_driver = 1,
        .port_number = 1,
        .is_root_hub = 0
    },
    {
        .name = "Logitech USB Mouse",
        .manufacturer = "Logitech",
        .product = "M185 Wireless Mouse",
        .serial = "LT00242",
        .vendor_id = "0x046d",
        .product_id = "0xc52b",
        .device_class = USB_CLASS_HID,
        .is_attached = 1,
        .is_configured = 1,
        .has_driver = 1,
        .port_number = 2,
        .is_root_hub = 0
    },
    {
        .name = "Generic USB Keyboard",
        .manufacturer = "Generic",
        .product = "USB Keyboard",
        .serial = "KB12345",
        .vendor_id = "0x04d9",
        .product_id = "0x1503",
        .device_class = USB_CLASS_HID,
        .is_attached = 1,
        .is_configured = 1,
        .has_driver = 1,
        .port_number = 3,
        .is_root_hub = 0
    },
    {
        .name = "Canon Inkjet Printer",
        .manufacturer = "Canon",
        .product = "PIXMA MG3650",
        .serial = "CN98765",
        .vendor_id = "0x04a9",
        .product_id = "0x176d",
        .device_class = USB_CLASS_PRINTER,
        .is_attached = 1,
        .is_configured = 0,
        .has_driver = 0,
        .port_number = 4,
        .is_root_hub = 0
    },
    {
        .name = "USB Root Hub",
        .manufacturer = "Intel",
        .product = "USB xHCI Root Hub",
        .serial = "0000",
        .vendor_id = "0x8086",
        .product_id = "0x8c31",
        .device_class = USB_CLASS_HUB,
        .is_attached = 1,
        .is_configured = 1,
        .has_driver = 1,
        .port_number = 0,
        .is_root_hub = 1
    }
};

// Demo USB sürücüleri
static usb_driver_t demo_usb_drivers[] = {
    {
        .name = "KALEM OS USB Depolama Sürücüsü",
        .device_class = USB_CLASS_MASS_STORAGE,
        .is_installed = 1,
        .id_count = 1,
        .vendor_ids = {"0x0951"},
        .product_ids = {"0x1666"}
    },
    {
        .name = "KALEM OS USB HID Sürücüsü",
        .device_class = USB_CLASS_HID,
        .is_installed = 1,
        .id_count = 2,
        .vendor_ids = {"0x046d", "0x04d9"},
        .product_ids = {"0xc52b", "0x1503"}
    },
    {
        .name = "KALEM OS USB Yazıcı Sürücüsü",
        .device_class = USB_CLASS_PRINTER,
        .is_installed = 0,
        .id_count = 1,
        .vendor_ids = {"0x04a9"},
        .product_ids = {"0x176d"}
    },
    {
        .name = "KALEM OS USB Hub Sürücüsü",
        .device_class = USB_CLASS_HUB,
        .is_installed = 1,
        .id_count = 1,
        .vendor_ids = {"0x8086"},
        .product_ids = {"0x8c31"}
    }
};

// Demo algılanan sorunlar
static auto_detection_issue_t demo_issues[] = {
    {
        .error_code = 1001,
        .device_id = "0x04a9:0x176d",
        .error_message = "Canon PIXMA yazıcı sürücüsü bulunamadı",
        .solution = "Canon PIXMA yazıcı sürücüsünü otomatik olarak indir ve kur.",
        .is_resolved = 0
    },
    {
        .error_code = 2002,
        .device_id = "0x0951:0x1666",
        .error_message = "Kingston USB bellek okuma performansı düşük",
        .solution = "USB bellek sürücüsünü güncelleyerek performans iyileştirmesi sağla.",
        .is_resolved = 0
    }
};

// USB İzleme
static usb_monitor_t usb_monitor = {0};

// USB İzleme başlat
void settings_usb_monitor_init() {
    // Demo USB aygıtlarını kopyala
    usb_monitor.device_count = sizeof(demo_usb_devices) / sizeof(usb_device_t);
    usb_monitor.devices = malloc(sizeof(usb_device_t) * usb_monitor.device_count);
    
    if (usb_monitor.devices) {
        memcpy(usb_monitor.devices, demo_usb_devices, sizeof(usb_device_t) * usb_monitor.device_count);
    }
    
    // Demo USB sürücülerini kopyala
    usb_monitor.driver_count = sizeof(demo_usb_drivers) / sizeof(usb_driver_t);
    usb_monitor.drivers = malloc(sizeof(usb_driver_t) * usb_monitor.driver_count);
    
    if (usb_monitor.drivers) {
        memcpy(usb_monitor.drivers, demo_usb_drivers, sizeof(usb_driver_t) * usb_monitor.driver_count);
    }
    
    // Demo sorunları kopyala
    usb_monitor.issue_count = sizeof(demo_issues) / sizeof(auto_detection_issue_t);
    usb_monitor.issues = malloc(sizeof(auto_detection_issue_t) * usb_monitor.issue_count);
    
    if (usb_monitor.issues) {
        memcpy(usb_monitor.issues, demo_issues, sizeof(auto_detection_issue_t) * usb_monitor.issue_count);
    }
    
    // Varsayılan ayarlar
    usb_monitor.selected_device = 0;
    usb_monitor.is_scanning = 0;
    usb_monitor.scan_progress = 0;
    usb_monitor.auto_fix_enabled = 1;  // Otomatik düzeltme aktif
    usb_monitor.ai_assist_enabled = 1; // Yapay zeka asistanı aktif
}

// USB cihaz sınıfı adını al
const char* settings_get_usb_class_name(usb_class_t device_class) {
    switch (device_class) {
        case USB_CLASS_AUDIO:           return "Ses";
        case USB_CLASS_COMM:            return "İletişim";
        case USB_CLASS_HID:             return "İnsan Arayüz Aygıtı";
        case USB_CLASS_PHYSICAL:        return "Fiziksel";
        case USB_CLASS_IMAGE:           return "Resim";
        case USB_CLASS_PRINTER:         return "Yazıcı";
        case USB_CLASS_MASS_STORAGE:    return "Depolama";
        case USB_CLASS_HUB:             return "Hub";
        case USB_CLASS_CDC_DATA:        return "CDC Veri";
        case USB_CLASS_SMART_CARD:      return "Akıllı Kart";
        case USB_CLASS_VIDEO:           return "Video";
        case USB_CLASS_WIRELESS:        return "Kablosuz";
        case USB_CLASS_VENDOR_SPECIFIC: return "Üretici Özel";
        default:                        return "Bilinmeyen";
    }
}

// USB cihazlarını tara
void settings_scan_usb_devices() {
    usb_monitor.is_scanning = 1;
    usb_monitor.scan_progress = 0;
    
    // Demo sürümünde yalnızca zamanlayıcı simülasyonu yapar
    // Gerçek sistemde asenkron olarak tarayıp USB aygıtlarını bulur
    
    // İlerlemeyi 100'e tamamlamak için gerçek sistemde bir zamanlayıcı kullanılır
    usb_monitor.scan_progress = 100;
    usb_monitor.is_scanning = 0;
}

// USB cihazını konfigüre et
int settings_configure_usb_device(usb_device_t* device) {
    if (!device) return -1;
    
    // Gerçek bir sistemde cihazı yapılandırır
    device->is_configured = 1;
    
    return 0;
}

// USB sürücüsü kurulumu
int settings_install_usb_driver(usb_driver_t* driver) {
    if (!driver) return -1;
    
    // Gerçek bir sistemde sürücüyü kurar
    driver->is_installed = 1;
    
    // İlgili cihazları güncelle
    for (uint32_t i = 0; i < usb_monitor.device_count; i++) {
        usb_device_t* device = &usb_monitor.devices[i];
        
        // Aynı sınıftaki cihazlar için
        if (device->device_class == driver->device_class) {
            // Vendor ID ve Product ID eşleşmelerini kontrol et
            for (uint8_t j = 0; j < driver->id_count; j++) {
                if (strcmp(device->vendor_id, driver->vendor_ids[j]) == 0 &&
                    strcmp(device->product_id, driver->product_ids[j]) == 0) {
                    device->has_driver = 1;
                    break;
                }
            }
        }
    }
    
    return 0;
}

// USB sürücüsü güncelle
int settings_update_usb_driver(usb_driver_t* driver) {
    // Gerçek bir sistemde sürücüyü günceller
    return settings_install_usb_driver(driver);
}

// USB sorununu çöz
int settings_resolve_usb_issue(auto_detection_issue_t* issue) {
    if (!issue) return -1;
    
    // Gerçek bir sistemde sorunu çöz
    issue->is_resolved = 1;
    
    // İlgili cihazı bul ve güncelle
    char vendor_id[8];
    char product_id[8];
    
    // "0xVVVV:0xPPPP" formatından ID'leri ayır
    sscanf(issue->device_id, "%7[^:]:%7s", vendor_id, product_id);
    
    // İlgili cihazı bul
    for (uint32_t i = 0; i < usb_monitor.device_count; i++) {
        usb_device_t* device = &usb_monitor.devices[i];
        
        if (strcmp(device->vendor_id, vendor_id) == 0 &&
            strcmp(device->product_id, product_id) == 0) {
            
            // Sorun tipine göre cihazı güncelle
            if (issue->error_code == 1001) {
                // Sürücü eksikliği sorunu
                device->has_driver = 1;
                
                // İlgili sürücüyü bul ve kur
                for (uint32_t j = 0; j < usb_monitor.driver_count; j++) {
                    usb_driver_t* driver = &usb_monitor.drivers[j];
                    
                    if (device->device_class == driver->device_class) {
                        driver->is_installed = 1;
                        break;
                    }
                }
            }
            else if (issue->error_code == 2002) {
                // Performans sorunu - sürücüyü güncelle
                for (uint32_t j = 0; j < usb_monitor.driver_count; j++) {
                    usb_driver_t* driver = &usb_monitor.drivers[j];
                    
                    if (device->device_class == driver->device_class &&
                        driver->is_installed) {
                        // Sürücüyü simüle et
                        break;
                    }
                }
            }
            
            break;
        }
    }
    
    return 0;
}

// Tüm USB sorunlarını otomatik çöz
void settings_auto_resolve_all_issues() {
    for (uint32_t i = 0; i < usb_monitor.issue_count; i++) {
        auto_detection_issue_t* issue = &usb_monitor.issues[i];
        
        if (!issue->is_resolved) {
            settings_resolve_usb_issue(issue);
        }
    }
}

// USB aygıtının sürücüsünü bul ve kur
usb_driver_t* settings_find_driver_for_device(usb_device_t* device) {
    if (!device) return NULL;
    
    for (uint32_t i = 0; i < usb_monitor.driver_count; i++) {
        usb_driver_t* driver = &usb_monitor.drivers[i];
        
        // Cihaz sınıfı eşleşiyor mu?
        if (device->device_class == driver->device_class) {
            // Vendor ID ve Product ID eşleşiyor mu?
            for (uint8_t j = 0; j < driver->id_count; j++) {
                if (strcmp(device->vendor_id, driver->vendor_ids[j]) == 0 &&
                    strcmp(device->product_id, driver->product_ids[j]) == 0) {
                    return driver;
                }
            }
        }
    }
    
    return NULL;
}

// Yapay zeka destekli otomatik sürücü kurulumu
void settings_ai_assisted_driver_setup() {
    // Bu fonksiyon gerçek bir sistemde yapay zeka algoritmaları kullanarak
    // USB cihazlarını analiz eder ve en uygun sürücüleri belirler

    // Her cihaz için uygun sürücüleri belirle ve kur
    for (uint32_t i = 0; i < usb_monitor.device_count; i++) {
        usb_device_t* device = &usb_monitor.devices[i];
        
        // Henüz sürücüsü olmayan cihazlar için
        if (!device->has_driver) {
            usb_driver_t* driver = settings_find_driver_for_device(device);
            
            if (driver) {
                // Sürücüyü kur
                settings_install_usb_driver(driver);
                device->has_driver = 1;
            }
            else {
                // Eşleşen sürücü yoksa, yeni bir sorun ekle
                // Gerçek sistemde yeni bir sorun oluşturulur ve çözülür
            }
        }
    }
    
    // Tüm tanınan sorunları çözmeye çalış
    if (usb_monitor.auto_fix_enabled) {
        settings_auto_resolve_all_issues();
    }
}

// USB Aygıtları sayfası
void settings_paint_usb_devices(gui_window_t* window) {
    if (!window || !active_settings) return;
    
    // Başlık
    gui_draw_text(window, "USB Aygıtları Yönetimi", 180, 50, COLOR_BLACK);
    
    // Akıllı tanıma durum bilgisi
    char ai_status[64];
    sprintf(ai_status, "Yapay Zeka Asistanı: %s", 
            usb_monitor.ai_assist_enabled ? "Aktif" : "Pasif");
    gui_draw_text(window, ai_status, 430, 50, usb_monitor.ai_assist_enabled ? COLOR_DARK_GREEN : COLOR_DARK_RED);
    
    // Tarama butonu
    gui_draw_button(window, 180, 80, 200, 30, "USB Aygıtlarını Tara", COLOR_BLUE, COLOR_WHITE);
    
    // Otomatik düzeltme
    gui_draw_text(window, "Otomatik Sorun Çözme:", 400, 85, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 550, 85, usb_monitor.auto_fix_enabled);
    
    // USB aygıtları
    gui_draw_text(window, "USB Aygıtları:", 180, 120, COLOR_DARK_GRAY);
    
    if (usb_monitor.device_count == 0 || !usb_monitor.devices) {
        gui_draw_text(window, "USB aygıtı bulunamadı.", 200, 150, COLOR_DARK_RED);
        return;
    }
    
    // Tarama durumu
    if (usb_monitor.is_scanning) {
        gui_draw_text(window, "Taranıyor...", 320, 120, COLOR_BLUE);
        
        // İlerleme çubuğu
        int progress_width = 240;
        int filled_width = (usb_monitor.scan_progress * progress_width) / 100;
        
        // Arka plan
        gui_draw_rect(window, 400, 120, progress_width, 15, COLOR_LIGHT_GRAY);
        
        // Doldurulmuş kısım
        gui_draw_rect(window, 400, 120, filled_width, 15, COLOR_BLUE);
    }
    
    // Aygıt listesi
    int y = 150;
    for (uint32_t i = 0; i < usb_monitor.device_count; i++) {
        usb_device_t* device = &usb_monitor.devices[i];
        
        // Aygıt seçili mi?
        int bg_color = (i == usb_monitor.selected_device) ? COLOR_LIGHT_BLUE : COLOR_LIGHT_GRAY;
        
        // Aygıt bilgisi
        char name[128];
        snprintf(name, sizeof(name), "%s - %s %s", 
                device->name, device->manufacturer, device->product);
        
        // Aygıt butonunu çiz
        gui_draw_rect(window, 180, y, 400, 25, bg_color);
        gui_draw_text(window, name, 190, y + 5, COLOR_BLACK);
        
        // Sürücü durumu
        int status_color = device->has_driver ? COLOR_DARK_GREEN : COLOR_DARK_RED;
        gui_draw_text(window, device->has_driver ? "Sürücü Var" : "Sürücü Yok", 
                     500, y + 5, status_color);
        
        y += 30;
    }
    
    // Seçili aygıt detayları
    if (usb_monitor.selected_device < usb_monitor.device_count) {
        usb_device_t* device = &usb_monitor.devices[usb_monitor.selected_device];
        
        y += 20;
        gui_draw_text(window, "Seçili Aygıt Detayları:", 180, y, COLOR_BLACK);
        y += 25;
        
        // Üretici ve ürün
        char mfg_text[128];
        snprintf(mfg_text, sizeof(mfg_text), "Üretici: %s", device->manufacturer);
        gui_draw_text(window, mfg_text, 200, y, COLOR_DARK_GRAY);
        y += 20;
        
        char prod_text[128];
        snprintf(prod_text, sizeof(prod_text), "Ürün: %s", device->product);
        gui_draw_text(window, prod_text, 200, y, COLOR_DARK_GRAY);
        y += 20;
        
        // Seri no
        char serial_text[128];
        snprintf(serial_text, sizeof(serial_text), "Seri No: %s", device->serial);
        gui_draw_text(window, serial_text, 200, y, COLOR_DARK_GRAY);
        y += 20;
        
        // USB sınıfı
        char class_text[128];
        snprintf(class_text, sizeof(class_text), "Cihaz Sınıfı: %s", 
                settings_get_usb_class_name(device->device_class));
        gui_draw_text(window, class_text, 200, y, COLOR_DARK_GRAY);
        y += 20;
        
        // Vendor ve product ID
        char id_text[128];
        snprintf(id_text, sizeof(id_text), "Vendor ID: %s, Product ID: %s", 
                device->vendor_id, device->product_id);
        gui_draw_text(window, id_text, 200, y, COLOR_DARK_GRAY);
        y += 20;
        
        // Port ve hub bilgisi
        char port_text[128];
        snprintf(port_text, sizeof(port_text), "Port: %d, %s", 
                device->port_number, 
                device->is_root_hub ? "Kök Hub" : "Cihaz");
        gui_draw_text(window, port_text, 200, y, COLOR_DARK_GRAY);
        y += 20;
        
        // Durum bilgisi
        char status_text[128];
        snprintf(status_text, sizeof(status_text), "Durum: %s, %s, %s", 
                device->is_attached ? "Takılı" : "Takılı Değil",
                device->is_configured ? "Yapılandırılmış" : "Yapılandırılmamış",
                device->has_driver ? "Sürücü Kurulu" : "Sürücü Eksik");
        gui_draw_text(window, status_text, 200, y, COLOR_DARK_GRAY);
        y += 30;
        
        // İşlem butonları
        if (!device->is_configured) {
            gui_draw_button(window, 200, y, 150, 25, "Yapılandır", COLOR_BLUE, COLOR_WHITE);
        }
        
        if (!device->has_driver) {
            gui_draw_button(window, 360, y, 150, 25, "Sürücü Kur", COLOR_BLUE, COLOR_WHITE);
        } else {
            gui_draw_button(window, 360, y, 150, 25, "Sürücü Güncelle", COLOR_GREEN, COLOR_WHITE);
        }
    }
    
    // Aygıt sorunları
    y += 40;
    gui_draw_text(window, "Tespit Edilen Sorunlar:", 180, y, COLOR_BLACK);
    y += 25;
    
    int has_issues = 0;
    for (uint32_t i = 0; i < usb_monitor.issue_count; i++) {
        auto_detection_issue_t* issue = &usb_monitor.issues[i];
        
        // Çözülmemiş sorunları göster
        if (!issue->is_resolved) {
            has_issues = 1;
            gui_draw_rect(window, 180, y, 400, 60, COLOR_LIGHT_RED);
            
            // Hata mesajı
            gui_draw_text(window, issue->error_message, 190, y + 5, COLOR_DARK_RED);
            
            // Çözüm önerisi
            gui_draw_text(window, issue->solution, 190, y + 25, COLOR_DARK_GRAY);
            
            // Çöz butonu
            gui_draw_button(window, 500, y + 15, 80, 30, "Çöz", COLOR_BLUE, COLOR_WHITE);
            
            y += 70;
        }
    }
    
    if (!has_issues) {
        gui_draw_text(window, "Tespit edilen sorun yok", 200, y, COLOR_DARK_GREEN);
    }
    
    // Yapay zeka asistanı butonları
    y += 40;
    gui_draw_text(window, "Yapay Zeka Asistanı:", 180, y, COLOR_BLACK);
    y += 25;
    
    gui_draw_button(window, 180, y, 180, 30, "Akıllı Tanıma Başlat", COLOR_DARK_GREEN, COLOR_WHITE);
    gui_draw_button(window, 370, y, 190, 30, "Tüm Sorunları Otomatik Çöz", COLOR_DARK_GREEN, COLOR_WHITE);
}

// İşlemci mimarisi türleri
typedef enum {
    CPU_ARCH_X86,
    CPU_ARCH_X64,
    CPU_ARCH_ARM,
    CPU_ARCH_ARM64,
    CPU_ARCH_UNKNOWN
} cpu_architecture_t;

// İşlemci ailesi
typedef enum {
    CPU_FAMILY_INTEL,
    CPU_FAMILY_AMD,
    CPU_FAMILY_ARM,
    CPU_FAMILY_OTHER
} cpu_family_t;

// İşlemci performans sınıfı
typedef enum {
    CPU_CLASS_ENTRY,     // Giriş seviyesi
    CPU_CLASS_MAINSTREAM,// Ana akım
    CPU_CLASS_PERFORMANCE,// Yüksek performans
    CPU_CLASS_ENTERPRISE // Kurumsal/Sunucu sınıfı
} cpu_performance_class_t;

// İşlemci optimizasyon profilleri
typedef enum {
    CPU_PROFILE_POWER_SAVE,  // Güç tasarrufu
    CPU_PROFILE_BALANCED,    // Dengeli
    CPU_PROFILE_PERFORMANCE, // Performans
    CPU_PROFILE_TURBO,       // Maksimum performans
    CPU_PROFILE_CUSTOM       // Özel
} cpu_profile_t;

// İşlemci özellikleri yapısı
typedef struct {
    uint8_t sse;     // SSE desteği
    uint8_t sse2;    // SSE2 desteği
    uint8_t sse3;    // SSE3 desteği
    uint8_t ssse3;   // SSSE3 desteği
    uint8_t sse4_1;  // SSE4.1 desteği
    uint8_t sse4_2;  // SSE4.2 desteği
    uint8_t avx;     // AVX desteği
    uint8_t avx2;    // AVX2 desteği
    uint8_t avx512;  // AVX-512 desteği
    uint8_t mmx;     // MMX desteği
    uint8_t aes;     // AES desteği
    uint8_t sha;     // SHA desteği
    uint8_t f16c;    // F16C desteği
    uint8_t fma;     // FMA desteği
    uint8_t vt;      // Sanallaştırma desteği
    uint8_t ht;      // Hyper-Threading desteği
    uint8_t turbo;   // Turbo Boost/Turbo Core desteği
} cpu_features_t;

// İşlemci bilgileri yapısı 
typedef struct {
    char vendor[16];                 // İşlemci üreticisi (Intel, AMD vb.)
    char brand[64];                  // İşlemci tam adı
    char model_name[64];             // Model adı
    uint32_t family;                 // İşlemci ailesi
    uint32_t model;                  // Model numarası
    uint32_t stepping;               // Stepping
    uint32_t cores;                  // Çekirdek sayısı
    uint32_t threads;                // İş parçacığı sayısı
    uint32_t base_freq_mhz;          // Taban frekans
    uint32_t max_freq_mhz;           // Maksimum frekans
    uint32_t cache_l1;               // L1 önbellek boyutu (KB)
    uint32_t cache_l2;               // L2 önbellek boyutu (KB)
    uint32_t cache_l3;               // L3 önbellek boyutu (KB)
    uint32_t tdp;                    // TDP değeri (Watt)
    uint8_t is_64bit;                // 64-bit desteği
    cpu_architecture_t architecture; // Mimari
    cpu_family_t family_type;        // Aile tipi
    cpu_performance_class_t performance_class; // Performans sınıfı
    cpu_features_t features;         // İşlemci özellikleri
} cpu_info_t;

// İşlemci yöneticisi yapısı
typedef struct {
    cpu_info_t cpu;                  // İşlemci bilgileri
    cpu_profile_t current_profile;   // Mevcut optimizasyon profili
    uint8_t is_optimized;            // Optimize edilmiş mi
    uint8_t auto_optimization;       // Otomatik optimizasyon aktif mi
    uint32_t current_freq;           // Anlık frekans
    uint8_t current_usage;           // İşlemci kullanımı (%)
    uint8_t current_temp;            // Sıcaklık
    uint8_t cpu_governor;            // CPU yönetici (0: ondemand, 1: performance, 2: powersave)
    uint8_t process_priority;        // Süreç önceliği (0-5)
} cpu_manager_t;

// Intel işlemci optimize edilmiş ayarları
static const uint8_t intel_optimized_settings[][4] = {
    // Güç, Denge, Performans, Turbo (her satır farklı bir ayar)
    {20, 40, 80, 100},  // İşlemci gücü (%)
    {0, 1, 2, 3},       // Öncelik (0: düşük, 3: yüksek)
    {1, 2, 3, 3},       // İş çizelgesi (1: güç tasarrufu, 3: performans)
    {0, 0, 1, 1}        // Turbo mod (0: kapalı, 1: açık)
};

// AMD işlemci optimize edilmiş ayarları
static const uint8_t amd_optimized_settings[][4] = {
    // Güç, Denge, Performans, Turbo (her satır farklı bir ayar)
    {20, 40, 80, 100},  // İşlemci gücü (%)
    {0, 1, 2, 3},       // Öncelik (0: düşük, 3: yüksek)
    {1, 2, 3, 3},       // İş çizelgesi (1: güç tasarrufu, 3: performans)
    {0, 0, 1, 1}        // Turbo mod (0: kapalı, 1: açık)
};

// CPU yöneticisi
static cpu_manager_t cpu_manager = {0};

// Intel CPU mimarileri ve optimizasyon ayarları
static const struct {
    char code_name[32];     // Kod adı
    uint32_t base_model;    // Temel model numarası
    char optimization[128]; // Özel optimizasyon notları
} intel_architectures[] = {
    {"Skylake", 0x5E, "AVX2 optimizasyonları, güç yönetimi iyileştirmeleri"},
    {"Kaby Lake", 0x9E, "Medya işleme hızlandırıcıları, güç verimliliği"},
    {"Coffee Lake", 0x9E, "Yüksek çekirdek sayısı için iş parçacığı yöneticisi optimizasyonu"},
    {"Comet Lake", 0xA5, "Güç limitlemesi, termal yönetim optimizasyonları"},
    {"Ice Lake", 0x7E, "AVX-512 desteği, yapay zeka hızlandırma optimizasyonları"},
    {"Tiger Lake", 0x8C, "Thunderbolt I/O optimizasyonları, güç yönetimi"},
    {"Rocket Lake", 0xA7, "PCIe 4.0 optimizasyonları, bellek kontrolcü iyileştirmeleri"},
    {"Alder Lake", 0x97, "Hibrit çekirdek mimarisi için P-core/E-core planlayıcısı"}
};

// AMD CPU mimarileri ve optimizasyon ayarları
static const struct {
    char code_name[32];     // Kod adı
    uint32_t base_model;    // Temel model numarası
    char optimization[128]; // Özel optimizasyon notları
} amd_architectures[] = {
    {"Zen", 0x1, "SMT planlayıcı optimizasyonları, CCX bellek erişimi"},
    {"Zen+", 0x8, "Precision Boost 2, bellek alt sistemi iyileştirmeleri"},
    {"Zen 2", 0x31, "CCD/CCX mimarisi için NUMA optimizasyonları"},
    {"Zen 3", 0x21, "Birleşik L3 önbellek, düşük gecikme optimizasyonları"},
    {"Zen 4", 0x61, "AVX-512 desteği, önceden getirme algoritmaları, ısı yönetimi"}
};

// Örnek Intel CPU modelleri
static const struct {
    char name[64];
    cpu_performance_class_t class;
    uint32_t cores;
    uint32_t threads;
    uint32_t base_freq_mhz;
    uint32_t max_freq_mhz;
} intel_cpu_models[] = {
    {"Intel Celeron N4000", CPU_CLASS_ENTRY, 2, 2, 1100, 2600},
    {"Intel Pentium Gold G5400", CPU_CLASS_ENTRY, 2, 4, 3700, 3700},
    {"Intel Core i3-10100", CPU_CLASS_MAINSTREAM, 4, 8, 3600, 4300},
    {"Intel Core i5-11600K", CPU_CLASS_PERFORMANCE, 6, 12, 3900, 4900},
    {"Intel Core i7-12700K", CPU_CLASS_PERFORMANCE, 12, 20, 3600, 5000},
    {"Intel Core i9-13900K", CPU_CLASS_PERFORMANCE, 24, 32, 3000, 5800},
    {"Intel Xeon E5-2690 v4", CPU_CLASS_ENTERPRISE, 14, 28, 2600, 3500}
};

// Örnek AMD CPU modelleri
static const struct {
    char name[64];
    cpu_performance_class_t class;
    uint32_t cores;
    uint32_t threads;
    uint32_t base_freq_mhz;
    uint32_t max_freq_mhz;
} amd_cpu_models[] = {
    {"AMD Athlon 3000G", CPU_CLASS_ENTRY, 2, 4, 3500, 3500},
    {"AMD Ryzen 3 3200G", CPU_CLASS_ENTRY, 4, 4, 3600, 4000},
    {"AMD Ryzen 5 5600X", CPU_CLASS_MAINSTREAM, 6, 12, 3700, 4600},
    {"AMD Ryzen 7 5800X", CPU_CLASS_PERFORMANCE, 8, 16, 3800, 4700},
    {"AMD Ryzen 9 7950X", CPU_CLASS_PERFORMANCE, 16, 32, 4500, 5700},
    {"AMD Threadripper 3970X", CPU_CLASS_PERFORMANCE, 32, 64, 3700, 4500},
    {"AMD EPYC 7763", CPU_CLASS_ENTERPRISE, 64, 128, 2450, 3500}
};

// CPU algılama ve tanımlama işlemi - Daha kapsamlı mimari desteği
void settings_detect_cpu() {
    // Gerçek sistemde CPUID ile işlemci bilgileri alınır
    // Örnek Intel/AMD işlemci bilgileri
    
    // Tespit edilen CPU, Intel mı AMD mi?
    // Demo amacıyla rastgele bir seçim yapalım
    int is_intel = 1; // 1: Intel, 0: AMD
    
    if (is_intel) {
        // Intel işlemcisi
        strcpy(cpu_manager.cpu.vendor, "GenuineIntel");
        strcpy(cpu_manager.cpu.brand, "Intel(R) Core(TM) i5-11600K CPU @ 3.90GHz");
        strcpy(cpu_manager.cpu.model_name, "Rocket Lake");
        
        cpu_manager.cpu.family = 6;
        cpu_manager.cpu.model = 167;
        cpu_manager.cpu.stepping = 1;
        
        cpu_manager.cpu.cores = 6;
        cpu_manager.cpu.threads = 12;
        
        cpu_manager.cpu.base_freq_mhz = 3900;
        cpu_manager.cpu.max_freq_mhz = 4900;
        
        cpu_manager.cpu.cache_l1 = 384; // KB
        cpu_manager.cpu.cache_l2 = 3072; // KB
        cpu_manager.cpu.cache_l3 = 12288; // KB
        
        cpu_manager.cpu.tdp = 125; // Watt
        
        cpu_manager.cpu.family_type = CPU_FAMILY_INTEL;
        cpu_manager.cpu.performance_class = CPU_CLASS_PERFORMANCE;
    }
    else {
        // AMD işlemcisi
        strcpy(cpu_manager.cpu.vendor, "AuthenticAMD");
        strcpy(cpu_manager.cpu.brand, "AMD Ryzen 7 5800X 8-Core Processor");
        strcpy(cpu_manager.cpu.model_name, "Zen 3");
        
        cpu_manager.cpu.family = 25;
        cpu_manager.cpu.model = 33;
        cpu_manager.cpu.stepping = 0;
        
        cpu_manager.cpu.cores = 8;
        cpu_manager.cpu.threads = 16;
        
        cpu_manager.cpu.base_freq_mhz = 3800;
        cpu_manager.cpu.max_freq_mhz = 4700;
        
        cpu_manager.cpu.cache_l1 = 512; // KB
        cpu_manager.cpu.cache_l2 = 4096; // KB
        cpu_manager.cpu.cache_l3 = 32768; // KB
        
        cpu_manager.cpu.tdp = 105; // Watt
        
        cpu_manager.cpu.family_type = CPU_FAMILY_AMD;
        cpu_manager.cpu.performance_class = CPU_CLASS_PERFORMANCE;
    }
    
    // Her iki işlemci için de geçerli ortak özellikler
    cpu_manager.cpu.is_64bit = 1;
    cpu_manager.cpu.architecture = CPU_ARCH_X64;
    
    // İşlemci özellikleri
    cpu_manager.cpu.features.sse = 1;
    cpu_manager.cpu.features.sse2 = 1;
    cpu_manager.cpu.features.sse3 = 1;
    cpu_manager.cpu.features.ssse3 = 1;
    cpu_manager.cpu.features.sse4_1 = 1;
    cpu_manager.cpu.features.sse4_2 = 1;
    cpu_manager.cpu.features.avx = 1;
    cpu_manager.cpu.features.avx2 = 1;
    
    // İşlemci ailesine göre farklı özellikler
    if (cpu_manager.cpu.family_type == CPU_FAMILY_INTEL) {
        cpu_manager.cpu.features.avx512 = (strcmp(cpu_manager.cpu.model_name, "Ice Lake") == 0 || 
                                          strcmp(cpu_manager.cpu.model_name, "Tiger Lake") == 0 ||
                                          strcmp(cpu_manager.cpu.model_name, "Rocket Lake") == 0);
        cpu_manager.cpu.features.turbo = 1; // Intel Turbo Boost
    }
    else if (cpu_manager.cpu.family_type == CPU_FAMILY_AMD) {
        cpu_manager.cpu.features.avx512 = (strcmp(cpu_manager.cpu.model_name, "Zen 4") == 0);
        cpu_manager.cpu.features.turbo = 1; // AMD Turbo Core / Precision Boost
    }
    
    cpu_manager.cpu.features.mmx = 1;
    cpu_manager.cpu.features.aes = 1;
    cpu_manager.cpu.features.sha = 1;
    cpu_manager.cpu.features.f16c = 1;
    cpu_manager.cpu.features.fma = 1;
    cpu_manager.cpu.features.vt = 1;
    cpu_manager.cpu.features.ht = 1;
    
    // Varsayılan profil ve durumlar
    cpu_manager.current_profile = CPU_PROFILE_BALANCED;
    cpu_manager.is_optimized = 0;
    cpu_manager.auto_optimization = 1;
    cpu_manager.current_freq = cpu_manager.cpu.base_freq_mhz;
    cpu_manager.current_usage = 25;
    cpu_manager.current_temp = 45;
    cpu_manager.cpu_governor = 1; // Dengeli
    cpu_manager.process_priority = 1; // Normal
}

// Daha ayrıntılı CPU optimizasyonu
void settings_optimize_cpu_for_stability() {
    // İşlemci ailesine ve performans sınıfına göre optimize et
    
    // İlk olarak önemli işlemci özelliklerini belirle
    int has_avx = cpu_manager.cpu.features.avx;
    int has_avx2 = cpu_manager.cpu.features.avx2;
    int has_avx512 = cpu_manager.cpu.features.avx512;
    int has_turbo = cpu_manager.cpu.features.turbo;
    int multi_core = (cpu_manager.cpu.cores > 1);
    int high_thread_count = (cpu_manager.cpu.threads >= 8);
    
    // İşlemci mimarisine özgü optimizasyonlar
    const char* model_name = cpu_manager.cpu.model_name;
    
    // Kararlı çalışma için işlemci kullanımını optimize et
    
    // Düşük güçlü işlemciler için
    if (cpu_manager.cpu.performance_class == CPU_CLASS_ENTRY) {
        // Dengeli profil kullan, sistemi aşırı yükleme
        settings_set_cpu_profile(CPU_PROFILE_BALANCED);
        
        // Düşük güçlü işlemcilerde çalışma sıcaklığını düşük tutmak için
        // çekirdeğe özel işlemci optimizasyonları yapılır
        if (cpu_manager.cpu.family_type == CPU_FAMILY_INTEL) {
            // Intel Atom, Celeron, Pentium için optimizasyonlar
            // Çift çekirdekli düşük güçlü işlemciler için yükü dengele
        }
        else if (cpu_manager.cpu.family_type == CPU_FAMILY_AMD) {
            // AMD Athlon, düşük güçlü Ryzen için optimizasyonlar
        }
    }
    // Orta seviye işlemciler için
    else if (cpu_manager.cpu.performance_class == CPU_CLASS_MAINSTREAM) {
        // İşlemci özelliklerine göre optimal profil seç
        if (has_turbo && multi_core) {
            settings_set_cpu_profile(CPU_PROFILE_PERFORMANCE);
            
            // Özellik detaylarına göre ince ayar yap
            if (cpu_manager.cpu.family_type == CPU_FAMILY_INTEL) {
                // Intel Core i3/i5 için
                if (strcmp(model_name, "Coffee Lake") == 0 || 
                    strcmp(model_name, "Comet Lake") == 0) {
                    // Özel Coffee Lake/Comet Lake optimizasyonları
                }
            }
            else if (cpu_manager.cpu.family_type == CPU_FAMILY_AMD) {
                // AMD Ryzen 3/5 için
                if (strcmp(model_name, "Zen 2") == 0 || 
                    strcmp(model_name, "Zen 3") == 0) {
                    // CCX mimarisi için özel optimizasyonlar
                }
            }
        } else {
            settings_set_cpu_profile(CPU_PROFILE_BALANCED);
        }
    }
    // Yüksek performanslı işlemciler için
    else if (cpu_manager.cpu.performance_class == CPU_CLASS_PERFORMANCE || 
             cpu_manager.cpu.performance_class == CPU_CLASS_ENTERPRISE) {
        // Tüm özellikleri kullan
        settings_set_cpu_profile(CPU_PROFILE_TURBO);
        
        // Yüksek performanslı işlemcilerde soğutma önemlidir
        // Bu tür işlemcilerde termal yönetim daha kritiktir
        if (cpu_manager.cpu.family_type == CPU_FAMILY_INTEL) {
            if (high_thread_count && has_avx512) {
                // AVX-512 talimatları yüksek güç kullanır
                // Rocket Lake, Tiger Lake, Ice Lake için özel termal yönetim
            }
        }
        else if (cpu_manager.cpu.family_type == CPU_FAMILY_AMD) {
            if (strcmp(model_name, "Zen 3") == 0 || strcmp(model_name, "Zen 4") == 0) {
                // Zen 3/4 mimarisi için CCD/CCX optimizasyonları
                // Birleşik L3 önbellek kullanımı optimizasyonları
            }
        }
    }
    
    // Mimari düzeyinde özel optimizasyonlar
    if (cpu_manager.cpu.family_type == CPU_FAMILY_INTEL) {
        // Intel işlemciler için özel optimizasyonlar
        for (size_t i = 0; i < sizeof(intel_architectures) / sizeof(intel_architectures[0]); i++) {
            if (strcmp(model_name, intel_architectures[i].code_name) == 0) {
                // Mimari özel optimizasyonlar uygula
                break;
            }
        }
    }
    else if (cpu_manager.cpu.family_type == CPU_FAMILY_AMD) {
        // AMD işlemciler için özel optimizasyonlar
        for (size_t i = 0; i < sizeof(amd_architectures) / sizeof(amd_architectures[0]); i++) {
            if (strcmp(model_name, amd_architectures[i].code_name) == 0) {
                // Mimari özel optimizasyonlar uygula
                break;
            }
        }
    }
    
    cpu_manager.is_optimized = 1;
}

// CPU optimizasyon profilini ayarla
void settings_set_cpu_profile(cpu_profile_t profile) {
    if (profile >= CPU_PROFILE_POWER_SAVE && profile <= CPU_PROFILE_CUSTOM) {
        cpu_manager.current_profile = profile;
        
        // Gerçek sistemde optimizasyon ayarları uygulanır
        cpu_manager.is_optimized = 1;
        
        // İşlemci ailesine göre optimizasyon ayarlarını kullan
        const uint8_t (*settings)[4];
        
        if (cpu_manager.cpu.family_type == CPU_FAMILY_INTEL) {
            settings = intel_optimized_settings;
        } else if (cpu_manager.cpu.family_type == CPU_FAMILY_AMD) {
            settings = amd_optimized_settings;
        } else {
            return; // Desteklenmeyen işlemci
        }
        
        // Varsayılan güçlü ayarları kur
        if (profile != CPU_PROFILE_CUSTOM) {
            int profile_idx = (int)profile;
            
            // Farklı ayarları değiştir (gerçek sistemde daha fazla ayar olur)
            // İşlemci gücü
            int power = settings[0][profile_idx];
            
            // Öncelik seviyesi
            cpu_manager.process_priority = settings[1][profile_idx];
            
            // İş çizelgesi
            cpu_manager.cpu_governor = settings[2][profile_idx];
            
            // Turbo modu
            int turbo = settings[3][profile_idx];
            
            // Anlık frekansı ayarla
            if (turbo && cpu_manager.cpu.features.turbo) {
                cpu_manager.current_freq = cpu_manager.cpu.max_freq_mhz;
            } else {
                // İşlemci gücüne göre frekans ayarla
                uint32_t freq_range = cpu_manager.cpu.max_freq_mhz - cpu_manager.cpu.base_freq_mhz;
                cpu_manager.current_freq = cpu_manager.cpu.base_freq_mhz + (freq_range * power / 100);
            }
        }
    }
}

// İşlemci sınıfını metne dönüştür
const char* settings_get_cpu_class_name(cpu_performance_class_t cpu_class) {
    switch (cpu_class) {
        case CPU_CLASS_ENTRY:      return "Giriş Seviyesi";
        case CPU_CLASS_MAINSTREAM: return "Ana Akım";
        case CPU_CLASS_PERFORMANCE: return "Yüksek Performans";
        case CPU_CLASS_ENTERPRISE: return "Kurumsal/Sunucu";
        default:                   return "Bilinmeyen";
    }
}

// İşlemci profili adını al
const char* settings_get_cpu_profile_name(cpu_profile_t profile) {
    switch (profile) {
        case CPU_PROFILE_POWER_SAVE:  return "Güç Tasarrufu";
        case CPU_PROFILE_BALANCED:    return "Dengeli";
        case CPU_PROFILE_PERFORMANCE: return "Performans";
        case CPU_PROFILE_TURBO:       return "Turbo";
        case CPU_PROFILE_CUSTOM:      return "Özel";
        default:                      return "Bilinmeyen";
    }
}

// İşlemci yöneticisini başlat
void settings_cpu_manager_init() {
    // İşlemciyi algıla
    settings_detect_cpu();
    
    // Otomatik optimizasyon aktifse
    if (cpu_manager.auto_optimization) {
        // İşlemciyi en stabil çalışacak şekilde optimize et
        settings_optimize_cpu_for_stability();
    }
}

// İşlemci Yönetimi sayfası
void settings_paint_cpu(gui_window_t* window) {
    if (!window || !active_settings) return;
    
    // Başlık
    gui_draw_text(window, "İşlemci Optimizasyonu", 180, 50, COLOR_BLACK);
    
    // İşlemci bilgileri
    gui_draw_text(window, "İşlemci Bilgileri:", 180, 85, COLOR_DARK_GRAY);
    
    char cpu_name[128];
    snprintf(cpu_name, sizeof(cpu_name), "%s", cpu_manager.cpu.brand);
    gui_draw_text(window, cpu_name, 200, 110, COLOR_BLACK);
    
    char cpu_details[128];
    snprintf(cpu_details, sizeof(cpu_details), 
            "%d çekirdek, %d iş parçacığı, %d MHz - %d MHz",
            cpu_manager.cpu.cores, cpu_manager.cpu.threads,
            cpu_manager.cpu.base_freq_mhz, cpu_manager.cpu.max_freq_mhz);
    gui_draw_text(window, cpu_details, 200, 135, COLOR_DARK_GRAY);
    
    // İşlemci sınıfı
    char cpu_class[64];
    snprintf(cpu_class, sizeof(cpu_class), "Sınıf: %s", 
            settings_get_cpu_class_name(cpu_manager.cpu.performance_class));
    gui_draw_text(window, cpu_class, 200, 160, COLOR_DARK_GRAY);
    
    // İşlemci ailesine göre üretici logosu
    if (cpu_manager.cpu.family_type == CPU_FAMILY_INTEL) {
        gui_draw_text(window, "Intel®", 500, 110, COLOR_BLUE);
    }
    else if (cpu_manager.cpu.family_type == CPU_FAMILY_AMD) {
        gui_draw_text(window, "AMD Ryzen™", 500, 110, COLOR_RED);
    }
    
    // Özellikler
    gui_draw_text(window, "İşlemci Özellikleri:", 180, 190, COLOR_DARK_GRAY);
    
    int y = 215;
    int x = 200;
    
    if (cpu_manager.cpu.features.sse) {
        gui_draw_text(window, "SSE", x, y, COLOR_DARK_GRAY);
        x += 50;
    }
    if (cpu_manager.cpu.features.sse2) {
        gui_draw_text(window, "SSE2", x, y, COLOR_DARK_GRAY);
        x += 50;
    }
    if (cpu_manager.cpu.features.sse3) {
        gui_draw_text(window, "SSE3", x, y, COLOR_DARK_GRAY);
        x += 50;
    }
    if (cpu_manager.cpu.features.ssse3) {
        gui_draw_text(window, "SSSE3", x, y, COLOR_DARK_GRAY);
        x += 60;
    }
    if (cpu_manager.cpu.features.sse4_1) {
        gui_draw_text(window, "SSE4.1", x, y, COLOR_DARK_GRAY);
        x += 60;
    }
    if (cpu_manager.cpu.features.sse4_2) {
        gui_draw_text(window, "SSE4.2", x, y, COLOR_DARK_GRAY);
        x += 60;
    }
    
    x = 200;
    y += 25;
    
    if (cpu_manager.cpu.features.avx) {
        gui_draw_text(window, "AVX", x, y, COLOR_DARK_GRAY);
        x += 50;
    }
    if (cpu_manager.cpu.features.avx2) {
        gui_draw_text(window, "AVX2", x, y, COLOR_DARK_GRAY);
        x += 50;
    }
    if (cpu_manager.cpu.features.avx512) {
        gui_draw_text(window, "AVX-512", x, y, COLOR_DARK_GRAY);
        x += 70;
    }
    if (cpu_manager.cpu.features.mmx) {
        gui_draw_text(window, "MMX", x, y, COLOR_DARK_GRAY);
        x += 50;
    }
    if (cpu_manager.cpu.features.aes) {
        gui_draw_text(window, "AES", x, y, COLOR_DARK_GRAY);
        x += 50;
    }
    if (cpu_manager.cpu.features.sha) {
        gui_draw_text(window, "SHA", x, y, COLOR_DARK_GRAY);
        x += 50;
    }
    
    x = 200;
    y += 25;
    
    if (cpu_manager.cpu.features.vt) {
        gui_draw_text(window, "Sanallaştırma", x, y, COLOR_DARK_GRAY);
        x += 100;
    }
    if (cpu_manager.cpu.features.ht) {
        gui_draw_text(window, "Hyper-Threading", x, y, COLOR_DARK_GRAY);
        x += 120;
    }
    if (cpu_manager.cpu.features.turbo) {
        gui_draw_text(window, "Turbo Boost", x, y, COLOR_DARK_GRAY);
    }
    
    // Performans durumu
    y = 290;
    gui_draw_text(window, "Mevcut Performans:", 180, y, COLOR_DARK_GRAY);
    
    // Anlık frekans
    char freq_text[64];
    snprintf(freq_text, sizeof(freq_text), "Frekans: %d MHz", cpu_manager.current_freq);
    gui_draw_text(window, freq_text, 200, y + 25, COLOR_DARK_GRAY);
    
    // Sıcaklık
    char temp_text[64];
    snprintf(temp_text, sizeof(temp_text), "Sıcaklık: %d°C", cpu_manager.current_temp);
    gui_draw_text(window, temp_text, 340, y + 25, COLOR_DARK_GRAY);
    
    // Kullanım
    char usage_text[64];
    snprintf(usage_text, sizeof(usage_text), "Kullanım: %%%d", cpu_manager.current_usage);
    gui_draw_text(window, usage_text, 480, y + 25, COLOR_DARK_GRAY);
    
    // Optimizasyon profilleri
    y = 345;
    gui_draw_text(window, "Optimizasyon Profili:", 180, y, COLOR_DARK_GRAY);
    
    // Mevcut profil
    char profile_text[64];
    snprintf(profile_text, sizeof(profile_text), "Mevcut Profil: %s", 
            settings_get_cpu_profile_name(cpu_manager.current_profile));
    gui_draw_text(window, profile_text, 330, y, COLOR_BLACK);
    
    // İlerleme çubuğu (profil gücünü gösterir)
    int profile_power = 0;
    switch (cpu_manager.current_profile) {
        case CPU_PROFILE_POWER_SAVE:  profile_power = 25; break;
        case CPU_PROFILE_BALANCED:    profile_power = 50; break;
        case CPU_PROFILE_PERFORMANCE: profile_power = 75; break;
        case CPU_PROFILE_TURBO:       profile_power = 100; break;
        case CPU_PROFILE_CUSTOM:      profile_power = 60; break; // Özel için varsayılan
    }
    
    int progress_width = 300;
    int filled_width = (profile_power * progress_width) / 100;
    
    // Çubuk arka planı
    gui_draw_rect(window, 200, y + 30, progress_width, 20, COLOR_LIGHT_GRAY);
    
    // Doldurulan kısım (renk profile göre değişir)
    int bar_color;
    if (cpu_manager.current_profile == CPU_PROFILE_POWER_SAVE)
        bar_color = COLOR_DARK_GREEN;
    else if (cpu_manager.current_profile == CPU_PROFILE_BALANCED)
        bar_color = COLOR_BLUE;
    else if (cpu_manager.current_profile == CPU_PROFILE_PERFORMANCE)
        bar_color = COLOR_ORANGE;
    else if (cpu_manager.current_profile == CPU_PROFILE_TURBO)
        bar_color = COLOR_RED;
    else
        bar_color = COLOR_PURPLE;
        
    gui_draw_rect(window, 200, y + 30, filled_width, 20, bar_color);
    
    // Profil seçme butonları - tüm profiller için
    y += 70;
    
    // Profiller için butonlar
    gui_draw_button(window, 200, y, 120, 30, "Güç Tasarrufu", 
                   (cpu_manager.current_profile == CPU_PROFILE_POWER_SAVE) ? COLOR_DARK_GREEN : COLOR_BLUE, 
                   COLOR_WHITE);
    
    gui_draw_button(window, 330, y, 90, 30, "Dengeli", 
                   (cpu_manager.current_profile == CPU_PROFILE_BALANCED) ? COLOR_DARK_GREEN : COLOR_BLUE, 
                   COLOR_WHITE);
    
    gui_draw_button(window, 430, y, 90, 30, "Performans", 
                   (cpu_manager.current_profile == CPU_PROFILE_PERFORMANCE) ? COLOR_DARK_GREEN : COLOR_BLUE, 
                   COLOR_WHITE);
    
    gui_draw_button(window, 530, y, 70, 30, "Turbo", 
                   (cpu_manager.current_profile == CPU_PROFILE_TURBO) ? COLOR_DARK_GREEN : COLOR_RED, 
                   COLOR_WHITE);
    
    // Otomatik optimizasyon seçeneği
    y += 50;
    gui_draw_text(window, "Otomatik Optimizasyon:", 200, y, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 370, y, cpu_manager.auto_optimization);
    
    // Ayrıca özel bir buton ekleyelim - tüm Intel ve AMD işlemcilerde sorunsuz çalışmak için
    y += 50;
    gui_draw_button(window, 200, y, 400, 40, 
                   "Intel ve AMD İşlemciler İçin Optimize Et",
                   cpu_manager.is_optimized ? COLOR_DARK_GREEN : COLOR_PURPLE, 
                   COLOR_WHITE);
    
    // Açıklama metni
    char optimize_text[128];
    if (cpu_manager.is_optimized) {
        snprintf(optimize_text, sizeof(optimize_text), 
                "İşlemci optimizasyonu yapıldı: %s için optimal ayarlar kullanılıyor", 
                cpu_manager.cpu.family_type == CPU_FAMILY_INTEL ? "Intel" : 
                (cpu_manager.cpu.family_type == CPU_FAMILY_AMD ? "AMD" : "Diğer"));
    } else {
        strcpy(optimize_text, "İşlemci henüz optimize edilmedi. Optimizasyon için tıklayın.");
    }
    gui_draw_text(window, optimize_text, 200, y + 50, COLOR_DARK_GRAY);
}

// Batarya durum türleri
typedef enum {
    BATTERY_STATUS_UNKNOWN,
    BATTERY_STATUS_CHARGING,
    BATTERY_STATUS_DISCHARGING,
    BATTERY_STATUS_FULL,
    BATTERY_STATUS_LOW,
    BATTERY_STATUS_CRITICAL
} battery_status_t;

// Güç planı türleri
typedef enum {
    POWER_PLAN_MAX_BATTERY,  // Maksimum batarya ömrü
    POWER_PLAN_BALANCED,     // Dengeli
    POWER_PLAN_MAX_PERFORMANCE, // Maksimum performans
    POWER_PLAN_CUSTOM        // Özel
} power_plan_t;

// Batarya sağlık durumu
typedef enum {
    BATTERY_HEALTH_UNKNOWN,
    BATTERY_HEALTH_GOOD,
    BATTERY_HEALTH_FAIR,
    BATTERY_HEALTH_POOR,
    BATTERY_HEALTH_REPLACE
} battery_health_t;

// Güç kaynağı türü
typedef enum {
    POWER_SOURCE_UNKNOWN,
    POWER_SOURCE_BATTERY,
    POWER_SOURCE_AC
} power_source_t;

// Ekran parlaklık seviyeleri
#define BRIGHTNESS_LEVELS 10
static const uint8_t brightness_presets[BRIGHTNESS_LEVELS] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

// Batarya optimizasyon durumları
typedef struct {
    uint8_t enable_screen_dimming;     // Ekran karartma
    uint8_t enable_sleep_mode;         // Uyku modu
    uint8_t enable_hard_disk_spindown; // HDD düşük güç modu
    uint8_t enable_wifi_powersave;     // WiFi güç tasarrufu
    uint8_t enable_bluetooth_powersave;// Bluetooth güç tasarrufu
    uint8_t enable_usb_powersave;      // USB güç tasarrufu
    uint8_t enable_cpu_throttling;     // CPU hız kısma
    uint8_t enable_gpu_throttling;     // GPU hız kısma
    uint16_t screen_off_timeout;       // Ekran kapanma süresi (saniye)
    uint16_t sleep_timeout;            // Uyku modu süresi (saniye)
    uint8_t brightness_level;          // Ekran parlaklığı
    uint8_t keyboard_backlight;        // Klavye aydınlatma seviyesi
} battery_optimizations_t;

// Batarya bilgileri
typedef struct {
    char manufacturer[64];          // Üretici
    char model[64];                 // Model
    char serial[32];                // Seri numarası
    uint32_t design_capacity;       // Tasarım kapasitesi (mWh)
    uint32_t current_capacity;      // Mevcut kapasite (mWh)
    uint32_t current_charge;        // Mevcut şarj (mWh)
    uint8_t percentage;             // Şarj yüzdesi
    uint16_t cycle_count;           // Şarj döngüsü sayısı
    uint16_t voltage;               // Voltaj (mV)
    int16_t current;                // Akım (mA, negatif=deşarj)
    int16_t temperature;            // Sıcaklık (0.1°C)
    uint16_t time_remaining;        // Kalan süre (dakika)
    battery_status_t status;        // Batarya durumu
    battery_health_t health;        // Batarya sağlığı
    power_source_t power_source;    // Güç kaynağı
    uint32_t last_full_capacity;    // Son tam kapasite
    uint8_t wear_percentage;        // Aşınma yüzdesi
} battery_info_t;

// Batarya yöneticisi
typedef struct {
    battery_info_t battery;                    // Batarya bilgileri
    power_plan_t current_plan;                 // Mevcut güç planı
    battery_optimizations_t power_plans[4];    // Güç planı optimizasyonları
    uint8_t is_present;                        // Batarya var mı?
    uint8_t is_optimization_enabled;           // Optimizasyon aktif mi?
    uint8_t is_critical_alert_enabled;         // Kritik uyarılar aktif mi?
    uint8_t critical_threshold;                // Kritik seviye eşiği (%)
    uint8_t low_threshold;                     // Düşük seviye eşiği (%)
    uint8_t auto_plan_switching;               // Otomatik plan değiştirme
    uint32_t statistics_days;                  // İstatistik günü
    uint32_t average_discharge_rate;           // Ortalama deşarj hızı (mW)
    uint32_t estimated_battery_life;           // Tahmini batarya ömrü (saat)
} battery_manager_t;

// Batarya yöneticisi
static battery_manager_t battery_manager = {0};

// Batarya durumunu algıla ve güncelle
void settings_detect_battery() {
    // Gerçek sistemde ACPI/WMI ile batarya bilgileri alınır
    // Örnek bir batarya simülasyonu
    strcpy(battery_manager.battery.manufacturer, "KALEM OS Battery Corp.");
    strcpy(battery_manager.battery.model, "Li-ion Smart Battery");
    strcpy(battery_manager.battery.serial, "BAT123456789");
    
    battery_manager.battery.design_capacity = 50000;  // 50 Wh
    battery_manager.battery.last_full_capacity = 48000; // 48 Wh
    battery_manager.battery.current_capacity = 48000; // 48 Wh
    battery_manager.battery.current_charge = 38400;   // 38.4 Wh (80%)
    
    battery_manager.battery.percentage = 80;          // %80 şarj
    battery_manager.battery.cycle_count = 120;        // 120 döngü
    battery_manager.battery.voltage = 11400;          // 11.4V
    battery_manager.battery.current = -1200;          // 1200mA deşarj
    battery_manager.battery.temperature = 320;        // 32.0°C
    battery_manager.battery.time_remaining = 180;     // 3 saat
    
    battery_manager.battery.status = BATTERY_STATUS_DISCHARGING;
    battery_manager.battery.health = BATTERY_HEALTH_GOOD;
    battery_manager.battery.power_source = POWER_SOURCE_BATTERY;
    
    battery_manager.battery.wear_percentage = 4;      // %4 aşınma
    
    battery_manager.is_present = 1;
    battery_manager.is_optimization_enabled = 1;
    battery_manager.is_critical_alert_enabled = 1;
    battery_manager.critical_threshold = 5;           // %5 kritik seviye
    battery_manager.low_threshold = 15;               // %15 düşük seviye
    battery_manager.auto_plan_switching = 1;
    
    battery_manager.statistics_days = 30;
    battery_manager.average_discharge_rate = 12000;   // 12W ortalama
    battery_manager.estimated_battery_life = 800;     // 800 saat tahmini ömür
}

// Varsayılan güç planlarını oluştur
void settings_initialize_power_plans() {
    // Maksimum Batarya Ömrü Planı
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].enable_screen_dimming = 1;
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].enable_sleep_mode = 1;
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].enable_hard_disk_spindown = 1;
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].enable_wifi_powersave = 1;
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].enable_bluetooth_powersave = 1;
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].enable_usb_powersave = 1;
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].enable_cpu_throttling = 1;
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].enable_gpu_throttling = 1;
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].screen_off_timeout = 60;   // 1 dakika
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].sleep_timeout = 300;       // 5 dakika
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].brightness_level = 3;      // %30 parlaklık
    battery_manager.power_plans[POWER_PLAN_MAX_BATTERY].keyboard_backlight = 0;    // Kapalı
    
    // Dengeli Plan
    battery_manager.power_plans[POWER_PLAN_BALANCED].enable_screen_dimming = 1;
    battery_manager.power_plans[POWER_PLAN_BALANCED].enable_sleep_mode = 1;
    battery_manager.power_plans[POWER_PLAN_BALANCED].enable_hard_disk_spindown = 1;
    battery_manager.power_plans[POWER_PLAN_BALANCED].enable_wifi_powersave = 0;
    battery_manager.power_plans[POWER_PLAN_BALANCED].enable_bluetooth_powersave = 1;
    battery_manager.power_plans[POWER_PLAN_BALANCED].enable_usb_powersave = 0;
    battery_manager.power_plans[POWER_PLAN_BALANCED].enable_cpu_throttling = 0;
    battery_manager.power_plans[POWER_PLAN_BALANCED].enable_gpu_throttling = 0;
    battery_manager.power_plans[POWER_PLAN_BALANCED].screen_off_timeout = 300;     // 5 dakika
    battery_manager.power_plans[POWER_PLAN_BALANCED].sleep_timeout = 900;          // 15 dakika
    battery_manager.power_plans[POWER_PLAN_BALANCED].brightness_level = 5;         // %50 parlaklık
    battery_manager.power_plans[POWER_PLAN_BALANCED].keyboard_backlight = 1;       // Düşük
    
    // Maksimum Performans Planı
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].enable_screen_dimming = 0;
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].enable_sleep_mode = 0;
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].enable_hard_disk_spindown = 0;
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].enable_wifi_powersave = 0;
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].enable_bluetooth_powersave = 0;
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].enable_usb_powersave = 0;
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].enable_cpu_throttling = 0;
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].enable_gpu_throttling = 0;
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].screen_off_timeout = 1800;// 30 dakika
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].sleep_timeout = 3600;     // 60 dakika
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].brightness_level = 9;     // %90 parlaklık
    battery_manager.power_plans[POWER_PLAN_MAX_PERFORMANCE].keyboard_backlight = 3;   // Yüksek
    
    // Özel Plan (başlangıçta Dengeli Plan ile aynı)
    memcpy(&battery_manager.power_plans[POWER_PLAN_CUSTOM], 
           &battery_manager.power_plans[POWER_PLAN_BALANCED], 
           sizeof(battery_optimizations_t));
    
    // Varsayılan planı ayarla
    battery_manager.current_plan = POWER_PLAN_BALANCED;
}

// Batarya yöneticisini başlat
void settings_battery_manager_init() {
    // Batarya durumunu algıla
    settings_detect_battery();
    
    // Güç planlarını oluştur
    settings_initialize_power_plans();
}

// Güç planını değiştir
void settings_set_power_plan(power_plan_t plan) {
    if (plan >= POWER_PLAN_MAX_BATTERY && plan <= POWER_PLAN_CUSTOM) {
        battery_manager.current_plan = plan;
        
        // Gerçek bir sistemde, plan ayarlarını cihaza uygulayacak kodlar burada yer alır
        // Örneğin: Ekran parlaklığı, CPU frekansı, uyku modu ayarları vb.
        
        // Bu demo uygulamada sadece geçerli planı değiştiriyoruz
    }
}

// Şarj yüzdesine göre güç planını otomatik ayarla
void settings_auto_adjust_power_plan() {
    if (!battery_manager.auto_plan_switching || 
        battery_manager.battery.power_source != POWER_SOURCE_BATTERY)
        return;
    
    uint8_t percentage = battery_manager.battery.percentage;
    
    // Şarj düzeyine göre plan önerisi
    if (percentage <= battery_manager.low_threshold) {
        // Düşük şarj, maksimum batarya tasarrufu
        settings_set_power_plan(POWER_PLAN_MAX_BATTERY);
    } else if (percentage <= 30) {
        // 30% altında, dengeli plan
        settings_set_power_plan(POWER_PLAN_BALANCED);
    } else {
        // Normal şarj, kullanıcı tercihine bağlı
        // Varsayılan olarak dengeli kullanıyoruz
        settings_set_power_plan(POWER_PLAN_BALANCED);
    }
}

// Batarya durumu metni al
const char* settings_get_battery_status_text(battery_status_t status) {
    switch (status) {
        case BATTERY_STATUS_CHARGING:    return "Şarj Oluyor";
        case BATTERY_STATUS_DISCHARGING: return "Pil Kullanılıyor";
        case BATTERY_STATUS_FULL:        return "Tam Şarjlı";
        case BATTERY_STATUS_LOW:         return "Düşük Şarj";
        case BATTERY_STATUS_CRITICAL:    return "Kritik Şarj Seviyesi";
        default:                         return "Bilinmiyor";
    }
}

// Batarya sağlık durumu metni al
const char* settings_get_battery_health_text(battery_health_t health) {
    switch (health) {
        case BATTERY_HEALTH_GOOD:    return "İyi";
        case BATTERY_HEALTH_FAIR:    return "Normal";
        case BATTERY_HEALTH_POOR:    return "Zayıf";
        case BATTERY_HEALTH_REPLACE: return "Değiştirilmeli";
        default:                     return "Bilinmiyor";
    }
}

// Güç kaynağı metni al
const char* settings_get_power_source_text(power_source_t source) {
    switch (source) {
        case POWER_SOURCE_BATTERY: return "Batarya";
        case POWER_SOURCE_AC:      return "Güç Adaptörü";
        default:                   return "Bilinmiyor";
    }
}

// Güç planı metni al
const char* settings_get_power_plan_text(power_plan_t plan) {
    switch (plan) {
        case POWER_PLAN_MAX_BATTERY:     return "Maksimum Batarya Ömrü";
        case POWER_PLAN_BALANCED:        return "Dengeli";
        case POWER_PLAN_MAX_PERFORMANCE: return "Maksimum Performans";
        case POWER_PLAN_CUSTOM:          return "Özel Plan";
        default:                         return "Bilinmiyor";
    }
}

// Ekran parlaklığını ayarla
void settings_set_screen_brightness(uint8_t level) {
    if (level < BRIGHTNESS_LEVELS) {
        // Mevcut planın parlaklık ayarını güncelle
        battery_manager.power_plans[battery_manager.current_plan].brightness_level = level;
        
        // Gerçek sistemde ekran parlaklığını ayarla
        // display_set_brightness(brightness_presets[level]);
    }
}

// Batarya kalibrasyonu yap
void settings_calibrate_battery() {
    // Gerçek bir sistemde batarya kalibrasyonu:
    // 1. Tam şarj
    // 2. Pil tamamen bitene kadar kullan
    // 3. Yeniden tam şarj
    // Bu işlem batarya durumu raporlamasını iyileştirir
    
    // Bu demo uygulamada sadece simülasyon
    battery_manager.battery.current_capacity = battery_manager.battery.last_full_capacity;
    battery_manager.battery.wear_percentage = 
        100 - ((battery_manager.battery.last_full_capacity * 100) / 
               battery_manager.battery.design_capacity);
}

// Ekranı çiz - Batarya Yönetimi
void settings_paint_battery(gui_window_t* window) {
    if (!window || !active_settings) return;
    
    // Başlık
    gui_draw_text(window, "Batarya Yönetimi", 180, 50, COLOR_BLACK);
    
    if (!battery_manager.is_present) {
        gui_draw_text(window, "Batarya takılı değil veya algılanamadı.", 200, 100, COLOR_DARK_RED);
        return;
    }
    
    // Batarya durumu
    gui_draw_text(window, "Batarya Durumu:", 180, 85, COLOR_DARK_GRAY);
    
    char battery_info[128];
    snprintf(battery_info, sizeof(battery_info), "%s, %s - %d%%", 
             battery_manager.battery.model,
             settings_get_battery_status_text(battery_manager.battery.status),
             battery_manager.battery.percentage);
    gui_draw_text(window, battery_info, 200, 110, COLOR_BLACK);
    
    // Güç kaynağı
    char power_source[64];
    snprintf(power_source, sizeof(power_source), "Güç Kaynağı: %s", 
             settings_get_power_source_text(battery_manager.battery.power_source));
    gui_draw_text(window, power_source, 200, 135, COLOR_DARK_GRAY);
    
    // Kalan süre
    char time_remaining[64];
    if (battery_manager.battery.status == BATTERY_STATUS_DISCHARGING) {
        int hours = battery_manager.battery.time_remaining / 60;
        int minutes = battery_manager.battery.time_remaining % 60;
        snprintf(time_remaining, sizeof(time_remaining), 
                "Kalan Süre: %d saat %d dakika", hours, minutes);
    } else if (battery_manager.battery.status == BATTERY_STATUS_CHARGING) {
        snprintf(time_remaining, sizeof(time_remaining), "Şarj Oluyor...");
    } else {
        snprintf(time_remaining, sizeof(time_remaining), "");
    }
    gui_draw_text(window, time_remaining, 200, 160, COLOR_DARK_GRAY);
    
    // Batarya şarj göstergesi
    gui_draw_text(window, "Şarj Durumu:", 180, 190, COLOR_DARK_GRAY);
    
    // Batarya şarj çubuğu (arka plan)
    int bar_width = 400;
    int bar_height = 20;
    gui_draw_rect(window, 200, 215, bar_width, bar_height, COLOR_LIGHT_GRAY);
    
    // Dolu kısım
    int filled_width = (battery_manager.battery.percentage * bar_width) / 100;
    
    // Renk (yeşil->sarı->kırmızı)
    int bar_color;
    if (battery_manager.battery.percentage > 50)
        bar_color = COLOR_GREEN;
    else if (battery_manager.battery.percentage > 20)
        bar_color = COLOR_YELLOW;
    else
        bar_color = COLOR_RED;
    
    gui_draw_rect(window, 200, 215, filled_width, bar_height, bar_color);
    
    // Batarya sağlığı
    gui_draw_text(window, "Batarya Sağlığı:", 180, 245, COLOR_DARK_GRAY);
    
    char health_info[128];
    snprintf(health_info, sizeof(health_info), 
            "Durum: %s, Aşınma: %%%d, Döngü Sayısı: %d", 
            settings_get_battery_health_text(battery_manager.battery.health),
            battery_manager.battery.wear_percentage,
            battery_manager.battery.cycle_count);
    gui_draw_text(window, health_info, 200, 270, COLOR_DARK_GRAY);
    
    // Batarya Kapasitesi
    char capacity_info[128];
    float design_wh = battery_manager.battery.design_capacity / 1000.0f;
    float current_wh = battery_manager.battery.current_capacity / 1000.0f;
    snprintf(capacity_info, sizeof(capacity_info), 
            "Tasarım Kapasitesi: %.1f Wh, Mevcut Kapasite: %.1f Wh",
            design_wh, current_wh);
    gui_draw_text(window, capacity_info, 200, 295, COLOR_DARK_GRAY);
    
    // Batarya Kalibrasyon Butonu
    gui_draw_button(window, 200, 320, 160, 30, "Batarya Kalibrasyonu", COLOR_BLUE, COLOR_WHITE);
    
    // Güç planları
    gui_draw_text(window, "Güç Planları:", 180, 360, COLOR_DARK_GRAY);
    
    // Mevcut plan
    char current_plan[64];
    snprintf(current_plan, sizeof(current_plan), "Aktif Plan: %s", 
             settings_get_power_plan_text(battery_manager.current_plan));
    gui_draw_text(window, current_plan, 200, 385, COLOR_BLACK);
    
    // Plan seçme butonları
    int y = 415;
    
    gui_draw_button(window, 200, y, 180, 30, "Maksimum Batarya Ömrü", 
                   (battery_manager.current_plan == POWER_PLAN_MAX_BATTERY) ? COLOR_DARK_GREEN : COLOR_BLUE, 
                   COLOR_WHITE);
    
    gui_draw_button(window, 390, y, 100, 30, "Dengeli", 
                   (battery_manager.current_plan == POWER_PLAN_BALANCED) ? COLOR_DARK_GREEN : COLOR_BLUE, 
                   COLOR_WHITE);
    
    gui_draw_button(window, 500, y, 180, 30, "Maksimum Performans", 
                   (battery_manager.current_plan == POWER_PLAN_MAX_PERFORMANCE) ? COLOR_DARK_GREEN : COLOR_BLUE, 
                   COLOR_WHITE);
    
    // Otomatik plan değiştirme
    y += 40;
    gui_draw_text(window, "Otomatik Plan Değiştirme:", 200, y, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 370, y, battery_manager.auto_plan_switching);
    
    // Güç planı öğeleri
    y += 40;
    gui_draw_text(window, "Aktif Plan Ayarları:", 180, y, COLOR_DARK_GRAY);
    y += 25;
    
    // Mevcut planın optimizasyon ayarları
    battery_optimizations_t* current_opts = &battery_manager.power_plans[battery_manager.current_plan];
    
    // Ekran parlaklığı
    gui_draw_text(window, "Ekran Parlaklığı:", 200, y, COLOR_DARK_GRAY);
    
    // Parlaklık çubuğu
    int brightness_width = 200;
    int brightness_height = 15;
    gui_draw_rect(window, 330, y, brightness_width, brightness_height, COLOR_LIGHT_GRAY);
    
    // Parlaklık seviyesi
    int brightness_filled = (current_opts->brightness_level * brightness_width) / (BRIGHTNESS_LEVELS - 1);
    gui_draw_rect(window, 330, y, brightness_filled, brightness_height, COLOR_BLUE);
    
    // Parlaklık butonu - azalt
    gui_draw_button(window, 330 - 25, y - 2, 20, 20, "-", COLOR_BLUE, COLOR_WHITE);
    
    // Parlaklık butonu - artır
    gui_draw_button(window, 330 + brightness_width + 5, y - 2, 20, 20, "+", COLOR_BLUE, COLOR_WHITE);
    
    // Parlaklık yüzdesi
    char brightness_text[8];
    snprintf(brightness_text, sizeof(brightness_text), "%%%d", 
             brightness_presets[current_opts->brightness_level]);
    gui_draw_text(window, brightness_text, 330 + brightness_width + 30, y, COLOR_DARK_GRAY);
    
    y += 30;
    
    // Zaman aşımı ayarları
    char timeout_text[128];
    snprintf(timeout_text, sizeof(timeout_text), 
            "Ekran Kapanma: %d dk, Uyku Modu: %d dk", 
            current_opts->screen_off_timeout / 60,
            current_opts->sleep_timeout / 60);
    gui_draw_text(window, timeout_text, 200, y, COLOR_DARK_GRAY);
    
    y += 30;
    
    // Güç tasarruf özellikleri
    gui_draw_text(window, "Ekran Karartma:", 200, y, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 330, y, current_opts->enable_screen_dimming);
    
    gui_draw_text(window, "HDD Güç Tasarrufu:", 370, y, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 500, y, current_opts->enable_hard_disk_spindown);
    
    y += 25;
    
    gui_draw_text(window, "WiFi Güç Tasarrufu:", 200, y, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 330, y, current_opts->enable_wifi_powersave);
    
    gui_draw_text(window, "Bluetooth Tasarrufu:", 370, y, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 500, y, current_opts->enable_bluetooth_powersave);
    
    y += 25;
    
    gui_draw_text(window, "CPU Hız Kısıtlama:", 200, y, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 330, y, current_opts->enable_cpu_throttling);
    
    gui_draw_text(window, "GPU Hız Kısıtlama:", 370, y, COLOR_DARK_GRAY);
    gui_draw_checkbox(window, 500, y, current_opts->enable_gpu_throttling);
}

// Pencere kapatma işleyicisi
static void window_close_handler(gui_window_t* window) {
    if (active_settings) {
        active_settings->window = NULL;
    }
}