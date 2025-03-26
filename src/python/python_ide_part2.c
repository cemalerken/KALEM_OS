// Python IDE ikinci kısım - ilk kısımdan devam
// Bu Python IDE kaynak kodunun ikinci kısmıdır
// İlk kısımdan fonksiyon tanımlarına devam ediyoruz

// Geçerli dosyada hata ayıklama yap
int python_ide_debug_current_file() {
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
    gui_terminal_print(output_terminal, "Hata ayıklama başlatılıyor: ");
    gui_terminal_print(output_terminal, current_file);
    gui_terminal_print(output_terminal, "\n\n");
    
    // Python hata ayıklayıcıyı başlat
    char debug_cmd[512];
    snprintf(debug_cmd, sizeof(debug_cmd), "import pdb; pdb.run('exec(open(\"%s\").read())')", current_file);
    
    // Komutu çalıştır
    int result = python_run_string(debug_cmd);
    
    if (result != 0) {
        gui_terminal_print(output_terminal, "\nHata: Hata ayıklayıcı başlatılamadı!\n");
        gui_terminal_print(output_terminal, python_get_last_error());
        gui_terminal_print(output_terminal, "\n");
        return -1;
    }
    
    return 0;
}

// IDE yapılandırmasını al
int python_ide_get_config(python_ide_config_t* config_out) {
    if (!config_out) {
        return -1;
    }
    
    memcpy(config_out, &ide_config, sizeof(python_ide_config_t));
    return 0;
}

// IDE yapılandırmasını ayarla
int python_ide_set_config(const python_ide_config_t* config) {
    if (!config) {
        return -1;
    }
    
    memcpy(&ide_config, config, sizeof(python_ide_config_t));
    
    // Eğer IDE başlatılmışsa, ayarları uygula
    if (code_editor) {
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
    }
    
    if (output_terminal) {
        gui_terminal_set_colors(output_terminal, 
                               ide_config.colors.terminal_fg_color, 
                               ide_config.colors.terminal_bg_color);
    }
    
    return 0;
}

// Proje aç
int python_ide_open_project(const char* project_path) {
    if (!project_path) {
        return -1;
    }
    
    // Proje dizini var mı kontrol et
    struct stat st = {0};
    if (stat(project_path, &st) == -1 || !S_ISDIR(st.st_mode)) {
        gui_terminal_print(output_terminal, "Hata: Proje dizini bulunamadı veya geçerli değil: ");
        gui_terminal_print(output_terminal, project_path);
        gui_terminal_print(output_terminal, "\n");
        return -1;
    }
    
    // Proje yapılandırma dosyasını ara
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/pyproject.toml", project_path);
    
    FILE* fp = fopen(config_path, "r");
    if (!fp) {
        // Yapılandırma dosyası yoksa, requirements.txt ara
        snprintf(config_path, sizeof(config_path), "%s/requirements.txt", project_path);
        fp = fopen(config_path, "r");
        
        // requirements.txt yoksa varsayılan bir yapılandırma oluştur
        if (!fp) {
            // Proje adını dizin adından al
            char* project_name = strrchr(project_path, '/');
            if (project_name) {
                project_name++; // '/' karakterini atla
            } else {
                project_name = (char*)project_path;
            }
            
            // Proje bilgilerini ayarla
            memset(&current_project, 0, sizeof(python_ide_project_t));
            strncpy(current_project.name, project_name, sizeof(current_project.name) - 1);
            strncpy(current_project.path, project_path, sizeof(current_project.path) - 1);
            
            // Ana Python dosyasını ara
            char main_py_path[512];
            snprintf(main_py_path, sizeof(main_py_path), "%s/main.py", project_path);
            if (stat(main_py_path, &st) == 0) {
                strncpy(current_project.main_file, main_py_path, sizeof(current_project.main_file) - 1);
                
                // Ana dosyayı aç
                python_ide_open_file(main_py_path);
            } else {
                // main.py yoksa, ilk .py dosyasını ara
                DIR* dir = opendir(project_path);
                if (dir) {
                    struct dirent* entry;
                    while ((entry = readdir(dir)) != NULL) {
                        char* ext = strrchr(entry->d_name, '.');
                        if (ext && strcmp(ext, ".py") == 0) {
                            char py_path[512];
                            snprintf(py_path, sizeof(py_path), "%s/%s", project_path, entry->d_name);
                            strncpy(current_project.main_file, py_path, sizeof(current_project.main_file) - 1);
                            
                            // Dosyayı aç
                            python_ide_open_file(py_path);
                            break;
                        }
                    }
                    closedir(dir);
                }
            }
            
            gui_terminal_print(output_terminal, "Proje açıldı: ");
            gui_terminal_print(output_terminal, current_project.name);
            gui_terminal_print(output_terminal, "\n");
            
            return 0;
        }
    }
    
    // Yapılandırma dosyasını oku
    if (fp) {
        // TODO: Yapılandırma dosyasını parse et
        // Bu örnekte basit bir şekilde işliyoruz
        
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            // Yeni satır karakterini kaldır
            int len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            
            // Proje adını bul
            if (strncmp(line, "name = ", 7) == 0) {
                char* name = line + 7;
                // Tırnak işaretlerini kaldır
                if (name[0] == '"' && name[strlen(name) - 1] == '"') {
                    name[strlen(name) - 1] = '\0';
                    name++;
                }
                strncpy(current_project.name, name, sizeof(current_project.name) - 1);
            }
            
            // Sanal ortam yolunu bul
            if (strncmp(line, "venv = ", 7) == 0) {
                char* venv = line + 7;
                // Tırnak işaretlerini kaldır
                if (venv[0] == '"' && venv[strlen(venv) - 1] == '"') {
                    venv[strlen(venv) - 1] = '\0';
                    venv++;
                }
                strncpy(current_project.venv_path, venv, sizeof(current_project.venv_path) - 1);
                current_project.use_venv = 1;
            }
            
            // Ana dosyayı bul
            if (strncmp(line, "main = ", 7) == 0) {
                char* main = line + 7;
                // Tırnak işaretlerini kaldır
                if (main[0] == '"' && main[strlen(main) - 1] == '"') {
                    main[strlen(main) - 1] = '\0';
                    main++;
                }
                
                // Tam yol oluştur
                char main_path[512];
                if (main[0] == '/') {
                    // Mutlak yol
                    strncpy(main_path, main, sizeof(main_path) - 1);
                } else {
                    // Göreceli yol
                    snprintf(main_path, sizeof(main_path), "%s/%s", project_path, main);
                }
                
                strncpy(current_project.main_file, main_path, sizeof(current_project.main_file) - 1);
            }
        }
        
        fclose(fp);
        
        // Proje yolunu ayarla
        strncpy(current_project.path, project_path, sizeof(current_project.path) - 1);
        
        // Gereksinimler dosyasını ayarla
        snprintf(current_project.requirements_file, sizeof(current_project.requirements_file), 
                "%s/requirements.txt", project_path);
        
        // Proje adı ayarlanmamışsa, dizin adından al
        if (strlen(current_project.name) == 0) {
            char* project_name = strrchr(project_path, '/');
            if (project_name) {
                project_name++; // '/' karakterini atla
            } else {
                project_name = (char*)project_path;
            }
            strncpy(current_project.name, project_name, sizeof(current_project.name) - 1);
        }
        
        // Ana dosya belirtilmişse aç
        if (strlen(current_project.main_file) > 0) {
            python_ide_open_file(current_project.main_file);
        }
        
        gui_terminal_print(output_terminal, "Proje açıldı: ");
        gui_terminal_print(output_terminal, current_project.name);
        gui_terminal_print(output_terminal, "\n");
        
        return 0;
    }
    
    return -1;
}

// Geçerli projeyi kapat
int python_ide_close_project() {
    // Açık dosyayı kaydet
    if (strlen(current_file) > 0 && ide_config.auto_save) {
        python_ide_save_file();
    }
    
    // Proje bilgilerini temizle
    memset(&current_project, 0, sizeof(python_ide_project_t));
    
    // Geçerli dosyayı temizle
    memset(current_file, 0, sizeof(current_file));
    
    // Editörü temizle
    if (code_editor) {
        gui_text_editor_set_text(code_editor, "");
    }
    
    // Terminal çıktısını temizle
    if (output_terminal) {
        gui_terminal_clear(output_terminal);
        gui_terminal_print(output_terminal, "Proje kapatıldı.\n");
    }
    
    // Pencere başlığını güncelle
    if (ide_window) {
        gui_window_set_title(ide_window, "KALEM OS Python IDE");
    }
    
    return 0;
}

// Yeni proje oluştur
int python_ide_create_project(const char* project_name, const char* project_path, uint8_t use_venv) {
    if (!project_name || !project_path) {
        return -1;
    }
    
    // Proje dizini oluştur
    struct stat st = {0};
    if (stat(project_path, &st) == -1) {
        #ifdef _WIN32
        mkdir(project_path);
        #else
        mkdir(project_path, 0755);
        #endif
    } else {
        // Dizin zaten var, içeriğini kontrol et
        DIR* dir = opendir(project_path);
        if (dir) {
            struct dirent* entry;
            int has_files = 0;
            
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    has_files = 1;
                    break;
                }
            }
            
            closedir(dir);
            
            if (has_files) {
                gui_terminal_print(output_terminal, "Uyarı: Proje dizini boş değil.\n");
            }
        }
    }
    
    // Proje yapılandırma dosyası oluştur
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/pyproject.toml", project_path);
    
    FILE* fp = fopen(config_path, "w");
    if (!fp) {
        gui_terminal_print(output_terminal, "Hata: Proje yapılandırma dosyası oluşturulamadı.\n");
        return -1;
    }
    
    // Proje yapılandırmasını yaz
    fprintf(fp, "[project]\n");
    fprintf(fp, "name = \"%s\"\n", project_name);
    fprintf(fp, "version = \"0.1.0\"\n");
    fprintf(fp, "description = \"\"\n");
    fprintf(fp, "authors = []\n");
    fprintf(fp, "\n");
    fprintf(fp, "[tool.kalem]\n");
    fprintf(fp, "main = \"main.py\"\n");
    
    if (use_venv) {
        fprintf(fp, "venv = \"%s/venv\"\n", project_path);
    }
    
    fclose(fp);
    
    // requirements.txt dosyası oluştur
    char req_path[512];
    snprintf(req_path, sizeof(req_path), "%s/requirements.txt", project_path);
    
    fp = fopen(req_path, "w");
    if (fp) {
        fprintf(fp, "# Proje gereksinimleri\n");
        fclose(fp);
    }
    
    // README.md dosyası oluştur
    char readme_path[512];
    snprintf(readme_path, sizeof(readme_path), "%s/README.md", project_path);
    
    fp = fopen(readme_path, "w");
    if (fp) {
        fprintf(fp, "# %s\n\n", project_name);
        fprintf(fp, "Bu proje KALEM OS Python IDE ile oluşturulmuştur.\n");
        fclose(fp);
    }
    
    // Ana Python dosyası oluştur
    char main_path[512];
    snprintf(main_path, sizeof(main_path), "%s/main.py", project_path);
    
    fp = fopen(main_path, "w");
    if (fp) {
        fprintf(fp, "#!/usr/bin/env python3\n");
        fprintf(fp, "# -*- coding: utf-8 -*-\n");
        fprintf(fp, "\n");
        fprintf(fp, "\"\"\"KALEM OS Python Projesi\n");
        fprintf(fp, "\n");
        fprintf(fp, "Bu modül, %s projesinin ana giriş noktasıdır.\n", project_name);
        fprintf(fp, "\"\"\"\n");
        fprintf(fp, "\n");
        fprintf(fp, "def main():\n");
        fprintf(fp, "    \"\"\"Ana program fonksiyonu\"\"\"\n");
        fprintf(fp, "    print(\"Merhaba, KALEM OS!\")\n");
        fprintf(fp, "    print(\"Bu %s projesinin başlangıç noktasıdır.\")\n", project_name);
        fprintf(fp, "\n");
        fprintf(fp, "if __name__ == \"__main__\":\n");
        fprintf(fp, "    main()\n");
        fclose(fp);
    }
    
    // Sanal ortam oluştur
    if (use_venv) {
        char venv_path[512];
        snprintf(venv_path, sizeof(venv_path), "%s/venv", project_path);
        
        char venv_cmd[1024];
        snprintf(venv_cmd, sizeof(venv_cmd), "python -m venv \"%s\"", venv_path);
        
        gui_terminal_print(output_terminal, "Sanal ortam oluşturuluyor...\n");
        gui_terminal_print(output_terminal, venv_cmd);
        gui_terminal_print(output_terminal, "\n");
        
        // TODO: Sisteminize uygun şekilde bu komutu çalıştırın
        // system(venv_cmd);
    }
    
    // Proje bilgilerini ayarla
    memset(&current_project, 0, sizeof(python_ide_project_t));
    strncpy(current_project.name, project_name, sizeof(current_project.name) - 1);
    strncpy(current_project.path, project_path, sizeof(current_project.path) - 1);
    strncpy(current_project.main_file, main_path, sizeof(current_project.main_file) - 1);
    strncpy(current_project.requirements_file, req_path, sizeof(current_project.requirements_file) - 1);
    
    if (use_venv) {
        snprintf(current_project.venv_path, sizeof(current_project.venv_path), "%s/venv", project_path);
        current_project.use_venv = 1;
    }
    
    // Ana dosyayı aç
    python_ide_open_file(main_path);
    
    gui_terminal_print(output_terminal, "Yeni proje oluşturuldu: ");
    gui_terminal_print(output_terminal, project_name);
    gui_terminal_print(output_terminal, "\n");
    
    return 0;
}

// Geçerli proje bilgisini al
int python_ide_get_project(python_ide_project_t* project_out) {
    if (!project_out) {
        return -1;
    }
    
    memcpy(project_out, &current_project, sizeof(python_ide_project_t));
    return 0;
}

// Buton olay işleyicileri
void on_run_button_clicked(gui_button_t* button, void* user_data) {
    python_ide_run_current_file();
}

void on_debug_button_clicked(gui_button_t* button, void* user_data) {
    python_ide_debug_current_file();
}

void on_save_button_clicked(gui_button_t* button, void* user_data) {
    python_ide_save_file();
}

void on_open_button_clicked(gui_button_t* button, void* user_data) {
    char* file_path = gui_show_open_dialog("Python Dosyası Aç", "*.py");
    if (file_path) {
        python_ide_open_file(file_path);
        free(file_path);
    }
}

void on_new_button_clicked(gui_button_t* button, void* user_data) {
    // Açık dosyayı kaydet
    if (strlen(current_file) > 0 && ide_config.auto_save) {
        python_ide_save_file();
    }
    
    // Yeni dosya
    memset(current_file, 0, sizeof(current_file));
    gui_text_editor_set_text(code_editor, "");
    gui_window_set_title(ide_window, "KALEM OS Python IDE - Yeni Dosya");
}

void on_find_button_clicked(gui_button_t* button, void* user_data) {
    // Basit arama iletişim kutusu göster
    char* search_text = gui_show_input_dialog("Ara", "Aranacak metin:");
    if (search_text && strlen(search_text) > 0) {
        gui_text_editor_find(code_editor, search_text);
        free(search_text);
    }
}

void on_replace_button_clicked(gui_button_t* button, void* user_data) {
    // Basit değiştirme iletişim kutusu göster
    char* search_text = gui_show_input_dialog("Değiştir", "Aranacak metin:");
    if (search_text && strlen(search_text) > 0) {
        char* replace_text = gui_show_input_dialog("Değiştir", "Yeni metin:");
        if (replace_text) {
            gui_text_editor_replace_all(code_editor, search_text, replace_text);
            free(replace_text);
        }
        free(search_text);
    }
} 