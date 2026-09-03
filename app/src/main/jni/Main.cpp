#include <jni.h>
#include <android/log.h>

#define LOG_TAG "ModMenuTest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    LOGI("GetFeatureList");

    jclass cls = env->FindClass("java/lang/String");
    if (!cls) {
        LOGI("FindClass failed");
        return nullptr;
    }

    jobjectArray result = env->NewObjectArray(1, cls, nullptr);
    if (!result) {
        LOGI("NewObjectArray failed");
        return nullptr;
    }

    jstring item = env->NewStringUTF("Test Toggle");
    env->SetObjectArrayElement(result, 0, item);
    env->DeleteLocalRef(item);

    return result;
}

void Changes(
        JNIEnv *env,
        jclass clazz,
        jobject context,
        int feature,
        jstring featureName,
        int value,
        jlong lng,
        jboolean boolean,
        jstring str) {

    LOGI("Changes: feature=%d value=%d boolean=%d",
         feature, value, boolean);
}
