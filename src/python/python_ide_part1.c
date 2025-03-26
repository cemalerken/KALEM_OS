#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "python_ide.h"
#include "python_manager.h"
#include "logger.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <time.h>

// IDE bileşenleri
static gui_window_t* ide_window = NULL;
static gui_text_editor_t* code_editor = NULL;
static gui_terminal_t* output_terminal = NULL;
static gui_button_t* run_button = NULL;
static gui_button_t* debug_button = NULL;
static gui_button_t* save_button = NULL;
static gui_button_t* open_button = NULL;
static gui_button_t* new_button = NULL;
static gui_button_t* find_button = NULL;
static gui_button_t* replace_button = NULL;

// Geçerli dosya
static char current_file[256] = {0};

// Proje bilgisi
static python_ide_project_t current_project = {0};

// IDE yapılandırması
static python_ide_config_t ide_config = {
    .show_line_numbers = 1,
    .syntax_highlighting = 1,
    .auto_indent = 1,
    .auto_complete = 1,
    .wrap_text = 0,
    .show_whitespace = 0,
    .highlight_current_line = 1,
    .auto_save = 1,
    .auto_save_interval = 300, // 5 dakika
    .tab_size = 4,
    .use_spaces_for_tab = 1,
    .theme = PYTHON_IDE_THEME_DARK,
    .colors = {
        .background_color = 0,   // Siyah
        .foreground_color = 7,   // Beyaz
        .line_number_bg_color = 8, // Koyu gri
        .line_number_fg_color = 7, // Beyaz
        .current_line_color = 8,  // Koyu gri
        .error_line_color = 4,    // Kırmızı
        .breakpoint_color = 4,    // Kırmızı
        .selection_color = 5,     // Magenta
        .toolbar_bg_color = 8,    // Koyu gri
        .toolbar_fg_color = 7,    // Beyaz
        .terminal_bg_color = 0,   // Siyah
        .terminal_fg_color = 7,   // Beyaz
        .hint_color = 6,          // Cyan
        .result_color = 2         // Yeşil
    },
    .syntax_colors = {
        .keyword_color = 1,       // Mavi
        .function_color = 3,      // Cyan
        .class_color = 5,         // Magenta
        .string_color = 2,        // Yeşil
        .number_color = 5,        // Magenta
        .comment_color = 8,       // Gri
        .decorator_color = 6,     // Cyan
        .operator_color = 7,      // Beyaz
        .variable_color = 7,      // Beyaz
        .builtin_color = 3,       // Cyan
        .constant_color = 5,      // Magenta
        .error_color = 4          // Kırmızı
    }
};

// İleri bildirimler
void on_run_button_clicked(gui_button_t* button, void* user_data);
void on_debug_button_clicked(gui_button_t* button, void* user_data);
void on_save_button_clicked(gui_button_t* button, void* user_data);
void on_open_button_clicked(gui_button_t* button, void* user_data);
void on_new_button_clicked(gui_button_t* button, void* user_data);
void on_find_button_clicked(gui_button_t* button, void* user_data);
void on_replace_button_clicked(gui_button_t* button, void* user_data);

// Python çıktı yakalayıcısı
void on_python_output(const char* output) {
    if (output_terminal) {
        gui_terminal_print(output_terminal, output);
    }
}

// IDE'yi başlat
int python_ide_init() {
    // Python yorumlayıcısını başlat
    if (python_manager_init() != 0) {
        return -1;
    }
    
    // Python çıktı yakalama işleyicisini ayarla
    python_set_output_callback(on_python_output);
    
    // Ana pencere
    ide_window = gui_window_create("KALEM OS Python IDE", 1024, 768, NULL);
    if (!ide_window) {
        return -1;
    }
    
    // Ana düzen
    gui_layout_t* main_layout = gui_layout_create(GUI_LAYOUT_VERTICAL);
    gui_window_set_layout(ide_window, main_layout);
    
    // Araç çubuğu
    gui_layout_t* toolbar = gui_layout_create(GUI_LAYOUT_HORIZONTAL);
    gui_layout_add_child(main_layout, toolbar);
    
    // Araç çubuğu butonları
    new_button = gui_button_create("Yeni", on_new_button_clicked, NULL);
    open_button = gui_button_create("Aç", on_open_button_clicked, NULL);
    save_button = gui_button_create("Kaydet", on_save_button_clicked, NULL);
    run_button = gui_button_create("Çalıştır", on_run_button_clicked, NULL);
    debug_button = gui_button_create("Hata Ayıkla", on_debug_button_clicked, NULL);
    find_button = gui_button_create("Bul", on_find_button_clicked, NULL);
    replace_button = gui_button_create("Değiştir", on_replace_button_clicked, NULL);
    
    // Butonları araç çubuğuna ekle
    gui_layout_add_child(toolbar, new_button);
    gui_layout_add_child(toolbar, open_button);
    gui_layout_add_child(toolbar, save_button);
    gui_layout_add_child(toolbar, gui_separator_create(GUI_SEPARATOR_VERTICAL));
    gui_layout_add_child(toolbar, run_button);
    gui_layout_add_child(toolbar, debug_button);
    gui_layout_add_child(toolbar, gui_separator_create(GUI_SEPARATOR_VERTICAL));
    gui_layout_add_child(toolbar, find_button);
    gui_layout_add_child(toolbar, replace_button);
    
    // Kod editörü ve terminal alanı
    gui_layout_t* editor_area = gui_layout_create(GUI_LAYOUT_HORIZONTAL);
    gui_layout_set_expand(editor_area, 1);
    gui_layout_add_child(main_layout, editor_area);
    
    // Kod editörü
    code_editor = gui_text_editor_create();
    gui_text_editor_set_syntax_highlighting(code_editor, GUI_SYNTAX_PYTHON);
    gui_text_editor_set_line_numbers(code_editor, ide_config.show_line_numbers);
    gui_text_editor_set_auto_indent(code_editor, ide_config.auto_indent);
    gui_text_editor_set_tab_size(code_editor, ide_config.tab_size);
    gui_text_editor_set_use_spaces_for_tab(code_editor, ide_config.use_spaces_for_tab);
    gui_text_editor_set_highlight_current_line(code_editor, ide_config.highlight_current_line);
    gui_text_editor_set_word_wrap(code_editor, ide_config.wrap_text);
    gui_text_editor_set_show_whitespace(code_editor, ide_config.show_whitespace);
    
    // Renkleri ayarla
    gui_text_editor_set_colors(code_editor, 
                               ide_config.colors.foreground_color, 
                               ide_config.colors.background_color);
    gui_text_editor_set_line_number_colors(code_editor, 
                                           ide_config.colors.line_number_fg_color, 
                                           ide_config.colors.line_number_bg_color);
    gui_text_editor_set_current_line_color(code_editor, ide_config.colors.current_line_color);
    gui_text_editor_set_selection_color(code_editor, ide_config.colors.selection_color);
    
    // Sözdizimi renklerini ayarla
    gui_text_editor_set_syntax_colors(code_editor, 
                                      ide_config.syntax_colors.keyword_color,
                                      ide_config.syntax_colors.function_color,
                                      ide_config.syntax_colors.string_color,
                                      ide_config.syntax_colors.number_color,
                                      ide_config.syntax_colors.comment_color);
    
    // Editörü düzene ekle
    gui_layout_add_child(editor_area, code_editor);
    gui_layout_set_expand_child(editor_area, code_editor, 2);
    
    // Terminal çıktısı
    output_terminal = gui_terminal_create();
    gui_terminal_set_colors(output_terminal, 
                           ide_config.colors.terminal_fg_color, 
                           ide_config.colors.terminal_bg_color);
    gui_layout_add_child(editor_area, output_terminal);
    gui_layout_set_expand_child(editor_area, output_terminal, 1);
    
    // Durum çubuğu
    gui_status_bar_t* status_bar = gui_status_bar_create();
    gui_window_set_status_bar(ide_window, status_bar);
    gui_status_bar_set_text(status_bar, "Python IDE hazır - kernel-python ortamı");
    
    return 0;
}

// IDE'yi göster
void python_ide_show() {
    if (!ide_window) {
        if (python_ide_init() != 0) {
            /* log_error("Python IDE başlatılamadı"); */
            printf("Python IDE başlatılamadı\n");
            return;
        }
    }
    
    gui_window_show(ide_window);
}

// IDE'yi temizle
int python_ide_cleanup() {
    if (ide_window) {
        // Dosya açıksa ve otomatik kaydetme etkinse, kaydet
        if (strlen(current_file) > 0 && ide_config.auto_save) {
            python_ide_save_file();
        }
        
        gui_window_destroy(ide_window);
        ide_window = NULL;
        code_editor = NULL;
        output_terminal = NULL;
        run_button = NULL;
        debug_button = NULL;
        save_button = NULL;
        open_button = NULL;
        new_button = NULL;
        find_button = NULL;
        replace_button = NULL;
    }
    
    python_manager_cleanup();
    
    return 0;
}

// Dosya aç
int python_ide_open_file(const char* file_path) {
    if (!file_path) {
        return -1;
    }
    
    if (!ide_window) {
        if (python_ide_init() != 0) {
            return -1;
        }
    }
    
    // Dosyayı aç
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        gui_terminal_print(output_terminal, "Hata: Dosya açılamadı: ");
        gui_terminal_print(output_terminal, file_path);
        gui_terminal_print(output_terminal, "\n");
        return -1;
    }
    
    // Dosya içeriğini oku
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        gui_terminal_print(output_terminal, "Hata: Bellek ayırma hatası\n");
        return -1;
    }
    
    size_t read_size = fread(buffer, 1, size, fp);
    buffer[read_size] = '\0';
    fclose(fp);
    
    // Editöre içeriği yükle
    gui_text_editor_set_text(code_editor, buffer);
    free(buffer);
    
    // Geçerli dosya yolunu kaydet
    strncpy(current_file, file_path, sizeof(current_file) - 1);
    
    // Pencere başlığını güncelle
    char title[300];
    snprintf(title, sizeof(title), "KALEM OS Python IDE - %s", file_path);
    gui_window_set_title(ide_window, title);
    
    gui_terminal_print(output_terminal, "Dosya açıldı: ");
    gui_terminal_print(output_terminal, file_path);
    gui_terminal_print(output_terminal, "\n");
    
    return 0;
}

// Geçerli dosyayı kaydet
int python_ide_save_file() {
    if (!ide_window || !code_editor) {
        return -1;
    }
    
    // Dosya yolu belirtilmemişse kullanıcıdan iste
    if (strlen(current_file) == 0) {
        return python_ide_save_as(NULL);
    }
    
    // Editör içeriğini al
    char* content = gui_text_editor_get_text(code_editor);
    if (!content) {
        gui_terminal_print(output_terminal, "Hata: Editör içeriği alınamadı\n");
        return -1;
    }
    
    // Dosyaya kaydet
    FILE* fp = fopen(current_file, "w");
    if (!fp) {
        gui_terminal_print(output_terminal, "Hata: Dosya kaydedilemedi: ");
        gui_terminal_print(output_terminal, current_file);
        gui_terminal_print(output_terminal, "\n");
        free(content);
        return -1;
    }
    
    fputs(content, fp);
    fclose(fp);
    free(content);
    
    gui_terminal_print(output_terminal, "Dosya kaydedildi: ");
    gui_terminal_print(output_terminal, current_file);
    gui_terminal_print(output_terminal, "\n");
    
    return 0;
}

// Dosyayı farklı kaydet
int python_ide_save_as(const char* file_path) {
    if (!ide_window || !code_editor) {
        return -1;
    }
    
    char new_path[256];
    
    // Eğer dosya yolu belirtilmemişse, kullanıcıdan iste
    if (!file_path) {
        char* selected_path = gui_show_save_dialog("Python Dosyası Kaydet", "*.py");
        if (!selected_path) {
            // Kullanıcı iptal etti
            return 0;
        }
        
        strncpy(new_path, selected_path, sizeof(new_path) - 1);
        free(selected_path);
    } else {
        strncpy(new_path, file_path, sizeof(new_path) - 1);
    }
    
    // Yeni dosya yolunu ayarla
    strncpy(current_file, new_path, sizeof(current_file) - 1);
    
    // Pencere başlığını güncelle
    char title[300];
    snprintf(title, sizeof(title), "KALEM OS Python IDE - %s", current_file);
    gui_window_set_title(ide_window, title);
    
    // Dosyayı kaydet
    return python_ide_save_file();
}

// Geçerli dosyayı çalıştır
int python_ide_run_current_file() {
    if (!ide_window || !code_editor) {
        return -1;
    }
    
    // Dosya açık değilse hata ver
    if (strlen(current_file) == 0) {
        gui_terminal_print(output_terminal, "Hata: Önce dosyayı kaydedin!\n");
        return -1;
    }
    
    // Dosyayı kaydet
    if (python_ide_save_file() != 0) {
        return -1;
    }
    
    // Terminal çıktısını temizle
    gui_terminal_clear(output_terminal);
    gui_terminal_print(output_terminal, "Çalıştırılıyor: ");
    gui_terminal_print(output_terminal, current_file);
    gui_terminal_print(output_terminal, "\n\n");
    
    // Betiği çalıştır
    int result = python_run_script(current_file);
    
    if (result != 0) {
        gui_terminal_print(output_terminal, "\nHata: Betik çalıştırılamadı!\n");
        gui_terminal_print(output_terminal, python_get_last_error());
        gui_terminal_print(output_terminal, "\n");
        return -1;
    } else {
        gui_terminal_print(output_terminal, "\nBetik başarıyla tamamlandı.\n");
    }
    
    return 0;
} 