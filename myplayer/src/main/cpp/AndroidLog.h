#ifndef MYMUSIC_ANDROIDLOG_H
#define MYMUSIC_ANDROIDLOG_H

#endif //MYMUSIC_ANDROIDLOG_H

#include "android/log.h"

#define LOG_DEBUG true


#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_DEBUG,"my_tag",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"my_tag",FORMAT,##__VA_ARGS__);

#define  ERROR_CODE_AVFORMAT_OPEN_FAIL (1000)     //avformat_open_input fail
#define  ERROR_CODE_AVFORMAT_FIND_STREAM_INFO (1001)     //can not find streams from
#define  ERROR_CODE_AVFORMAT_FIND_DECODER (1002)     //avformat find decoder fail
#define  ERROR_CODE_AVCODEC_ALLOC_CONTEXT (1003)
#define  ERROR_CODE_AVCODEC_PARAM_TO_CONTEXT (1004)
#define  ERROR_CODE_AVCODEC_OPEN (1000)
//#define  ERROR_CODE_AVFORMAT_OPEN_FAIL (1005)     //avformat_open_input fail
