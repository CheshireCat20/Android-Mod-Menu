#include <jni.h>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include <cstdint>

// Nhúng các header mặc định của LGL
#if __has_include("Includes/Logger.h")
    #include "Includes/Logger.h"
#elif __has_include("Logger.h")
    #include "Logger.h"
#else
    #define LOG_TAG "MCPE_MOD"
    #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
    #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif

#if __has_include("Includes/Utils.h")
    #include "Includes/Utils.h"
#elif __has_include("Utils.h")
    #include "Utils.h"
#else
    extern bool isLibraryLoaded(const char *libraryName);
    extern uintptr_t getAbsoluteAddress(const char *libraryName, uintptr_t relativeAddr);
#endif

#if __has_include("Includes/Dobby/dobby.h")
    #include "Includes/Dobby/dobby.h"
#elif __has_include("Dobby/dobby.h")
    #include "Dobby/dobby.h"
#elif __has_include("dobby.h")
    #include "dobby.h"
#else
    extern "C" int DobbyHook(void *function_address, void *replace_call, void **origin_call);
#endif

// ---------------------------------------------------------------------------
// CẤU HÌNH HOOK MCPE
// ---------------------------------------------------------------------------

const char *targetLib = "libminecraftpe.so";
bool isBlockNativeCall = false;
uintptr_t nativeCallOffset = 0xd3f05b4;

// Con trỏ lưu hàm gốc
void* (*orig_NativeCall)(void* arg1, void* arg2, void* arg3) = nullptr;

// Hàm Hook thế chỗ
void* hook_NativeCall(void* arg1, void* arg2, void* arg3) {
    if (isBlockNativeCall) {
        LOGI("Native call từ JS đã bị chặn!");
        return nullptr;
    }
    return orig_NativeCall(arg1, arg2, arg3);
}

// Luồng khởi tạo Hook (Setup.cpp sẽ tự gọi luồng này)
void *hack_thread(void *) {
    LOGI("Đang chờ thư viện %s...", targetLib);

    while (!isLibraryLoaded(targetLib)) {
        sleep(1);
    }

    LOGI("%s đã nạp xong! Đang thực hiện Hook...", targetLib);

    uintptr_t targetAddr = getAbsoluteAddress(targetLib, nativeCallOffset);

    if (targetAddr != 0) {
        DobbyHook((void*)targetAddr, (void*)hook_NativeCall, (void**)&orig_NativeCall);
        LOGI("Đã Hook thành công tại địa chỉ: 0x%lx", targetAddr);
    } else {
        LOGE("Lỗi: Không tìm thấy địa chỉ thư viện!");
    }

    return nullptr;
}

// ---------------------------------------------------------------------------
// ĐĂNG KÝ HÀM CHO LGL MOD MENU (SETUP.CPP SẼ GỌI CÁC HÀM NÀY)
// ---------------------------------------------------------------------------
extern "C" {

// Hàm trả về danh sách tính năng trên Menu
jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    const char *features[] = {
        "1_Toggle_Chặn Native Call JS",
    };

    int Total_Features = sizeof(features) / sizeof(features[0]);
    jobjectArray ret = env->NewObjectArray(Total_Features, env->FindClass("java/lang/String"), env->NewStringUTF(""));

    for (int i = 0; i < Total_Features; i++) {
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));
    }
    return ret;
}

// Hàm nhận phản hồi khi người dùng thao tác trên Menu
void Changes(JNIEnv *env, jclass clazz, jobject context, int feature, jstring featureName, int value, long lng, jboolean boolean, jstring str) {
    switch (feature) {
        case 0:
            isBlockNativeCall = boolean;
            if (isBlockNativeCall) {
                LOGI("Đã BẬT chặn Native Call");
            } else {
                LOGI("Đã TẮT chặn Native Call");
            }
            break;
    }
}

} // extern "C"
