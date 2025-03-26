#ifndef PTHREAD_ADAPTER_H
#define PTHREAD_ADAPTER_H

/**
 * @file pthread_adapter.h
 * @brief Windows uyumluluğu için pthread API'si simülasyonu
 * 
 * Bu dosya, Windows platformunda POSIX thread API'sini simüle eden
 * basit bir adaptör sağlar. Windows thread API'lerini POSIX benzeri
 * bir arayüz ile kapsar.
 */

#ifdef _WIN32
#include <windows.h>
#include <process.h>

/* Temel pthread yapıları */
typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;

/* Mutex sabitleri */
#define PTHREAD_MUTEX_INITIALIZER {0}

/* Thread oluşturma */
static inline int pthread_create(pthread_t* thread, void* attr, void* (*start_routine)(void*), void* arg) {
    *thread = (HANDLE)_beginthreadex(NULL, 0, 
                                    (unsigned int(__stdcall*)(void*))start_routine, 
                                    arg, 0, NULL);
    return (*thread == NULL) ? -1 : 0;
}

/* Thread bekleme */
static inline int pthread_join(pthread_t thread, void** retval) {
    DWORD result = WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return (result == WAIT_OBJECT_0) ? 0 : -1;
}

/* Mutex başlatma */
static inline int pthread_mutex_init(pthread_mutex_t* mutex, void* attr) {
    InitializeCriticalSection(mutex);
    return 0;
}

/* Mutex kilitleme */
static inline int pthread_mutex_lock(pthread_mutex_t* mutex) {
    EnterCriticalSection(mutex);
    return 0;
}

/* Mutex açma */
static inline int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    LeaveCriticalSection(mutex);
    return 0;
}

/* Mutex temizleme */
static inline int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    DeleteCriticalSection(mutex);
    return 0;
}

/* Thread ID alma */
static inline pthread_t pthread_self(void) {
    return GetCurrentThread();
}

/* Thread sonlandırma */
static inline void pthread_exit(void* retval) {
    ExitThread((DWORD)(size_t)retval);
}

/* Sleep alternatifi */
struct timespec {
    long tv_sec;
    long tv_nsec;
};

static inline int nanosleep(const struct timespec* req, struct timespec* rem) {
    Sleep((DWORD)((req->tv_sec * 1000) + (req->tv_nsec / 1000000)));
    if (rem != NULL) {
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }
    return 0;
}

#else
/* Gerçek sistemlerde normal pthread.h'ı kullan */
#include <pthread.h>
#include <time.h>
#endif /* _WIN32 */

#endif /* PTHREAD_ADAPTER_H */ 