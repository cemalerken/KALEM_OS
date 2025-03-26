#include "../include/kalem_shell.h"
#include "../include/gui.h"
#include "../include/font.h"
#include "../include/vga.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Aktif kabuk örneği
static kalem_shell_t* active_shell = NULL;

// Dahili komutlar listesi
static kalem_shell_cmd_t internal_commands[] = {
    {"cd", NULL, "Çalışma dizinini değiştirir. Kullanım: cd [dizin]", 0},
    {"ls", NULL, "Dosya ve dizinleri listeler. Kullanım: ls [dizin]", 0},
    {"clear", NULL, "Ekranı temizler.", 0},
    {"cat", NULL, "Dosya içeriğini gösterir. Kullanım: cat <dosya>", 0},
    {"touch", NULL, "Boş bir dosya oluşturur. Kullanım: touch <dosya>", 0},
    {"mkdir", NULL, "Yeni bir dizin oluşturur. Kullanım: mkdir <dizin>", 0},
    {"rm", NULL, "Dosya veya dizin siler. Kullanım: rm [-r] <dosya/dizin>", 0},
    {"echo", NULL, "Ekrana metin yazdırır. Kullanım: echo <metin>", 0},
    {"help", NULL, "Mevcut komutları listeler. Kullanım: help [komut]", 0},
    {"exit", NULL, "KALEM Shell'i kapatır.", 0},
    {"history", NULL, "Komut geçmişini gösterir.", 0},
    {"pwd", NULL, "Mevcut çalışma dizinini gösterir.", 0},
    {"sudo", NULL, "Komutu root olarak çalıştırır. Kullanım: sudo <komut>", 0},
    {"sysinfo", NULL, "Sistem bilgilerini görüntüler.", 0},
    {"ps", NULL, "Çalışan işlemleri gösterir.", 0},
    {"kill", NULL, "İşlem sonlandırır. Kullanım: kill <PID>", 1},
    {"mount", NULL, "Dosya sistemini bağlar. Kullanım: mount <kaynak> <hedef>", 1},
    {"umount", NULL, "Dosya sistemini ayırır. Kullanım: umount <hedef>", 1},
    {"reboot", NULL, "Sistemi yeniden başlatır.", 1},
    {"shutdown", NULL, "Sistemi kapatır.", 1},
    {"passwd", NULL, "Parola değiştirir. Kullanım: passwd [kullanıcı]", 0},
    {"whoami", NULL, "Mevcut kullanıcı adını görüntüler.", 0},
    {"grep", NULL, "Metin içinde arama yapar. Kullanım: grep <desen> <dosya>", 0},
    {"find", NULL, "Dosya arar. Kullanım: find <dizin> -name <desen>", 0},
    {"chmod", NULL, "Dosya izinlerini değiştirir. Kullanım: chmod <mod> <dosya>", 0},
    {"chown", NULL, "Dosya sahibini değiştirir. Kullanım: chown <kullanıcı> <dosya>", 1},
    {NULL, NULL, NULL, 0} // Son eleman
};

// Komut işlevi bağlantıları
static void kalem_shell_connect_commands();

// Kabuk başlatma
void kalem_shell_init() {
    // İlk çalıştırma kontrolü
    if (active_shell == NULL) {
        active_shell = (kalem_shell_t*)malloc(sizeof(kalem_shell_t));
        if (!active_shell) return;
        
        // Yapıyı sıfırla
        memset(active_shell, 0, sizeof(kalem_shell_t));
        
        // Varsayılan değerleri ayarla
        active_shell->status = KALEM_SHELL_STATUS_NORMAL;
        active_shell->cmd_buffer[0] = '\0';
        active_shell->output_buffer[0] = '\0';
        active_shell->history_count = 0;
        active_shell->history_pos = 0;
        active_shell->cmd_pos = 0;
        active_shell->output_pos = 0;
        active_shell->cursor_visible = 1;
        active_shell->cursor_blink_time = 0;
        strcpy(active_shell->current_dir, "/");
        active_shell->is_root = 0;
        
        // Geçmiş için bellek ayır
        for (int i = 0; i < KALEM_SHELL_HISTORY_MAX; i++) {
            active_shell->history[i] = NULL;
        }
        
        // Dahili komutları bağla
        kalem_shell_connect_commands();
        
        // Hoş geldiniz mesajı
        kalem_shell_println("KALEM Shell v1.0 - KALEM OS Komut Yöneticisi");
        kalem_shell_println("(c) 2024 KALEM OS Geliştirme Ekibi");
        kalem_shell_println("-------------------------------------------------------");
        kalem_shell_println("Komutları görmek için 'help' yazabilirsiniz.");
        kalem_shell_println("");
    }
}

// Dahili komutları fonksiyonlara bağla
static void kalem_shell_connect_commands() {
    for (int i = 0; internal_commands[i].name != NULL; i++) {
        // Burada her bir komutu işlev işaretçisine bağlıyoruz
        if (strcmp(internal_commands[i].name, "cd") == 0) {
            internal_commands[i].func = (kalem_shell_cmd_func_t)kalem_shell_change_dir;
        } else if (strcmp(internal_commands[i].name, "ls") == 0) {
            internal_commands[i].func = (kalem_shell_cmd_func_t)kalem_shell_list_dir;
        } else if (strcmp(internal_commands[i].name, "clear") == 0) {
            internal_commands[i].func = (kalem_shell_cmd_func_t)kalem_shell_clear;
        } else if (strcmp(internal_commands[i].name, "cat") == 0) {
            internal_commands[i].func = (kalem_shell_cmd_func_t)kalem_shell_cat;
        } else if (strcmp(internal_commands[i].name, "touch") == 0) {
            internal_commands[i].func = (kalem_shell_cmd_func_t)kalem_shell_touch;
        } else if (strcmp(internal_commands[i].name, "mkdir") == 0) {
            internal_commands[i].func = (kalem_shell_cmd_func_t)kalem_shell_mkdir;
        } else if (strcmp(internal_commands[i].name, "rm") == 0) {
            internal_commands[i].func = (kalem_shell_cmd_func_t)kalem_shell_rm;
        } else if (strcmp(internal_commands[i].name, "help") == 0) {
            internal_commands[i].func = (kalem_shell_cmd_func_t)kalem_shell_help;
        } else if (strcmp(internal_commands[i].name, "exit") == 0) {
            internal_commands[i].func = (kalem_shell_cmd_func_t)kalem_shell_exit;
        } else if (strcmp(internal_commands[i].name, "history") == 0) {
            internal_commands[i].func = (kalem_shell_cmd_func_t)kalem_shell_history;
        } else if (strcmp(internal_commands[i].name, "sysinfo") == 0) {
            internal_commands[i].func = (kalem_shell_cmd_func_t)kalem_shell_sysinfo;
        }
        // Diğer komutlar da benzer şekilde eklenir...
    }
}

// Kabuk penceresini göster
void kalem_shell_show() {
    // Pencere zaten açık mı?
    if (active_shell && active_shell->window) {
        gui_window_bring_to_front(active_shell->window);
        return;
    }
    
    // Kabuk yapısını oluştur
    if (!active_shell) {
        kalem_shell_init();
    }
    
    // Pencereyi oluştur
    active_shell->window = gui_window_create("KALEM Shell", 100, 100, 640, 480, GUI_WINDOW_STYLE_NORMAL);
    if (!active_shell->window) return;
    
    // Pencere işleyicilerini ayarla
    active_shell->window->on_paint = kalem_shell_paint;
    
    // Pencereyi göster
    gui_window_show(active_shell->window);
}

// Kabuk çizim fonksiyonu
void kalem_shell_paint(gui_window_t* window) {
    if (!window || !active_shell) return;
    
    uint32_t x = window->client.x;
    uint32_t y = window->client.y;
    uint32_t width = window->client.width;
    uint32_t height = window->client.height;
    
    // Arkaplan
    vga_fill_rect(x, y, width, height, GUI_COLOR_BLACK);
    
    // İçeriği çiz
    int line_height = 16;
    int max_lines = height / line_height;
    int visible_lines = 0;
    
    // Çıktı içeriğini satır satır göster
    char* output = active_shell->output_buffer;
    int len = strlen(output);
    int line_start = 0;
    int line_count = 0;
    
    // Her satırı ayrı ayrı çiz
    for (int i = 0; i <= len; i++) {
        if (output[i] == '\n' || output[i] == '\0') {
            // Satırı çiz
            if (line_count >= (max_lines - 1)) {
                // Sadece son satırları göster
                char temp[256];
                strncpy(temp, &output[line_start], i - line_start);
                temp[i - line_start] = '\0';
                font_draw_string(x + 10, y + (visible_lines * line_height), temp, GUI_COLOR_LIGHT_GREEN, 0);
                visible_lines++;
            }
            
            line_start = i + 1;
            line_count++;
        }
    }
    
    // Mevcut komutu çiz
    char current_line[320];
    sprintf(current_line, "%s%s", KALEM_SHELL_PROMPT, active_shell->cmd_buffer);
    font_draw_string(x + 10, y + height - 30, current_line, GUI_COLOR_LIGHT_GREEN, 0);
    
    // İmleç çizimi
    if (active_shell->cursor_visible) {
        int cursor_x = x + 10 + (strlen(KALEM_SHELL_PROMPT) + active_shell->cmd_pos) * 8; // Varsayılan font genişliği
        vga_fill_rect(cursor_x, y + height - 28, 8, 14, GUI_COLOR_LIGHT_GREEN);
    }
}

// Kabuk penceresini kapat
void kalem_shell_close() {
    if (active_shell && active_shell->window) {
        gui_window_destroy(active_shell->window);
        active_shell->window = NULL;
    }
}

// Tuş girişini işle
void kalem_shell_handle_key(uint8_t key) {
    if (!active_shell) return;
    
    // Enter tuşu
    if (key == '\n' || key == '\r') {
        kalem_shell_execute_command();
        return;
    }
    
    // Backspace tuşu
    if (key == '\b') {
        if (active_shell->cmd_pos > 0) {
            memmove(&active_shell->cmd_buffer[active_shell->cmd_pos - 1], 
                   &active_shell->cmd_buffer[active_shell->cmd_pos], 
                   strlen(active_shell->cmd_buffer) - active_shell->cmd_pos + 1);
            active_shell->cmd_pos--;
        }
        return;
    }
    
    // Tab tuşu
    if (key == '\t') {
        kalem_shell_tab_complete();
        return;
    }
    
    // Yukarı ok
    if (key == 0x11) {
        kalem_shell_navigate_history(-1);
        return;
    }
    
    // Aşağı ok
    if (key == 0x12) {
        kalem_shell_navigate_history(1);
        return;
    }
    
    // Sol ok
    if (key == 0x13) {
        if (active_shell->cmd_pos > 0) {
            active_shell->cmd_pos--;
        }
        return;
    }
    
    // Sağ ok
    if (key == 0x14) {
        if (active_shell->cmd_pos < strlen(active_shell->cmd_buffer)) {
            active_shell->cmd_pos++;
        }
        return;
    }
    
    // ESC tuşu
    if (key == 0x1B) {
        active_shell->cmd_buffer[0] = '\0';
        active_shell->cmd_pos = 0;
        return;
    }
    
    // Normal karakter girişi
    if (key >= 32 && key <= 126) {
        if (strlen(active_shell->cmd_buffer) < KALEM_SHELL_CMD_MAX - 1) {
            // İmleç pozisyonuna karakter ekle
            memmove(&active_shell->cmd_buffer[active_shell->cmd_pos + 1], 
                   &active_shell->cmd_buffer[active_shell->cmd_pos], 
                   strlen(active_shell->cmd_buffer) - active_shell->cmd_pos + 1);
            active_shell->cmd_buffer[active_shell->cmd_pos] = key;
            active_shell->cmd_pos++;
        }
    }
    
    // Pencereyi yeniden çiz
    if (active_shell->window) {
        gui_window_redraw(active_shell->window);
    }
}

// Komutu yürüt
void kalem_shell_execute_command() {
    if (!active_shell) return;
    
    // Komutu çıktıya ekle
    kalem_shell_add_to_output();
    
    // Komut boş mu?
    if (strlen(active_shell->cmd_buffer) == 0) {
        kalem_shell_println("");
        active_shell->cmd_buffer[0] = '\0';
        active_shell->cmd_pos = 0;
        return;
    }
    
    // Komutu geçmişe ekle
    char* cmd_copy = strdup(active_shell->cmd_buffer);
    if (cmd_copy) {
        // Eski geçmişi temizle
        if (active_shell->history_count == KALEM_SHELL_HISTORY_MAX) {
            free(active_shell->history[0]);
            
            // Tüm elemanları bir yukarı taşı
            for (int i = 0; i < KALEM_SHELL_HISTORY_MAX - 1; i++) {
                active_shell->history[i] = active_shell->history[i + 1];
            }
            
            active_shell->history_count--;
        }
        
        // Yeni komutu ekle
        active_shell->history[active_shell->history_count++] = cmd_copy;
        active_shell->history_pos = active_shell->history_count;
    }
    
    // Komutu yorumla ve çalıştır
    kalem_shell_parse_command(active_shell->cmd_buffer);
    
    // Komutu temizle
    active_shell->cmd_buffer[0] = '\0';
    active_shell->cmd_pos = 0;
    
    // Pencereyi yeniden çiz
    if (active_shell->window) {
        gui_window_redraw(active_shell->window);
    }
}

// Komut satırını çıktıya ekle
void kalem_shell_add_to_output() {
    if (!active_shell) return;
    
    char line[320];
    sprintf(line, "%s%s", KALEM_SHELL_PROMPT, active_shell->cmd_buffer);
    kalem_shell_println(line);
}

// Çıktıya metin ekle
void kalem_shell_print(const char* text) {
    if (!active_shell || !text) return;
    
    // Çıktı tamponunda yer var mı?
    size_t current_len = strlen(active_shell->output_buffer);
    size_t text_len = strlen(text);
    
    if (current_len + text_len >= KALEM_SHELL_OUTPUT_MAX - 1) {
        // Tampon doluysa kaydırma yap
        size_t to_move = current_len + text_len - KALEM_SHELL_OUTPUT_MAX + 1024; // 1KB ekstra alan
        
        // İlk satırları atla
        char* new_start = strchr(&active_shell->output_buffer[to_move], '\n');
        if (new_start) {
            new_start++; // \n'den sonraki karakter
            memmove(active_shell->output_buffer, new_start, strlen(new_start) + 1);
            current_len = strlen(active_shell->output_buffer);
        } else {
            // Son çare olarak tamamen temizle
            active_shell->output_buffer[0] = '\0';
            current_len = 0;
        }
    }
    
    // Metni ekle
    strcpy(&active_shell->output_buffer[current_len], text);
    
    // Pencereyi yeniden çiz
    if (active_shell->window) {
        gui_window_redraw(active_shell->window);
    }
}

// Çıktıya satır ekle
void kalem_shell_println(const char* text) {
    if (!active_shell) return;
    
    kalem_shell_print(text);
    kalem_shell_print("\n");
}

// Komut geçmişinde gezinme
void kalem_shell_navigate_history(int direction) {
    if (!active_shell || active_shell->history_count == 0) return;
    
    // Geçmiş içinde gezin
    if (direction < 0) {
        // Yukarı (daha eski komutlar)
        if (active_shell->history_pos > 0) {
            active_shell->history_pos--;
        }
    } else {
        // Aşağı (daha yeni komutlar)
        if (active_shell->history_pos < active_shell->history_count) {
            active_shell->history_pos++;
        }
    }
    
    // Geçmiş öğesini al
    if (active_shell->history_pos < active_shell->history_count) {
        strcpy(active_shell->cmd_buffer, active_shell->history[active_shell->history_pos]);
        active_shell->cmd_pos = strlen(active_shell->cmd_buffer);
    } else {
        // En son pozisyonda boş komut
        active_shell->cmd_buffer[0] = '\0';
        active_shell->cmd_pos = 0;
    }
    
    // Pencereyi yeniden çiz
    if (active_shell->window) {
        gui_window_redraw(active_shell->window);
    }
}

// Tab tamamlama
void kalem_shell_tab_complete() {
    if (!active_shell) return;
    
    // TODO: Komut ve dosya adı tamamlama özelliğini ekle
    
    kalem_shell_println("Tab tamamlama henüz eklenmedi.");
}

// Parametreleri ayrıştır
static int kalem_shell_split_args(char* cmd, char** argv, int max_args) {
    int argc = 0;
    char* token = strtok(cmd, " \t\n");
    
    while (token != NULL && argc < max_args) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    
    return argc;
}

// Komutu yorumla ve çalıştır
int kalem_shell_parse_command(char* cmd) {
    if (!active_shell || !cmd) return -1;
    
    // Komutu ayrıştır
    char* argv[32]; // Maksimum 32 parametre
    int argc = kalem_shell_split_args(cmd, argv, 32);
    
    if (argc == 0) return 0; // Boş komut
    
    // sudo komutu kontrolü
    if (strcmp(argv[0], "sudo") == 0) {
        if (argc < 2) {
            kalem_shell_println("Hata: sudo komutu için yeterli argüman yok.");
            return -1;
        }
        
        // Root yetkileri iste
        if (!active_shell->is_root) {
            if (kalem_shell_authenticate_root() != 0) {
                kalem_shell_println("Hata: Yetkilendirme başarısız oldu.");
                return -1;
            }
        }
        
        // Orijinal komutu argv[1]'den başlayarak çalıştır
        return kalem_shell_run_internal_command(argv[1], argc - 1, &argv[1]);
    }
    
    // Dahili komutu kontrol et
    return kalem_shell_run_internal_command(argv[0], argc, argv);
}

// Dahili komutu çalıştır
int kalem_shell_run_internal_command(const char* cmd_name, int argc, char** argv) {
    if (!active_shell || !cmd_name) return -1;
    
    // Komutu ara
    for (int i = 0; internal_commands[i].name != NULL; i++) {
        if (strcmp(internal_commands[i].name, cmd_name) == 0) {
            // Komut bulundu
            
            // Root yetkisi gerekiyor mu?
            if (internal_commands[i].requires_root && !active_shell->is_root) {
                kalem_shell_println("Hata: Bu komut root yetkileri gerektirir. 'sudo' kullanın.");
                return -1;
            }
            
            // Komut işlevini çağır
            if (internal_commands[i].func) {
                return internal_commands[i].func(argc, argv);
            } else {
                kalem_shell_println("Hata: Komut uygulanmamış.");
                return -1;
            }
        }
    }
    
    // Dahili komut bulunamadı, harici komut dene
    return kalem_shell_run_external_command(cmd_name, argc, argv);
}

// Harici komutu çalıştır
int kalem_shell_run_external_command(const char* cmd_name, int argc, char** argv) {
    // Gerçek bir sistemde burada PATH içinde arama yapılıp program yürütülecek
    char output[256];
    sprintf(output, "Komut bulunamadı: %s", cmd_name);
    kalem_shell_println(output);
    return -1;
}

// Root yetkilendirme
int kalem_shell_authenticate_root() {
    // Gerçek bir sistemde burada parola doğrulama olacak
    // Şimdilik her zaman başarılı
    active_shell->is_root = 1;
    kalem_shell_println("Root yetkileri etkinleştirildi.");
    return 0;
}

// Ekranı temizle
int kalem_shell_clear() {
    if (!active_shell) return -1;
    
    // Çıktı tamponunu temizle
    active_shell->output_buffer[0] = '\0';
    
    // Hoş geldiniz mesajını yeniden ekle
    kalem_shell_println("KALEM Shell v1.0 - KALEM OS Komut Yöneticisi");
    kalem_shell_println("-------------------------------------------------------");
    
    return 0;
}

// Mevcut çalışma dizinini değiştir
int kalem_shell_change_dir(const char* path) {
    if (!active_shell || !path) return -1;
    
    // Dizini değiştir (demo için yol kontrolü yok)
    strcpy(active_shell->current_dir, path);
    
    return 0;
}

// Dosya ve dizinleri listele
int kalem_shell_list_dir(const char* path) {
    if (!active_shell) return -1;
    
    // Demo içeriği göster
    kalem_shell_println("Dizin içeriği:");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  .");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  ..");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  bin");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  boot");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  dev");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  etc");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  home");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  lib");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  proc");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  sbin");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  sys");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  tmp");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  usr");
    kalem_shell_println("drwxr-xr-x   root root    4096 Mar 24 16:30  var");
    kalem_shell_println("-rw-r--r--   root root    2048 Mar 24 16:30  README.md");
    kalem_shell_println("-rwxr-xr-x   root root   10240 Mar 24 16:30  kalem.sh");
    
    return 0;
}

// Dosya içeriğini göster
int kalem_shell_cat(const char* filename) {
    if (!active_shell || !filename) return -1;
    
    // Demo dosya içeriği göster
    if (strcmp(filename, "README.md") == 0) {
        kalem_shell_println("# KALEM OS");
        kalem_shell_println("");
        kalem_shell_println("KALEM OS, Türkiye'nin kendi işletim sistemidir.");
        kalem_shell_println("");
        kalem_shell_println("## Özellikler");
        kalem_shell_println("");
        kalem_shell_println("- Türkçe arayüz");
        kalem_shell_println("- Güçlü güvenlik özellikleri");
        kalem_shell_println("- Performans odaklı");
        kalem_shell_println("- Kolay kullanım");
    } else if (strcmp(filename, "kalem.sh") == 0) {
        kalem_shell_println("#!/bin/sh");
        kalem_shell_println("# KALEM OS betik dosyası");
        kalem_shell_println("");
        kalem_shell_println("echo \"KALEM OS'a Hoş Geldiniz!\"");
        kalem_shell_println("echo \"Bu bir demo betik dosyasıdır.\"");
    } else {
        char output[256];
        sprintf(output, "Hata: '%s' dosyası bulunamadı veya okunamadı.", filename);
        kalem_shell_println(output);
        return -1;
    }
    
    return 0;
}

// Dosya oluştur
int kalem_shell_touch(const char* filename) {
    if (!active_shell || !filename) return -1;
    
    char output[256];
    sprintf(output, "Dosya oluşturuldu: %s", filename);
    kalem_shell_println(output);
    
    return 0;
}

// Dizin oluştur
int kalem_shell_mkdir(const char* dirname) {
    if (!active_shell || !dirname) return -1;
    
    char output[256];
    sprintf(output, "Dizin oluşturuldu: %s", dirname);
    kalem_shell_println(output);
    
    return 0;
}

// Dosya veya dizin sil
int kalem_shell_rm(const char* path) {
    if (!active_shell || !path) return -1;
    
    char output[256];
    sprintf(output, "Silindi: %s", path);
    kalem_shell_println(output);
    
    return 0;
}

// Sistem bilgilerini göster
int kalem_shell_sysinfo() {
    if (!active_shell) return -1;
    
    kalem_shell_println("KALEM OS Sistem Bilgileri");
    kalem_shell_println("-------------------------");
    kalem_shell_println("Sürüm: 1.0 Beta");
    kalem_shell_println("Çekirdek: KALEM Kernel 0.1.0");
    kalem_shell_println("Makine: Sanal Makine");
    kalem_shell_println("İşlemci: Virtual CPU");
    kalem_shell_println("Mimari: x86_64");
    kalem_shell_println("Bellek: 512 MB");
    kalem_shell_println("Disk: 4 GB");
    kalem_shell_println("Ana makine adı: kalem-os");
    kalem_shell_println("Kullanıcı: user");
    kalem_shell_println("Çalışma süresi: 0 gün, 0 saat, 10 dakika");
    kalem_shell_println("Yükleme: 0.02, 0.01, 0.00");
    
    return 0;
}

// Mevcut komutları listele
int kalem_shell_help() {
    if (!active_shell) return -1;
    
    kalem_shell_println("KALEM Shell Komutları");
    kalem_shell_println("---------------------");
    
    for (int i = 0; internal_commands[i].name != NULL; i++) {
        char line[256];
        sprintf(line, "%-10s - %s", internal_commands[i].name, internal_commands[i].help);
        kalem_shell_println(line);
    }
    
    return 0;
}

// Komut geçmişini göster
int kalem_shell_history() {
    if (!active_shell) return -1;
    
    kalem_shell_println("Komut Geçmişi");
    kalem_shell_println("-------------");
    
    for (int i = 0; i < active_shell->history_count; i++) {
        char line[256];
        sprintf(line, "%3d  %s", i + 1, active_shell->history[i]);
        kalem_shell_println(line);
    }
    
    return 0;
}

// Kabuktan çık
int kalem_shell_exit() {
    if (!active_shell) return -1;
    
    kalem_shell_println("KALEM Shell'den çıkılıyor...");
    kalem_shell_close();
    
    return 0;
}

// Yaşam döngüsü fonksiyonu
void kalem_shell_app() {
    kalem_shell_show();
} 