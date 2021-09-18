//
// Created by lz6600 on 2021/8/31.
//

#ifndef WEMUSIC_MYAUDIO_H
#define WEMUSIC_MYAUDIO_H

#include "SafeQueue.h"
#include "CallJava.h"
#include "SoundTouch.h"

using namespace soundtouch;

extern "C"
{
#include "libavcodec/avcodec.h"
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <libavutil/time.h>
};

class MyAudio {
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

    CallJava *callJava = nullptr;
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
    SLVolumeItf pcmVolumePlay = NULL;
    SLMuteSoloItf pcmMutePlay = NULL;

    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;


    int duration = 0;
    AVRational time_base;
    double clock;//总的播放时长
    double now_time;//当前Frame的时间
    double last_time;//上一次调用的时间

    int muteType = 2;//默认设置立体声

    //SoundTouch
    SoundTouch *soundTouch = nullptr;
    SAMPLETYPE *sampleBuffer = nullptr;
    bool isFinished = true;
    uint8_t *out_buffer = nullptr;
    int nbSize = 0;
    int num = 0;

    double speed = 1.0;
    double pitch = 1.0;

    bool isRecordPcm=false;

    bool isReadFrameFinished = true;

    int countAVPacket = 0;
    int countAVFrame = 0;

    bool isCut = false;
    double endTime = 0;
    bool isShowPcm = false;//表示是否回调裁剪的pcm数据给app

public:
    MyAudio(PlayStatus *playStatus, int sample_rate, CallJava *callJava_);

    ~MyAudio();

    void play();

    int resampleAudio(void **ppPcmbuf);

    void initOpenELSL();

    SLuint32 getCurrentSampleRateForOpensles(int sample_rate);

    void pause();

    void resume();

    void release();

    void setVolume(int percent);

    void setMute(int muteType);

    int getSoundTouchData();
    void setPitch(double pitch_);
    void setSpeed(double speed_);
    int getPcmDb(char * pcmData, size_t pcmSize);

    void startStopRecord(bool startRecord);
};


#endif //WEMUSIC_MYAUDIO_H
