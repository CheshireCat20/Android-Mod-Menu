#include <jni.h>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define LOG_TAG "Mod_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Khai báo DobbyHook
extern "C" {
    int DobbyHook(void *function_address, void *replace_call, void **origin_call);
}

// ========================================================================
// HÀM TIM ĐỊA CHỈ BỘ NHỚ (THÊM STATIC ĐỂ TRÁNH XUNG ĐỘT LINKER)
// ========================================================================
static uintptr_t getAbsoluteAddress(const char* libraryName, uintptr_t relativeAddr) {
    FILE *fp = fopen("/proc/self/maps", "rt");
    if (!fp) return 0;

    uintptr_t baseAddr = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, libraryName)) {
            baseAddr = (uintptr_t)strtoull(line, NULL, 16);
            break;
        }
    }
    fclose(fp);
    if (baseAddr == 0) return 0;
    return baseAddr + relativeAddr;
}

static bool isLibraryLoaded(const char* libraryName) {
    return getAbsoluteAddress(libraryName, 0) != 0;
}

// ========================================================================
// CẤU HÌNH HOOK & HACK THREAD
// ========================================================================
const char *targetLib = "libminecraftpe.so";
bool isBlockNativeCall = false;
uintptr_t nativeCallOffset = 0xd3f05b4; // Offset MCPE

void* (*orig_NativeCall)(void* arg1, void* arg2, void* arg3) = nullptr;

void* hook_NativeCall(void* arg1, void* arg2, void* arg3) {
    if (isBlockNativeCall) {
        return nullptr; // Chặn gọi hàm
    }
    if (orig_NativeCall) {
        return orig_NativeCall(arg1, arg2, arg3);
    }
    return nullptr;
}

void *hack_thread(void *) {
    LOGI("Đang chờ thư viện %s...", targetLib);
    while (!isLibraryLoaded(targetLib)) {
        sleep(1);
    }
    LOGI("Thư viện %s đã load! Tiến hành hook...", targetLib);

    uintptr_t targetAddress = getAbsoluteAddress(targetLib, nativeCallOffset);
    if (targetAddress != 0) {
        DobbyHook((void*)targetAddress, (void*)hook_NativeCall, (void**)&orig_NativeCall);
        LOGI("Hook Dobby thành công tại: 0x%lx", (unsigned long)targetAddress);
    } else {
        LOGE("Không tìm thấy địa chỉ của %s", targetLib);
    }
    return nullptr;
}

// ========================================================================
// CÁC HÀM JNI XUẤT RA CHO JAVA
// ========================================================================
extern "C" {

JNIEXPORT void JNICALL Java_com_android_support_Menu_Init(JNIEnv *env, jobject thiz, jobject context, jobject title, jobject subTitle) {
    pthread_t ptid;
    pthread_create(&ptid, nullptr, hack_thread, nullptr);
}

JNIEXPORT jstring JNICALL Java_com_android_support_Menu_Icon(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF(""); 
}

JNIEXPORT jstring JNICALL Java_com_android_support_Menu_IconWebViewData(JNIEnv *env, jobject thiz) {
    return nullptr;
}

JNIEXPORT jobjectArray JNICALL Java_com_android_support_Menu_GetFeatureList(JNIEnv *env, jobject thiz) {
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray features = env->NewObjectArray(1, stringClass, env->NewStringUTF(""));
    env->SetObjectArrayElement(features, 0, env->NewStringUTF("Toggle_Chặn gọi Native"));
    return features;
}

JNIEXPORT jobjectArray JNICALL Java_com_android_support_Menu_SettingsList(JNIEnv *env, jobject thiz) {
    jclass stringClass = env->FindClass("java/lang/String");
    return env->NewObjectArray(0, stringClass, nullptr);
}

JNIEXPORT jboolean JNICALL Java_com_android_support_Menu_IsGameLibLoaded(JNIEnv *env, jobject thiz) {
    return isLibraryLoaded(targetLib) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_com_android_support_Preferences_Changes(JNIEnv *env, jclass clazz, jobject context, jint feature, jstring featureName, jint value, jlong lng, jboolean boolean, jstring str) {
    if (feature == 0) {
        isBlockNativeCall = boolean;
        if (boolean) LOGI("Đã BẬT chặn NativeCall");
        else LOGI("Đã TẮT chặn NativeCall");
    }
}

} // end extern "C"
