#include "../include/noteplus.h"
#include "../include/gui.h"
#include "../include/font.h"
#include "../include/vga.h"
#include <string.h>
#include <stdlib.h>

// Aktif editör örneği
static noteplus_editor_t* active_editor = NULL;

// Varsayılan örnek metin içeriği
static const char* default_text = 
    "/* Not++ - KALEM OS Metin Düzenleyici */\n\n"
    "#include <stdio.h>\n\n"
    "int main() {\n"
    "    // Hoş geldiniz\n"
    "    printf("KALEM OS Not++ Editörü'ne Hoş Geldiniz!\n");\n"
    "    \n"
    "    /* Bu bir demo metin dosyasıdır */\n"
    "    int sayi = 42;\n"
    "    double pi = 3.14159;\n"
    "    \n"
    "    if (sayi > 0) {\n"
    "        printf(\"Pozitif bir sayı\\n\");\n"
    "    }\n"
    "    \n"
    "    return 0;\n"
    "}\n";

// Anahtar kelimeler dizisi
static const char* c_keywords[] = {
    "auto", "break", "case", "char", "const", "continue",
    "default", "do", "double", "else", "enum", "extern",
    "float", "for", "goto", "if", "int", "long",
    "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while", "include", "define", "pragma", "printf",
    NULL
};

// Not++ uygulamasını başlat
void noteplus_init() {
    // İlk çalıştırma kontrolü
    if (active_editor == NULL) {
        active_editor = noteplus_create_editor();
        
        // Varsayılan metni ekle
        noteplus_insert_text(active_editor, default_text);
        
        // İmleci başa getir
        active_editor->cursor_pos = 0;
        active_editor->cursor_line = 0;
        active_editor->cursor_col = 0;
    }
}

// Not++ penceresini göster
void noteplus_show_window() {
    // Uygulama başlatılmadıysa başlat
    if (active_editor == NULL) {
        noteplus_init();
    }
    
    // Pencere zaten açıksa odakla
    if (active_editor->window) {
        gui_window_bring_to_front(active_editor->window);
        return;
    }
    
    // Pencereyi oluştur
    active_editor->window = gui_window_create("Not++ Editör", 100, 50, 480, 360, GUI_WINDOW_STYLE_NORMAL);
    if (!active_editor->window) return;
    
    // Pencere kapatıldığında referansı temizleyecek kapanış fonksiyonu
    active_editor->window->on_close = [](gui_window_t* window) {
        if (active_editor) {
            active_editor->window = NULL;
        }
    };
    
    // Çizim fonksiyonunu ayarla
    active_editor->window->on_paint = noteplus_paint;
    
    // Görünür satır ve sütun sayısını hesapla
    if (active_editor->window) {
        uint32_t client_height = active_editor->window->client.height;
        uint32_t client_width = active_editor->window->client.width;
        
        // Araç çubuğu ve durum çubuğu alanını çıkar
        uint32_t text_area_height = client_height - 50; // 30px araç çubuğu, 20px durum çubuğu
        
        active_editor->visible_lines = text_area_height / system_font->height;
        active_editor->visible_cols = client_width / system_font->width;
        
        if (active_editor->show_line_numbers) {
            active_editor->visible_cols -= 5; // Satır numarası için 5 karakter alanı çıkar
        }
    }
    
    // Pencereyi göster
    gui_window_show(active_editor->window);
}

// Açık noteplus editörünü al
noteplus_editor_t* noteplus_get_active_editor() {
    return active_editor;
}

// Düzenleyici yapısını oluştur
noteplus_editor_t* noteplus_create_editor() {
    noteplus_editor_t* editor = (noteplus_editor_t*)malloc(sizeof(noteplus_editor_t));
    if (!editor) return NULL;
    
    memset(editor, 0, sizeof(noteplus_editor_t));
    
    // Tampon belleği oluştur
    editor->text_buffer = (char*)malloc(NOTEPLUS_MAX_TEXT_SIZE);
    if (!editor->text_buffer) {
        free(editor);
        return NULL;
    }
    
    memset(editor->text_buffer, 0, NOTEPLUS_MAX_TEXT_SIZE);
    
    // Varsayılan değerleri ayarla
    editor->buffer_size = 0;
    editor->cursor_pos = 0;
    editor->cursor_line = 0;
    editor->cursor_col = 0;
    editor->scroll_y = 0;
    editor->scroll_x = 0;
    editor->visible_lines = 20;
    editor->visible_cols = 80;
    editor->modified = 0;
    editor->show_line_numbers = 1;
    editor->syntax_highlight = 1;
    strcpy(editor->current_file, "yeni_dosya.txt");
    
    return editor;
}

// Düzenleyici yapısını yok et
void noteplus_destroy_editor(noteplus_editor_t* editor) {
    if (!editor) return;
    
    // Tampon belleği temizle
    if (editor->text_buffer) {
        free(editor->text_buffer);
        editor->text_buffer = NULL;
    }
    
    // Editör yapısını temizle
    free(editor);
}

// Metin ekle
void noteplus_insert_text(noteplus_editor_t* editor, const char* text) {
    if (!editor || !text) return;
    
    uint32_t len = strlen(text);
    if (editor->buffer_size + len >= NOTEPLUS_MAX_TEXT_SIZE) {
        // Tampon dolu, daha fazla metin eklenemiyor
        return;
    }
    
    // İmleç konumunda yer aç
    memmove(editor->text_buffer + editor->cursor_pos + len, 
            editor->text_buffer + editor->cursor_pos, 
            editor->buffer_size - editor->cursor_pos);
    
    // Yeni metni ekle
    memcpy(editor->text_buffer + editor->cursor_pos, text, len);
    
    // Buffer boyutunu ve imleç konumunu güncelle
    editor->buffer_size += len;
    editor->cursor_pos += len;
    
    // NULL karakter ile sonlandır
    editor->text_buffer[editor->buffer_size] = '\0';
    
    // Satır ve sütun numaralarını güncelle
    uint32_t line = 0;
    uint32_t col = 0;
    for (uint32_t i = 0; i < editor->cursor_pos; i++) {
        if (editor->text_buffer[i] == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
    }
    
    editor->cursor_line = line;
    editor->cursor_col = col;
    
    // Değişiklik bayrağını ayarla
    editor->modified = 1;
}

// Karakter ekle
void noteplus_insert_char(noteplus_editor_t* editor, char c) {
    if (!editor) return;
    
    char buf[2] = {c, '\0'};
    noteplus_insert_text(editor, buf);
}

// Karakteri sil
void noteplus_delete_char(noteplus_editor_t* editor) {
    if (!editor || editor->cursor_pos == 0 || editor->buffer_size == 0) return;
    
    // İmleç konumundan bir önceki karakteri sil
    memmove(editor->text_buffer + editor->cursor_pos - 1, 
            editor->text_buffer + editor->cursor_pos, 
            editor->buffer_size - editor->cursor_pos);
    
    // Buffer boyutunu ve imleç konumunu güncelle
    editor->buffer_size--;
    editor->cursor_pos--;
    
    // NULL karakter ile sonlandır
    editor->text_buffer[editor->buffer_size] = '\0';
    
    // Satır ve sütun numaralarını güncelle
    uint32_t line = 0;
    uint32_t col = 0;
    for (uint32_t i = 0; i < editor->cursor_pos; i++) {
        if (editor->text_buffer[i] == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
    }
    
    editor->cursor_line = line;
    editor->cursor_col = col;
    
    // Değişiklik bayrağını ayarla
    editor->modified = 1;
}

// İmleci taşı
void noteplus_move_cursor(noteplus_editor_t* editor, int delta_x, int delta_y) {
    if (!editor) return;
    
    // Satır ve sütun numaralarını güncelle
    int new_line = (int)editor->cursor_line + delta_y;
    int new_col = (int)editor->cursor_col + delta_x;
    
    // Sınırları kontrol et
    if (new_line < 0) new_line = 0;
    if (new_col < 0) new_col = 0;
    
    uint32_t line_count = noteplus_get_line_count(editor);
    if (new_line >= (int)line_count) new_line = line_count - 1;
    
    uint32_t line_length = noteplus_get_line_length(editor, new_line);
    if (new_col > (int)line_length) new_col = line_length;
    
    // İmleç pozisyonunu ayarla
    noteplus_set_cursor(editor, new_line, new_col);
    
    // Kaydırma kontrolü
    if (editor->cursor_line < editor->scroll_y) {
        editor->scroll_y = editor->cursor_line;
    } else if (editor->cursor_line >= editor->scroll_y + editor->visible_lines) {
        editor->scroll_y = editor->cursor_line - editor->visible_lines + 1;
    }
    
    if (editor->cursor_col < editor->scroll_x) {
        editor->scroll_x = editor->cursor_col;
    } else if (editor->cursor_col >= editor->scroll_x + editor->visible_cols) {
        editor->scroll_x = editor->cursor_col - editor->visible_cols + 1;
    }
}

// İmleç pozisyonunu ayarla
void noteplus_set_cursor(noteplus_editor_t* editor, uint32_t line, uint32_t col) {
    if (!editor) return;
    
    // Satır pozisyonunu bul
    uint32_t pos = 0;
    uint32_t current_line = 0;
    
    while (current_line < line && pos < editor->buffer_size) {
        if (editor->text_buffer[pos] == '\n') {
            current_line++;
        }
        pos++;
    }
    
    // Sütun pozisyonunu ekle
    uint32_t current_col = 0;
    while (current_col < col && pos < editor->buffer_size && editor->text_buffer[pos] != '\n') {
        pos++;
        current_col++;
    }
    
    // İmleç pozisyonunu güncelle
    editor->cursor_pos = pos;
    editor->cursor_line = line;
    editor->cursor_col = col;
}

// Satırı al
const char* noteplus_get_line(noteplus_editor_t* editor, uint32_t line) {
    if (!editor) return NULL;
    
    // Satır başlangıç pozisyonunu bul
    uint32_t pos = 0;
    uint32_t current_line = 0;
    
    while (current_line < line && pos < editor->buffer_size) {
        if (editor->text_buffer[pos] == '\n') {
            current_line++;
        }
        pos++;
    }
    
    return &editor->text_buffer[pos];
}

// Satır uzunluğunu al
uint32_t noteplus_get_line_length(noteplus_editor_t* editor, uint32_t line) {
    if (!editor) return 0;
    
    const char* line_start = noteplus_get_line(editor, line);
    if (!line_start) return 0;
    
    uint32_t len = 0;
    while (line_start[len] && line_start[len] != '\n') {
        len++;
    }
    
    return len;
}

// Toplam satır sayısını al
uint32_t noteplus_get_line_count(noteplus_editor_t* editor) {
    if (!editor) return 0;
    
    uint32_t count = 1; // Boş dosya bile en az 1 satırdır
    
    for (uint32_t i = 0; i < editor->buffer_size; i++) {
        if (editor->text_buffer[i] == '\n') {
            count++;
        }
    }
    
    return count;
}

// Bir token türünü belirle
noteplus_token_type_t noteplus_get_token_type(const char* text, uint32_t pos) {
    // Yorum kontrolü
    if (pos > 0 && text[pos-1] == '/' && text[pos] == '/') {
        return NOTEPLUS_TOKEN_COMMENT;
    }
    
    if (pos > 0 && text[pos-1] == '/' && text[pos] == '*') {
        return NOTEPLUS_TOKEN_COMMENT;
    }
    
    // String kontrolü
    if (text[pos] == '"') {
        return NOTEPLUS_TOKEN_STRING;
    }
    
    // Sayı kontrolü
    if (text[pos] >= '0' && text[pos] <= '9') {
        return NOTEPLUS_TOKEN_NUMBER;
    }
    
    // Anahtar kelime kontrolü
    if ((text[pos] >= 'a' && text[pos] <= 'z') ||
        (text[pos] >= 'A' && text[pos] <= 'Z') ||
        text[pos] == '_') {
        
        // Kelimenin başlangıcını bul
        uint32_t start = pos;
        while (start > 0 && 
               ((text[start-1] >= 'a' && text[start-1] <= 'z') ||
                (text[start-1] >= 'A' && text[start-1] <= 'Z') ||
                (text[start-1] >= '0' && text[start-1] <= '9') ||
                text[start-1] == '_')) {
            start--;
        }
        
        // Kelimenin sonunu bul
        uint32_t end = pos;
        while ((text[end] >= 'a' && text[end] <= 'z') ||
               (text[end] >= 'A' && text[end] <= 'Z') ||
               (text[end] >= '0' && text[end] <= '9') ||
               text[end] == '_') {
            end++;
        }
        
        // Kelimeyi çıkar
        char word[64] = {0};
        uint32_t len = end - start;
        if (len < sizeof(word) - 1) {
            strncpy(word, &text[start], len);
            word[len] = '\0';
            
            // Kelimeyi anahtar kelimeler listesinde ara
            for (int i = 0; c_keywords[i] != NULL; i++) {
                if (strcmp(word, c_keywords[i]) == 0) {
                    return NOTEPLUS_TOKEN_KEYWORD;
                }
            }
        }
    }
    
    // Önişlemci direktifi kontrolü
    if (text[pos] == '#') {
        return NOTEPLUS_TOKEN_PREPROCESSOR;
    }
    
    return NOTEPLUS_TOKEN_NORMAL;
}

// Renk seçimi
uint8_t noteplus_get_token_color(noteplus_token_type_t type) {
    switch (type) {
        case NOTEPLUS_TOKEN_KEYWORD:
            return GUI_COLOR_LIGHT_BLUE;
        case NOTEPLUS_TOKEN_STRING:
            return GUI_COLOR_LIGHT_RED;
        case NOTEPLUS_TOKEN_NUMBER:
            return GUI_COLOR_LIGHT_MAGENTA;
        case NOTEPLUS_TOKEN_COMMENT:
            return GUI_COLOR_GREEN;
        case NOTEPLUS_TOKEN_PREPROCESSOR:
            return GUI_COLOR_MAGENTA;
        default:
            return GUI_COLOR_BLACK;
    }
}

// Kaydırma çubuğunu çiz
void noteplus_draw_scrollbar(noteplus_editor_t* editor) {
    if (!editor || !editor->window) return;
    
    uint32_t x = editor->window->x + editor->window->client.x;
    uint32_t y = editor->window->y + editor->window->client.y;
    uint32_t width = editor->window->client.width;
    uint32_t height = editor->window->client.height;
    
    // Dikey kaydırma çubuğu arkaplanı
    vga_fill_rect(x + width - 10, y + 30, 10, height - 50, GUI_COLOR_WINDOW_BG);
    
    // Dikey kaydırma çubuğu çerçevesi
    vga_draw_rect(x + width - 10, y + 30, 10, height - 50, GUI_COLOR_DARK_GRAY);
    
    // Dikey kaydırma çubuğu gezdirici
    uint32_t total_lines = noteplus_get_line_count(editor);
    uint32_t scrollbar_height = height - 50;
    uint32_t thumb_height = scrollbar_height * editor->visible_lines / total_lines;
    if (thumb_height < 10) thumb_height = 10;
    
    uint32_t thumb_pos = scrollbar_height * editor->scroll_y / total_lines;
    if (thumb_pos > scrollbar_height - thumb_height) {
        thumb_pos = scrollbar_height - thumb_height;
    }
    
    vga_fill_rect(x + width - 9, y + 31 + thumb_pos, 8, thumb_height, GUI_COLOR_BUTTON);
}

// Araç çubuğunu çiz
void noteplus_draw_toolbar(noteplus_editor_t* editor) {
    if (!editor || !editor->window) return;
    
    uint32_t x = editor->window->x + editor->window->client.x;
    uint32_t y = editor->window->y + editor->window->client.y;
    uint32_t width = editor->window->client.width;
    
    // Araç çubuğu arkaplanı
    vga_fill_rect(x, y, width, 30, GUI_COLOR_LIGHT_GRAY);
    
    // Araç çubuğu ayırıcı çizgisi
    vga_draw_hline(x, y + 29, width, GUI_COLOR_DARK_GRAY);
    
    // Butonlar
    const char* button_labels[] = {"Yeni", "Aç", "Kaydet", "Kes", "Kopyala", "Yapıştır", "Bul"};
    uint32_t button_x = x + 5;
    
    for (int i = 0; i < 7; i++) {
        uint32_t btn_width = strlen(button_labels[i]) * 8 + 10;
        
        vga_fill_rect(button_x, y + 5, btn_width, 20, GUI_COLOR_BUTTON);
        
        // 3D kenarlar
        vga_draw_hline(button_x, y + 5, btn_width, GUI_COLOR_WHITE);
        vga_draw_vline(button_x, y + 5, 20, GUI_COLOR_WHITE);
        vga_draw_hline(button_x, y + 24, btn_width, GUI_COLOR_DARK_GRAY);
        vga_draw_vline(button_x + btn_width - 1, y + 5, 20, GUI_COLOR_DARK_GRAY);
        
        // Buton metni
        font_draw_string(button_x + 5, y + 8, button_labels[i], GUI_COLOR_BLACK, 0xFF);
        
        button_x += btn_width + 5;
    }
}

// Durum çubuğunu çiz
void noteplus_draw_statusbar(noteplus_editor_t* editor) {
    if (!editor || !editor->window) return;
    
    uint32_t x = editor->window->x + editor->window->client.x;
    uint32_t y = editor->window->y + editor->window->client.y;
    uint32_t width = editor->window->client.width;
    uint32_t height = editor->window->client.height;
    
    // Durum çubuğu arkaplanı
    vga_fill_rect(x, y + height - 20, width, 20, GUI_COLOR_LIGHT_GRAY);
    
    // Durum çubuğu ayırıcı çizgisi
    vga_draw_hline(x, y + height - 20, width, GUI_COLOR_DARK_GRAY);
    
    // İmleç pozisyonu
    char status[64];
    sprintf(status, "Satır: %u, Sütun: %u", editor->cursor_line + 1, editor->cursor_col + 1);
    font_draw_string(x + 10, y + height - 15, status, GUI_COLOR_BLACK, 0xFF);
    
    // Satır sayısı
    sprintf(status, "Toplam satır: %u", noteplus_get_line_count(editor));
    font_draw_string(x + 200, y + height - 15, status, GUI_COLOR_BLACK, 0xFF);
    
    // Dosya adı
    if (editor->modified) {
        sprintf(status, "%s *", editor->current_file);
    } else {
        strcpy(status, editor->current_file);
    }
    font_draw_string(x + width - 150, y + height - 15, status, GUI_COLOR_DARK_BLUE, 0xFF);
}

// Metin alanını çiz
void noteplus_draw_text_area(noteplus_editor_t* editor) {
    if (!editor || !editor->window) return;
    
    uint32_t x = editor->window->x + editor->window->client.x;
    uint32_t y = editor->window->y + editor->window->client.y;
    uint32_t width = editor->window->client.width;
    uint32_t height = editor->window->client.height;
    
    // Metin arkaplanı
    vga_fill_rect(x, y + 30, width - 10, height - 50, GUI_COLOR_WHITE);
    
    // Satır numaraları arkaplanı
    if (editor->show_line_numbers) {
        vga_fill_rect(x, y + 30, 30, height - 50, GUI_COLOR_LIGHT_GRAY);
        vga_draw_vline(x + 30, y + 30, height - 50, GUI_COLOR_DARK_GRAY);
    }
    
    // Görünür satırları çiz
    uint32_t line_count = noteplus_get_line_count(editor);
    uint32_t line_height = system_font->height;
    uint32_t start_line = editor->scroll_y;
    uint32_t end_line = start_line + editor->visible_lines;
    
    if (end_line > line_count) {
        end_line = line_count;
    }
    
    uint32_t line_y = y + 30;
    
    for (uint32_t line = start_line; line < end_line; line++) {
        // Satır numarası
        if (editor->show_line_numbers) {
            char line_num[10];
            sprintf(line_num, "%4u", line + 1);
            font_draw_string(x + 5, line_y, line_num, GUI_COLOR_DARK_GRAY, 0xFF);
        }
        
        // Satır metnini al
        const char* line_text = noteplus_get_line(editor, line);
        if (!line_text) continue;
        
        // Satırı çiz
        uint32_t text_x = x + (editor->show_line_numbers ? 35 : 5);
        
        // Sözdizimi vurgulamasını uygula
        if (editor->syntax_highlight) {
            uint32_t line_len = noteplus_get_line_length(editor, line);
            uint32_t col = 0;
            
            while (col < line_len) {
                noteplus_token_type_t token_type = noteplus_get_token_type(line_text, col);
                uint8_t token_color = noteplus_get_token_color(token_type);
                
                // Karakteri çiz
                if (col >= editor->scroll_x && 
                    col < editor->scroll_x + editor->visible_cols) {
                    font_draw_char(text_x, line_y, line_text[col], token_color, 0xFF);
                    text_x += system_font->width;
                }
                
                col++;
            }
        } else {
            // Vurgulamasız satır çizimi
            font_draw_string(text_x, line_y, line_text, GUI_COLOR_BLACK, 0xFF);
        }
        
        // İmleç çizimi
        if (line == editor->cursor_line && 
            editor->cursor_col >= editor->scroll_x && 
            editor->cursor_col < editor->scroll_x + editor->visible_cols) {
            
            uint32_t cursor_x = x + (editor->show_line_numbers ? 35 : 5) + 
                               (editor->cursor_col - editor->scroll_x) * system_font->width;
            
            vga_draw_vline(cursor_x, line_y, line_height, GUI_COLOR_BLACK);
        }
        
        line_y += line_height;
    }
}

// Not++ uygulama penceresini çiz
void noteplus_paint(gui_window_t* window) {
    if (!window || !active_editor) return;
    
    // Araç çubuğunu çiz
    noteplus_draw_toolbar(active_editor);
    
    // Metin alanını çiz
    noteplus_draw_text_area(active_editor);
    
    // Kaydırma çubuğunu çiz
    noteplus_draw_scrollbar(active_editor);
    
    // Durum çubuğunu çiz
    noteplus_draw_statusbar(active_editor);
} 