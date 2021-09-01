#include <jni.h>
#include <string>
#include "AndroidLog.h"
#include "CallJava.h"
#include "FFmpeg.h"

_JavaVM *javaVm = nullptr;
CallJava *callJava = nullptr;
FFmpeg *fFmpeg = nullptr;
PlayStatus *playStatus = nullptr;

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    jint result = -1;

    javaVm = vm;
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK)
    {
        LOGE("GetEnv fail");
        return result;
    }
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_native_1start(JNIEnv *env, jobject thiz)
{
    LOGD("native_start start +");
    if(fFmpeg != nullptr){
        fFmpeg->start();
    }
    LOGD("native_start start -");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_native_1prepared(JNIEnv *env, jobject thiz, jstring source_)
{
    LOGD("prepared +, source:%s", source_);
    const char *source = env->GetStringUTFChars(source_, 0);

    if (fFmpeg == nullptr)
    {
        if (callJava == nullptr)
        {
            callJava = new CallJava(javaVm, env, &thiz);
        }
        playStatus = new PlayStatus();
        fFmpeg = new FFmpeg(source, callJava, playStatus);
        LOGD("fFmpeg->prepared");
        fFmpeg->prepared();
    }

    LOGD("prepared -");
}