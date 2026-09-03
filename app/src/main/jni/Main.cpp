#include <list>
#include <vector>
#include <cstring>
#include <pthread.h>
#include <thread>
#include <string>
#include <jni.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <dlfcn.h>
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.hpp"
#include "Menu/Menu.hpp"
#include "Menu/Jni.hpp"
#include "Includes/Macros.h"
#include "dobby.h"

// Đổi tên thư viện mục tiêu sang thư viện của Minecraft PE
#define targetLibName OBFUSCATE("libminecraftpe.so")

// --- KHAI BÁO MENU ---
jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    jobjectArray ret;

    // Danh sách các nút trong Menu
    const char *features[] = {
            OBFUSCATE("Toggle_Block Native Call JS (0xd3f05b4)") // Tính năng số 0
    };

    int Total_Feature = (sizeof features / sizeof features[0]);
    ret = (jobjectArray)
            env->NewObjectArray(Total_Feature, env->FindClass(OBFUSCATE("java/lang/String")),
                                env->NewStringUTF(""));

    for (int i = 0; i < Total_Feature; i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));

    return (ret);
}

// --- XỬ LÝ SỰ KIỆN BẬT/TẮT ---
void Changes(JNIEnv *env, jclass clazz, jobject obj, jint featNum, jstring featName, jint value, jlong Lvalue, jboolean boolean, jstring text) {

    switch (featNum) {
        case 0:
            // Tính năng 0: Khi gạt công tắc, ghi đè lệnh RET (C0 03 5F D6) vào offset 0xd3f05b4
            // LGL Team Mod Menu có sẵn Macro PATCH_SWITCH tự động bật/tắt hex rất tiện lợi
            PATCH_SWITCH(targetLibName, "0xd3f05b4", "C0 03 5F D6", boolean);
            break;

        default:
            break;
    }
}

// --- LUỒNG CHỜ GAME LOAD ---
void hack_thread() {
    // Vòng lặp chờ cho đến khi libminecraftpe.so được nạp vào bộ nhớ
    while (!isLibraryLoaded(targetLibName)) {
        sleep(1); 
    }
    
    LOGI(OBFUSCATE("MCPE Mod: Da tim thay libminecraftpe.so!"));
}

// Khởi tạo luồng chạy ngầm khi file .so được inject
__attribute__((constructor))
void lib_main() {
    std::thread(hack_thread).detach();
}
