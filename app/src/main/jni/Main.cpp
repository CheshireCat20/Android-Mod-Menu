#include <jni.h>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include "Includes/Utils.h"
#include "Includes/Dobby/dobby.h"

#define LOG_TAG "Mod_Menu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ========================================================================
// 1. CẤU HÌNH DOBBY HOOK & HACK THREAD
// ========================================================================
const char *targetLib = "libminecraftpe.so";
bool isBlockNativeCall = false;
uintptr_t nativeCallOffset = 0xd3f05b4; // Đảm bảo offset này chính xác với phiên bản MCPE

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

    uintptr_t libBase = getAbsoluteAddress(targetLib, 0);
    if (libBase != 0) {
        uintptr_t targetAddress = getAbsoluteAddress(targetLib, nativeCallOffset);
        DobbyHook((void*)targetAddress, (void*)hook_NativeCall, (void**)&orig_NativeCall);
        LOGI("Hook Dobby thành công tại: 0x%lx", (unsigned long)targetAddress);
    } else {
        LOGE("Không tìm thấy base address của %s", targetLib);
    }
    return nullptr;
}

// ========================================================================
// 2. CÁC HÀM CUNG CẤP CHO SETUP.CPP (Liên kết Menu)
// ========================================================================
jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray features = env->NewObjectArray(1, stringClass, env->NewStringUTF(""));
    // Khai báo công tắc bật/tắt trong Menu
    env->SetObjectArrayElement(features, 0, env->NewStringUTF("Toggle_Chặn gọi Native"));
    return features;
}

void Changes(JNIEnv *env, jclass clazz, jobject context, int feature, jstring featureName, int value, long lng, jboolean boolean, jstring str) {
    if (feature == 0) {
        isBlockNativeCall = boolean;
        if (boolean) LOGI("Đã BẬT chặn NativeCall");
        else LOGI("Đã TẮT chặn NativeCall");
    }
}

// ========================================================================
// 3. CÁC HÀM JNI XUẤT RA CHO MENU.JAVA (BẮT BUỘC PHẢI CÓ ĐỂ TRÁNH CRASH)
// ========================================================================
extern "C" {

JNIEXPORT void JNICALL Java_com_android_support_Menu_Init(JNIEnv *env, jobject thiz, jobject context, jobject title, jobject subTitle) {
    // Khởi tạo luồng ngầm chạy hack_thread khi Menu mở lên
    pthread_t ptid;
    pthread_create(&ptid, nullptr, hack_thread, nullptr);
}

JNIEXPORT jstring JNICALL Java_com_android_support_Menu_Icon(JNIEnv *env, jobject thiz) {
    // Trả về chuỗi rỗng (không sử dụng custom icon Base64 để tránh lỗi memory)
    return env->NewStringUTF(""); 
}

JNIEXPORT jstring JNICALL Java_com_android_support_Menu_IconWebViewData(JNIEnv *env, jobject thiz) {
    return nullptr;
}

JNIEXPORT jobjectArray JNICALL Java_com_android_support_Menu_SettingsList(JNIEnv *env, jobject thiz) {
    jclass stringClass = env->FindClass("java/lang/String");
    return env->NewObjectArray(0, stringClass, nullptr);
}

JNIEXPORT jboolean JNICALL Java_com_android_support_Menu_IsGameLibLoaded(JNIEnv *env, jobject thiz) {
    return isLibraryLoaded(targetLib) ? JNI_TRUE : JNI_FALSE;
}

} // end extern "C"
