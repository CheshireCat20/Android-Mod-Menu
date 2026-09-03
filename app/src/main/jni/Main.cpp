#include <jni.h>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>

// Các file header mặc định của LGL Mod Menu 5.0
#include "Includes/Logger.h"
#include "Includes/Utils.h"
#include "Includes/Dobby/dobby.h"

// Tên thư viện mục tiêu
const char *targetLib = "libminecraftpe.so";

// Biến trạng thái liên kết với Switch trên Mod Menu
bool isBlockNativeCall = false;

// Offset bạn đã cung cấp
uintptr_t nativeCallOffset = 0xd3f05b4;

// Con trỏ lưu giữ địa chỉ của hàm gốc
void* (*orig_NativeCall)(void* arg1, void* arg2, void* arg3) = nullptr;

// ---------------------------------------------------------------------------
// HÀM HOOK (Hàm thế chỗ cho hàm gốc trong game)
// ---------------------------------------------------------------------------
void* hook_NativeCall(void* arg1, void* arg2, void* arg3) {
    // Nếu nút trên Mod Menu đang BẬT -> Chặn việc thực thi (Return sớm)
    if (isBlockNativeCall) {
        LOGI("Native call từ JS đã bị chặn!");
        return nullptr; // Trả về null để ngắt hàm
    }

    // Nếu nút đang TẮT -> Cho phép chạy hàm gốc bình thường
    return orig_NativeCall(arg1, arg2, arg3);
}

// ---------------------------------------------------------------------------
// LUỒNG CHẠY NGẦM (Khởi tạo Hook khi thư viện game đã load xong)
// ---------------------------------------------------------------------------
void *hack_thread(void *) {
    LOGI("Đang chờ thư viện %s...", targetLib);

    // Chờ libminecraftpe.so được nạp vào bộ nhớ RAM
    while (!isLibraryLoaded(targetLib)) {
        sleep(1);
    }

    LOGI("%s đã load thành công! Đang tiến hành Hook...", targetLib);

    // Lấy địa chỉ thực tế = Base Address + Offset
    uintptr_t targetAddr = getAbsoluteAddress(targetLib, nativeCallOffset);

    if (targetAddr != 0) {
        // Thực hiện Hook bằng Dobby
        DobbyHook((void*)targetAddr, (void*)hook_NativeCall, (void**)&orig_NativeCall);
        LOGI("Đã Hook thành công tại địa chỉ thực tế: 0x%lx", targetAddr);
    } else {
        LOGE("Lỗi: Không thể tính toán địa chỉ thực tế!");
    }

    return nullptr;
}

// ---------------------------------------------------------------------------
// KHU VỰC CẤU HÌNH GIAO DIỆN LGL MOD MENU (JNI EXPORTS)
// ---------------------------------------------------------------------------
extern "C" {

// 1. Danh sách tính năng hiển thị lên Menu
JNIEXPORT jobjectArray JNICALL
Java_com_android_support_Menu_getFeatures(JNIEnv *env, jobject thiz) {
    jobjectArray ret;
    
    // Tạo nút Bật/Tắt (Toggle) có ID là 0
    const char *features[] = {
        "1_Toggle_Chặn Native Call JS", 
    };

    int Total_Features = sizeof(features) / sizeof(features[0]);
    ret = (jobjectArray) env->NewObjectArray(Total_Features, env->FindClass("java/lang/String"), env->NewStringUTF(""));

    for (int i = 0; i < Total_Features; i++) {
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));
    }
    return ret;
}

// 2. Xử lý sự kiện khi bạn gạt nút Bật/Tắt trên Menu
JNIEXPORT void JNICALL
Java_com_android_support_Preferences_Changes(JNIEnv *env, jclass clazz, jobject obj, jint feature, jint value, jboolean boolean, jstring str) {
    switch (feature) {
        case 0: // Tương ứng với nút "1_Toggle_Chặn Native Call JS" ở vị trí đầu tiên
            isBlockNativeCall = boolean; // Trả về true khi Bật, false khi Tắt
            if (isBlockNativeCall) {
                LOGI("Đã BẬT chặn Native Call");
            } else {
                LOGI("Đã TẮT chặn Native Call");
            }
            break;
    }
}

} // extern "C"

// ---------------------------------------------------------------------------
// ĐIỂM KHỞI ĐẦU KHI LIBRARY ĐƯỢC LOAD
// ---------------------------------------------------------------------------
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    // Tạo luồng riêng biệt để thực hiện Hook, tránh làm đóng băng giao diện Game
    pthread_t ptid;
    pthread_create(&ptid, nullptr, hack_thread, nullptr);

    return JNI_VERSION_1_6;
}
