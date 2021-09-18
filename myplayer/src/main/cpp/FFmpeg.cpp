//
// Created by lz6600 on 2021/8/31.
//

#include "FFmpeg.h"

static int getCurrentTime()
{
    struct timeval t;
    gettimeofday(&t, nullptr);
    return static_cast<int>(t.tv_sec * 1000 + t.tv_usec / 1000);
}

FFmpeg::FFmpeg(const char *source, CallJava *callJava_, PlayStatus *playStatus_)
{
    this->source = source;
    this->callJava = callJava_;
    this->playStatus = playStatus_;
    exitFfmpeg = false;
    memset(errMsg, 0, sizeof(errMsg));
    pthread_mutex_init(&init_mutex, nullptr);
    pthread_mutex_init(&seek_mutex, nullptr);
}

FFmpeg::~FFmpeg()
{
    pthread_mutex_destroy(&init_mutex);
    pthread_mutex_destroy((&seek_mutex));
}

//todo 打开文件，做好准备，给上层回调结果
void *decodeFFmpegRun(void *data)
{
    LOGE("decodeFFmpegRun +");
    FFmpeg *pFfmpeg = static_cast<FFmpeg *>(data);
    pFfmpeg->prepareDecodeThread();

    LOGE("decodeFFmpegRun -");
//    return nullptr;
    pthread_exit(&pFfmpeg->decodeThread);//pthread_exit 和 return 0 有什么区别？
}

void FFmpeg::prepared()
{
    LOGE("prepared +");
    pthread_create(&decodeThread, 0, decodeFFmpegRun, this);
    LOGE("prepared -");
}

int avformat_callback(void *ctx)
{
    FFmpeg *fFmpeg = (FFmpeg *) ctx;
    if (fFmpeg->playStatus->exit)
    {
        return AVERROR_EOF;
    }
    return 0;
}

//TODO 完成打开文件，准备好开始做解封装
void FFmpeg::prepareDecodeThread()
{
    LOGE("prepareDecodeThread + :%s", source);

    av_register_all();//TODO 4.2.0废弃，可以不设置照常用
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    pFormatCtx->interrupt_callback.callback = avformat_callback;
    pFormatCtx->interrupt_callback.opaque = this;

    int now_time = getCurrentTime();

    LOGE("prepareDecodeThread 1 avformat_open_input, :%s", source);
    int retVal = avformat_open_input(&pFormatCtx, source, NULL, NULL);
    if (retVal != 0)
    {
        if (LOG_DEBUG)
        {
            LOGE("can not open source :%s, errcode:%d", source, retVal);
        }
        if (callJava)
        {
            sprintf(errMsg, "can not open source:%s, errcode:%d", source, retVal);
            callJava->onCallErrorMsg(CHILD_THREAD,
                                     ERROR_CODE_AVFORMAT_OPEN_FAIL, errMsg);
        }
        exitFfmpeg = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    LOGE("prepareDecodeThread 2, avformat_find_stream_info");
    retVal = avformat_find_stream_info(pFormatCtx, NULL);
    if (retVal < 0)
    {
        if (LOG_DEBUG)
        {
            LOGE("can not find streams from %s, errcode:%d", source, retVal);
        }
        if (callJava)
        {
            sprintf(errMsg, "can not find streams from %s, errcode:%d", source, retVal);
            callJava->onCallErrorMsg(CHILD_THREAD,
                                     ERROR_CODE_AVFORMAT_FIND_STREAM_INFO, errMsg);
        }
        exitFfmpeg = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    LOGE("prepareDecodeThread 3, pFormatCtx->nb_streams:%d", pFormatCtx->nb_streams);
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)//得到音频流
        {
            if (audio == NULL)
            {
                audio = new MyAudio(playStatus, pFormatCtx->streams[i]->codecpar->sample_rate,
                                    callJava);
                audio->streamIndex = i;
                audio->avCodecParameters = pFormatCtx->streams[i]->codecpar;
                audio->duration = pFormatCtx->duration / AV_TIME_BASE;
                audio->time_base = pFormatCtx->streams[i]->time_base;
                duration = audio->duration;
                if(callJava){
                    callJava->onCallPcmSampleRate(CHILD_THREAD,audio->sample_rate,
                                                  pFormatCtx->streams[i]->codecpar->channels);
                }
            }
        }
    }
    LOGE("prepareDecodeThread 4, avcodec_find_decoder");
    AVCodec *dec = avcodec_find_decoder(audio->avCodecParameters->codec_id);
    if (!dec)
    {
        if (LOG_DEBUG)
        {
            LOGE("can not find decoder");
        }
        if (callJava)
        {
            sprintf(errMsg, "can not find decoder");
            callJava->onCallErrorMsg(CHILD_THREAD,
                                     ERROR_CODE_AVFORMAT_FIND_DECODER, errMsg);
        }
        exitFfmpeg = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    LOGE("prepareDecodeThread 5, avcodec_alloc_context3");
    audio->avCodecContext = avcodec_alloc_context3(dec);
    if (!audio->avCodecContext)
    {
        if (LOG_DEBUG)
        {
            LOGE("can not alloc new decodecctx");
        }
        if (callJava)
        {
            sprintf(errMsg, "can not alloc new decodecctx");
            callJava->onCallErrorMsg(CHILD_THREAD,
                                     ERROR_CODE_AVCODEC_ALLOC_CONTEXT, errMsg);
        }
        exitFfmpeg = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    LOGE("prepareDecodeThread 6, avcodec_parameters_to_context");
    if (avcodec_parameters_to_context(audio->avCodecContext, audio->avCodecParameters) < 0)
    {
        if (LOG_DEBUG)
        {
            LOGE("can not fill decodecctx");
        }
        if (callJava)
        {
            sprintf(errMsg, "can not fill decodecct");
            callJava->onCallErrorMsg(CHILD_THREAD,
                                     ERROR_CODE_AVCODEC_PARAM_TO_CONTEXT, errMsg);
        }
        exitFfmpeg = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    LOGE("prepareDecodeThread 7, avcodec_open2");
    retVal = avcodec_open2(audio->avCodecContext, dec, 0);
    if (retVal != 0)
    {
        if (LOG_DEBUG)
        {
            LOGE("cant not open audio streams, errcode:%d", retVal);
        }
        if (callJava)
        {
            sprintf(errMsg, "cant not open audio streams, errcode:%d", retVal);
            callJava->onCallErrorMsg(CHILD_THREAD,
                                     ERROR_CODE_AVCODEC_OPEN, errMsg);
        }
        exitFfmpeg = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    LOGD("Open audio streams success, decode ffmpeg costTime:%d", getCurrentTime() - now_time);
    if (callJava != nullptr)
    {
        if (playStatus != nullptr && !playStatus->exit)
        {
            callJava->onCallPrepared(CHILD_THREAD);
        }
        else
        {
            exitFfmpeg = true;//TODO Why? if playstatus->exit is true,then set exit to true
        }
    }

    pthread_mutex_unlock(&init_mutex);
    LOGE("prepareDecodeThread -");
}

//TODO 开始解码，把解码好的数据（Avpacket）放到audio->queue中  audio->queue->putAvpacket(avPacket)
void FFmpeg::startDecode()
{
    LOGE("FFmpeg start +");
    if (audio == NULL)
    {
        if (LOG_DEBUG)
        {
            LOGE("audio is null");
            return;
        }
    }
    audio->play();//do what?
    int count = 0;
    while (playStatus != NULL && !playStatus->exit)
    {
        if (playStatus->seekStatus)
        {
            av_usleep(1000 * 100);//TODO sleep 100ms for CPU using
            continue;
        }
        //TODO 如果解码太快，让他sleep等一下
        if (audio->queue->getQueueSize() > 30)
        { //考虑ape格式文件  wanli set 40
            av_usleep(1000 * 100);//TODO sleep 100ms for CPU using
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();
        if (av_read_frame(pFormatCtx, avPacket) == 0)
        {
            if (avPacket->stream_index == audio->streamIndex)//index 0 is Audio, 1 is video
            {
                count++;
                LOGE("ffmpeg decode avpacket, 解码第 %d 帧", count);
                audio->queue->putAvpacket(avPacket);
            }
            else
            {
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        }
        else
        {
            av_packet_free(&avPacket);
            av_free(avPacket);

            while (playStatus != NULL && !playStatus->exit)
            {
                //TODO 全部解码完成，但是音频还没播完，还有一些剩余帧等着播放
                if (audio->queue->getQueueSize() > 0)
                {
                    LOGD("audio->queue->getQueueSize() = %d", audio->queue->getQueueSize());
                    av_usleep(1000 * 100);//TODO sleep 100ms for CPU using
                    continue;
                }
                else
                {
//                    playStatus->exit = true;  //TODO save this will crash
                    break;
                }
            }
            break;
        }
    }
    exitFfmpeg = true;
    if (audio != nullptr)
    {
        audio->isCut = false;
    }

    if (callJava != nullptr)
    {
        callJava->onCallComplete(CHILD_THREAD);
    }
    //模拟出队
/*    while (audio->queue->getQueueSize() > 0)
    {
        AVPacket *packet = av_packet_alloc();
        audio->queue->getAvpacket(packet);
        av_packet_free(&packet);
        av_free(packet);
        packet = NULL;
    }*/
    if (LOG_DEBUG)
    {
        LOGD("解码完成");
    }
    LOGE("FFmpeg start -");
}

void FFmpeg::pause()
{
    if (audio != nullptr)
    {
        audio->pause();
    }
}

void FFmpeg::resume()
{
    if (audio != nullptr)
    {
        audio->resume();
    }
}

void FFmpeg::release()
{
    if (LOG_DEBUG)
    {
        LOGE("开始释放Ffmpeg");
    }
    if (playStatus->exit)
    {
        return;
    }

    if (LOG_DEBUG)
    {
        LOGE("开始释放Ffmpeg2");
    }
    playStatus->exit = true;

    pthread_mutex_lock(&init_mutex);
    int sleepCount = 0;

    int now_time = getCurrentTime();
    while (!exitFfmpeg)
    {
        if (sleepCount > 1000)
        {
            exitFfmpeg = true;
        }
        if (LOG_DEBUG)
        {
            LOGE("wait ffmpeg  exit, sleepCount:%d", sleepCount);
        }
        sleepCount++;
        av_usleep(1000 * 10);//暂停10毫秒 TODO 用av_usleep()合适？
    }

    if (LOG_DEBUG)
    {
        LOGE("释放 Audio, sleep costTime:%d ms", getCurrentTime() - now_time);
    }

    if (audio != NULL)
    {
        audio->release();
        delete (audio);
        audio = NULL;
    }

    if (LOG_DEBUG)
    {
        LOGE("释放 封装格式上下文");
    }
    if (pFormatCtx != NULL)
    {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }

    if (LOG_DEBUG)
    {
        LOGE("释放 callJava");
    }
    if (callJava != NULL)
    {
        callJava = NULL;
    }

    if (LOG_DEBUG)
    {
        LOGE("释放 playStatus");
    }
    if (playStatus != NULL)
    {
        playStatus = NULL;
    }
    pthread_mutex_unlock(&init_mutex);
}

void FFmpeg::seek(int64_t seconds)
{
    if (duration <= 0)
    {
        return;
    }
    if (seconds >= 0 && seconds <= duration)
    {
        if (audio != NULL)
        {
            playStatus->seekStatus = true;
            audio->queue->clearAvpacket();
            audio->clock = 0;
            audio->last_time = 0;
            pthread_mutex_lock(&seek_mutex);
            int64_t realTime = seconds * AV_TIME_BASE;
            //TODO 由于一个AVPacket里面有多个AVFrame，当seek时，FFmpeg解码器中还残留
            //     AVFrame，所以会导致seek后，不能立即播放当前音乐,需要做下 avcodec_flush_buffers 来清除FFmpeg解码器中残留的AVFrame
            avcodec_flush_buffers(audio->avCodecContext);
            avformat_seek_file(pFormatCtx, -1, INT64_MIN, realTime, INT64_MAX, 0);
            pthread_mutex_unlock(&seek_mutex);
            playStatus->seekStatus = false;
        }
    }
}

void FFmpeg::setVolume(int percent)
{
    if (audio != nullptr)
    {
        audio->setVolume(percent);
    }
}

void FFmpeg::setMute(int muteType)
{
    if (audio != nullptr)
    {
        audio->setMute(muteType);
    }
}

void FFmpeg::setSpeed(double speed)
{
    if (audio != nullptr)
    {
        audio->setSpeed(speed);
    }
}

void FFmpeg::setPitch(double pitch)
{
    if (audio != nullptr)
    {
        audio->setPitch(pitch);
    }
}

int FFmpeg::getAudioSampleRate()
{
    if (audio != nullptr)
    {
        return audio->avCodecContext->sample_rate;
    }
    return 0;
}

void FFmpeg::startRecord(bool start)
{
    if (audio != nullptr)
    {
        audio->startStopRecord(start);
    }
}

bool FFmpeg::cutAudioPlay(double startTime, double endTime, bool isShowPcm)
{
    if (startTime >= 0 && endTime <= duration && startTime < endTime)
    {
        if (audio != nullptr)
        {
            audio->isCut = true;
            audio->endTime = endTime;
            audio->isShowPcm = isShowPcm;
            int64_t tempTime = (int64_t) startTime;
            LOGD("startTime:%lf, tempTime:%ld", startTime, tempTime);
            seek(tempTime);//这里数据精度有损失，如果想精准，传double类型下来
            return true;
        }
    }
    return false;
}
