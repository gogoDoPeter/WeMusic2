//
// Created by lz6600 on 2021/8/31.
//

#ifndef WEMUSIC_FFMPEG_H
#define WEMUSIC_FFMPEG_H

#include "pthread.h"
#include "AndroidLog.h"
#include "MyAudio.h"
#include "PlayStatus.h"
#include "CallJava.h"

extern "C"
{
#include "libavformat/avformat.h"
#include <libavutil/time.h>
};

class FFmpeg
{
public:
    const char *source = nullptr;
    pthread_t decodeThread;
    AVFormatContext *pFormatCtx = nullptr;
    MyAudio *audio = nullptr;
    PlayStatus *playStatus = nullptr;
    CallJava *callJava = nullptr;

    pthread_mutex_t init_mutex;
    bool exitFfmpeg = false;

    char errMsg[40];
public:
    FFmpeg(const char *source, CallJava *callJava_, PlayStatus *playStatus_);

    ~FFmpeg();

    void prepared();

    void decodeFFmpegThread();

    void start();
    void pause();
    void resume();

    void release();
};


#endif //WEMUSIC_FFMPEG_H
