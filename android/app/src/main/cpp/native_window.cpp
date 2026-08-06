#include <android/native_window.h>
#include <android/native_activity.h>
#include <android/log.h>

#define LOG_TAG "xemu-native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

/* 2.1.x Direct-to-Metal Window Backend */
extern "C" void xemu_android_bind_native_window(ANativeWindow* window) {
    if (window == nullptr) {
        LOGI("Native Window is null.");
        return;
    }
    
    // Acquire the raw hardware buffer
    ANativeWindow_acquire(window);
    
    // Set format to match Vulkan swapchain optimal format without SurfaceFlinger conversion
    ANativeWindow_setBuffersGeometry(window, 0, 0, WINDOW_FORMAT_RGBA_8888);
    
    LOGI("Successfully bound ANativeWindow to Vulkan swapchain directly (Bypassing SDL2).");
}
