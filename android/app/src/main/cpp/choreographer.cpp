#include <android/choreographer.h>
#include <android/log.h>

#define LOG_TAG "xemu-choreographer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

/* 2.1.x Direct-to-Metal Frame Pacing */
static void xemu_frame_callback(long frameTimeNanos, void* data) {
    // This callback is fired by the Android hardware composer exactly when the VSync interval occurs
    // We signal the GPU push-buffer thread to submit the frame to the swapchain now
    
    // Trigger Vulkan QueueSubmit...
    
    // Re-register for the next VSync
    AChoreographer_postFrameCallback(AChoreographer_getInstance(), xemu_frame_callback, nullptr);
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
