//
// Created by lz6600 on 2021/8/31.
//

#ifndef WEMUSIC_CALLJAVA_H
#define WEMUSIC_CALLJAVA_H

#include "AndroidLog.h"
#include "jni.h"

#define MAIN_THREAD 0
#define CHILD_THREAD 1

class CallJava
{
public:
    _JavaVM *javaVm = nullptr;
    JNIEnv *jniEnv=nullptr;
    jobject jobj;
    jmethodID jmid_prepared;
    jmethodID jmid_load;

public:
    CallJava(_JavaVM *javaVm, JNIEnv *env, jobject *obj);

    ~CallJava();

    void onCallPrepared(int type);
    void onCallLoadStatus(int type,bool load);
};


#endif //WEMUSIC_CALLJAVA_H
