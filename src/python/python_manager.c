#include "../include/python_manager.h"
#include "../include/kernel_api.h"
#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

// NOT: Bu dosya Python C API'si ile derlenmeli (Python.h gerekli)
// Gerçek implementasyonda Python header dosyaları eklenmelidir
#ifndef MOCK_PYTHON
#define MOCK_PYTHON 1
#endif

#if MOCK_PYTHON
// Python API simülasyonu
typedef void PyObject;
#define Py_Initialize() (void)0
#define Py_Finalize() (void)0
#define PyRun_SimpleString(cmd) (python_log(cmd), 0)
#define PyRun_SimpleFile(fp, fn) (python_log(fn), 0)
#define PyErr_Print() (void)0
#define PyErr_Occurred() 0
#define Py_DECREF(obj) (void)0
#define Py_INCREF(obj) (void)0
#define PyBool_FromLong(v) ((PyObject*)0)
#define PyModule_Create(def) ((PyObject*)0)
#define PyDict_SetItemString(dict, key, val) 0
#define PyDict_New() ((PyObject*)0)
#define PyUnicode_FromString(s) ((PyObject*)0)
#define PyList_Append(list, item) 0
#define PySys_GetObject(name) ((PyObject*)0)
#define PyImport_GetModuleDict() ((PyObject*)0)
#else
#include <Python.h>
#endif

// Python yorumlayıcısının durumu
static int is_initialized = 0;
static PyObject* kernel_module = NULL;
static char last_error[256] = {0};
static void (*output_callback)(const char* output) = NULL;
static uint8_t stdout_color = 0;
static uint8_t stderr_color = 0;
static uint8_t prompt_color = 0;

// Kayıtlı Python betikleri listesi
static python_script_t* script_list = NULL;

// Python betik dizini
static char scripts_directory[256] = "/usr/share/kalem-python/scripts";

// Python paket yöneticisi yapılandırması
static python_package_config_t package_config = {
    .auto_install_deps = 1,
    .use_pip_cache = 1,
    .offline_mode = 0,
    .custom_repo_url = "",
    .venv_path = "/usr/share/kalem-python/venv",
    .pip_cache_dir = "/var/cache/kalem-python/pip"
};

// Python ortam bilgisi
static python_environment_t python_env = {
    .version = "3.9.0",
    .path = "/usr/share/kalem-python",
    .home = "/usr/share/kalem-python",
    .exec = "/usr/bin/python3",
    .site_packages = "/usr/share/kalem-python/lib/python3.9/site-packages",
    .is_cpython = 1,
    .is_micropython = 0,
    .memory_limit_kb = 0
};

// Yardımcı fonksiyonlar
static void python_log(const char* message) {
    // Çıktı geri çağrısı varsa kullan
    if (output_callback) {
        output_callback(message);
    }
    
    // Log sistemine kaydet
    /* log_info("Python: %s", message); */
    printf("Python: %s\n", message);
}

static void python_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(last_error, sizeof(last_error), format, args);
    va_end(args);
    
    // Log sistemine hata kaydet
    /* log_error("Python Error: %s", last_error); */
    printf("Python Error: %s\n", last_error);
    
    // Çıktı geri çağrısı varsa kullan
    if (output_callback) {
        char error_message[300];
        snprintf(error_message, sizeof(error_message), "Hata: %s", last_error);
        output_callback(error_message);
    }
}

// Python yöneticisini başlat
int python_manager_init() {
    // Zaten başlatılmışsa tekrar başlatma
    if (is_initialized) {
        return 0;
    }
    
    // Python yorumlayıcısını başlat
    Py_Initialize();
    
    // Kernel modülünü oluştur
    #if !MOCK_PYTHON
    // Gerçek Python bütünleşiminde burada Python C API kullanılacak
    // Kernel modülünü tanımla ve gerekli fonksiyonları ekle
    #endif
    
    // Başarılı başlatıldığını işaretle
    is_initialized = 1;
    
    // Betik dizini varlığını kontrol et veya oluştur
    struct stat st = {0};
    if (stat(scripts_directory, &st) == -1) {
        #ifdef _WIN32
        mkdir(scripts_directory);
        #else
        mkdir(scripts_directory, 0755);
        #endif
    }
    
    // Pip önbellek dizinini kontrol et veya oluştur
    if (stat(package_config.pip_cache_dir, &st) == -1) {
        #ifdef _WIN32
        mkdir(package_config.pip_cache_dir);
        #else
        mkdir(package_config.pip_cache_dir, 0755);
        #endif
    }
    
    python_log("Python yorumlayıcısı başlatıldı");
    return 0;
}

// Python yöneticisini temizle
int python_manager_cleanup() {
    // Başlatılmamışsa hata döndür
    if (!is_initialized) {
        return 0;
    }
    
    // Betik listesini temizle
    python_script_t* current = script_list;
    while (current) {
        python_script_t* next = current->next;
        free(current);
        current = next;
    }
    script_list = NULL;
    
    // Python yorumlayıcısını kapat
    Py_XDECREF(kernel_module);
    Py_Finalize();
    
    is_initialized = 0;
    python_log("Python yorumlayıcısı kapatıldı");
    return 0;
}

// Python betiği çalıştır
int python_run_script(const char* script_path) {
    // Python başlatılmamışsa başlat
    if (!is_initialized) {
        if (python_manager_init() != 0) {
            python_error("Python yorumlayıcısı başlatılamadı");
            return PYTHON_ERROR_INIT;
        }
    }
    
    // Betik dosyasını aç
    FILE* fp = fopen(script_path, "r");
    if (!fp) {
        python_error("Python betiği açılamadı: %s", script_path);
        return PYTHON_ERROR_SCRIPT;
    }
    
    // Betiği çalıştır
    python_log("Betik çalıştırılıyor: ");
    python_log(script_path);
    int result = PyRun_SimpleFile(fp, script_path);
    fclose(fp);
    
    if (result != 0) {
        PyErr_Print();
        python_error("Python betiği çalıştırılırken hata oluştu: %s", script_path);
        return PYTHON_ERROR_SCRIPT;
    }
    
    python_log("Betik başarıyla tamamlandı");
    return 0;
}

// Python kodu çalıştır
int python_run_string(const char* code) {
    // Python başlatılmamışsa başlat
    if (!is_initialized) {
        if (python_manager_init() != 0) {
            python_error("Python yorumlayıcısı başlatılamadı");
            return PYTHON_ERROR_INIT;
        }
    }
    
    // Kodu çalıştır
    int result = PyRun_SimpleString(code);
    
    if (result != 0) {
        PyErr_Print();
        python_error("Python kodu çalıştırılırken hata oluştu");
        return PYTHON_ERROR_SCRIPT;
    }
    
    return 0;
}

// Python kodu çalıştır ve çıktıyı döndür
int python_run_string_with_output(const char* code, char** output_out) {
    // Python başlatılmamışsa başlat
    if (!is_initialized) {
        if (python_manager_init() != 0) {
            python_error("Python yorumlayıcısı başlatılamadı");
            return PYTHON_ERROR_INIT;
        }
    }
    
    // Çıktıyı yakalamak için geçici dosya oluştur
    const char* temp_file = "/tmp/python_output.txt";
    
    // Çıktıyı dosyaya yönlendiren kod oluştur
    char redirected_code[strlen(code) + 200];
    snprintf(redirected_code, sizeof(redirected_code), 
             "import sys\n"
             "old_stdout = sys.stdout\n"
             "sys.stdout = open('%s', 'w')\n"
             "try:\n"
             "    %s\n"
             "finally:\n"
             "    sys.stdout.close()\n"
             "    sys.stdout = old_stdout\n", 
             temp_file, code);
    
    // Kodu çalıştır
    int result = PyRun_SimpleString(redirected_code);
    
    if (result != 0) {
        PyErr_Print();
        python_error("Python kodu çalıştırılırken hata oluştu");
        return PYTHON_ERROR_SCRIPT;
    }
    
    // Geçici dosyadan çıktıyı oku
    FILE* fp = fopen(temp_file, "r");
    if (!fp) {
        python_error("Çıktı dosyası açılamadı");
        return PYTHON_ERROR_IO;
    }
    
    // Dosya boyutunu oku
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // Bellek ayır
    *output_out = (char*)malloc(file_size + 1);
    if (!*output_out) {
        fclose(fp);
        python_error("Bellek ayırma hatası");
        return PYTHON_ERROR_MEMORY;
    }
    
    // Dosyayı oku
    size_t read_size = fread(*output_out, 1, file_size, fp);
    (*output_out)[read_size] = '\0';
    
    fclose(fp);
    remove(temp_file);
    
    return 0;
}

// Son Python hatasını döndür
const char* python_get_last_error() {
    if (strlen(last_error) == 0) {
        return "Bilinmeyen hata";
    }
    return last_error;
}

// Python ortam bilgisini al
int python_get_environment(python_environment_t* env_out) {
    if (!env_out) {
        python_error("Geçersiz parametre");
        return PYTHON_ERROR_PARAM;
    }
    
    // Ortam bilgisini kopyala
    memcpy(env_out, &python_env, sizeof(python_environment_t));
    
    return 0;
}

// Python paket yöneticisi yapılandırmasını al
int python_get_package_config(python_package_config_t* config_out) {
    if (!config_out) {
        python_error("Geçersiz parametre");
        return PYTHON_ERROR_PARAM;
    }
    
    // Yapılandırma bilgisini kopyala
    memcpy(config_out, &package_config, sizeof(python_package_config_t));
    
    return 0;
}

// Python paket yöneticisi yapılandırmasını ayarla
int python_set_package_config(const python_package_config_t* config) {
    if (!config) {
        python_error("Geçersiz parametre");
        return PYTHON_ERROR_PARAM;
    }
    
    // Yapılandırma bilgisini kopyala
    memcpy(&package_config, config, sizeof(python_package_config_t));
    
    return 0;
}

// Python çıktı yakalama geri çağrısını ayarla
void python_set_output_callback(void (*callback)(const char* output)) {
    output_callback = callback;
}

// Python çıktı renklerini ayarla
void python_set_output_colors(uint8_t stdout_color_arg, uint8_t stderr_color_arg, uint8_t prompt_color_arg) {
    stdout_color = stdout_color_arg;
    stderr_color = stderr_color_arg;
    prompt_color = prompt_color_arg;
}

// Python betik taramasını başlat
int python_scan_scripts(const char* directory) {
    DIR* dir;
    struct dirent* entry;
    int count = 0;
    
    // Belirtilen dizini aç
    if (!(dir = opendir(directory))) {
        python_error("Dizin açılamadı: %s", directory);
        return PYTHON_ERROR_IO;
    }
    
    // Dizindeki dosyaları tara
    while ((entry = readdir(dir)) != NULL) {
        // . ve .. dizinlerini atla
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Python dosyalarını kontrol et
        char* ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".py") == 0) {
            // Tam dosya yolunu oluştur
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);
            
            // Kayıt bilgilerini oluştur
            char name[64];
            char* dot = strrchr(entry->d_name, '.');
            if (dot) {
                int len = dot - entry->d_name;
                strncpy(name, entry->d_name, len);
                name[len] = '\0';
            } else {
                strncpy(name, entry->d_name, sizeof(name) - 1);
                name[sizeof(name) - 1] = '\0';
            }
            
            // Betiği otomatik kaydet
            uint8_t autorun = (strcmp(name, "autorun") == 0 || 
                            strcmp(name, "startup") == 0 ||
                            strcmp(name, "init") == 0);
            
            // Betiği kaydet
            if (python_register_script(full_path, "", autorun, 0) == 0) {
                count++;
            }
        }
    }
    
    closedir(dir);
    return count;
}

// Python betiklerini listele
python_script_t* python_list_scripts(uint8_t system_only) {
    // Betik listemiz yoksa NULL döndür
    if (!script_list) {
        return NULL;
    }
    
    // Sadece sistem betiklerini göster
    if (system_only) {
        python_script_t* sys_list = NULL;
        python_script_t* current = script_list;
        
        while (current) {
            if (current->system_script) {
                python_script_t* copy = (python_script_t*)malloc(sizeof(python_script_t));
                if (copy) {
                    memcpy(copy, current, sizeof(python_script_t));
                    copy->next = sys_list;
                    sys_list = copy;
                }
            }
            current = current->next;
        }
        
        return sys_list;
    }
    
    // Tüm betik listesini döndür
    return script_list;
}

// Python betiğini kayıt et
int python_register_script(const char* script_path, const char* description,
                           uint8_t autorun, uint8_t system_script) {
    if (!script_path) {
        python_error("Geçersiz parametre");
        return PYTHON_ERROR_PARAM;
    }
    
    // Dosya var mı kontrol et
    FILE* fp = fopen(script_path, "r");
    if (!fp) {
        python_error("Dosya bulunamadı: %s", script_path);
        return PYTHON_ERROR_IO;
    }
    fclose(fp);
    
    // Betik zaten kayıtlı mı kontrol et
    python_script_t* current = script_list;
    while (current) {
        if (strcmp(current->path, script_path) == 0) {
            // Betik zaten kayıtlı, bilgileri güncelle
            strncpy(current->description, description, sizeof(current->description) - 1);
            current->autorun = autorun;
            current->system_script = system_script;
            return 0;
        }
        current = current->next;
    }
    
    // Yeni betik kaydı oluştur
    python_script_t* new_script = (python_script_t*)malloc(sizeof(python_script_t));
    if (!new_script) {
        python_error("Bellek ayırma hatası");
        return PYTHON_ERROR_MEMORY;
    }
    
    // Betik bilgilerini ayarla
    strncpy(new_script->path, script_path, sizeof(new_script->path) - 1);
    
    // Betik adını ayarla (dosya adını al)
    char* filename = strrchr(script_path, '/');
    if (filename) {
        filename++; // '/' karakterini atla
    } else {
        filename = (char*)script_path; // '/' yoksa dosya adı olarak kullan
    }
    
    // Uzantıyı kaldır
    char name[64];
    strcpy(name, filename);
    char* dot = strrchr(name, '.');
    if (dot) {
        *dot = '\0';
    }
    
    strncpy(new_script->name, name, sizeof(new_script->name) - 1);
    strncpy(new_script->description, description, sizeof(new_script->description) - 1);
    new_script->autorun = autorun;
    new_script->system_script = system_script;
    new_script->last_run_time = 0;
    new_script->exit_code = 0;
    
    // Listeye ekle
    new_script->next = script_list;
    script_list = new_script;
    
    return 0;
}

// Python betiğini kayıttan çıkar
int python_unregister_script(const char* script_path) {
    if (!script_path) {
        python_error("Geçersiz parametre");
        return PYTHON_ERROR_PARAM;
    }
    
    // Betik listesi boşsa hata döndür
    if (!script_list) {
        python_error("Betik listesi boş");
        return PYTHON_ERROR_NOT_FOUND;
    }
    
    // İlk elemanı kontrol et
    if (strcmp(script_list->path, script_path) == 0) {
        python_script_t* temp = script_list;
        script_list = script_list->next;
        free(temp);
        return 0;
    }
    
    // Listeyi tara
    python_script_t* current = script_list;
    while (current->next) {
        if (strcmp(current->next->path, script_path) == 0) {
            python_script_t* temp = current->next;
            current->next = current->next->next;
            free(temp);
            return 0;
        }
        current = current->next;
    }
    
    python_error("Betik bulunamadı: %s", script_path);
    return PYTHON_ERROR_NOT_FOUND;
} 