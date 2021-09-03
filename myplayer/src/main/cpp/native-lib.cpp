#include <jni.h>
#include <string>
#include "AndroidLog.h"
#include "CallJava.h"
#include "FFmpeg.h"

_JavaVM *javaVm = nullptr;
CallJava *callJava = nullptr;
FFmpeg *fFmpeg = nullptr;
PlayStatus *playStatus = nullptr;
bool nexit = true;
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
    fFmpeg->start();
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
Java_com_peter_myplayer_player_WeAudioPlayer_nativeStop(JNIEnv *env, jobject thiz) {
    if (!nexit) {
        return;
    }

    nexit = false;

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

    nexit = true;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_peter_myplayer_player_WeAudioPlayer_NativeSeek(JNIEnv *env, jobject thiz, jint seconds) {
    if (fFmpeg != nullptr) {
        fFmpeg->seek(seconds);
    }
}