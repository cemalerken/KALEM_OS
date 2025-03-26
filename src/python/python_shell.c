#include "../include/python_shell.h"
#include "../include/python_manager.h"
#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// PythonShell bileşenleri
static gui_window_t* shell_window = NULL;
static gui_terminal_t* shell_terminal = NULL;
static char command_buffer[1024] = {0};
static int command_pos = 0;
static int multiline_mode = 0;
static int continuation_lines = 0;

// Komut geçmişi
static python_shell_history_t history = {0};

// Yapılandırma
static python_shell_config_t config = {
    .show_line_numbers = 1,
    .syntax_highlighting = 1,
    .auto_indent = 1,
    .tab_complete = 1,
    .save_history = 1,
    .history_size = 100,
    .show_suggestions = 1,
    .multiline_mode = 0,
    .history_file = "/usr/share/kalem-python/history.txt",
    .colors = {
        .background_color = 0,   // Siyah
        .foreground_color = 7,   // Beyaz
        .prompt_color = 2,       // Yeşil
        .error_color = 4,        // Kırmızı
        .success_color = 2,      // Yeşil
        .highlight_color = 5,    // Magenta
        .comment_color = 8,      // Gri
        .keyword_color = 1,      // Mavi
        .function_color = 3,     // Cyan
        .string_color = 2,       // Yeşil
        .number_color = 5        // Magenta
    }
};

// İleri bildirimler
static void python_shell_process_command(const char* command);
static void python_shell_add_to_history(const char* command);
static void python_shell_navigate_history(int direction);
static void python_shell_show_prompt();
static void python_shell_autocompletion();
static int python_shell_is_complete_command(const char* command);

// Python çıktı işleyicisi
static void python_output_handler(const char* output) {
    if (shell_terminal) {
        gui_terminal_print(shell_terminal, output);
        gui_terminal_print(shell_terminal, "\n");
    }
}

// Python kabuğunu başlat
int python_shell_init() {
    // Python yorumlayıcısını başlat
    if (python_manager_init() != 0) {
        return -1;
    }
    
    // Python çıktı işleyicisini ayarla
    python_set_output_callback(python_output_handler);
    
    // Komut geçmişini yükle
    python_shell_load_history();
    
    // Ana pencere
    shell_window = gui_window_create("KALEM OS Python Shell", 800, 600, NULL);
    if (!shell_window) {
        return -1;
    }
    
    // Terminal
    shell_terminal = gui_terminal_create();
    gui_terminal_set_font(shell_terminal, "Monospace", 12);
    gui_terminal_set_colors(shell_terminal, config.colors.foreground_color, config.colors.background_color);
    
    gui_layout_t* layout = gui_layout_create(GUI_LAYOUT_VERTICAL);
    gui_layout_add_child(layout, shell_terminal);
    gui_window_set_layout(shell_window, layout);
    
    // Klavye olaylarını bağla
    gui_terminal_set_key_callback(shell_terminal, on_key_press);
    
    // Hoş geldiniz mesajını göster
    gui_terminal_print(shell_terminal, "KALEM OS Python Shell v1.0\n");
    gui_terminal_print(shell_terminal, "Python ");
    gui_terminal_print(shell_terminal, "3.9.0"); // Gerçek Python sürümü burada gösterilecek
    gui_terminal_print(shell_terminal, "\n\n");
    gui_terminal_print(shell_terminal, "Kullanılabilir kernel API modülleri:\n");
    gui_terminal_print(shell_terminal, "  - kernel.get_hardware_info()\n");
    gui_terminal_print(shell_terminal, "  - kernel.manage_driver(driver_name, action)\n");
    gui_terminal_print(shell_terminal, "  - kernel.process_manager()\n");
    gui_terminal_print(shell_terminal, "\n");
    gui_terminal_print(shell_terminal, "Yardım için 'help()' veya '?' yazabilirsiniz.\n");
    gui_terminal_print(shell_terminal, "Çıkmak için 'exit()' veya 'quit()' yazabilirsiniz.\n\n");
    
    // Komut istemini göster
    python_shell_show_prompt();
    
    return 0;
}

// Python kabuğunu göster
void python_shell_show() {
    if (!shell_window) {
        if (python_shell_init() != 0) {
            /* log_error("Python shell başlatılamadı"); */
            printf("Python shell başlatılamadı\n");
            return;
        }
    }
    
    gui_window_show(shell_window);
}

// Python kabuğunu gizle
void python_shell_hide() {
    if (shell_window) {
        gui_window_set_visibility(shell_window, 0);
    }
}

// Python kabuğunu temizle
int python_shell_cleanup() {
    // Komut geçmişini kaydet
    if (config.save_history) {
        python_shell_save_history();
    }
    
    // Geçmiş için ayrılan belleği temizle
    if (history.commands) {
        for (int i = 0; i < history.count; i++) {
            free(history.commands[i]);
        }
        free(history.commands);
        history.commands = NULL;
        history.count = 0;
        history.capacity = 0;
        history.current_index = 0;
    }
    
    if (shell_window) {
        gui_window_destroy(shell_window);
        shell_window = NULL;
        shell_terminal = NULL;
    }
    
    python_manager_cleanup();
    
    return 0;
}

// Python kabuğu yapılandırmasını al
int python_shell_get_config(python_shell_config_t* config_out) {
    if (!config_out) {
        return -1;
    }
    
    memcpy(config_out, &config, sizeof(python_shell_config_t));
    return 0;
}

// Python kabuğu yapılandırmasını ayarla
int python_shell_set_config(const python_shell_config_t* new_config) {
    if (!new_config) {
        return -1;
    }
    
    memcpy(&config, new_config, sizeof(python_shell_config_t));
    
    // Terminal renklerini güncelle
    if (shell_terminal) {
        gui_terminal_set_colors(shell_terminal, config.colors.foreground_color, config.colors.background_color);
    }
    
    return 0;
}

// Python kabuğunda komut çalıştır
int python_shell_execute(const char* command) {
    if (!command) {
        return -1;
    }
    
    // Komutu işle
    python_shell_process_command(command);
    
    return 0;
}

// Python kabuğuna metin ekle
void python_shell_append_text(const char* text, uint8_t color) {
    if (!shell_terminal || !text) {
        return;
    }
    
    // Terminale renkli metin ekle
    gui_terminal_set_color(shell_terminal, color);
    gui_terminal_print(shell_terminal, text);
    gui_terminal_set_color(shell_terminal, config.colors.foreground_color);
}

// Python kabuğuna yeni satır ekle
void python_shell_append_newline() {
    if (!shell_terminal) {
        return;
    }
    
    gui_terminal_print(shell_terminal, "\n");
}

// Python kabuğunu temizle
void python_shell_clear() {
    if (!shell_terminal) {
        return;
    }
    
    gui_terminal_clear(shell_terminal);
}

// Python kabuğu komut geçmişini kaydet
int python_shell_save_history() {
    if (!history.commands || history.count == 0) {
        return 0;
    }
    
    FILE* fp = fopen(config.history_file, "w");
    if (!fp) {
        return -1;
    }
    
    for (int i = 0; i < history.count; i++) {
        fprintf(fp, "%s\n", history.commands[i]);
    }
    
    fclose(fp);
    return 0;
}

// Python kabuğu komut geçmişini yükle
int python_shell_load_history() {
    // Eski geçmişi temizle
    if (history.commands) {
        for (int i = 0; i < history.count; i++) {
            free(history.commands[i]);
        }
        free(history.commands);
        history.commands = NULL;
        history.count = 0;
        history.capacity = 0;
    }
    
    // Yeni geçmiş için bellek ayır
    history.capacity = config.history_size;
    history.commands = (char**)malloc(history.capacity * sizeof(char*));
    if (!history.commands) {
        return -1;
    }
    
    // Geçmiş dosyasını aç
    FILE* fp = fopen(config.history_file, "r");
    if (!fp) {
        return 0; // Dosya yoksa hata değil
    }
    
    // Dosyadan satır satır oku
    char line[1024];
    while (fgets(line, sizeof(line), fp) && history.count < history.capacity) {
        // Satır sonundaki yeni satır karakterini kaldır
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Boş satırları atla
        if (strlen(line) == 0) {
            continue;
        }
        
        // Geçmişe ekle
        history.commands[history.count] = strdup(line);
        history.count++;
    }
    
    fclose(fp);
    
    // Geçerli indeksi geçmiş sonuna ayarla
    history.current_index = history.count;
    
    return 0;
}

// Klavye olay işleyicisi
void on_key_press(gui_terminal_t* terminal, int key, int modifier) {
    // Enter tuşuna basıldı, komutu çalıştır
    if (key == '\r' || key == '\n') {
        // Komut arabelleğini sonlandır
        command_buffer[command_pos] = '\0';
        
        // Komut boş değilse...
        if (command_pos > 0) {
            // Çok satırlı mod kontrolü
            if (multiline_mode || !python_shell_is_complete_command(command_buffer)) {
                // Komut tamamlanmamış, çok satırlı moda geç
                multiline_mode = 1;
                continuation_lines++;
                
                // Yeni satır ekle
                gui_terminal_print(terminal, "\n");
                
                // Devam komut istemi göster
                char prompt[16];
                sprintf(prompt, "... ");
                gui_terminal_set_color(terminal, config.colors.prompt_color);
                gui_terminal_print(terminal, prompt);
                gui_terminal_set_color(terminal, config.colors.foreground_color);
                
                // Komuta yeni satır ekle
                strcat(command_buffer, "\n");
                command_pos = strlen(command_buffer);
                
                return;
            }
            
            // Komutu geçmişe ekle
            python_shell_add_to_history(command_buffer);
            
            // Komut çıktısı için yeni satır
            gui_terminal_print(terminal, "\n");
            
            // Komutu işle
            python_shell_process_command(command_buffer);
        }
        
        // Komut modlarını sıfırla
        multiline_mode = 0;
        continuation_lines = 0;
        
        // Komut arabelleğini temizle
        command_pos = 0;
        memset(command_buffer, 0, sizeof(command_buffer));
        
        // Yeni komut istemi göster
        python_shell_show_prompt();
        return;
    }
    
    // Backspace tuşuna basıldı, karakteri sil
    if (key == 8 || key == 127) {
        if (command_pos > 0) {
            command_pos--;
            command_buffer[command_pos] = '\0';
            
            // Terminal görüntüsünü güncelle
            gui_terminal_backspace(terminal);
        }
        return;
    }
    
    // Tab tuşuna basıldı, otomatik tamamlama
    if (key == 9) {
        if (config.tab_complete) {
            python_shell_autocompletion();
        }
        return;
    }
    
    // Yukarı ok tuşuna basıldı, geçmişte geri git
    if (key == 1000) { // Yukarı ok simüle ediliyor
        python_shell_navigate_history(-1);
        return;
    }
    
    // Aşağı ok tuşuna basıldı, geçmişte ileri git
    if (key == 1001) { // Aşağı ok simüle ediliyor
        python_shell_navigate_history(1);
        return;
    }
    
    // ESC tuşuna basıldı, çok satırlı modu iptal et
    if (key == 27) {
        if (multiline_mode) {
            multiline_mode = 0;
            continuation_lines = 0;
            command_pos = 0;
            memset(command_buffer, 0, sizeof(command_buffer));
            
            gui_terminal_print(terminal, "\n");
            python_shell_show_prompt();
        }
        return;
    }
    
    // Normal karakter girdisi
    if (key >= 32 && key <= 126) {
        // Komut arabelleği dolmadıysa karakteri ekle
        if (command_pos < sizeof(command_buffer) - 1) {
            command_buffer[command_pos++] = (char)key;
            
            // Karakteri terminalde göster
            char c[2] = {(char)key, '\0'};
            gui_terminal_print(terminal, c);
        }
    }
}

// Komutu işle
static void python_shell_process_command(const char* command) {
    // Özel komutları işle
    if (strcmp(command, "exit") == 0 || strcmp(command, "exit()") == 0 || 
        strcmp(command, "quit") == 0 || strcmp(command, "quit()") == 0) {
        // Python kabuğunu kapat
        python_shell_cleanup();
        return;
    }
    
    if (strcmp(command, "clear") == 0 || strcmp(command, "cls") == 0) {
        // Terminal ekranını temizle
        python_shell_clear();
        return;
    }
    
    if (strcmp(command, "help") == 0 || strcmp(command, "help()") == 0 || strcmp(command, "?") == 0) {
        // Yardım göster
        gui_terminal_print(shell_terminal, "KALEM OS Python Shell Yardım\n");
        gui_terminal_print(shell_terminal, "-----------------------------\n");
        gui_terminal_print(shell_terminal, "Kernel API Komutları:\n");
        gui_terminal_print(shell_terminal, "  kernel.get_hardware_info() - Donanım bilgilerini gösterir\n");
        gui_terminal_print(shell_terminal, "  kernel.manage_driver(driver_name, action) - Sürücü yönetimi\n");
        gui_terminal_print(shell_terminal, "  kernel.process_manager() - Süreç listesini gösterir\n\n");
        gui_terminal_print(shell_terminal, "Sistem Komutları:\n");
        gui_terminal_print(shell_terminal, "  exit() veya quit() - Python kabuğundan çıkar\n");
        gui_terminal_print(shell_terminal, "  clear veya cls - Ekranı temizler\n");
        gui_terminal_print(shell_terminal, "  help() veya ? - Bu yardım mesajını gösterir\n\n");
        gui_terminal_print(shell_terminal, "Kısayollar:\n");
        gui_terminal_print(shell_terminal, "  Tab - Otomatik tamamlama\n");
        gui_terminal_print(shell_terminal, "  Yukarı/Aşağı Ok - Komut geçmişinde gezinme\n");
        gui_terminal_print(shell_terminal, "  Esc - Çok satırlı modu iptal etme\n\n");
        return;
    }
    
    // Komutu Python yorumlayıcısına gönder
    int result = python_run_string(command);
    
    if (result != 0) {
        // Hata mesajını göster
        const char* err = python_get_last_error();
        if (err) {
            gui_terminal_set_color(shell_terminal, config.colors.error_color);
            gui_terminal_print(shell_terminal, "Hata: ");
            gui_terminal_print(shell_terminal, err);
            gui_terminal_print(shell_terminal, "\n");
            gui_terminal_set_color(shell_terminal, config.colors.foreground_color);
        }
    }
}

// Komut geçmişine komut ekle
static void python_shell_add_to_history(const char* command) {
    // Komut boşsa ekleme
    if (!command || strlen(command) == 0) {
        return;
    }
    
    // Geçmiş boşsa veya geçmiş kapasitesi 0 ise ekleme
    if (!history.commands || history.capacity == 0) {
        return;
    }
    
    // Aynı komutu tekrar ekleme
    if (history.count > 0 && strcmp(history.commands[history.count - 1], command) == 0) {
        return;
    }
    
    // Geçmiş doluysa ilk komutu sil
    if (history.count >= history.capacity) {
        free(history.commands[0]);
        for (int i = 1; i < history.count; i++) {
            history.commands[i - 1] = history.commands[i];
        }
        history.count--;
    }
    
    // Komutu geçmişe ekle
    history.commands[history.count] = strdup(command);
    history.count++;
    history.current_index = history.count;
}

// Geçmişte gezin
static void python_shell_navigate_history(int direction) {
    // Geçmiş boşsa hiçbir şey yapma
    if (!history.commands || history.count == 0) {
        return;
    }
    
    // Yeni indeksi hesapla
    int new_index = history.current_index + direction;
    
    // İndeks sınırlarını kontrol et
    if (new_index < 0 || new_index > history.count) {
        return;
    }
    
    // Geçerli komutu temizle
    while (command_pos > 0) {
        gui_terminal_backspace(shell_terminal);
        command_pos--;
    }
    
    // Eğer indeks geçmiş sonundaysa boş komut göster
    if (new_index == history.count) {
        command_buffer[0] = '\0';
        command_pos = 0;
    } else {
        // Geçmişteki komutu göster
        strncpy(command_buffer, history.commands[new_index], sizeof(command_buffer) - 1);
        command_pos = strlen(command_buffer);
        gui_terminal_print(shell_terminal, command_buffer);
    }
    
    // Geçerli indeksi güncelle
    history.current_index = new_index;
}

// Komut istemi göster
static void python_shell_show_prompt() {
    if (!shell_terminal) {
        return;
    }
    
    char prompt[16];
    sprintf(prompt, ">>> ");
    gui_terminal_set_color(shell_terminal, config.colors.prompt_color);
    gui_terminal_print(shell_terminal, prompt);
    gui_terminal_set_color(shell_terminal, config.colors.foreground_color);
}

// Otomatik tamamlama
static void python_shell_autocompletion() {
    // Komut boşsa hiçbir şey yapma
    if (command_pos == 0) {
        return;
    }
    
    // Komutun son kelimesini bul
    char* last_word = command_buffer;
    for (int i = 0; i < command_pos; i++) {
        if (command_buffer[i] == ' ' || command_buffer[i] == '.') {
            last_word = &command_buffer[i + 1];
        }
    }
    
    // TODO: Python yorumlayıcısından tamamlama önerilerini al
    // Bu örnek için basit öneriler sunuyoruz
    const char* suggestions[] = {
        "print", "kernel", "import", "def", "class", "if", "else", "elif", "for", "while", 
        "break", "continue", "return", "True", "False", "None", "and", "or", "not", "in", 
        "is", "try", "except", "finally", "with", "as", "from", "global", "nonlocal", "assert"
    };
    
    // Eşleşen önerileri bul
    char matching[10][32]; // En fazla 10 eşleşme ve her biri en fazla 32 karakter
    int match_count = 0;
    
    for (int i = 0; i < sizeof(suggestions) / sizeof(suggestions[0]); i++) {
        if (strncmp(last_word, suggestions[i], strlen(last_word)) == 0) {
            strncpy(matching[match_count], suggestions[i], sizeof(matching[0]) - 1);
            match_count++;
            
            if (match_count >= 10) {
                break;
            }
        }
    }
    
    // Tek eşleşme varsa tamamla
    if (match_count == 1) {
        // Son kelimenin sonraki kısmını ekleyin
        int last_word_len = strlen(last_word);
        int suggestion_len = strlen(matching[0]);
        
        for (int i = last_word_len; i < suggestion_len; i++) {
            if (command_pos < sizeof(command_buffer) - 1) {
                command_buffer[command_pos++] = matching[0][i];
                char c[2] = {matching[0][i], '\0'};
                gui_terminal_print(shell_terminal, c);
            }
        }
    }
    // Birden fazla eşleşme varsa listeyi göster
    else if (match_count > 1) {
        gui_terminal_print(shell_terminal, "\n");
        
        for (int i = 0; i < match_count; i++) {
            gui_terminal_print(shell_terminal, matching[i]);
            gui_terminal_print(shell_terminal, "  ");
        }
        
        gui_terminal_print(shell_terminal, "\n");
        python_shell_show_prompt();
        gui_terminal_print(shell_terminal, command_buffer);
    }
}

// Komutun tamamlandığını kontrol et
static int python_shell_is_complete_command(const char* command) {
    // Boş komutu tamamlanmış olarak kabul et
    if (!command || strlen(command) == 0) {
        return 1;
    }
    
    // Son karakteri kontrol et
    int len = strlen(command);
    if (command[len - 1] == ':') {
        return 0; // if, for, while gibi blok başlangıçları
    }
    
    // Parantez, köşeli parantez ve süslü parantez sayılarını kontrol et
    int parens = 0, brackets = 0, braces = 0;
    for (int i = 0; i < len; i++) {
        if (command[i] == '(') parens++;
        else if (command[i] == ')') parens--;
        else if (command[i] == '[') brackets++;
        else if (command[i] == ']') brackets--;
        else if (command[i] == '{') braces++;
        else if (command[i] == '}') braces--;
    }
    
    // Eğer parantezler eşleşmiyorsa, komut tamamlanmamış
    if (parens != 0 || brackets != 0 || braces != 0) {
        return 0;
    }
    
    // Tırnak işareti sayısını kontrol et
    int single_quotes = 0, double_quotes = 0;
    for (int i = 0; i < len; i++) {
        if (command[i] == '\'' && (i == 0 || command[i-1] != '\\')) single_quotes++;
        else if (command[i] == '"' && (i == 0 || command[i-1] != '\\')) double_quotes++;
    }
    
    // Eğer tırnak işaretleri tek sayıdaysa, komut tamamlanmamış
    if (single_quotes % 2 != 0 || double_quotes % 2 != 0) {
        return 0;
    }
    
    // Aksi halde, komut tamamlanmış
    return 1;
} 