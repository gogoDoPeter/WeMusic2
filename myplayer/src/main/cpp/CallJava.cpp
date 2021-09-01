//
// Created by lz6600 on 2021/8/31.
//

#include "CallJava.h"

CallJava::CallJava(_JavaVM *javaVm, JNIEnv *env, jobject *obj) {
    this->javaVm = javaVm;
    this->jniEnv = env;
    this->jobj = *obj;
    this->jobj = env->NewGlobalRef(jobj);

    jclass jclaz = jniEnv->GetObjectClass(jobj);
    if (!jclaz) {
        LOGE("get jclass fail");
        return;
    }
    jmid_prepared = env->GetMethodID(jclaz, "onCallPrepared", "()V");
    jmid_load = env->GetMethodID(jclaz, "onCallLoadStatus", "(Z)V");
}

CallJava::~CallJava() {

}

void CallJava::onCallPrepared(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_prepared);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("Get child thread JNIEnv fail");
            return;
        }
        env->CallVoidMethod(jobj, jmid_prepared);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallLoadStatus(int type, bool load) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_load);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("Get child thread JNIEnv fail");
            return;
        }
        env->CallVoidMethod(jobj, jmid_load, load);
        javaVm->DetachCurrentThread();
    }
}
