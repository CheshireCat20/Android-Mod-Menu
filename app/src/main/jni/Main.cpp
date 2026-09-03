#include <jni.h>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include <cstdint>

// Định nghĩa Log Android
#define LOG_TAG "MCPE_MOD"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Tự động kiểm tra file header Dobby
#if __has_include("dobby.h")
    #include "dobby.h"
#elif __has_include("Dobby/dobby.h")
    #include "Dobby/dobby.h"
#elif __has_include("Includes/Dobby/dobby.h")
    #include "Includes/Dobby/dobby.h"
#else
    // Khai báo dự phòng nếu không tìm thấy header Dobby
    extern "C" int DobbyHook(void *function_address, void *replace_call, void **origin_call);
#endif

// ---------------------------------------------------------------------------
// TỰ ĐỊNH NGHĨA HÀM XỬ LÝ BỘ NHỚ (KHÔNG CẦN UTILS.H)
// ---------------------------------------------------------------------------

// Kiểm tra thư viện đã được load vào RAM chưa
bool isLibraryLoaded(const char *libraryName) {
    FILE *fp = fopen("/proc/self/maps", "rt");
    if (!fp) return false;

    bool loaded = false;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, libraryName)) {
            loaded = true;
            break;
        }
    }
    fclose(fp);
    return loaded;
}

// Lấy địa chỉ tuyệt đối = Base Address + Offset
uintptr_t getAbsoluteAddress(const char *libraryName, uintptr_t relativeAddr) {
    FILE *fp = fopen("/proc/self/maps", "rt");
    if (!fp) return 0;

    uintptr_t baseAddr = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, libraryName)) {
            baseAddr = (uintptr_t)strtoul(line, nullptr, 16);
            break;
        }
    }
    fclose(fp);

    if (baseAddr == 0) return 0;
    return baseAddr + relativeAddr;
}

// ---------------------------------------------------------------------------
// CẤU HÌNH HOOK MCPE
// ---------------------------------------------------------------------------

const char *targetLib = "libminecraftpe.so";
bool isBlockNativeCall = false;
uintptr_t nativeCallOffset = 0xd3f05b4;

// Con trỏ lưu hàm gốc
void* (*orig_NativeCall)(void* arg1, void* arg2, void* arg3) = nullptr;

// Hàm Hook
void* hook_NativeCall(void* arg1, void* arg2, void* arg3) {
    if (isBlockNativeCall) {
        LOGI("Native call từ JS đã bị chặn!");
        return nullptr;
    }
    return orig_NativeCall(arg1, arg2, arg3);
}

// Luồng khởi tạo Hook
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
// CHỨC NĂNG GIAO DIỆN LGL MOD MENU
// ---------------------------------------------------------------------------
extern "C" {

JNIEXPORT jobjectArray JNICALL
Java_com_android_support_Menu_getFeatures(JNIEnv *env, jobject thiz) {
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

JNIEXPORT void JNICALL
Java_com_android_support_Preferences_Changes(JNIEnv *env, jclass clazz, jobject obj, jint feature, jint value, jboolean boolean, jstring str) {
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

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    pthread_t ptid;
    pthread_create(&ptid, nullptr, hack_thread, nullptr);

    return JNI_VERSION_1_6;
}
