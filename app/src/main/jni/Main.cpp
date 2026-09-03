#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <atomic>
#include <android/log.h>

#define LOG_TAG "TestMenu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static std::atomic_bool g_enabled{false};

static void* menu_thread(void*) {
    LOGI("menu thread started");

    // Chỉ test lifecycle, chưa đụng tới game native function.
    while (true) {
        LOGI("toggle state = %s",
             g_enabled.load() ? "ON" : "OFF");

        sleep(2);
    }

    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_support_Menu_Init(
        JNIEnv* env,
        jobject thiz,
        jobject context,
        jobject title,
        jobject subTitle) {

    LOGI("Init called");

    static bool started = false;
    if (started)
        return;

    started = true;

    pthread_t thread;
    if (pthread_create(&thread, nullptr, menu_thread, nullptr) == 0) {
        pthread_detach(thread);
        LOGI("thread created");
    } else {
        LOGI("pthread_create failed");
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_support_Menu_Changes(
        JNIEnv* env,
        jclass clazz,
        jobject context,
        jint feature,
        jstring featureName,
        jint value,
        jlong lng,
        jboolean boolean,
        jstring str) {

    if (feature == 0) {
        g_enabled.store(boolean == JNI_TRUE);
        LOGI("Block Native Call toggle = %s",
             g_enabled.load() ? "ON" : "OFF");
    }
}
