#include "../include/hardware_manager.h"
#include "hardware_manager_internal.h"
#include "pthread_adapter.h"  /* pthread.h yerine adaptörümüzü kullan */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

/* Dışarıdan erişilebilecek değişkenler */
extern bool g_initialized;
extern hw_manager_config_t g_config;
extern hw_manager_status_t g_status;
extern hw_component_t* g_components;
extern uint32_t g_component_count;
extern uint32_t g_component_capacity;
extern uint32_t g_next_component_id;
extern hw_monitor_t* g_monitors;
extern uint32_t g_monitor_count;
extern uint32_t g_monitor_capacity;
extern uint32_t g_next_monitor_id;
extern pthread_mutex_t g_component_mutex;
extern pthread_mutex_t g_monitor_mutex;
extern pthread_mutex_t g_status_mutex;
extern FILE* g_log_file;
extern uint32_t g_log_level;

/* Dışarıdan erişilebilecek fonksiyonlar */
extern int hw_find_component_index(uint32_t component_id);
extern int hw_update_component_status(hw_component_t* component);
extern void hw_log(uint32_t level, const char* format, ...);

/**
 * Donanım sürücüsünü yükler
 */
int hw_manager_load_driver(uint32_t component_id, bool force_reload) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_component_mutex);
    
    int index = hw_find_component_index(component_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_component_mutex);
        return HW_ERROR_NOT_FOUND;
    }
    
    hw_component_t* component = &g_components[index];
    
    /* Mevcut durumu kontrol et */
    if (!force_reload && 
        (component->driver.status == DRIVER_STATUS_ACTIVE || 
         component->driver.status == DRIVER_STATUS_LOADED)) {
        /* Sürücü zaten yüklü */
        pthread_mutex_unlock(&g_component_mutex);
        return HW_ERROR_NONE;
    }
    
    /* Şu anda gerçek bir yükleme yapmıyoruz, sadece durumu güncelliyoruz */
    hw_log(2, "Sürücü yükleniyor: %s (%s)", 
           component->driver.name, component->info.name);
    
    /* Gerçek bir uygulamada, burada modprobe veya benzer bir komut çalıştırılır */
    
    /* Başarılı olduğunu varsay */
    component->driver.status = DRIVER_STATUS_ACTIVE;
    component->driver.load_time = (uint32_t)time(NULL);
    
    pthread_mutex_unlock(&g_component_mutex);
    
    /* Durumu güncelle */
    hw_manager_update_status(component_id);
    
    return HW_ERROR_NONE;
}

/**
 * Donanım sürücüsünü kaldırır
 */
int hw_manager_unload_driver(uint32_t component_id) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_component_mutex);
    
    int index = hw_find_component_index(component_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_component_mutex);
        return HW_ERROR_NOT_FOUND;
    }
    
    hw_component_t* component = &g_components[index];
    
    /* Mevcut durumu kontrol et */
    if (component->driver.status != DRIVER_STATUS_ACTIVE && 
        component->driver.status != DRIVER_STATUS_LOADED) {
        /* Sürücü zaten aktif değil */
        pthread_mutex_unlock(&g_component_mutex);
        return HW_ERROR_NONE;
    }
    
    /* Şu anda gerçek bir kaldırma yapmıyoruz, sadece durumu güncelliyoruz */
    hw_log(2, "Sürücü kaldırılıyor: %s (%s)", 
           component->driver.name, component->info.name);
    
    /* Gerçek bir uygulamada, burada modprobe -r veya benzer bir komut çalıştırılır */
    
    /* Başarılı olduğunu varsay */
    component->driver.status = DRIVER_STATUS_INSTALLED;
    
    pthread_mutex_unlock(&g_component_mutex);
    
    /* Durumu güncelle */
    hw_manager_update_status(component_id);
    
    return HW_ERROR_NONE;
}

/**
 * Donanım sürücüsünü günceller
 */
int hw_manager_update_driver(uint32_t component_id, bool check_only, void* update_info) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    /* Tüm sürücüleri güncelle */
    if (component_id == 0) {
        int update_count = 0;
        
        pthread_mutex_lock(&g_component_mutex);
        
        for (uint32_t i = 0; i < g_component_count; i++) {
            /* Mevcut durumu kontrol et */
            if (g_components[i].driver.status == DRIVER_STATUS_ACTIVE || 
                g_components[i].driver.status == DRIVER_STATUS_LOADED ||
                g_components[i].driver.status == DRIVER_STATUS_INSTALLED) {
                
                /* Burada gerçekte sürücü güncellemesi kontrolü yapılır */
                bool needs_update = (rand() % 10) == 0; /* Demo için rastgele */
                
                if (needs_update) {
                    hw_log(2, "Sürücü güncellemesi gerekiyor: %s (%s)", 
                           g_components[i].driver.name, g_components[i].info.name);
                    
                    update_count++;
                    
                    /* Sadece kontrol modunda değilse güncelle */
                    if (!check_only) {
                        /* Sürücüyü kaldır */
                        hw_manager_unload_driver(g_components[i].info.id);
                        
                        /* Güncelleme simülasyonu */
                        snprintf(g_components[i].driver.version, 
                                sizeof(g_components[i].driver.version),
                                "%d.%d.%d", 
                                rand() % 10, rand() % 10, rand() % 100);
                        
                        /* Sürücüyü tekrar yükle */
                        hw_manager_load_driver(g_components[i].info.id, true);
                    }
                }
            }
        }
        
        pthread_mutex_unlock(&g_component_mutex);
        
        return update_count;
    }
    
    /* Tek bir sürücüyü güncelle */
    pthread_mutex_lock(&g_component_mutex);
    
    int index = hw_find_component_index(component_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_component_mutex);
        return HW_ERROR_NOT_FOUND;
    }
    
    hw_component_t* component = &g_components[index];
    
    /* Mevcut durumu kontrol et */
    if (component->driver.status != DRIVER_STATUS_ACTIVE && 
        component->driver.status != DRIVER_STATUS_LOADED &&
        component->driver.status != DRIVER_STATUS_INSTALLED) {
        /* Sürücü uygun durumda değil */
        pthread_mutex_unlock(&g_component_mutex);
        return HW_ERROR_NONE;
    }
    
    /* Burada gerçekte sürücü güncellemesi kontrolü yapılır */
    bool needs_update = (rand() % 5) == 0; /* Demo için rastgele */
    
    if (needs_update) {
        hw_log(2, "Sürücü güncellemesi gerekiyor: %s (%s)", 
               component->driver.name, component->info.name);
        
        /* Sadece kontrol modunda değilse güncelle */
        if (!check_only) {
            /* Sürücüyü kaldır */
            hw_manager_unload_driver(component_id);
            
            /* Güncelleme simülasyonu */
            snprintf(component->driver.version, 
                    sizeof(component->driver.version),
                    "%d.%d.%d", 
                    rand() % 10, rand() % 10, rand() % 100);
            
            /* Sürücüyü tekrar yükle */
            hw_manager_load_driver(component_id, true);
        }
        
        pthread_mutex_unlock(&g_component_mutex);
        return 1;
    }
    
    pthread_mutex_unlock(&g_component_mutex);
    return 0;
}

/**
 * Donanım bileşeninin güç durumunu değiştirir
 */
int hw_manager_set_power_state(uint32_t component_id, hw_power_state_t power_state) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_component_mutex);
    
    int index = hw_find_component_index(component_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_component_mutex);
        return HW_ERROR_NOT_FOUND;
    }
    
    hw_component_t* component = &g_components[index];
    
    /* Şu anda gerçek bir güç durumu değişimi yapmıyoruz, sadece durumu güncelliyoruz */
    hw_log(2, "Güç durumu değiştiriliyor: %s -> %d", 
           component->info.name, power_state);
    
    /* Değişikliği uyguladığımızı varsay */
    component->status.power_state = power_state;
    
    /* Güç tasarrufu veya uyku moduna geçildiğinde kullanım azalır */
    if (power_state == HW_POWER_SAVING || power_state == HW_POWER_SLEEP) {
        component->status.utilization = component->status.utilization / 2;
        component->status.power_usage = component->status.power_usage / 3;
    }
    /* Yüksek performans moduna geçildiğinde kullanım artar */
    else if (power_state == HW_POWER_HIGH_PERF) {
        component->status.utilization = component->status.utilization * 2;
        if (component->status.utilization > 100) {
            component->status.utilization = 100;
        }
        component->status.power_usage = component->status.power_usage * 2;
    }
    
    pthread_mutex_unlock(&g_component_mutex);
    
    return HW_ERROR_NONE;
}

/**
 * Donanım bileşenini etkinleştirir veya devre dışı bırakır
 */
int hw_manager_set_enabled(uint32_t component_id, bool enable) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_component_mutex);
    
    int index = hw_find_component_index(component_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_component_mutex);
        return HW_ERROR_NOT_FOUND;
    }
    
    hw_component_t* component = &g_components[index];
    
    /* Şu anda gerçek bir değişim yapmıyoruz, sadece durumu güncelliyoruz */
    hw_log(2, "Bileşen durumu değiştiriliyor: %s -> %s", 
           component->info.name, enable ? "etkin" : "devre dışı");
    
    /* Değişikliği uyguladığımızı varsay */
    if (enable) {
        component->status.status = HW_STATUS_OK;
        hw_manager_load_driver(component_id, false);
    } else {
        component->status.status = HW_STATUS_DISABLED;
        hw_manager_unload_driver(component_id);
    }
    
    pthread_mutex_unlock(&g_component_mutex);
    
    return HW_ERROR_NONE;
}

/**
 * İzleme thread fonksiyonu
 */
static void* hw_monitor_thread(void* arg) {
    hw_monitor_t* monitor = (hw_monitor_t*)arg;
    
    while (monitor->running) {
        /* Bileşen durumunu güncelle */
        pthread_mutex_lock(&g_component_mutex);
        
        int index = hw_find_component_index(monitor->component_id);
        if (index >= 0) {
            hw_update_component_status(&g_components[index]);
            
            /* Callback fonksiyonunu çağır (gerçek bir uygulama için) */
            /* 
            if (monitor->callback != NULL) {
                monitor->callback(&g_components[index]);
            }
            */
        }
        
        pthread_mutex_unlock(&g_component_mutex);
        
        /* Bekle */
        usleep(monitor->interval_ms * 1000);
    }
    
    return NULL;
}

/**
 * Donanım durumunu izlemeyi başlatır
 */
int hw_manager_start_monitoring(uint32_t component_id, uint32_t interval_ms, void* callback) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    /* İzleme aralığı kontrolü */
    if (interval_ms < 100) {
        interval_ms = 100; /* Minimum 100ms */
    }
    
    pthread_mutex_lock(&g_monitor_mutex);
    
    /* Kapasite kontrolü */
    if (g_monitor_count >= g_monitor_capacity) {
        /* Kapasiteyi artır */
        uint32_t new_capacity = g_monitor_capacity * 2;
        hw_monitor_t* new_monitors = (hw_monitor_t*)realloc(g_monitors, 
                                                          new_capacity * sizeof(hw_monitor_t));
        if (new_monitors == NULL) {
            pthread_mutex_unlock(&g_monitor_mutex);
            return HW_ERROR_MEMORY;
        }
        
        g_monitors = new_monitors;
        g_monitor_capacity = new_capacity;
    }
    
    /* Bileşen kontrol et (sadece component_id != 0 ise) */
    if (component_id != 0) {
        pthread_mutex_lock(&g_component_mutex);
        int index = hw_find_component_index(component_id);
        if (index < 0) {
            pthread_mutex_unlock(&g_component_mutex);
            pthread_mutex_unlock(&g_monitor_mutex);
            return HW_ERROR_NOT_FOUND;
        }
        pthread_mutex_unlock(&g_component_mutex);
    }
    
    /* Yeni izleme */
    hw_monitor_t* monitor = &g_monitors[g_monitor_count];
    monitor->id = g_next_monitor_id++;
    monitor->component_id = component_id;
    monitor->interval_ms = interval_ms;
    monitor->callback = callback;
    monitor->running = true;
    
    /* Thread oluştur */
    if (pthread_create(&monitor->thread, NULL, hw_monitor_thread, monitor) != 0) {
        pthread_mutex_unlock(&g_monitor_mutex);
        return HW_ERROR_INTERNAL;
    }
    
    /* İzleme sayısını artır */
    g_monitor_count++;
    
    pthread_mutex_unlock(&g_monitor_mutex);
    
    hw_log(2, "İzleme başlatıldı: ID=%d, Bileşen=%d, Aralık=%dms", 
           monitor->id, component_id, interval_ms);
    
    return monitor->id;
}

/**
 * Donanım durumunu izlemeyi durdurur
 */
int hw_manager_stop_monitoring(uint32_t monitor_id) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_monitor_mutex);
    
    /* Tüm izlemeleri durdur */
    if (monitor_id == 0) {
        for (uint32_t i = 0; i < g_monitor_count; i++) {
            g_monitors[i].running = false;
            pthread_join(g_monitors[i].thread, NULL);
        }
        
        g_monitor_count = 0;
        pthread_mutex_unlock(&g_monitor_mutex);
        
        hw_log(2, "Tüm izlemeler durduruldu");
        return HW_ERROR_NONE;
    }
    
    /* Belirli bir izlemeyi durdur */
    for (uint32_t i = 0; i < g_monitor_count; i++) {
        if (g_monitors[i].id == monitor_id) {
            g_monitors[i].running = false;
            pthread_join(g_monitors[i].thread, NULL);
            
            /* Son izleme değilse, diziden sil */
            if (i < g_monitor_count - 1) {
                memmove(&g_monitors[i], &g_monitors[i + 1], 
                        (g_monitor_count - i - 1) * sizeof(hw_monitor_t));
            }
            
            g_monitor_count--;
            pthread_mutex_unlock(&g_monitor_mutex);
            
            hw_log(2, "İzleme durduruldu: ID=%d", monitor_id);
            return HW_ERROR_NONE;
        }
    }
    
    pthread_mutex_unlock(&g_monitor_mutex);
    
    return HW_ERROR_NOT_FOUND;
}

/**
 * Yapay zeka destekli optimizasyonu yapılandırır
 */
int hw_manager_configure_ai(bool enable, uint8_t optimization_level) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    /* Seviyeleri kontrol et */
    if (optimization_level > 3) {
        optimization_level = 3;
    }
    
    /* Yapılandırmayı güncelle */
    g_config.enable_ai_optimization = enable;
    
    hw_log(2, "Yapay zeka optimizasyonu %s, seviye=%d", 
           enable ? "etkinleştirildi" : "devre dışı bırakıldı", 
           optimization_level);
    
    /* Gerçek bir uygulama için burada yapay zeka servislerini başlatabilir veya durdurabilirsiniz */
    
    return HW_ERROR_NONE;
}

/**
 * Sistem donanımı durum raporunu oluşturur
 */
int hw_manager_generate_report(const char* report_file, uint8_t format) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    /* Çıktı dosyası veya stdout */
    FILE* out = stdout;
    if (report_file != NULL) {
        out = fopen(report_file, "w");
        if (out == NULL) {
            return HW_ERROR_IO;
        }
    }
    
    /* Başlık yaz */
    fprintf(out, "=== KALEM OS Donanım Raporu ===\n");
    fprintf(out, "Oluşturulma Zamanı: %s\n", ctime(&(time_t){time(NULL)}));
    fprintf(out, "Toplam Bileşen Sayısı: %d\n", g_component_count);
    fprintf(out, "Toplam Sürücü Sayısı: %d\n", g_status.driver_count);
    fprintf(out, "Sistem Sağlığı: %%%d\n\n", g_status.system_health);
    
    /* Bileşenleri listele */
    pthread_mutex_lock(&g_component_mutex);
    
    for (uint32_t i = 0; i < g_component_count; i++) {
        hw_component_t* component = &g_components[i];
        
        /* Temel bilgiler */
        fprintf(out, "--- Bileşen #%d: %s ---\n", 
               component->info.id, component->info.name);
        fprintf(out, "Tür: %d\n", component->info.type);
        fprintf(out, "Üretici: %s\n", component->info.vendor);
        fprintf(out, "Model: %s\n", component->info.model);
        
        /* Sürücü bilgileri */
        fprintf(out, "Sürücü: %s (v%s)\n", 
               component->driver.name, component->driver.version);
        fprintf(out, "Sürücü Durumu: %d\n", component->driver.status);
        
        /* Durum bilgileri */
        fprintf(out, "Durum: %d\n", component->status.status);
        fprintf(out, "Sıcaklık: %d°C\n", component->status.temperature);
        fprintf(out, "Kullanım: %%%d\n", component->status.utilization);
        fprintf(out, "Güç Tüketimi: %.2f W\n", component->status.power_usage / 1000.0f);
        
        /* Özel bilgiler */
        if (component->info.type == HW_TYPE_CPU && component->extra_data != NULL) {
            hw_cpu_info_t* cpu_info = (hw_cpu_info_t*)component->extra_data;
            fprintf(out, "CPU Çekirdek Sayısı: %d\n", cpu_info->core_count);
            fprintf(out, "CPU İş Parçacığı Sayısı: %d\n", cpu_info->thread_count);
            fprintf(out, "CPU Frekansı: %.2f GHz (Max: %.2f GHz)\n", 
                   cpu_info->base_freq / 1000.0f, cpu_info->max_freq / 1000.0f);
        }
        else if (component->info.type == HW_TYPE_GPU && component->extra_data != NULL) {
            hw_gpu_info_t* gpu_info = (hw_gpu_info_t*)component->extra_data;
            fprintf(out, "GPU Çekirdek Sayısı: %d\n", gpu_info->core_count);
            fprintf(out, "GPU Frekansı: %.2f GHz\n", gpu_info->core_freq / 1000.0f);
            fprintf(out, "GPU Bellek: %.2f GB\n", 
                   gpu_info->memory_size / (1024.0f * 1024.0f * 1024.0f));
        }
        else if (component->info.type == HW_TYPE_STORAGE && component->extra_data != NULL) {
            hw_storage_info_t* storage_info = (hw_storage_info_t*)component->extra_data;
            fprintf(out, "Depolama Türü: %d\n", storage_info->type);
            fprintf(out, "Toplam Boyut: %.2f GB\n", 
                   storage_info->total_size / (1024.0f * 1024.0f * 1024.0f));
            fprintf(out, "Boş Alan: %.2f GB (%.1f%%)\n", 
                   storage_info->free_size / (1024.0f * 1024.0f * 1024.0f),
                   (float)storage_info->free_size / storage_info->total_size * 100);
            fprintf(out, "Okuma/Yazma Hızı: %d/%d MB/s\n", 
                   storage_info->read_speed, storage_info->write_speed);
            fprintf(out, "Bağlama Noktası: %s\n", storage_info->mount_point);
        }
        else if (component->info.type == HW_TYPE_NETWORK && component->extra_data != NULL) {
            hw_network_info_t* network_info = (hw_network_info_t*)component->extra_data;
            fprintf(out, "MAC Adresi: %s\n", network_info->mac_address);
            fprintf(out, "IP Adresi: %s\n", network_info->ip_address);
            fprintf(out, "Ağ Maskesi: %s\n", network_info->netmask);
            fprintf(out, "Ağ Geçidi: %s\n", network_info->gateway);
            fprintf(out, "Bağlantı Hızı: %d Mbps\n", network_info->speed);
            fprintf(out, "Alınan/Gönderilen: %.2f/%.2f MB\n", 
                   network_info->rx_bytes / (1024.0f * 1024.0f),
                   network_info->tx_bytes / (1024.0f * 1024.0f));
        }
        
        fprintf(out, "\n");
    }
    
    pthread_mutex_unlock(&g_component_mutex);
    
    /* Dosyayı kapat */
    if (report_file != NULL) {
        fclose(out);
    }
    
    return HW_ERROR_NONE;
}

/**
 * Donanım bileşeni için özel işlemi çalıştırır
 */
int hw_manager_execute_command(uint32_t component_id, const char* command, 
                              const void* params, void* result) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    if (command == NULL) {
        return HW_ERROR_INVALID_ARG;
    }
    
    pthread_mutex_lock(&g_component_mutex);
    
    int index = hw_find_component_index(component_id);
    if (index < 0) {
        pthread_mutex_unlock(&g_component_mutex);
        return HW_ERROR_NOT_FOUND;
    }
    
    hw_component_t* component = &g_components[index];
    
    /* Komutu işle */
    hw_log(2, "Komut çalıştırılıyor: %s, Bileşen: %s", 
           command, component->info.name);
    
    /* Bu örnekte sadece birkaç komutu tanıyoruz */
    if (strcmp(command, "restart") == 0) {
        /* Sürücüyü yeniden başlat */
        hw_manager_unload_driver(component_id);
        hw_manager_load_driver(component_id, true);
    }
    else if (strcmp(command, "identify") == 0) {
        /* Bileşeni tanımla (örnekte sadece log) */
        hw_log(2, "Bileşen tanımlama: %s (ID=%d, Tür=%d)", 
               component->info.name, component->info.id, component->info.type);
    }
    else if (strcmp(command, "benchmark") == 0) {
        /* Benchmark çalıştır (simülasyon) */
        hw_log(2, "Benchmark çalıştırılıyor: %s", component->info.name);
        
        /* Örnek sonuçlar */
        if (result != NULL) {
            int* score = (int*)result;
            
            switch (component->info.type) {
                case HW_TYPE_CPU:
                    *score = 9500 + (rand() % 1000);
                    break;
                case HW_TYPE_GPU:
                    *score = 12500 + (rand() % 2000);
                    break;
                case HW_TYPE_STORAGE:
                    *score = 5500 + (rand() % 500);
                    break;
                default:
                    *score = 1000 + (rand() % 500);
                    break;
            }
        }
    }
    else {
        pthread_mutex_unlock(&g_component_mutex);
        return HW_ERROR_NOT_SUPPORTED;
    }
    
    pthread_mutex_unlock(&g_component_mutex);
    
    return HW_ERROR_NONE;
}

/**
 * Donanım yöneticisi günlük ayarlarını yapılandırır
 */
int hw_manager_configure_logging(uint32_t log_level, const char* log_file) {
    if (log_level > 3) {
        log_level = 3;
    }
    
    g_log_level = log_level;
    
    /* Mevcut günlük dosyasını kapat */
    if (g_log_file != NULL && g_log_file != stderr) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    /* Yeni günlük dosyasını aç */
    if (log_file != NULL) {
        g_log_file = fopen(log_file, "a");
        if (g_log_file == NULL) {
            g_log_file = stderr;
            return HW_ERROR_IO;
        }
    } else {
        g_log_file = stderr;
    }
    
    hw_log(2, "Günlük seviyesi değiştirildi: %d", log_level);
    
    return HW_ERROR_NONE;
}

/**
 * Donanım yöneticisi yapılandırmasını alır
 */
int hw_manager_get_config(hw_manager_config_t* config) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    if (config == NULL) {
        return HW_ERROR_INVALID_ARG;
    }
    
    memcpy(config, &g_config, sizeof(hw_manager_config_t));
    
    return HW_ERROR_NONE;
}

/**
 * Donanım yöneticisi yapılandırmasını ayarlar
 */
int hw_manager_set_config(const hw_manager_config_t* config) {
    if (!g_initialized) {
        return HW_ERROR_NOT_INITIALIZED;
    }
    
    if (config == NULL) {
        return HW_ERROR_INVALID_ARG;
    }
    
    /* Doğrudan kopyalamak yerine değerleri tek tek güncelle */
    g_config.enable_hotplug = config->enable_hotplug;
    g_config.enable_monitoring = config->enable_monitoring;
    g_config.scan_interval = config->scan_interval;
    g_config.monitor_interval = config->monitor_interval;
    g_config.enable_ai_optimization = config->enable_ai_optimization;
    g_config.auto_driver_update = config->auto_driver_update;
    g_config.security_checks = config->security_checks;
    g_config.log_level = config->log_level;
    strncpy(g_config.driver_repo_url, config->driver_repo_url, sizeof(g_config.driver_repo_url) - 1);
    
    /* Günlük seviyesini güncelle */
    g_log_level = g_config.log_level;
    
    hw_log(2, "Donanım yöneticisi yapılandırması güncellendi");
    
    return HW_ERROR_NONE;
} 