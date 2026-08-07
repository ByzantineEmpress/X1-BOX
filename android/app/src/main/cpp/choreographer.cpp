#include <android/choreographer.h>
#include <android/log.h>
#include <mutex>
#include <condition_variable>
#include <chrono>

#define LOG_TAG "xemu-choreographer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static std::mutex vsync_mutex;
static std::condition_variable vsync_cv;
static bool vsync_ready = false;

/* Direct-to-Metal Hardware Frame Pacing Callback */
static void xemu_frame_callback(long frameTimeNanos, void* data) {
    // 1. Lock the mutex and signal the Vulkan rendering thread
    {
        std::lock_guard<std::mutex> lock(vsync_mutex);
        vsync_ready = true;
    }
    vsync_cv.notify_one();
    
    // 2. Re-register for the next hardware VSync
    AChoreographer_postFrameCallback(AChoreographer_getInstance(), xemu_frame_callback, nullptr);
}

// Called by Vulkan rendering / swapchain loop right before frame submission
extern "C" void xemu_wait_for_vsync(void) {
    std::unique_lock<std::mutex> lock(vsync_mutex);
    
    // Wait for up to 33ms. If the hardware VSync fails to signal, we drop the lock and render anyway.
    bool signaled = vsync_cv.wait_for(lock, std::chrono::milliseconds(33), []{ return vsync_ready; });
    
    if (!signaled) {
        LOGI("VSync timeout (33ms)! Force-pushing frame to prevent deadlock.");
    }
    
    vsync_ready = false; // Reset for the next frame
}

extern "C" void xemu_android_init_choreographer(void) {
    AChoreographer* choreographer = AChoreographer_getInstance();
    if (choreographer) {
        AChoreographer_postFrameCallback(choreographer, xemu_frame_callback, nullptr);
        LOGI("Hardware VSync Choreographer initialized perfectly.");
    } else {
        LOGI("Failed to acquire AChoreographer instance.");
    }
}
