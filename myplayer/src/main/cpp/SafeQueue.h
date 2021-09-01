//
// Created by lz6600 on 2021/8/31.
//

#ifndef WEMUSIC_SAFEQUEUE_H
#define WEMUSIC_SAFEQUEUE_H

#include "queue"
#include "pthread.h"
#include "AndroidLog.h"
#include "PlayStatus.h"

extern "C"
{
#include "libavcodec/avcodec.h"
};


class SafeQueue
{
public:
    std::queue<AVPacket *> queuePacket;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;
    PlayStatus *playstatus = NULL;

public:

    SafeQueue(PlayStatus *playstatus);
    ~SafeQueue();

    int putAvpacket(AVPacket *packet);
    int getAvpacket(AVPacket *packet);

    int getQueueSize();

    void clearAvpacket();

};


#endif //WEMUSIC_SAFEQUEUE_H
