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
    jmethodID jmid_timeinfo;
    jmethodID jmid_errormsg;
    jmethodID jmid_complete;
    jmethodID jmid_volumeDb;
    jmethodID jmid_encodepcmdata;
    jmethodID jmid_cut_pcmdata;
    jmethodID jmid_pcmsamplerate;
public:
    CallJava(_JavaVM *javaVm, JNIEnv *env, jobject *obj);

    ~CallJava();

    void onCallPrepared(int type);
    void onCallLoadStatus(int type,bool load);
    void onCallTimeInfo(int type,int currentTime,int totalTime);
    void onCallErrorMsg(int type, int code, char *msg);
    void onCallComplete(int type);
    void onCallVolumeDb(int type, int dbValue);
    void onCallEncodePCM2AAC(int type, int size, void *buffer);
    void onCallCutPcmData(int type, void *buffer, int size);
    void onCallPcmSampleRate(int type, int sampleRate, int channels);
};


#endif //WEMUSIC_CALLJAVA_H
