//
// Created by lz6600 on 2021/8/31.
//

#include "CallJava.h"

CallJava::CallJava(_JavaVM *javaVm, JNIEnv *env, jobject *obj)
{
    this->javaVm = javaVm;
    this->jniEnv = env;
    this->jobj = *obj;
    this->jobj = env->NewGlobalRef(jobj);

    jclass jclaz = jniEnv->GetObjectClass(jobj);
    if (!jclaz)
    {
        LOGE("get jclass fail");
        return;
    }
    jmid_prepared = env->GetMethodID(jclaz, "onCallPrepared", "()V");
    jmid_load = env->GetMethodID(jclaz, "onCallLoadStatus", "(Z)V");
    jmid_timeinfo = env->GetMethodID(jclaz, "onCallTimeInfo", "(II)V");
    jmid_errormsg = env->GetMethodID(jclaz, "onCallError", "(ILjava/lang/String;)V");
    jmid_complete = env->GetMethodID(jclaz, "onCallComplete", "()V");
    jmid_volumeDb = env->GetMethodID(jclaz, "onCallVolumeDB", "(I)V");
    jmid_encodepcmdata = env->GetMethodID(jclaz, "onCallEncodePcmToAAC", "(I[B)V");
    jmid_cut_pcmdata = env->GetMethodID(jclaz, "onCallCutPcmData", "([BI)V");
    jmid_pcmsamplerate= env->GetMethodID(jclaz,"onCallPcmRatesample","(II)V");
}

CallJava::~CallJava()
{

}

void CallJava::onCallPrepared(int type)
{
    if (type == MAIN_THREAD)
    {
        jniEnv->CallVoidMethod(jobj, jmid_prepared);
    }
    else if (type == CHILD_THREAD)
    {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK)
        {
            LOGE("Get child thread JNIEnv fail");
            return;
        }
        env->CallVoidMethod(jobj, jmid_prepared);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallLoadStatus(int type, bool load)
{
    if (type == MAIN_THREAD)
    {
        jniEnv->CallVoidMethod(jobj, jmid_load);
    }
    else if (type == CHILD_THREAD)
    {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK)
        {
            LOGE("Get child thread JNIEnv fail");
            return;
        }
        env->CallVoidMethod(jobj, jmid_load, load);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallTimeInfo(int type, int currentTime, int totalTime)
{
    if (type == MAIN_THREAD)
    {
        jniEnv->CallVoidMethod(jobj, jmid_timeinfo, currentTime, totalTime);
    }
    else if (type == CHILD_THREAD)
    {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK)
        {
            LOGE("Get child thread JNIEnv fail");
            return;
        }
        env->CallVoidMethod(jobj, jmid_timeinfo, currentTime, totalTime);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallErrorMsg(int type, int code, char *msg)
{
    if (type == MAIN_THREAD)
    {
        jstring jmsg = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jobj, jmid_errormsg, code, jmsg);
        jniEnv->DeleteLocalRef(jmsg);
    }
    else if (type == CHILD_THREAD)
    {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK)
        {
            LOGE("Get child thread JNIEnv fail");
            return;
        }
        jstring jmsg = jniEnv->NewStringUTF(msg);
        env->CallVoidMethod(jobj, jmid_errormsg, code, jmsg);
        jniEnv->DeleteLocalRef(jmsg);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallComplete(int type)
{
    if (type == MAIN_THREAD)
    {
        jniEnv->CallVoidMethod(jobj, jmid_complete);
    }
    else if (type == CHILD_THREAD)
    {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK)
        {
            LOGE("Get child thread JNIEnv fail");
            return;
        }
        env->CallVoidMethod(jobj, jmid_complete);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallVolumeDb(int type, int dbValue)
{
    if (type == MAIN_THREAD)
    {
        jniEnv->CallVoidMethod(jobj, jmid_volumeDb, dbValue);
    }
    else if (type == CHILD_THREAD)
    {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK)
        {
            LOGE("Get child thread JNIEnv fail");
            return;
        }
        env->CallVoidMethod(jobj, jmid_volumeDb, dbValue);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallEncodePCM2AAC(int type, int size, void *buffer)
{
    if (type == MAIN_THREAD)
    {
        jbyteArray jBuffer = jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(jBuffer, 0, size, static_cast<const jbyte *>(buffer));
        jniEnv->CallVoidMethod(jobj, jmid_encodepcmdata, size, jBuffer);
        jniEnv->DeleteLocalRef(jBuffer);
    }
    else if (type == CHILD_THREAD)
    {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK)
        {
            LOGE("Get child thread JNIEnv fail");
            return;
        }
        jbyteArray jBuffer = env->NewByteArray(size);
        env->SetByteArrayRegion(jBuffer, 0, size, static_cast<const jbyte *>(buffer));
        env->CallVoidMethod(jobj, jmid_encodepcmdata, size, jBuffer);
        env->DeleteLocalRef(jBuffer);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallCutPcmData(int type, void *buffer, int size)
{
    if (type == MAIN_THREAD)
    {
        jbyteArray jBuffer = jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(jBuffer, 0, size, static_cast<const jbyte *>(buffer));
        jniEnv->CallVoidMethod(jobj, jmid_cut_pcmdata, jBuffer, size);
        jniEnv->DeleteLocalRef(jBuffer);
    }
    else if (type == CHILD_THREAD)
    {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK)
        {
            LOGE("Get child thread JNIEnv fail");
            return;
        }
        jbyteArray jBuffer = env->NewByteArray(size);
        env->SetByteArrayRegion(jBuffer, 0, size, static_cast<const jbyte *>(buffer));
        env->CallVoidMethod(jobj, jmid_cut_pcmdata, jBuffer, size);
        env->DeleteLocalRef(jBuffer);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallPcmSampleRate(int type, int sampleRate, int channels)
{
    if (type == MAIN_THREAD)
    {
        jniEnv->CallVoidMethod(jobj, jmid_pcmsamplerate, sampleRate, channels);
    }
    else if (type == CHILD_THREAD)
    {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK)
        {
            LOGE("Get child thread JNIEnv fail");
            return;
        }
        env->CallVoidMethod(jobj, jmid_pcmsamplerate, sampleRate, channels);
        javaVm->DetachCurrentThread();
    }
}
