#ifndef ANDROID_RUNTIME_H
#define ANDROID_RUNTIME_H

#include <stdint.h>

// Android Runtime işleme modları
typedef enum {
    ART_MODE_INTERPRETER,   // Yorumlayıcı mod (daha yavaş, daha az bellek)
    ART_MODE_JIT,           // Anında derleme (Just-In-Time Compilation)
    ART_MODE_AOT            // Önceden derlenmiş (Ahead-Of-Time Compilation)
} art_mode_t;

// Dex dosya formatı yapıları
typedef struct dex_header {
    uint8_t magic[8];       // DEX dosya imzası
    uint32_t checksum;      // Dosya sağlama
    uint8_t signature[20];  // SHA-1 imzası
    uint32_t file_size;     // Dosya boyutu
    uint32_t header_size;   // Başlık boyutu
    uint32_t endian_tag;    // Endian tanımlayıcı
    uint32_t link_size;     // Bağlantı boyutu
    uint32_t link_off;      // Bağlantı offseti
    uint32_t map_off;       // Map offset
    uint32_t string_ids_size; // String tanımlayıcı sayısı
    uint32_t string_ids_off;  // String tanımlayıcı offseti
    uint32_t type_ids_size;   // Tip tanımlayıcı sayısı
    uint32_t type_ids_off;    // Tip tanımlayıcı offseti
    uint32_t proto_ids_size;  // Prototip tanımlayıcı sayısı
    uint32_t proto_ids_off;   // Prototip tanımlayıcı offseti
    uint32_t field_ids_size;  // Alan tanımlayıcı sayısı
    uint32_t field_ids_off;   // Alan tanımlayıcı offseti
    uint32_t method_ids_size; // Metod tanımlayıcı sayısı
    uint32_t method_ids_off;  // Metod tanımlayıcı offseti
    uint32_t class_defs_size; // Sınıf tanımlayıcı sayısı
    uint32_t class_defs_off;  // Sınıf tanımlayıcı offseti
    uint32_t data_size;     // Veri boyutu
    uint32_t data_off;      // Veri offseti
} dex_header_t;

// JNI (Java Native Interface) köprüsü
typedef struct jni_env {
    void* functions;        // JNI fonksiyon tablosu
    void* reserved0;        // İleride kullanım için ayrılmış
    void* reserved1;        // İleride kullanım için ayrılmış
    void* reserved2;        // İleride kullanım için ayrılmış
} jni_env_t;

// Android Runtime durumu
typedef struct art_runtime {
    art_mode_t mode;        // Çalışma modu
    uint8_t initialized;    // Başlatıldı mı?
    void* heap;             // Java heap bellek alanı
    uint32_t heap_size;     // Heap boyutu
    void* jit_cache;        // JIT önbelleği
    jni_env_t* jni_env;     // JNI ortamı
    
    // İstatistikler
    uint32_t loaded_classes; // Yüklenen sınıf sayısı
    uint32_t active_threads; // Aktif thread sayısı
    uint32_t gc_count;       // Çöp toplama sayısı
    uint64_t total_allocated; // Toplam ayrılan bellek
} art_runtime_t;

// Android Runtime API
int art_initialize(art_mode_t mode, uint32_t heap_size);
int art_load_dex(const char* dex_path);
int art_load_apk(const char* apk_path);
int art_invoke_method(const char* class_name, const char* method_name, const char* signature);
int art_cleanup();

// Dalvik uyumluluk katmanı
int dalvik_compatibility_init();
int convert_dalvik_to_art(const char* odex_path, const char* dex_path);

// JNI API
jni_env_t* jni_get_env();
int jni_attach_thread();
int jni_detach_thread();

// Dex işleme API
int dex_optimize(const char* input_dex, const char* output_dex);
int dex_verify(const char* dex_path);

#endif /* ANDROID_RUNTIME_H */ 