#include <jni.h>

jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    jclass stringClass = env->FindClass("java/lang/String");

    jobjectArray features =
        env->NewObjectArray(1, stringClass, nullptr);

    env->SetObjectArrayElement(
        features,
        0,
        env->NewStringUTF("Toggle Test")
    );

    return features;
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

    if (feature == 0) {
        // Chỉ test menu trước
        // Không hook/can thiệp Minecraft ở đây.
    }
}
