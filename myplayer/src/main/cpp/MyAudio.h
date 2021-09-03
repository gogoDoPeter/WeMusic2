//
// Created by lz6600 on 2021/8/31.
//

#ifndef WEMUSIC_MYAUDIO_H
#define WEMUSIC_MYAUDIO_H

#include "SafeQueue.h"
#include "CallJava.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
};

class MyAudio
{
public:
    int streamIndex = -1;
    AVCodecContext *avCodecContext = nullptr;
    AVCodecParameters *avCodecParameters = nullptr;
    SafeQueue *queue = nullptr;
    PlayStatus *playStatus = nullptr;

    pthread_t thread_play;
    AVPacket *avPacket = NULL;
    AVFrame *avFrame = NULL;
    int ret = 0;
    uint8_t *buffer = NULL;
    int data_size = 0;


    int sample_rate = 0;

    CallJava *callJava= nullptr;
    // 引擎接口
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine = NULL;

    //混音器
    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //pcm
    SLObjectItf pcmPlayerObject = NULL;
    SLPlayItf pcmPlayerPlay = NULL;

    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;


    int duration = 0;
    AVRational time_base;
    double clock;//总的播放时长
    double now_time;//当前Frame的时间
    double last_time;//上一次调用的时间

public:
    MyAudio(PlayStatus *playStatus, int sample_rate, CallJava *callJava_);
    ~MyAudio();

    void play();
    int resampleAudio();

    void initOpenELSL();
    SLuint32 getCurrentSampleRateForOpensles(int sample_rate);

    void pause();
    void resume();
    void release();
};


#endif //WEMUSIC_MYAUDIO_H
