//
// Created by lz6600 on 2021/8/31.
//

#include "FFmpeg.h"

FFmpeg::FFmpeg(const char *source, CallJava *callJava_, PlayStatus *playStatus_)
{
    this->source = source;
    this->callJava = callJava_;
    this->playStatus = playStatus_;
}

FFmpeg::~FFmpeg()
{

}

void *decodeFFmpegRun(void *data)
{
    LOGE("decodeFFmpegRun +");
    FFmpeg *pFfmpeg = static_cast<FFmpeg *>(data);
    pFfmpeg->decodeFFmpegThread();

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

void FFmpeg::decodeFFmpegThread()
{
    LOGE("decodeFFmpegThread + :%s", source);
    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    LOGE("decodeFFmpegThread 1 avformat_open_input, :%s", source);
    int retVal = avformat_open_input(&pFormatCtx, source, NULL, NULL) ;
    if (retVal != 0)
    {
        if (LOG_DEBUG)
        {
            LOGE("can not open source :%s, errcode:%d", source,retVal);
        }
        return;
    }
    LOGE("decodeFFmpegThread 2, avformat_find_stream_info");
    retVal = avformat_find_stream_info(pFormatCtx, NULL);
    if (retVal < 0)
    {
        if (LOG_DEBUG)
        {
            LOGE("can not find streams from %s, errcode:%d", source,retVal);
        }
        return;
    }
    LOGE("decodeFFmpegThread 3, pFormatCtx->nb_streams:%d",pFormatCtx->nb_streams);
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)//得到音频流
        {
            if (audio == NULL)
            {
                audio = new MyAudio(playStatus, pFormatCtx->streams[i]->codecpar->sample_rate);
                audio->streamIndex = i;
                audio->avCodecParameters = pFormatCtx->streams[i]->codecpar;
            }
        }
    }
    LOGE("decodeFFmpegThread 4, avcodec_find_decoder");
    AVCodec *dec = avcodec_find_decoder(audio->avCodecParameters->codec_id);
    if (!dec)
    {
        if (LOG_DEBUG)
        {
            LOGE("can not find decoder");
        }
        return;
    }
    LOGE("decodeFFmpegThread 5, avcodec_alloc_context3");
    audio->avCodecContext = avcodec_alloc_context3(dec);
    if (!audio->avCodecContext)
    {
        if (LOG_DEBUG)
        {
            LOGE("can not alloc new decodecctx");
        }
        return;
    }
    LOGE("decodeFFmpegThread 6, avcodec_parameters_to_context");
    if (avcodec_parameters_to_context(audio->avCodecContext, audio->avCodecParameters) < 0)
    {
        if (LOG_DEBUG)
        {
            LOGE("can not fill decodecctx");
        }
        return;
    }
    LOGE("decodeFFmpegThread 7, avcodec_open2");
    retVal = avcodec_open2(audio->avCodecContext, dec, 0);
    if ( retVal != 0)
    {
        if (LOG_DEBUG)
        {
            LOGE("cant not open audio streams, errcode:%d",retVal);
        }
        return;
    }

    LOGD("Open audio streams success");
    callJava->onCallPrepared(CHILD_THREAD);
    LOGE("decodeFFmpegThread -");
}

void FFmpeg::start()
{
    LOGE("FFmpeg start +");
    if(audio == NULL){
        if(LOG_DEBUG){
            LOGE("audio is null");
            return;
        }
    }
    audio->play();

    while(1)
    {
        AVPacket *avPacket = av_packet_alloc();
        if(av_read_frame(pFormatCtx, avPacket) == 0)
        {
            if(avPacket->stream_index == audio->streamIndex)//index 0 is Audio, 1 is video
            {
                //解码操作
                audio->queue->putAvpacket(avPacket);
            } else{
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else{
            if(LOG_DEBUG)
            {
                LOGE("decode finished");
            }
            av_packet_free(&avPacket);
            av_free(avPacket);
            break;
        }
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
    if(LOG_DEBUG)
    {
        LOGD("解码完成");
    }

    LOGE("FFmpeg start -");
}
