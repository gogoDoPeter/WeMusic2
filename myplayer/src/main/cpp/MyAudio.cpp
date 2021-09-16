//
// Created by lz6600 on 2021/8/31.
//

#include "MyAudio.h"

MyAudio::MyAudio(PlayStatus *playStatus, int sample_rate, CallJava *callJava_) {
    this->playStatus = playStatus;
    queue = new SafeQueue(playStatus);
    //buffer size = 采样率 * 声道数 *位数大小
    buffer = (uint8_t *) av_malloc(sample_rate * 2 * 2);
    this->sample_rate = sample_rate;

    this->callJava = callJava_;

    sampleBuffer = static_cast<SAMPLETYPE *>(malloc(sample_rate * 2 * 2));//TODO need release
    soundTouch = new SoundTouch();
    soundTouch->setSampleRate(sample_rate);
    soundTouch->setChannels(2);
    soundTouch->setPitch(1.5);//tone
    soundTouch->setTempo(2.0);//speed
}


MyAudio::~MyAudio() {

}

void *decodePlay(void *data) {
    LOGE("MyAudio::decodePlayThread +");
    MyAudio *myAudio = (MyAudio *) data;

    myAudio->initOpenELSL();

    LOGE("MyAudio::decodePlayThread -");
    pthread_exit(&myAudio->thread_play);
}


void MyAudio::play() {
    LOGE("MyAudio::play +");
    pthread_create(&thread_play, NULL, decodePlay, this);
    LOGE("MyAudio::play -");
}

//FILE *outFile = fopen("/mnt/sdcard/mymusic.pcm", "wb");

int MyAudio::resampleAudio(void **ppPcmbuf) {
//    LOGE("MyAudio::resampleAudio +");
    while (playStatus != NULL && !playStatus->exit) {
        if (queue->getQueueSize() == 0)//加载中
        {
            if (!playStatus->load) {
                playStatus->load = true;
                callJava->onCallLoadStatus(CHILD_THREAD, true);
            }
            continue;
        } else {
            if (playStatus->load) {
                playStatus->load = false;
                callJava->onCallLoadStatus(CHILD_THREAD, false);
            }
        }
        avPacket = av_packet_alloc();
        if (queue->getAvpacket(avPacket) != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }
//        LOGE("MyAudio::resampleAudio avcodec_send_packet");
        ret = avcodec_send_packet(avCodecContext, avPacket);
        if (ret != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }
//        LOGE("MyAudio::resampleAudio avcodec_receive_frame");
        avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        if (ret == 0) {
//            LOGE("MyAudio::resampleAudio avcodec_receive_frame ret:%d",ret);
            if (avFrame->channels > 0 && avFrame->channel_layout == 0) {//TODO Why?
                avFrame->channel_layout = av_get_default_channel_layout(avFrame->channels);
            } else if (avFrame->channels == 0 && avFrame->channel_layout > 0) {
                avFrame->channels = av_get_channel_layout_nb_channels(avFrame->channel_layout);
            }

//            LOGE("swr_alloc_set_opts, ret:%d", ret);
            SwrContext *swr_ctx;
            swr_ctx = swr_alloc_set_opts(
                    NULL,// 传NULL
                    AV_CH_LAYOUT_STEREO,//输出声道布局
                    AV_SAMPLE_FMT_S16,//输出采样位数格式
                    avFrame->sample_rate,//输出采样率
                    avFrame->channel_layout,//输入声道布局
                    (AVSampleFormat) avFrame->format,//输入采样位数格式
                    avFrame->sample_rate,//输入采样率
                    NULL, NULL
            );
//            LOGE("start call swr_init, ret:%d", ret);
            if (!swr_ctx || swr_init(swr_ctx) < 0)   //swr init fail
            {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = NULL;
                swr_free(&swr_ctx);
                continue;
            }

//            LOGE("swr_convert start, ret:%d", ret);
            nbSize = swr_convert(
                    swr_ctx,
                    &buffer,//转码后输出的PCM数据大小
                    avFrame->nb_samples,//输出采样个数
                    (const uint8_t **) avFrame->data,//原始压缩数据
                    avFrame->nb_samples);//输入采样个数
//            LOGE("done swr_convert, nb:%d", nb);

            int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
//            LOGE("calculate data_size, nb:%d, out_channels:%d, saveBit:%d",
//                 nb,out_channels,av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));
            data_size = nbSize * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

            now_time = avFrame->pts * av_q2d(time_base);//unit s 获取当前时间
            if (now_time < clock) {//clock 表示当前播放时间
                now_time = clock;
            }
            clock = now_time;

            *ppPcmbuf = buffer;//Get resample pcm buf

//            fwrite(buffer, 1, data_size, outFile);
//            LOGE("Write data_size is %d ", data_size);
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            swr_free(&swr_ctx);

            break;  //TODO

        } else {
            LOGE("avcodec_receive_frame fail, ret:%d", ret);
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            continue;
        }
    }

//    fclose(outFile);
    //TODO buffer need release

//    LOGE("MyAudio::resampleAudio -, data_size:%d",data_size);
    return data_size;
}

int MyAudio::getSoundTouchData() {
//    LOGD("getSoundTouchData +");
    while (playStatus != NULL && !playStatus->exit) {
        out_buffer = NULL;
        if (isFinished) {//TODO 针对正处理中，有二次进入的异常情况
            isFinished = false;
            data_size = resampleAudio(reinterpret_cast<void **>(&out_buffer));
            //TODO 8bitPCM转16bitPCM
            /* 因为FFmpeg解码出来的PCM数据是8bit （uint8）的，而SoundTouch中最低是16bit（ 16bit integer samples ），
             * 所以我们需要将8bit的数据转换成16bit后再给SoundTouch处理。
             处理方式：
             由于PCM数据在内存中是顺序排列的，所以我们先将第一个8bit的数据复制到16bit内存的前8位，
             然后将8bit的数据再复制给16bit内存的后8bit，就能把16bit的内存填满，
             然后循环复制，直到把8bit的内存全部复制到16bit的内存中*/
            if (data_size > 0) {
                for (int i = 0; i < data_size / 2 + 1; i++) {
                    sampleBuffer[i] = (out_buffer[i * 2] | ((out_buffer[i * 2 + 1]) << 8));
                }
                soundTouch->putSamples(sampleBuffer, nbSize);
                //TODO why data_size /4?  获取soundTouch转换后的数据，需要除以2（双声道），再除以2（两个8bit拼成一个16bit，总数少一倍）
                num = soundTouch->receiveSamples(sampleBuffer, data_size / 4);
            } else {
                soundTouch->flush();
            }
        }
        if (num == 0) {//TODO 只有在receiveSamples返回num为0时，才算finished
            isFinished = true;
            continue;
        } else {
            if (out_buffer == NULL) {
                num = soundTouch->receiveSamples(sampleBuffer, data_size / 4);
                if (num == 0) {
                    isFinished = true;
                    continue;
                }
            }
            return num;//TODO 正常流程走这里直接返回number
        }
    }
//    LOGD("getSoundTouchData -");
    return 0;
}

void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void *context) {
    //assert(NULL == context);
    MyAudio *myAudio = (MyAudio *) context;
    if (myAudio != NULL) {
        int bufferSize = myAudio->getSoundTouchData();
        if (bufferSize > 0) {
            //当前播放时间，公式: PCM实际数据大小 /每秒理论PCM大小
            myAudio->clock += bufferSize / ((double) myAudio->sample_rate * 2 * 2);
            if (myAudio->clock - myAudio->last_time >= 0.1)//unit s
            {
                myAudio->last_time = myAudio->clock;
                myAudio->callJava->onCallTimeInfo(CHILD_THREAD, myAudio->clock, myAudio->duration);
            }

            myAudio->callJava->onCallVolumeDb(CHILD_THREAD,
                                              myAudio->getPcmDb(
                                                      reinterpret_cast<char *>(myAudio->sampleBuffer),
                                                      bufferSize * 4)
            );
            //TODO  (*wlAudio->pcmBufferQueue)->Enqueue被循环调用，把处理数据送给ffmpeg来处理播放；
            // 这里的buf经过soundtouch处理转换了，所以填写处理的buf地址：wlAudio->sampleBuffer,
            // 原来这里的size是buffersize（不经过soundtouch处理前设置的值），现在要改为buffersize * 2 * 2，第一个2是双通道，第二个2表示位数
            (*myAudio->pcmBufferQueue)->Enqueue(myAudio->pcmBufferQueue,
                                                (char *) myAudio->sampleBuffer, bufferSize * 2 * 2);
        }
    }
}

void MyAudio::initOpenELSL() {
    SLresult result;
    //第一步------------------------------------------
    // 创建引擎对象
    result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);

    //第二步，创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);
    (void) result;
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    (void) result;
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        (void) result;
    }
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, 0};


    // 第三步，配置PCM格式信息
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};

    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//播放pcm格式的数据
            2,//2个声道（立体声）
            getCurrentSampleRateForOpensles(
                    sample_rate),//44100hz的频率  SL_SAMPLINGRATE_44_1不能写死，适配性不好
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcm};


    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_MUTESOLO};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    (*engineEngine)->CreateAudioPlayer(engineEngine,
                                       &pcmPlayerObject,
                                       &slDataSource, &audioSnk,
                                       2,/*numberInterface*/
                                       ids, req);
    //初始化播放器
    (*pcmPlayerObject)->Realize(pcmPlayerObject, SL_BOOLEAN_FALSE);

//    得到接口后调用  获取Player接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_PLAY, &pcmPlayerPlay);

    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_VOLUME, &pcmVolumePlay);
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_MUTESOLO, &pcmMutePlay);

//    注册回调缓冲区 获取缓冲队列接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_BUFFERQUEUE, &pcmBufferQueue);
    //缓冲接口回调
    (*pcmBufferQueue)->RegisterCallback(pcmBufferQueue, pcmBufferCallBack, this);
//    获取播放状态接口
    (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);

    pcmBufferCallBack(pcmBufferQueue, this);

}

SLuint32 MyAudio::getCurrentSampleRateForOpensles(int sample_rate) {
    SLuint32 rate = 0;
    switch (sample_rate) {
        case 8000:
            rate = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            rate = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            rate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            rate = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            rate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            rate = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            rate = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            rate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            rate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            rate = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            rate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            rate = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            rate = SL_SAMPLINGRATE_192;
            break;
        default:
            rate = SL_SAMPLINGRATE_44_1;
    }
    return rate;
}

void MyAudio::resume() {
    if (pcmPlayerPlay != nullptr) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    }
}

void MyAudio::pause() {
    if (pcmPlayerPlay != nullptr) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PAUSED);
    }
}

void MyAudio::release() {
    if (queue != nullptr) {
        delete (queue);
        queue = nullptr;
    }
    if (pcmPlayerObject != nullptr) {
        (*pcmPlayerObject)->Destroy(pcmPlayerObject);
        pcmPlayerObject = nullptr;
        pcmPlayerPlay = nullptr;
        pcmBufferQueue = nullptr;
    }
    if (outputMixObject != nullptr) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = nullptr;
        outputMixEnvironmentalReverb = nullptr;
    }

    if (engineObject != nullptr) {
        (*engineObject)->Destroy(engineObject);
        engineObject = nullptr;
        engineEngine = nullptr;
    }

    if (buffer != nullptr) {
        av_free(buffer);//free(buffer);
        buffer = nullptr;
    }

    if (avCodecContext != nullptr) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = nullptr;
    }
    if (playStatus != nullptr) {
        playStatus = nullptr;
    }
    if (callJava != nullptr) {
        callJava = nullptr;
    }

//    if (soundTouch != nullptr) {
//        delete soundTouch;
//        soundTouch = nullptr;
//    }
}

void MyAudio::setVolume(int percent) {
    if (pcmVolumePlay != NULL) {
        if (percent > 30) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -20);
        } else if (percent > 25) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -22);
        } else if (percent > 20) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -25);
        } else if (percent > 15) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -28);
        } else if (percent > 10) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -30);
        } else if (percent > 5) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -34);
        } else if (percent > 3) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -37);
        } else if (percent > 0) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -40);
        } else {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -100);
        }
    }
}

void MyAudio::setMute(int muteType) {
    this->muteType = muteType;
    if (pcmMutePlay != NULL) {
        if (muteType == 0) {//right 右声道
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, true);
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, false);
        } else if (muteType == 1) {//left 左声道
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, false);
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, true);
        } else if (muteType == 2) {//stereo 立体声
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, false);
            (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, false);
        }
    }
}

void MyAudio::setPitch(double pitch_) {
    this->pitch = pitch_;
    if (soundTouch != nullptr) {
        LOGD("pitch:%lf", pitch);
        soundTouch->setPitch(pitch);
    }
}

void MyAudio::setSpeed(double speed_) {
    this->speed = speed_;
    if (soundTouch != nullptr) {
        LOGD("speed:%lf", speed);
        soundTouch->setTempo(speed);
    }
}

int MyAudio::getPcmDb(char *pcmData, size_t pcmSize) {
    int db = 0;
    short int preValue = 0;
    double sum = 0;
    for (int i = 0; i < pcmSize; i += 2) {
        memcpy(&preValue, pcmData + i, 2);
        sum += fabs(preValue);
    }
    sum = sum / (pcmSize / 2);
    if (sum > 0) {
        db = 20.0 * log10(sum);
    }

    return db;
}
