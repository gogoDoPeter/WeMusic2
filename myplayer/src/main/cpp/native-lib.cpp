#include <jni.h>
#include <string>
#include "AndroidLog.h"
#include "CallJava.h"
#include "FFmpeg.h"

_JavaVM *javaVm = nullptr;
CallJava *callJava = nullptr;
FFmpeg *fFmpeg = nullptr;
PlayStatus *playStatus = nullptr;
bool isStopDone = true;
pthread_t thread_start;

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    jint result = -1;

    javaVm = vm;
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv fail");
        return result;
    }
    return JNI_VERSION_1_6;
}

void *myStartCallback(void *data) {
    FFmpeg *fFmpeg = static_cast<FFmpeg *>(data);//WlFFmpeg *fFmpeg = (WlFFmpeg *) data;
    fFmpeg->startDecode();
    pthread_exit(&thread_start);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_native_1start(JNIEnv *env, jobject thiz) {
    LOGD("native_start start +");
    if (fFmpeg != nullptr) {
//        fFmpeg->start();
        pthread_create(&thread_start, NULL, myStartCallback, fFmpeg);

    }
    LOGD("native_start start -");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_native_1prepared(JNIEnv *env, jobject thiz,
                                                              jstring source_) {
    LOGD("prepared +");
    const char *source = env->GetStringUTFChars(source_, 0);
    LOGD("prepared, source:%s", source);
    if (fFmpeg == nullptr) {
        if (callJava == nullptr) {
            callJava = new CallJava(javaVm, env, &thiz);
        }
        playStatus = new PlayStatus();
        fFmpeg = new FFmpeg(source, callJava, playStatus);
        LOGD("fFmpeg->prepared");
        fFmpeg->prepared();
    }

    LOGD("prepared -");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_resumeNative(JNIEnv *env, jobject thiz) {
    if (fFmpeg != nullptr) {
        fFmpeg->resume();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_pauseNative(JNIEnv *env, jobject thiz) {
    if (fFmpeg != nullptr) {
        fFmpeg->pause();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_nativeStop(JNIEnv *env, jobject instance) {
    if (!isStopDone) {// TODO 解决退出过程中又点击stop，导致流程走多次异常
        return;
    }

    jclass jClaz = env->GetObjectClass(instance);
    jmethodID jmid_next = env->GetMethodID(jClaz, "onCallNext", "()V");

    isStopDone = false;

    //TODO 注意：内存释放原则，哪里申请的，哪里释放，其他地方使用的话，其他地方释放时置位NULL
    if (fFmpeg != nullptr) {
        fFmpeg->release();
        delete (fFmpeg); //Call ffmpeg destructor
        fFmpeg = nullptr;

        if (callJava != nullptr) {
            delete (callJava);//Call callJava destructor
            callJava = nullptr;
        }
        if (playStatus != nullptr) {
            delete (playStatus);//Call playStatus destructor
            playStatus = nullptr;
        }
    }

    isStopDone = true;
    env->CallVoidMethod(instance, jmid_next);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_NativeSeek(JNIEnv *env, jobject thiz, jint seconds) {
    if (fFmpeg != nullptr) {
        fFmpeg->seek(seconds);
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_nativeGetDuration(JNIEnv *env, jobject thiz) {
    if (fFmpeg != nullptr) {
        return fFmpeg->duration;
    }
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_nativeSetVolume(JNIEnv *env, jobject thiz,
                                                             jint percent) {
    if (fFmpeg != nullptr) {
        fFmpeg->setVolume(percent);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_nativeSetMute(JNIEnv *env, jobject thiz,
                                                           jint muteType) {

    if (fFmpeg != nullptr) {
        fFmpeg->setMute(muteType);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_nativeSetSpeed(JNIEnv *env, jobject thiz,
                                                            jdouble speed) {
    if (fFmpeg != nullptr) {
        fFmpeg->setSpeed(speed);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_nativeSetPitch(JNIEnv *env, jobject thiz,
                                                            jdouble pitch) {
    if (fFmpeg != nullptr) {
        fFmpeg->setPitch(pitch);
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_nativeGetSampleRate(JNIEnv *env, jobject thiz)
{
    if(fFmpeg!= nullptr){
        return fFmpeg->getAudioSampleRate();
    }
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_nativeStartStopRecord(JNIEnv *env, jobject thiz, jboolean b_start_record)
{
    if(fFmpeg!= nullptr){
        fFmpeg->startRecord(b_start_record);
    }
}