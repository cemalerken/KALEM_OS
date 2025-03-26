#include "../include/python_ai.h"
#include "../include/python_manager.h"
#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Yapay zeka motor durumu
static int ai_initialized = 0;

// Yüklü model listesi
#define MAX_MODELS 8
static struct {
    int id;
    ai_model_type_t type;
    ai_model_config_t config;
    void* model_handle;
    uint8_t is_loaded;
} loaded_models[MAX_MODELS];

static int model_count = 0;
static int next_model_id = 1;

// Python AI hata kodları
typedef enum {
    AI_ERROR_NONE = 0,
    AI_ERROR_INIT = -1,
    AI_ERROR_MODEL_LOAD = -2,
    AI_ERROR_INFERENCE = -3,
    AI_ERROR_MEMORY = -4,
    AI_ERROR_PARAM = -5,
    AI_ERROR_NOT_FOUND = -6,
    AI_ERROR_NOT_SUPPORTED = -7,
    AI_ERROR_UNKNOWN = -99
} ai_error_t;

// Son hata mesajı
static char last_error_message[512] = {0};

// Hata ayıklama için loglama
static void ai_log(const char* message) {
    // Burada gerçek bir loglama fonksiyonu kullanılabilir
    printf("[AI] %s\n", message);
}

// Hata mesajını ayarla
static void ai_set_error(ai_error_t error, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(last_error_message, sizeof(last_error_message), format, args);
    va_end(args);
    
    ai_log(last_error_message);
}

// Yapay zeka modülünü başlat
int python_ai_init() {
    if (ai_initialized) {
        return 0; // Zaten başlatılmış
    }
    
    ai_log("Yapay zeka modülü başlatılıyor...");
    
    // Python yöneticisinin başlatıldığından emin ol
    if (python_manager_init() != 0) {
        ai_set_error(AI_ERROR_INIT, "Python yöneticisi başlatılamadı");
        return AI_ERROR_INIT;
    }
    
    // Gerekli Python modüllerini yükle
    int result = python_run_string(
        "try:\n"
        "    import numpy as np\n"
        "    import ctypes\n"
        "    import json\n"
        "    import sys\n"
        "    print('AI için gerekli Python modülleri yüklendi')\n"
        "    kalem_ai_ready = True\n"
        "except ImportError as e:\n"
        "    print(f'AI modüllerinin yüklenmesinde hata: {e}')\n"
        "    kalem_ai_ready = False\n"
    );
    
    if (result != 0) {
        ai_set_error(AI_ERROR_INIT, "Gerekli Python modülleri yüklenemedi");
        return AI_ERROR_INIT;
    }
    
    // Python değişkeninden hazır olup olmadığını kontrol et
    char* output = NULL;
    result = python_run_string_with_output("print(kalem_ai_ready)", &output);
    
    if (result != 0 || !output || strstr(output, "True") == NULL) {
        ai_set_error(AI_ERROR_INIT, "AI modülleri başlatılamadı");
        free(output);
        return AI_ERROR_INIT;
    }
    
    free(output);
    
    // Model listesini temizle
    memset(loaded_models, 0, sizeof(loaded_models));
    model_count = 0;
    next_model_id = 1;
    
    ai_initialized = 1;
    ai_log("Yapay zeka modülü başarıyla başlatıldı");
    return 0;
}

// Yapay zeka modülünü temizle
int python_ai_cleanup() {
    if (!ai_initialized) {
        return 0; // Zaten temizlenmiş
    }
    
    ai_log("Yapay zeka modülü temizleniyor...");
    
    // Tüm modelleri boşalt
    for (int i = 0; i < MAX_MODELS; i++) {
        if (loaded_models[i].is_loaded) {
            // Python modeli temizleme kodu
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "if 'model_%d' in globals(): del model_%d", 
                    loaded_models[i].id, loaded_models[i].id);
            python_run_string(cmd);
            
            loaded_models[i].is_loaded = 0;
            loaded_models[i].model_handle = NULL;
        }
    }
    
    ai_initialized = 0;
    ai_log("Yapay zeka modülü temizlendi");
    return 0;
}

// Model yükleme destek fonksiyonu
static int load_text_model(int model_id, const ai_model_config_t* config) {
    char cmd[1024];
    
    // Model yükleme kodu
    snprintf(cmd, sizeof(cmd),
        "try:\n"
        "    from transformers import AutoModelForCausalLM, AutoTokenizer\n"
        "    import torch\n"
        "    \n"
        "    model_name = '%s'\n"
        "    use_gpu = %s\n"
        "    quantized = %s\n"
        "    \n"
        "    device = 'cuda' if torch.cuda.is_available() and use_gpu else 'cpu'\n"
        "    \n"
        "    tokenizer_%d = AutoTokenizer.from_pretrained(model_name)\n"
        "    \n"
        "    if quantized:\n"
        "        model_%d = AutoModelForCausalLM.from_pretrained(\n"
        "            model_name,\n"
        "            device_map=device,\n"
        "            load_in_8bit=True\n"
        "        )\n"
        "    else:\n"
        "        model_%d = AutoModelForCausalLM.from_pretrained(\n"
        "            model_name,\n"
        "            device_map=device\n"
        "        )\n"
        "    \n"
        "    print(f'Model {model_name} yüklendi (ID: %d)')\n"
        "    kalem_model_loaded_%d = True\n"
        "except Exception as e:\n"
        "    print(f'Model yükleme hatası: {e}')\n"
        "    kalem_model_loaded_%d = False\n",
        config->model_name,
        config->use_gpu ? "True" : "False",
        config->quantized ? "True" : "False",
        model_id, model_id, model_id, model_id, model_id, model_id
    );
    
    // Komutu çalıştır
    int result = python_run_string(cmd);
    if (result != 0) {
        ai_set_error(AI_ERROR_MODEL_LOAD, "Model yükleme komutu çalıştırılamadı");
        return AI_ERROR_MODEL_LOAD;
    }
    
    // Başarılı yüklenip yüklenmediğini kontrol et
    char check_cmd[128];
    snprintf(check_cmd, sizeof(check_cmd), "print(kalem_model_loaded_%d)", model_id);
    
    char* output = NULL;
    result = python_run_string_with_output(check_cmd, &output);
    
    if (result != 0 || !output || strstr(output, "True") == NULL) {
        ai_set_error(AI_ERROR_MODEL_LOAD, "Model yüklenemedi");
        free(output);
        return AI_ERROR_MODEL_LOAD;
    }
    
    free(output);
    return 0;
}

// Yapay zeka modelini yükle
int python_ai_load_model(ai_model_type_t model_type, const ai_model_config_t* config) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    if (!config) {
        ai_set_error(AI_ERROR_PARAM, "Geçersiz model yapılandırması");
        return AI_ERROR_PARAM;
    }
    
    // Model sayısını kontrol et
    if (model_count >= MAX_MODELS) {
        ai_set_error(AI_ERROR_MODEL_LOAD, "Maksimum model sayısına ulaşıldı");
        return AI_ERROR_MODEL_LOAD;
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Model yükleniyor: %s", config->model_name);
    ai_log(log_msg);
    
    // Yeni model ID'si
    int model_id = next_model_id++;
    
    // Model tipine göre yükleme işlemi
    int result = AI_ERROR_NOT_SUPPORTED;
    
    switch (model_type) {
        case AI_MODEL_TEXT:
            result = load_text_model(model_id, config);
            break;
            
        case AI_MODEL_IMAGE:
        case AI_MODEL_AUDIO:
        case AI_MODEL_HYBRID:
        case AI_MODEL_OPTIMIZATION:
            ai_set_error(AI_ERROR_NOT_SUPPORTED, "Bu model tipi henüz desteklenmiyor");
            return AI_ERROR_NOT_SUPPORTED;
            
        default:
            ai_set_error(AI_ERROR_PARAM, "Geçersiz model tipi");
            return AI_ERROR_PARAM;
    }
    
    if (result != 0) {
        return result; // Hata zaten ayarlandı
    }
    
    // Modeli kaydet
    int idx = model_count++;
    loaded_models[idx].id = model_id;
    loaded_models[idx].type = model_type;
    loaded_models[idx].is_loaded = 1;
    loaded_models[idx].model_handle = (void*)(intptr_t)model_id; // ID'yi handle olarak kullan
    memcpy(&loaded_models[idx].config, config, sizeof(ai_model_config_t));
    
    snprintf(log_msg, sizeof(log_msg), "Model başarıyla yüklendi (ID: %d)", model_id);
    ai_log(log_msg);
    
    return model_id;
}

// Yapay zeka modelini kaldır
int python_ai_unload_model(int model_id) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    // Modeli bul
    int model_idx = -1;
    for (int i = 0; i < model_count; i++) {
        if (loaded_models[i].id == model_id && loaded_models[i].is_loaded) {
            model_idx = i;
            break;
        }
    }
    
    if (model_idx == -1) {
        ai_set_error(AI_ERROR_NOT_FOUND, "Model bulunamadı");
        return AI_ERROR_NOT_FOUND;
    }
    
    // Python'da modeli temizle
    char cmd[256];
    snprintf(cmd, sizeof(cmd), 
        "try:\n"
        "    if 'model_%d' in globals():\n"
        "        del model_%d\n"
        "        del tokenizer_%d\n"
        "        import gc\n"
        "        gc.collect()\n"
        "        if 'torch' in sys.modules and hasattr(torch, 'cuda') and torch.cuda.is_available():\n"
        "            torch.cuda.empty_cache()\n"
        "        print('Model %d kaldırıldı')\n"
        "except Exception as e:\n"
        "    print(f'Model kaldırma hatası: {e}')\n",
        model_id, model_id, model_id, model_id
    );
    
    int result = python_run_string(cmd);
    if (result != 0) {
        ai_set_error(AI_ERROR_UNKNOWN, "Model kaldırılırken hata oluştu");
        return AI_ERROR_UNKNOWN;
    }
    
    // Modeli listeden çıkar (tüm modelleri kaydırarak)
    loaded_models[model_idx].is_loaded = 0;
    if (model_idx < model_count - 1) {
        memmove(&loaded_models[model_idx], &loaded_models[model_idx + 1], 
                (model_count - model_idx - 1) * sizeof(loaded_models[0]));
    }
    model_count--;
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Model kaldırıldı (ID: %d)", model_id);
    ai_log(log_msg);
    
    return 0;
}

// Yapay zeka modeliyle metin üret
int python_ai_generate_text(int model_id, const char* prompt, char** result_out) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    if (!prompt || !result_out) {
        ai_set_error(AI_ERROR_PARAM, "Geçersiz parametreler");
        return AI_ERROR_PARAM;
    }
    
    // Modeli bul
    int model_idx = -1;
    for (int i = 0; i < model_count; i++) {
        if (loaded_models[i].id == model_id && loaded_models[i].is_loaded) {
            model_idx = i;
            break;
        }
    }
    
    if (model_idx == -1) {
        ai_set_error(AI_ERROR_NOT_FOUND, "Model bulunamadı");
        return AI_ERROR_NOT_FOUND;
    }
    
    // Sadece metin modellerini kabul et
    if (loaded_models[model_idx].type != AI_MODEL_TEXT) {
        ai_set_error(AI_ERROR_NOT_SUPPORTED, "Bu model tipi metin üretimi için uygun değil");
        return AI_ERROR_NOT_SUPPORTED;
    }
    
    // Metin üretme kodu
    char cmd[4096];
    
    // Prompt'u güvenli bir şekilde kaçış
    char* escaped_prompt = (char*)malloc(strlen(prompt) * 2 + 1);
    if (!escaped_prompt) {
        ai_set_error(AI_ERROR_MEMORY, "Bellek ayırma hatası");
        return AI_ERROR_MEMORY;
    }
    
    const char* src = prompt;
    char* dst = escaped_prompt;
    while (*src) {
        if (*src == '\'' || *src == '\"' || *src == '\\') {
            *dst++ = '\\';
        }
        *dst++ = *src++;
    }
    *dst = '\0';
    
    const ai_model_config_t* config = &loaded_models[model_idx].config;
    
    snprintf(cmd, sizeof(cmd),
        "try:\n"
        "    prompt = \"\"\"%s\"\"\"\n"
        "    temperature = %f\n"
        "    max_tokens = %d\n"
        "    \n"
        "    inputs = tokenizer_%d(prompt, return_tensors='pt')\n"
        "    if torch.cuda.is_available() and model_%d.device.type == 'cuda':\n"
        "        inputs = {k: v.cuda() for k, v in inputs.items()}\n"
        "    \n"
        "    with torch.no_grad():\n"
        "        output = model_%d.generate(\n"
        "            **inputs,\n"
        "            max_length=len(inputs['input_ids'][0]) + max_tokens,\n"
        "            temperature=temperature,\n"
        "            do_sample=temperature > 0.0,\n"
        "            pad_token_id=tokenizer_%d.eos_token_id\n"
        "        )\n"
        "    \n"
        "    generated_text = tokenizer_%d.decode(output[0], skip_special_tokens=True)\n"
        "    response_text = generated_text[len(prompt):].strip()\n"
        "    \n"
        "    kalem_ai_result = response_text\n"
        "    print(f'Metin üretildi (model: %d)')\n"
        "except Exception as e:\n"
        "    print(f'Metin üretme hatası: {e}')\n"
        "    kalem_ai_result = None\n",
        escaped_prompt,
        config->temperature,
        config->max_tokens,
        model_id, model_id, model_id, model_id, model_id, model_id
    );
    
    free(escaped_prompt);
    
    int result = python_run_string(cmd);
    if (result != 0) {
        ai_set_error(AI_ERROR_INFERENCE, "Metin üretme komutu çalıştırılamadı");
        return AI_ERROR_INFERENCE;
    }
    
    // Sonucu al
    char* output = NULL;
    result = python_run_string_with_output(
        "if kalem_ai_result is not None:\n"
        "    print(kalem_ai_result)\n"
        "else:\n"
        "    print('ERROR: Metin üretme başarısız')",
        &output
    );
    
    if (result != 0 || !output || strstr(output, "ERROR") != NULL) {
        ai_set_error(AI_ERROR_INFERENCE, "Metin üretme başarısız");
        free(output);
        return AI_ERROR_INFERENCE;
    }
    
    *result_out = output; // Çıktıyı döndür (çağıran serbest bırakmalı)
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Metin üretildi (model ID: %d, uzunluk: %zu)",
            model_id, output ? strlen(output) : 0);
    ai_log(log_msg);
    
    return 0;
}

// Yapay zeka ile sistem metriklerini analiz et
int python_ai_analyze_system_metrics(const ai_system_metrics_t* metrics, 
                                     ai_optimization_recommendations_t* recommendations_out) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    if (!metrics || !recommendations_out) {
        ai_set_error(AI_ERROR_PARAM, "Geçersiz parametreler");
        return AI_ERROR_PARAM;
    }
    
    // Basit optimizasyon analizi yap
    
    // CPU yönetimi
    if (metrics->cpu_usage > 80.0f) {
        recommendations_out->cpu_governor = 1; // Performans
        recommendations_out->cpu_max_freq_pct = 100;
        recommendations_out->enable_core_parking = 0;
        recommendations_out->available_core_pct = 100;
    } else if (metrics->cpu_usage > 50.0f) {
        recommendations_out->cpu_governor = 2; // Dengeli
        recommendations_out->cpu_max_freq_pct = 80;
        recommendations_out->enable_core_parking = 0;
        recommendations_out->available_core_pct = 100;
    } else {
        recommendations_out->cpu_governor = 0; // Güç tasarrufu
        recommendations_out->cpu_max_freq_pct = 60;
        recommendations_out->enable_core_parking = 1;
        recommendations_out->available_core_pct = 75;
    }
    
    // Bellek yönetimi
    uint32_t total_memory_mb = metrics->memory_used_mb + metrics->memory_free_mb;
    float memory_usage_pct = (float)metrics->memory_used_mb / total_memory_mb * 100.0f;
    
    if (memory_usage_pct > 90.0f) {
        recommendations_out->memory_compression = 3; // Agresif
    } else if (memory_usage_pct > 70.0f) {
        recommendations_out->memory_compression = 2; // Normal
    } else {
        recommendations_out->memory_compression = 1; // Minimal
    }
    
    // Disk I/O önceliği
    if (metrics->disk_read_mbps > 50.0f || metrics->disk_write_mbps > 30.0f) {
        recommendations_out->disk_io_priority = 2; // Yüksek
    } else {
        recommendations_out->disk_io_priority = 1; // Normal
    }
    
    // Ağ QoS
    if (metrics->network_rx_mbps > 10.0f || metrics->network_tx_mbps > 5.0f) {
        recommendations_out->network_qos = 2; // Yüksek
    } else {
        recommendations_out->network_qos = 1; // Normal
    }
    
    // Termal profil
    if (metrics->cpu_usage > 80.0f || metrics->gpu_usage > 80.0f) {
        recommendations_out->thermal_profile = 2; // Performans
    } else if (metrics->battery_drain_rate > 15.0f) {
        recommendations_out->thermal_profile = 0; // Güç tasarrufu
    } else {
        recommendations_out->thermal_profile = 1; // Dengeli
    }
    
    // Süreç limitleri JSON olarak
    snprintf(recommendations_out->process_limits, sizeof(recommendations_out->process_limits),
             "{\"high_cpu_limit\": %d, \"high_memory_limit\": %d, \"background_cpu_limit\": %d}",
             (int)(metrics->cpu_usage > 80.0f ? 80 : 50),
             (int)(memory_usage_pct > 80.0f ? 80 : 50),
             (int)(metrics->cpu_usage > 80.0f ? 10 : 20));
    
    ai_log("Sistem metrikleri analiz edildi, optimizasyon önerileri oluşturuldu");
    return 0;
}

// Sistem kaynaklarını yapay zeka ile optimize et
int python_ai_optimize_resources(ai_optimization_target_t target, 
                                 uint8_t strength, char** result_out) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    if (!result_out) {
        ai_set_error(AI_ERROR_PARAM, "Geçersiz parametreler");
        return AI_ERROR_PARAM;
    }
    
    // Geçerli sistem metriklerini al
    ai_system_metrics_t metrics;
    // Bu normalde kernel_api'den alınacak, şimdilik örnek değerler kullanıyoruz
    metrics.cpu_usage = 65.0f;
    metrics.memory_used_mb = 2048;
    metrics.memory_free_mb = 2048;
    metrics.disk_read_mbps = 20.0f;
    metrics.disk_write_mbps = 10.0f;
    metrics.network_rx_mbps = 2.0f;
    metrics.network_tx_mbps = 1.0f;
    metrics.battery_drain_rate = 10.0f;
    metrics.gpu_usage = 30;
    
    // Optimizasyon önerileri al
    ai_optimization_recommendations_t recommendations;
    memset(&recommendations, 0, sizeof(recommendations));
    
    int result = python_ai_analyze_system_metrics(&metrics, &recommendations);
    if (result != 0) {
        return result; // Hata zaten ayarlandı
    }
    
    // Hedef ve güce göre ayarla
    switch (target) {
        case AI_OPTIMIZE_PERFORMANCE:
            recommendations.cpu_governor = 1; // Performans
            recommendations.cpu_max_freq_pct = 100;
            recommendations.enable_core_parking = 0;
            recommendations.available_core_pct = 100;
            recommendations.memory_compression = 1; // Düşük
            recommendations.disk_io_priority = 2; // Yüksek
            recommendations.network_qos = 2; // Yüksek
            recommendations.thermal_profile = 2; // Performans
            break;
            
        case AI_OPTIMIZE_POWER:
            recommendations.cpu_governor = 0; // Güç tasarrufu
            recommendations.cpu_max_freq_pct = 60 - (strength / 5);
            recommendations.enable_core_parking = 1;
            recommendations.available_core_pct = 75 - (strength / 4);
            recommendations.memory_compression = 2; // Normal
            recommendations.disk_io_priority = 1; // Normal
            recommendations.network_qos = 1; // Normal
            recommendations.thermal_profile = 0; // Güç tasarrufu
            break;
            
        case AI_OPTIMIZE_MEMORY:
            recommendations.memory_compression = 2 + (strength / 50); // Normal -> Agresif
            break;
            
        case AI_OPTIMIZE_STORAGE:
            recommendations.disk_io_priority = 1; // Normal
            break;
            
        case AI_OPTIMIZE_BALANCE:
            // Mevcut öneriler zaten dengeli
            break;
            
        default:
            // Önerilen değerleri kullan
            break;
    }
    
    // Sonuç JSON'u oluştur
    char* json_result = (char*)malloc(2048);
    if (!json_result) {
        ai_set_error(AI_ERROR_MEMORY, "Bellek ayırma hatası");
        return AI_ERROR_MEMORY;
    }
    
    snprintf(json_result, 2048,
             "{\n"
             "  \"target\": \"%s\",\n"
             "  \"strength\": %d,\n"
             "  \"recommendations\": {\n"
             "    \"cpu\": {\n"
             "      \"governor\": %d,\n"
             "      \"max_freq_percent\": %d,\n"
             "      \"core_parking\": %s,\n"
             "      \"available_cores_percent\": %d\n"
             "    },\n"
             "    \"memory\": {\n"
             "      \"compression_level\": %d\n"
             "    },\n"
             "    \"disk\": {\n"
             "      \"io_priority\": %d\n"
             "    },\n"
             "    \"network\": {\n"
             "      \"qos_level\": %d\n"
             "    },\n"
             "    \"thermal\": {\n"
             "      \"profile\": %d\n"
             "    },\n"
             "    \"process_limits\": %s\n"
             "  },\n"
             "  \"expected_impact\": {\n"
             "    \"performance\": %d,\n"
             "    \"power_efficiency\": %d,\n"
             "    \"thermal\": %d,\n"
             "    \"user_experience\": %d\n"
             "  }\n"
             "}",
             target == AI_OPTIMIZE_PERFORMANCE ? "performance" :
             target == AI_OPTIMIZE_POWER ? "power" :
             target == AI_OPTIMIZE_MEMORY ? "memory" :
             target == AI_OPTIMIZE_STORAGE ? "storage" :
             target == AI_OPTIMIZE_BALANCE ? "balance" : "unknown",
             strength,
             recommendations.cpu_governor,
             recommendations.cpu_max_freq_pct,
             recommendations.enable_core_parking ? "true" : "false",
             recommendations.available_core_pct,
             recommendations.memory_compression,
             recommendations.disk_io_priority,
             recommendations.network_qos,
             recommendations.thermal_profile,
             recommendations.process_limits,
             // Tahmini etki (0-100)
             target == AI_OPTIMIZE_PERFORMANCE ? 90 - strength/2 : 
             target == AI_OPTIMIZE_POWER ? 40 + strength/2 : 70,
             target == AI_OPTIMIZE_POWER ? 90 - strength/2 : 
             target == AI_OPTIMIZE_PERFORMANCE ? 40 + strength/2 : 70,
             target == AI_OPTIMIZE_PERFORMANCE ? 60 - strength/2 : 
             target == AI_OPTIMIZE_POWER ? 85 - strength/2 : 75,
             target == AI_OPTIMIZE_PERFORMANCE ? 80 + strength/5 : 
             target == AI_OPTIMIZE_POWER ? 65 + strength/10 : 75
    );
    
    *result_out = json_result;
    
    ai_log("Sistem kaynakları optimize edildi");
    return 0;
}

// Yapay zeka ile kod tamamlaması yap
int python_ai_code_completion(const char* code_prefix, char** result_out, uint8_t max_suggestions) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    if (!code_prefix || !result_out) {
        ai_set_error(AI_ERROR_PARAM, "Geçersiz parametreler");
        return AI_ERROR_PARAM;
    }
    
    // Basit kod tamamlama örneği
    const char* python_keywords[] = {
        "def ", "class ", "if ", "elif ", "else:", "for ", "while ", "try:", "except ", 
        "finally:", "import ", "from ", "return ", "with ", "as ", "in ", "not ", "is ", 
        "and ", "or ", "True", "False", "None", "lambda ", "pass", "break", "continue"
    };
    const int keyword_count = sizeof(python_keywords) / sizeof(python_keywords[0]);
    
    // Son kelimeyi al
    const char* last_word_start = code_prefix;
    const char* p = code_prefix;
    while (*p) {
        if (*p == ' ' || *p == '\n' || *p == '\t' || *p == '(' || *p == '.') {
            last_word_start = p + 1;
        }
        p++;
    }
    
    // Önerileri oluştur
    char* suggestions = (char*)malloc(2048);
    if (!suggestions) {
        ai_set_error(AI_ERROR_MEMORY, "Bellek ayırma hatası");
        return AI_ERROR_MEMORY;
    }
    
    // Sonuç dizisine JSON başlangıcı ekle
    strcpy(suggestions, "[\n");
    
    // Son kelime ile eşleşen anahtar kelimeleri bul
    int match_count = 0;
    size_t last_word_len = strlen(last_word_start);
    
    for (int i = 0; i < keyword_count && match_count < max_suggestions; i++) {
        if (strncmp(last_word_start, python_keywords[i], last_word_len) == 0) {
            // JSON öğesi ekle
            char suggestion[256];
            snprintf(suggestion, sizeof(suggestion),
                     "%s  {\n    \"suggestion\": \"%s\",\n    \"type\": \"keyword\"\n  }",
                     match_count > 0 ? ",\n" : "",
                     python_keywords[i]);
            strcat(suggestions, suggestion);
            match_count++;
        }
    }
    
    // Eğer hiç eşleşme yoksa, bağlama göre olası öneriler
    if (match_count == 0) {
        // Son satırdaki girinti seviyesini tespit et
        int indent_level = 0;
        const char* last_line = strrchr(code_prefix, '\n');
        if (last_line) {
            last_line++; // \n karakterini atla
            while (*last_line == ' ' || *last_line == '\t') {
                indent_level++;
                last_line++;
            }
        }
        
        // Bağlama göre öneriler
        if (strstr(code_prefix, "def ") != NULL && strstr(code_prefix, ":") != NULL) {
            strcat(suggestions, "  {\n    \"suggestion\": \"return \",\n    \"type\": \"keyword\"\n  },\n");
            strcat(suggestions, "  {\n    \"suggestion\": \"if \",\n    \"type\": \"keyword\"\n  },\n");
            strcat(suggestions, "  {\n    \"suggestion\": \"print(\",\n    \"type\": \"function\"\n  }");
            match_count = 3;
        } else if (strstr(code_prefix, "if ") != NULL && strstr(code_prefix, ":") != NULL) {
            strcat(suggestions, "  {\n    \"suggestion\": \"else:\",\n    \"type\": \"keyword\"\n  },\n");
            strcat(suggestions, "  {\n    \"suggestion\": \"elif \",\n    \"type\": \"keyword\"\n  }");
            match_count = 2;
        } else if (strstr(code_prefix, "import ") != NULL) {
            strcat(suggestions, "  {\n    \"suggestion\": \"numpy as np\",\n    \"type\": \"module\"\n  },\n");
            strcat(suggestions, "  {\n    \"suggestion\": \"pandas as pd\",\n    \"type\": \"module\"\n  },\n");
            strcat(suggestions, "  {\n    \"suggestion\": \"matplotlib.pyplot as plt\",\n    \"type\": \"module\"\n  }");
            match_count = 3;
        } else {
            strcat(suggestions, "  {\n    \"suggestion\": \"def \",\n    \"type\": \"keyword\"\n  },\n");
            strcat(suggestions, "  {\n    \"suggestion\": \"class \",\n    \"type\": \"keyword\"\n  },\n");
            strcat(suggestions, "  {\n    \"suggestion\": \"import \",\n    \"type\": \"keyword\"\n  }");
            match_count = 3;
        }
    }
    
    // JSON sonunu ekle
    strcat(suggestions, "\n]");
    
    *result_out = suggestions;
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Kod tamamlama yapıldı, %d öneri bulundu", match_count);
    ai_log(log_msg);
    
    return 0;
}

// Yapay zeka ile kod açıklaması üret
int python_ai_explain_code(const char* code, char** result_out) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    if (!code || !result_out) {
        ai_set_error(AI_ERROR_PARAM, "Geçersiz parametreler");
        return AI_ERROR_PARAM;
    }
    
    // Kod içeriğini analiz et
    size_t code_len = strlen(code);
    if (code_len == 0) {
        ai_set_error(AI_ERROR_PARAM, "Boş kod");
        return AI_ERROR_PARAM;
    }
    
    // İçerikte Python dili yapıları ara
    int has_def = strstr(code, "def ") != NULL;
    int has_class = strstr(code, "class ") != NULL;
    int has_import = strstr(code, "import ") != NULL;
    int has_for = strstr(code, "for ") != NULL;
    int has_if = strstr(code, "if ") != NULL;
    
    // Kod türünü belirle
    char code_type[32] = "kod parçası";
    if (has_class) {
        strcpy(code_type, "sınıf tanımı");
    } else if (has_def) {
        strcpy(code_type, "fonksiyon tanımı");
    } else if (has_import) {
        strcpy(code_type, "modül içe aktarma");
    } else if (has_for) {
        strcpy(code_type, "döngü yapısı");
    } else if (has_if) {
        strcpy(code_type, "koşul ifadesi");
    }
    
    // Yorum satırlarını say
    int comment_count = 0;
    const char* p = code;
    while ((p = strstr(p, "#")) != NULL) {
        comment_count++;
        p++;
    }
    
    // Açıklama metni için bellek ayır
    char* explanation = (char*)malloc(4096);
    if (!explanation) {
        ai_set_error(AI_ERROR_MEMORY, "Bellek ayırma hatası");
        return AI_ERROR_MEMORY;
    }
    
    // Açıklamayı oluştur
    snprintf(explanation, 4096,
            "# Kod Açıklaması\n\n"
            "Bu %s, aşağıdaki işlevleri gerçekleştirmektedir:\n\n",
            code_type
    );
    
    // Sınıf tanımı için
    if (has_class) {
        const char* class_name = strstr(code, "class ") + 6;
        char class_name_buf[64] = {0};
        int i = 0;
        while (*class_name && *class_name != '(' && *class_name != ':' && i < 63) {
            class_name_buf[i++] = *class_name++;
        }
        
        // Sonundaki boşlukları kaldır
        while (i > 0 && class_name_buf[i-1] == ' ') {
            class_name_buf[--i] = '\0';
        }
        
        strcat(explanation, "## Sınıf: ");
        strcat(explanation, class_name_buf);
        strcat(explanation, "\n\n");
        
        if (strstr(code, "__init__")) {
            strcat(explanation, "- Bir başlatıcı metot içermektedir (`__init__`).\n");
        }
        
        // Sınıf içindeki metodları say
        int method_count = 0;
        p = code;
        while ((p = strstr(p, "def ")) != NULL) {
            method_count++;
            p += 4;
        }
        
        char method_str[128];
        snprintf(method_str, sizeof(method_str), "- Toplam %d metot içermektedir.\n", method_count);
        strcat(explanation, method_str);
        
        if (strstr(code, "__str__") || strstr(code, "__repr__")) {
            strcat(explanation, "- Dizge gösterimi için özel metot içermektedir.\n");
        }
    }
    // Fonksiyon tanımı için
    else if (has_def) {
        const char* func_name = strstr(code, "def ") + 4;
        char func_name_buf[64] = {0};
        int i = 0;
        while (*func_name && *func_name != '(' && i < 63) {
            func_name_buf[i++] = *func_name++;
        }
        
        strcat(explanation, "## Fonksiyon: ");
        strcat(explanation, func_name_buf);
        strcat(explanation, "\n\n");
        
        // Docstring kontrolü
        if (strstr(code, "\"\"\"") || strstr(code, "'''")) {
            strcat(explanation, "- Fonksiyon belgeleme dizgesi (docstring) içermektedir.\n");
        }
        
        // Return ifadesi kontrolü
        if (strstr(code, "return ")) {
            strcat(explanation, "- Bir değer döndürmektedir.\n");
        } else {
            strcat(explanation, "- Değer döndürmemektedir (None).\n");
        }
        
        // Parametre sayımı
        const char* params_start = strstr(code, "(");
        const char* params_end = strstr(code, ")");
        if (params_start && params_end && params_end > params_start) {
            char params[256] = {0};
            strncpy(params, params_start + 1, params_end - params_start - 1);
            
            if (strlen(params) == 0) {
                strcat(explanation, "- Parametre almamaktadır.\n");
            } else {
                int param_count = 1;
                for (i = 0; params[i]; i++) {
                    if (params[i] == ',') param_count++;
                }
                
                char param_str[128];
                snprintf(param_str, sizeof(param_str), "- %d parametre almaktadır.\n", param_count);
                strcat(explanation, param_str);
            }
        }
    }
    
    // Genel açıklama
    strcat(explanation, "\n## Genel Analiz\n\n");
    
    // Kod satır sayısı
    int line_count = 1;
    for (i = 0; code[i]; i++) {
        if (code[i] == '\n') line_count++;
    }
    
    char line_str[128];
    snprintf(line_str, sizeof(line_str), "- Toplam %d satır kod içermektedir.\n", line_count);
    strcat(explanation, line_str);
    
    // Yorum satırları
    if (comment_count > 0) {
        char comment_str[128];
        snprintf(comment_str, sizeof(comment_str), "- %d adet yorum satırı bulunmaktadır.\n", comment_count);
        strcat(explanation, comment_str);
    } else {
        strcat(explanation, "- Hiç yorum satırı içermemektedir.\n");
    }
    
    // Genel özellikler
    if (has_import) {
        strcat(explanation, "- Dış modül içe aktarmaları bulunmaktadır.\n");
    }
    
    if (has_for) {
        strcat(explanation, "- Döngü yapıları kullanılmaktadır.\n");
    }
    
    if (has_if) {
        strcat(explanation, "- Koşul ifadeleri içermektedir.\n");
    }
    
    if (strstr(code, "try") && strstr(code, "except")) {
        strcat(explanation, "- Hata yakalama mekanizması içermektedir.\n");
    }
    
    if (strstr(code, "with ")) {
        strcat(explanation, "- Bağlam yöneticisi (context manager) kullanılmaktadır.\n");
    }
    
    // Kod kalitesi değerlendirmesi
    strcat(explanation, "\n## Kalite Değerlendirmesi\n\n");
    
    float comment_ratio = (float)comment_count / line_count;
    if (comment_ratio < 0.1) {
        strcat(explanation, "- Daha fazla yorum ekleyerek kodun okunabilirliği artırılabilir.\n");
    } else if (comment_ratio > 0.3) {
        strcat(explanation, "- Yorum yoğunluğu iyi düzeydedir.\n");
    }
    
    if (strstr(code, "  ") && strstr(code, "\t")) {
        strcat(explanation, "- Kod içinde hem boşluk hem de sekme karakterleri karışık kullanılmıştır. Girintileme stilini standartlaştırın.\n");
    }
    
    if (line_count > 50 && !has_def && !has_class) {
        strcat(explanation, "- Kod oldukça uzun. Fonksiyonlara ayırarak modülerleştirmek faydalı olabilir.\n");
    }
    
    if (strstr(code, "print(") || strstr(code, "print (")) {
        strcat(explanation, "- Hata ayıklama amaçlı print ifadeleri bulunmaktadır. Üretime geçmeden önce kontrol edilmeli.\n");
    }
    
    *result_out = explanation;
    
    ai_log("Kod açıklaması oluşturuldu");
    return 0;
}

// Yapay zeka ile kod analizi yap
int python_ai_analyze_code(const char* code, char** result_out) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    if (!code || !result_out) {
        ai_set_error(AI_ERROR_PARAM, "Geçersiz parametreler");
        return AI_ERROR_PARAM;
    }
    
    // JSON formatında analiz sonuçları oluştur
    char* analysis = (char*)malloc(4096);
    if (!analysis) {
        ai_set_error(AI_ERROR_MEMORY, "Bellek ayırma hatası");
        return AI_ERROR_MEMORY;
    }
    
    // Kodu Python'a gönderip analiz et
    char* escaped_code = (char*)malloc(strlen(code) * 2 + 1);
    if (!escaped_code) {
        ai_set_error(AI_ERROR_MEMORY, "Bellek ayırma hatası");
        free(analysis);
        return AI_ERROR_MEMORY;
    }
    
    // Kod içeriğini kaçış
    const char* src = code;
    char* dst = escaped_code;
    while (*src) {
        if (*src == '\\' || *src == '\'' || *src == '\"') {
            *dst++ = '\\';
        }
        if (*src == '\n') {
            *dst++ = '\\';
            *dst++ = 'n';
            src++;
            continue;
        }
        *dst++ = *src++;
    }
    *dst = '\0';
    
    // Python kod analizi komutu
    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
            "try:\n"
            "    import json\n"
            "    import ast\n"
            "    import re\n"
            "    \n"
            "    code = \"\"\"%s\"\"\"\n"
            "    \n"
            "    analysis = {\n"
            "        'syntax_valid': True,\n"
            "        'line_count': code.count('\\n') + 1,\n"
            "        'char_count': len(code),\n"
            "        'comment_count': len(re.findall(r'#.*$', code, re.MULTILINE)),\n"
            "        'functions': [],\n"
            "        'classes': [],\n"
            "        'imports': [],\n"
            "        'variables': [],\n"
            "        'complexity': {\n"
            "            'cyclomatic': 0,\n"
            "            'cognitive': 0\n"
            "        },\n"
            "        'issues': []\n"
            "    }\n"
            "    \n"
            "    # Sözdizimi kontrolü\n"
            "    try:\n"
            "        tree = ast.parse(code)\n"
            "    except SyntaxError as e:\n"
            "        analysis['syntax_valid'] = False\n"
            "        analysis['issues'].append({\n"
            "            'type': 'syntax_error',\n"
            "            'message': str(e),\n"
            "            'line': e.lineno if hasattr(e, 'lineno') else 0\n"
            "        })\n"
            "    \n"
            "    if analysis['syntax_valid']:\n"
            "        # Fonksiyonları bul\n"
            "        for node in ast.walk(tree):\n"
            "            if isinstance(node, ast.FunctionDef):\n"
            "                analysis['functions'].append({\n"
            "                    'name': node.name,\n"
            "                    'line': node.lineno,\n"
            "                    'args': len(node.args.args),\n"
            "                    'returns': node.returns is not None\n"
            "                })\n"
            "                # Karmaşıklık ölçümü\n"
            "                analysis['complexity']['cyclomatic'] += sum(1 for _ in ast.walk(node) if isinstance(_, (ast.If, ast.For, ast.While, ast.Try)))\n"
            "            \n"
            "            # Sınıfları bul\n"
            "            elif isinstance(node, ast.ClassDef):\n"
            "                analysis['classes'].append({\n"
            "                    'name': node.name,\n"
            "                    'line': node.lineno,\n"
            "                    'methods': len([m for m in node.body if isinstance(m, ast.FunctionDef)]),\n"
            "                    'has_init': any(m.name == '__init__' for m in node.body if isinstance(m, ast.FunctionDef))\n"
            "                })\n"
            "            \n"
            "            # İçe aktarmaları bul\n"
            "            elif isinstance(node, ast.Import):\n"
            "                for name in node.names:\n"
            "                    analysis['imports'].append({\n"
            "                        'name': name.name,\n"
            "                        'alias': name.asname\n"
            "                    })\n"
            "            elif isinstance(node, ast.ImportFrom):\n"
            "                for name in node.names:\n"
            "                    analysis['imports'].append({\n"
            "                        'name': f'{node.module}.{name.name}',\n"
            "                        'alias': name.asname\n"
            "                    })\n"
            "        \n"
            "        # Karmaşıklık puanı\n"
            "        analysis['complexity']['cognitive'] = analysis['complexity']['cyclomatic'] + len(analysis['functions']) // 2\n"
            "        \n"
            "        # Olası sorunları tespit et\n"
            "        if analysis['line_count'] > 300:\n"
            "            analysis['issues'].append({\n"
            "                'type': 'code_size',\n"
            "                'message': 'Dosya çok uzun, daha küçük modüllere bölünmesi önerilir',\n"
            "                'severity': 'warning'\n"
            "            })\n"
            "        \n"
            "        if analysis['comment_count'] < analysis['line_count'] // 10:\n"
            "            analysis['issues'].append({\n"
            "                'type': 'documentation',\n"
            "                'message': 'Yorum sayısı düşük, daha fazla belgeleme yapılması önerilir',\n"
            "                'severity': 'info'\n"
            "            })\n"
            "        \n"
            "        if any(len(f.get('name', '')) < 3 for f in analysis['functions']):\n"
            "            analysis['issues'].append({\n"
            "                'type': 'naming',\n"
            "                'message': 'Bazı fonksiyon isimleri çok kısa, daha açıklayıcı isimler kullanılması önerilir',\n"
            "                'severity': 'info'\n"
            "            })\n"
            "    \n"
            "    kalem_code_analysis = json.dumps(analysis, indent=2)\n"
            "    print('Kod analizi tamamlandı')\n"
            "except Exception as e:\n"
            "    kalem_code_analysis = json.dumps({\n"
            "        'error': True,\n"
            "        'message': str(e)\n"
            "    })\n"
            "    print(f'Kod analizi hatası: {e}')\n",
            escaped_code
    );
    
    free(escaped_code);
    
    // Analiz komutunu çalıştır
    int result = python_run_string(cmd);
    if (result != 0) {
        ai_set_error(AI_ERROR_INFERENCE, "Kod analizi komutu çalıştırılamadı");
        free(analysis);
        return AI_ERROR_INFERENCE;
    }
    
    // Analiz sonucunu al
    char* output = NULL;
    result = python_run_string_with_output(
        "print(kalem_code_analysis)",
        &output
    );
    
    if (result != 0 || !output) {
        ai_set_error(AI_ERROR_INFERENCE, "Kod analizi sonucu alınamadı");
        free(analysis);
        if (output) free(output);
        return AI_ERROR_INFERENCE;
    }
    
    // Output'u doğrudan result_out'a ata
    *result_out = output;
    
    ai_log("Kod analizi tamamlandı");
    return 0;
}

// Yapay zeka ile doğal dil komutunu yorumla
int python_ai_interpret_command(const char* nl_command, char** result_out) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    if (!nl_command || !result_out) {
        ai_set_error(AI_ERROR_PARAM, "Geçersiz parametreler");
        return AI_ERROR_PARAM;
    }
    
    // Basit bir doğal dil komut yorumlayıcı
    char* interpretation = (char*)malloc(2048);
    if (!interpretation) {
        ai_set_error(AI_ERROR_MEMORY, "Bellek ayırma hatası");
        return AI_ERROR_MEMORY;
    }
    
    // Komutları tanıma ve yorumlama
    const char* command = nl_command;
    
    // JSON çıktısı başlat
    strcpy(interpretation, "{\n");
    
    // Komut türünü belirle
    if (strstr(command, "aç") || strstr(command, "göster") || strstr(command, "başlat")) {
        strcat(interpretation, "  \"action\": \"open\",\n");
        
        // Açılacak şeyi belirle
        if (strstr(command, "dosya") || strstr(command, "belge")) {
            strcat(interpretation, "  \"target_type\": \"file\",\n");
        } else if (strstr(command, "uygulama") || strstr(command, "program")) {
            strcat(interpretation, "  \"target_type\": \"application\",\n");
        } else if (strstr(command, "ayarlar") || strstr(command, "tercihler")) {
            strcat(interpretation, "  \"target_type\": \"settings\",\n");
        } else {
            strcat(interpretation, "  \"target_type\": \"unknown\",\n");
        }
    } else if (strstr(command, "kapat") || strstr(command, "çık") || strstr(command, "sonlandır")) {
        strcat(interpretation, "  \"action\": \"close\",\n");
        
        // Kapatılacak şeyi belirle
        if (strstr(command, "dosya") || strstr(command, "belge")) {
            strcat(interpretation, "  \"target_type\": \"file\",\n");
        } else if (strstr(command, "uygulama") || strstr(command, "program")) {
            strcat(interpretation, "  \"target_type\": \"application\",\n");
        } else if (strstr(command, "tümü") || strstr(command, "hepsi")) {
            strcat(interpretation, "  \"target_type\": \"all\",\n");
        } else {
            strcat(interpretation, "  \"target_type\": \"current\",\n");
        }
    } else if (strstr(command, "kaydet") || strstr(command, "sakla")) {
        strcat(interpretation, "  \"action\": \"save\",\n");
        
        // Kaydetme türünü belirle
        if (strstr(command, "farklı")) {
            strcat(interpretation, "  \"save_type\": \"save_as\",\n");
        } else {
            strcat(interpretation, "  \"save_type\": \"save\",\n");
        }
    } else if (strstr(command, "ara") || strstr(command, "bul")) {
        strcat(interpretation, "  \"action\": \"search\",\n");
        
        // Arama türünü belirle
        if (strstr(command, "dosya")) {
            strcat(interpretation, "  \"search_type\": \"file\",\n");
        } else if (strstr(command, "metin") || strstr(command, "kelime")) {
            strcat(interpretation, "  \"search_type\": \"text\",\n");
        } else {
            strcat(interpretation, "  \"search_type\": \"general\",\n");
        }
    } else if (strstr(command, "yenile") || strstr(command, "güncelle")) {
        strcat(interpretation, "  \"action\": \"refresh\",\n");
    } else if (strstr(command, "yardım") || strstr(command, "destek")) {
        strcat(interpretation, "  \"action\": \"help\",\n");
    } else {
        strcat(interpretation, "  \"action\": \"unknown\",\n");
    }
    
    // Güven derecesi - basit bir tahmin
    int confidence = 70; // Varsayılan güven derecesi
    
    // Daha kesin komutlar için güveni artır
    if (strlen(command) < 20) {
        confidence += 10; // Kısa ve öz komutlar daha kesin olabilir
    }
    
    // Belirsiz ifadeler için güveni azalt
    if (strstr(command, "belki") || strstr(command, "sanırım") || strstr(command, "galiba")) {
        confidence -= 20;
    }
    
    // Spesifik hedefler için güveni artır
    if (strstr(command, ".py") || strstr(command, ".txt") || strstr(command, ".c") ||
        strstr(command, ".json") || strstr(command, ".xml")) {
        confidence += 15; // Dosya uzantısı belirtilmişse
    }
    
    char confidence_str[64];
    snprintf(confidence_str, sizeof(confidence_str), "  \"confidence\": %d,\n", confidence);
    strcat(interpretation, confidence_str);
    
    // Orijinal komutu ekle
    strcat(interpretation, "  \"original_command\": \"");
    strcat(interpretation, nl_command);
    strcat(interpretation, "\",\n");
    
    // Varsa özel parametreleri çıkar
    if (strstr(command, "dosya") && (strstr(command, ".py") || strstr(command, ".txt") || 
                                      strstr(command, ".c") || strstr(command, ".json"))) {
        // Dosya adını bulmaya çalış
        const char* p = command;
        char file_name[128] = {0};
        int found = 0;
        
        while (*p) {
            if ((*p == '.' && (*(p+1) == 'p' || *(p+1) == 't' || *(p+1) == 'c' || *(p+1) == 'j')) ||
                (*(p-1) == '.' && (*p == 'p' || *p == 't' || *p == 'c' || *p == 'j'))) {
                // Dosya adının başlangıcını bul
                const char* start = p;
                while (start > command && *(start-1) != ' ' && *(start-1) != '\'' && *(start-1) != '"') {
                    start--;
                }
                
                // Dosya adının sonunu bul
                const char* end = p;
                while (*end && *end != ' ' && *end != '\'' && *end != '"') {
                    end++;
                }
                
                // Dosya adını kopyala
                if (end > start && (end - start) < sizeof(file_name)) {
                    strncpy(file_name, start, end - start);
                    file_name[end - start] = '\0';
                    found = 1;
                    break;
                }
            }
            p++;
        }
        
        if (found) {
            strcat(interpretation, "  \"parameters\": {\n");
            strcat(interpretation, "    \"file_name\": \"");
            strcat(interpretation, file_name);
            strcat(interpretation, "\"\n");
            strcat(interpretation, "  }\n");
        } else {
            strcat(interpretation, "  \"parameters\": {}\n");
        }
    } else {
        strcat(interpretation, "  \"parameters\": {}\n");
    }
    
    // JSON çıktısını kapat
    strcat(interpretation, "}");
    
    *result_out = interpretation;
    
    ai_log("Doğal dil komutu yorumlandı");
    return 0;
}

// Yapay zeka ile doğal dil komutunu eyleme dönüştür
int python_ai_command_to_action(const char* nl_command, char** action_out) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    if (!nl_command || !action_out) {
        ai_set_error(AI_ERROR_PARAM, "Geçersiz parametreler");
        return AI_ERROR_PARAM;
    }
    
    // Önce komutu yorumla
    char* interpretation = NULL;
    int result = python_ai_interpret_command(nl_command, &interpretation);
    
    if (result != 0 || !interpretation) {
        ai_set_error(AI_ERROR_INFERENCE, "Komut yorumlanamadı");
        return AI_ERROR_INFERENCE;
    }
    
    // Yorumlama sonucundan eylem oluştur
    char* action = (char*)malloc(4096);
    if (!action) {
        ai_set_error(AI_ERROR_MEMORY, "Bellek ayırma hatası");
        free(interpretation);
        return AI_ERROR_MEMORY;
    }
    
    // Basit bir JSON ayrıştırıcı ile eylem bilgilerini çıkar
    const char* action_type = strstr(interpretation, "\"action\":");
    const char* target_type = strstr(interpretation, "\"target_type\":");
    const char* parameters = strstr(interpretation, "\"parameters\":");
    const char* confidence = strstr(interpretation, "\"confidence\":");
    
    // Eylem JSON'ını oluştur
    strcpy(action, "{\n");
    strcat(action, "  \"status\": \"success\",\n");
    
    // Güven derecesi kontrolü
    int confidence_value = 0;
    if (confidence) {
        sscanf(strstr(confidence, ":") + 1, "%d", &confidence_value);
    }
    
    if (confidence_value < 50) {
        strcpy(action, "{\n");
        strcat(action, "  \"status\": \"error\",\n");
        strcat(action, "  \"error\": \"low_confidence\",\n");
        strcat(action, "  \"message\": \"Komut anlaşılamadı, lütfen daha açık ifade ediniz.\",\n");
        strcat(action, "  \"confidence\": ");
        
        char confidence_str[8];
        snprintf(confidence_str, sizeof(confidence_str), "%d", confidence_value);
        strcat(action, confidence_str);
        strcat(action, "\n}");
        
        free(interpretation);
        *action_out = action;
        return 0;
    }
    
    // Eylem türünü belirle
    char action_name[32] = {0};
    if (action_type) {
        const char* start = strchr(action_type, ':') + 1;
        while (*start == ' ' || *start == '\"') start++;
        
        const char* end = strchr(start, '\"');
        if (end && (end - start) < sizeof(action_name)) {
            strncpy(action_name, start, end - start);
        }
    }
    
    // Hedef türünü belirle
    char target_name[32] = {0};
    if (target_type) {
        const char* start = strchr(target_type, ':') + 1;
        while (*start == ' ' || *start == '\"') start++;
        
        const char* end = strchr(start, '\"');
        if (end && (end - start) < sizeof(target_name)) {
            strncpy(target_name, start, end - start);
        }
    }
    
    // Komut yapısını oluştur
    if (strcmp(action_name, "open") == 0) {
        strcat(action, "  \"command\": \"open\",\n");
        strcat(action, "  \"arguments\": [\n");
        
        if (strcmp(target_name, "file") == 0) {
            // Dosya adını parametrelerden çıkar
            const char* file_param = strstr(parameters, "\"file_name\":");
            if (file_param) {
                const char* start = strchr(file_param, ':') + 1;
                while (*start == ' ' || *start == '\"') start++;
                
                const char* end = strchr(start, '\"');
                if (end) {
                    strcat(action, "    \"");
                    strncat(action, start, end - start);
                    strcat(action, "\"\n");
                }
            } else {
                strcat(action, "    \"file_dialog\"\n");
            }
        } else if (strcmp(target_name, "application") == 0) {
            strcat(action, "    \"app_launcher\"\n");
        } else if (strcmp(target_name, "settings") == 0) {
            strcat(action, "    \"settings_panel\"\n");
        } else {
            strcat(action, "    \"unknown\"\n");
        }
        
        strcat(action, "  ],\n");
    } else if (strcmp(action_name, "close") == 0) {
        strcat(action, "  \"command\": \"close\",\n");
        
        if (strcmp(target_name, "all") == 0) {
            strcat(action, "  \"arguments\": [\"all\"],\n");
        } else {
            strcat(action, "  \"arguments\": [],\n");
        }
    } else if (strcmp(action_name, "save") == 0) {
        const char* save_type = strstr(interpretation, "\"save_type\":");
        if (save_type && strstr(save_type, "save_as")) {
            strcat(action, "  \"command\": \"save_as\",\n");
        } else {
            strcat(action, "  \"command\": \"save\",\n");
        }
        strcat(action, "  \"arguments\": [],\n");
    } else if (strcmp(action_name, "search") == 0) {
        strcat(action, "  \"command\": \"search\",\n");
        strcat(action, "  \"arguments\": [\n");
        
        const char* search_type = strstr(interpretation, "\"search_type\":");
        if (search_type) {
            const char* start = strchr(search_type, ':') + 1;
            while (*start == ' ' || *start == '\"') start++;
            
            const char* end = strchr(start, '\"');
            if (end) {
                strcat(action, "    \"");
                strncat(action, start, end - start);
                strcat(action, "\"\n");
            }
        }
        
        strcat(action, "  ],\n");
    } else if (strcmp(action_name, "refresh") == 0) {
        strcat(action, "  \"command\": \"refresh\",\n");
        strcat(action, "  \"arguments\": [],\n");
    } else if (strcmp(action_name, "help") == 0) {
        strcat(action, "  \"command\": \"help\",\n");
        strcat(action, "  \"arguments\": [],\n");
    } else {
        strcat(action, "  \"command\": \"unknown\",\n");
        strcat(action, "  \"arguments\": [],\n");
        strcat(action, "  \"original_text\": \"");
        strcat(action, nl_command);
        strcat(action, "\",\n");
    }
    
    // Güven derecesini ekle
    char confidence_str[64];
    snprintf(confidence_str, sizeof(confidence_str), "  \"confidence\": %d\n", confidence_value);
    strcat(action, confidence_str);
    
    // JSON'ı kapat
    strcat(action, "}");
    
    free(interpretation);
    *action_out = action;
    
    ai_log("Doğal dil komutu eyleme dönüştürüldü");
    return 0;
}

// Yapay zeka modülleri listesini al
int python_ai_list_models(char** models_out) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    if (!models_out) {
        ai_set_error(AI_ERROR_PARAM, "Geçersiz parametre");
        return AI_ERROR_PARAM;
    }
    
    // JSON formatında model listesi oluştur
    char* json_models = (char*)malloc(2048);
    if (!json_models) {
        ai_set_error(AI_ERROR_MEMORY, "Bellek ayırma hatası");
        return AI_ERROR_MEMORY;
    }
    
    // JSON başlangıcı
    strcpy(json_models, "[\n");
    
    // Her model için bir JSON nesnesi ekle
    for (int i = 0; i < model_count; i++) {
        char model_json[512];
        snprintf(model_json, sizeof(model_json),
                "%s  {\n"
                "    \"id\": %d,\n"
                "    \"name\": \"%s\",\n"
                "    \"type\": %d,\n"
                "    \"path\": \"%s\",\n"
                "    \"options\": {\n"
                "      \"temperature\": %.2f,\n"
                "      \"max_tokens\": %d,\n"
                "      \"use_gpu\": %s,\n"
                "      \"quantized\": %s\n"
                "    }\n"
                "  }",
                i > 0 ? ",\n" : "",
                loaded_models[i].id,
                loaded_models[i].config.model_name,
                loaded_models[i].type,
                loaded_models[i].config.model_path,
                loaded_models[i].config.temperature,
                loaded_models[i].config.max_tokens,
                loaded_models[i].config.use_gpu ? "true" : "false",
                loaded_models[i].config.quantized ? "true" : "false"
        );
        
        strcat(json_models, model_json);
    }
    
    // JSON sonu
    strcat(json_models, "\n]");
    
    *models_out = json_models;
    
    ai_log("Model listesi oluşturuldu");
    return 0;
}

// Yapay zeka model bilgisini al
int python_ai_get_model_info(int model_id, char** info_out) {
    if (!ai_initialized) {
        ai_set_error(AI_ERROR_INIT, "Yapay zeka modülü başlatılmadı");
        return AI_ERROR_INIT;
    }
    
    if (!info_out) {
        ai_set_error(AI_ERROR_PARAM, "Geçersiz parametre");
        return AI_ERROR_PARAM;
    }
    
    // Modeli bul
    int model_idx = -1;
    for (int i = 0; i < model_count; i++) {
        if (loaded_models[i].id == model_id) {
            model_idx = i;
            break;
        }
    }
    
    if (model_idx == -1) {
        ai_set_error(AI_ERROR_NOT_FOUND, "Model bulunamadı");
        return AI_ERROR_NOT_FOUND;
    }
    
    // Model türü ve yapılandırması hakkında ek bilgiler
    const char* model_type_str = NULL;
    const char* model_capability_str = NULL;
    
    switch (loaded_models[model_idx].type) {
        case AI_MODEL_TEXT:
            model_type_str = "text";
            model_capability_str = "[\"text_generation\", \"completion\", \"chat\"]";
            break;
        case AI_MODEL_IMAGE:
            model_type_str = "image";
            model_capability_str = "[\"image_generation\", \"image_classification\"]";
            break;
        case AI_MODEL_AUDIO:
            model_type_str = "audio";
            model_capability_str = "[\"speech_recognition\", \"speech_synthesis\"]";
            break;
        case AI_MODEL_HYBRID:
            model_type_str = "hybrid";
            model_capability_str = "[\"multimodal\", \"text_generation\", \"image_understanding\"]";
            break;
        case AI_MODEL_OPTIMIZATION:
            model_type_str = "optimization";
            model_capability_str = "[\"resource_optimization\", \"performance_tuning\"]";
            break;
        default:
            model_type_str = "unknown";
            model_capability_str = "[]";
            break;
    }
    
    // JSON formatında model bilgisi oluştur
    char* json_info = (char*)malloc(2048);
    if (!json_info) {
        ai_set_error(AI_ERROR_MEMORY, "Bellek ayırma hatası");
        return AI_ERROR_MEMORY;
    }
    
    const ai_model_config_t* config = &loaded_models[model_idx].config;
    
    snprintf(json_info, 2048,
            "{\n"
            "  \"id\": %d,\n"
            "  \"name\": \"%s\",\n"
            "  \"type\": \"%s\",\n"
            "  \"capabilities\": %s,\n"
            "  \"path\": \"%s\",\n"
            "  \"configuration\": {\n"
            "    \"temperature\": %.2f,\n"
            "    \"max_tokens\": %d,\n"
            "    \"use_gpu\": %s,\n"
            "    \"quantized\": %s,\n"
            "    \"memory_limit_mb\": %d,\n"
            "    \"thread_count\": %d,\n"
            "    \"optimization_level\": %d\n"
            "  },\n"
            "  \"status\": {\n"
            "    \"loaded\": %s,\n"
            "    \"active\": true,\n"
            "    \"memory_usage_mb\": %d\n"
            "  }\n"
            "}",
            loaded_models[model_idx].id,
            config->model_name,
            model_type_str,
            model_capability_str,
            config->model_path,
            config->temperature,
            config->max_tokens,
            config->use_gpu ? "true" : "false",
            config->quantized ? "true" : "false",
            config->max_memory_mb,
            config->thread_count,
            config->optimization_level,
            loaded_models[model_idx].is_loaded ? "true" : "false",
            config->max_memory_mb / 2  // Tahmini bellek kullanımı
    );
    
    *info_out = json_info;
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Model bilgisi alındı (ID: %d)", model_id);
    ai_log(log_msg);
    
    return 0;
} 