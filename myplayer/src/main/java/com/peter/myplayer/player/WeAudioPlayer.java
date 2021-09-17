package com.peter.myplayer.player;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.text.TextUtils;
import android.util.Log;

import com.peter.myplayer.bean.TimeInfoBean;
import com.peter.myplayer.listener.MyOnCompleteListener;
import com.peter.myplayer.listener.MyOnErrorListener;
import com.peter.myplayer.listener.MyOnLoadListener;
import com.peter.myplayer.listener.MyOnPauseResumeListener;
import com.peter.myplayer.listener.MyOnTimeInfoListener;
import com.peter.myplayer.listener.OnPreparedListener;
import com.peter.myplayer.listener.OnRecordAacTimeListener;
import com.peter.myplayer.listener.OnVolumeDBListener;
import com.peter.myplayer.utils.MuteEnum;
import com.peter.myplayer.utils.MyLog;
import com.peter.myplayer.utils.TimeUtil;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class WeAudioPlayer {
    private static final String TAG = "my_tag_" + WeAudioPlayer.class.getSimpleName();

    static {
        System.loadLibrary("native-lib2");
        System.loadLibrary("avcodec-57");
//        System.loadLibrary("avdevice-57");
//        System.loadLibrary("avfilter-6");
        System.loadLibrary("avformat-57");
        System.loadLibrary("avutil-55");
//        System.loadLibrary("postproc-54");
        System.loadLibrary("swresample-2");
        System.loadLibrary("swscale-4");
    }

    private String source;//数据源
    private OnPreparedListener onPreparedListener;
    private MyOnLoadListener onLoadListener;
    private MyOnPauseResumeListener onPauseResumeListener;
    private MyOnTimeInfoListener onTimeInfoListener;
    private MyOnErrorListener onErrorListener;
    private MyOnCompleteListener onCompleteListener;
    private OnVolumeDBListener onVolumeDBListener;
    private OnRecordAacTimeListener onRecordAacTimeListener;

    private static TimeInfoBean timeInfoBean;
    private static boolean isPlayNext = false;
    private static int duration = -1;
    private static int volumePercent = 0;
    private static MuteEnum muteEnum = MuteEnum.MUTE_STEREO;
    private static double speed = 1.0;
    private static double pitch = 1.0;

    //mediacodec
    private static final int BUFFER_SIZE_IN_BYTES = 4608;   //old 4096
    private static boolean bInitMediaCodec = false;
    private int aacSampleRate = 4;
    private MediaFormat encoderFormat = null;
    private MediaCodec encoder = null;
    private FileOutputStream outputStream = null;
    private MediaCodec.BufferInfo bufferInfo = null;
    private int perPcmSize = 0;
    private byte[] outByteBuffer = null;

    private double recordTime = 0;
    private int audioSamplerate = 0;

    public WeAudioPlayer() {
        MyLog.d("WeAudioPlayer constructor in MyLog");
//        Log.d(TAG, "WeAudioPlayer constructor in Log");
    }

    /**
     * 设置数据源
     *
     * @param source
     */
    public void setSource(String source) {
        this.source = source;
    }

    /**
     * 设置准备接口回调
     *
     * @param onPreparedListener
     */
    public void setOnPreparedListener(OnPreparedListener onPreparedListener) {
        this.onPreparedListener = onPreparedListener;
    }

    public void setOnLoadListener(MyOnLoadListener onLoadListener) {
        this.onLoadListener = onLoadListener;
    }

    public void setOnPauseResumeListener(MyOnPauseResumeListener onPauseResumeListener) {
        this.onPauseResumeListener = onPauseResumeListener;
    }

    public void setOnTimeInfoListener(MyOnTimeInfoListener onTimeInfoListener) {
        this.onTimeInfoListener = onTimeInfoListener;
    }

    public void setOnErrorListener(MyOnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    public void setOnCompleteListener(MyOnCompleteListener onCompleteListener) {
        this.onCompleteListener = onCompleteListener;
    }

    public void setOnVolumeDBListener(OnVolumeDBListener onVolumeDBListener) {
        this.onVolumeDBListener = onVolumeDBListener;
    }

    public void setOnRecordAacTimeListener(OnRecordAacTimeListener onRecordAacTimeListener) {
        this.onRecordAacTimeListener = onRecordAacTimeListener;
    }

    //TODO 为什么这里要开子线程做？  prepare将数据解封装，有延迟动作，需要放在子线程中执行
    public void prepared() {
        if (TextUtils.isEmpty(source)) {
            MyLog.d("data source not be empty");
            return;
        }
        onCallLoadStatus(true);
        new Thread(new Runnable() {
            @Override
            public void run() {
                native_prepared(source);
            }
        }).start();
    }

    //TODO 为什么这里要开子线程做？  start 将数据做解码，有延迟动作，需要放在子线程中执行
    public void start() {
        if (TextUtils.isEmpty(source)) {
            MyLog.d("data source not be empty");
            return;
        }
        //TODO start new child thread1, native-lib new child thread2,
        // finally execute func in which thread?
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "native start");
                setVolume(volumePercent);
                setMute(muteEnum);
                setPitch(pitch);
                setSpeed(speed);
                native_start();
            }
        }).start();
    }

    public void stop() {//TODO why stop use new thread?
        timeInfoBean = null;
        duration = -1;
        recordTime = 0;
        stopRecord();//TODO notice here
        new Thread(new Runnable() {
            @Override
            public void run() {
                nativeStop();
            }
        }).start();
    }

    public void seek(int second) {
        NativeSeek(second);
    }

    public void pause() {
        pauseNative();
        if (onPauseResumeListener != null) {
            onPauseResumeListener.onPause(true);
        }
    }

    public void resume() {
        resumeNative();
        if (onPauseResumeListener != null) {
            onPauseResumeListener.onPause(false);
        }
    }

    public void playNext(String source_) {
        source = source_;
        isPlayNext = true;
        stop();
    }

    public void setVolume(int percent) {
        if (percent >= 0 && percent <= 100) {
            volumePercent = percent;
            nativeSetVolume(percent);
        }
    }

    public int getDuration() {
        if (duration < 0) {
            duration = nativeGetDuration();
        }
        return duration;
    }

    public int getVolumePercent() {
        return volumePercent;
    }

    public void setMute(MuteEnum mute) {
        muteEnum = mute;
        nativeSetMute(mute.getValue());
    }

    public void setSpeed(double speed_) {
        speed = speed_;
        nativeSetSpeed(speed);
    }

    public void setPitch(double pitch_) {
        this.pitch = pitch_;
        nativeSetPitch(pitch);
    }

    public void startRecord(File saveFileName) {
        if (!bInitMediaCodec) {
            audioSamplerate = nativeGetSampleRate();
            if (audioSamplerate > 0) {
                bInitMediaCodec = true;
                initMediaCodec(audioSamplerate, saveFileName);
                nativeStartStopRecord(true);
                Log.d(TAG, "Start record pcm2aac");
            }

        }
    }

    public void stopRecord() {
        if (bInitMediaCodec) {
            nativeStartStopRecord(false);
            releaseMediaCodec();
            Log.d(TAG, "Stop record pcm2aac");
        }
    }

    public void pauseRecord() {
        if (bInitMediaCodec) {
            nativeStartStopRecord(false);
            Log.d(TAG, "Pause record aac");
        }
    }

    public void goonRecord() {
        if (bInitMediaCodec) {
            nativeStartStopRecord(true);
            Log.d(TAG, "Goon record aac");
        }
    }

    private void initMediaCodec(int sampleRate, File saveFileName) {
        try {
            recordTime = 0;
            aacSampleRate = getADTSSampleRate(sampleRate);
            Log.d(TAG, "sampleRate=" + sampleRate + ", aacSampleRate=" + aacSampleRate);
            encoderFormat = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, sampleRate, 2);
            //设置码率
            encoderFormat.setInteger(MediaFormat.KEY_BIT_RATE, 96000);
            //设置profile
            encoderFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
            //设置最大输入size， 也是缓冲队列的capacity
            encoderFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, BUFFER_SIZE_IN_BYTES);

            encoder = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);
            bufferInfo = new MediaCodec.BufferInfo();
            if (encoder == null) {
                MyLog.d("Create encoder wrong");
                return;
            }
            //这里只编码不显示，所以surface为null
            encoder.configure(encoderFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            outputStream = new FileOutputStream(saveFileName);
            encoder.start();
            Log.d(TAG, "encoder start");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    private void releaseMediaCodec() {
        if (encoder == null) {
            return;
        }
        try {
            outputStream.close();
            outputStream = null;
            encoder.stop();
            encoder.release();
            encoder = null;
            encoderFormat = null;
            bufferInfo = null;
            bInitMediaCodec = false;
            recordTime = 0;
            MyLog.d("录制完成...");
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                outputStream = null;
            }
        }
    }

    private void addADTSHeader2Packet(byte[] packet, int packetLen, int sampleRate) {
        int profile = 2; // AAC LC
        int freqIdx = sampleRate; // samplerate // 4 <==> 44.1KHz
        int chanCfg = 2; // CPE
        packet[0] = (byte) 0xFF; // 0xFFF(12bit) 这里只取了8位，还差4位放到下一个里面
        packet[1] = (byte) 0xF9; // 第一个4bit位放F   1001 = 0x9
        packet[2] = (byte) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));//(chanCfg >> 2) 取chanCfg的高1位
        //((chanCfg & 3) << 6) 取chanCfg 的高2位
        packet[3] = (byte) (((chanCfg & 3) << 6) + (packetLen >> 11));//(packetLen >> 11)取packetLen高2位
        packet[4] = (byte) ((packetLen & 0x7FF) >> 3);//取packetLen高8位
        packet[5] = (byte) (((packetLen & 7) << 5) + 0x1F);//((packetLen & 7) << 5) 取packetLen低3位
        packet[6] = (byte) 0xFC;
    }

    private int getADTSSampleRate(int samplerate) {
        int rate = 4;
        switch (samplerate) {
            case 96000:
                rate = 0;
                break;
            case 88200:
                rate = 1;
                break;
            case 64000:
                rate = 2;
                break;
            case 48000:
                rate = 3;
                break;
            case 44100:
                rate = 4;
                break;
            case 32000:
                rate = 5;
                break;
            case 24000:
                rate = 6;
                break;
            case 22050:
                rate = 7;
                break;
            case 16000:
                rate = 8;
                break;
            case 12000:
                rate = 9;
                break;
            case 11025:
                rate = 10;
                break;
            case 8000:
                rate = 11;
                break;
            case 7350:
                rate = 12;
                break;
        }
        return rate;
    }

    /**
     * c++回调java的方法
     */
    public void onCallPrepared() {
        if (onPreparedListener != null) {
            onPreparedListener.onPrepared();
        }
    }

    public void onCallLoadStatus(boolean load) {
        if (onLoadListener != null) {
            onLoadListener.onLoad(load);
        }
    }

    public void onCallTimeInfo(int currentTime, int totalTime) {
        if (onTimeInfoListener != null) {
            if (timeInfoBean == null) {
                timeInfoBean = new TimeInfoBean();
            }
            timeInfoBean.setCurrentTime(currentTime);
            timeInfoBean.setTotalTime(totalTime);
            onTimeInfoListener.onTimeInfo(timeInfoBean);
        }
    }

    public void onCallError(int code, String msg) {

        if (onErrorListener != null) {
            stop();
            onErrorListener.onError(code, msg);
        }
    }

    public void onCallComplete() {
        if (onCompleteListener != null) {
            stop();
            onCompleteListener.onComplete();
        }
    }

    public void onCallNext() {
        if (isPlayNext) {
            isPlayNext = false;
            prepared();
        }
    }

    public void onCallVolumeDB(int db) {
        if (onVolumeDBListener != null) {
            onVolumeDBListener.onVolumeDbValue(db);
        }
    }

    private void encodePcmToAAC(int size, byte[] buffer) {
        Log.d(TAG, "input buffer size=" + buffer.length +
                ", input size=" + size+", audioSamplerate="+audioSamplerate);
        if (buffer != null && encoder != null) {
            recordTime += size * 1.0 / (audioSamplerate * 2 * (16 / 8));
            MyLog.d("recordTime = " + recordTime);
            if (onRecordAacTimeListener != null) {
                onRecordAacTimeListener.onRecordAacTimeListener((int) recordTime);
            }
            int inputBufferindex = encoder.dequeueInputBuffer(0);
            if (inputBufferindex >= 0) {
                ByteBuffer byteBuffer = encoder.getInputBuffers()[inputBufferindex];
                TimeUtil.printByteBufInfo(byteBuffer);

//               ByteBuffer用法 一般设置clear后，切换为写入模式
//                把position设为0，把limit设为capacity，一般在把数据写入Buffer前调用。
                byteBuffer.clear();
//                int newSize= Math.max(size, buffer.length);
//                Log.d(TAG,"after clear, buffer size="+buffer.length+", input size="+size+", newSize="+newSize);
//                byteBuffer.limit(newSize);//todo
                TimeUtil.printByteBufInfo(byteBuffer);
                //TODO 将pcm数据buffer放入 byteBuffer中，即放入mediacodec的输入队列
                byteBuffer.put(buffer);
                encoder.queueInputBuffer(inputBufferindex, 0, size, 0, 0);
            }

            int index = encoder.dequeueOutputBuffer(bufferInfo, 0);
            MyLog.d("0 编码..." + index);
            while (index >= 0) {
                try {
                    perPcmSize = bufferInfo.size + 7;
                    Log.d(TAG, "outputIndex=" + index + ", perPcmSize=" + perPcmSize +
                            ", bufferInfo.size=" + bufferInfo.size);
                    outByteBuffer = new byte[perPcmSize];

                    //TODO byteBuffer这里使用它读取数据
                    ByteBuffer byteBuffer = encoder.getOutputBuffers()[index];
                    TimeUtil.printByteBufInfo(byteBuffer);
                    //TODO 设置position
                    byteBuffer.position(bufferInfo.offset);
                    byteBuffer.limit(bufferInfo.offset + bufferInfo.size);
                    int totalSize = bufferInfo.offset + bufferInfo.size;
                    Log.d(TAG, "bufferInfo.offset =" + bufferInfo.offset +
                            ", bufferInfo.offset + bufferInfo.size=" + totalSize);
                    TimeUtil.printByteBufInfo(byteBuffer);

                    addADTSHeader2Packet(outByteBuffer, perPcmSize, aacSampleRate);
                    Log.d(TAG, "after addADTSHeader2Packet");
                    byteBuffer.get(outByteBuffer, 7, bufferInfo.size);
                    TimeUtil.printByteBufInfo(byteBuffer);
                    byteBuffer.position(bufferInfo.offset);
                    TimeUtil.printByteBufInfo(byteBuffer);
                    outputStream.write(outByteBuffer, 0, perPcmSize);

                    encoder.releaseOutputBuffer(index, false);
                    outByteBuffer = null;

                    //TODO 继续下一帧处理，从输出队列中取出处理后数据
                    index = encoder.dequeueOutputBuffer(bufferInfo, 0);
                    MyLog.d("编码..." + index);

                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private native void native_start();

    private native void native_prepared(String source);

    private native void pauseNative();

    private native void resumeNative();

    private native void nativeStop();

    private native void NativeSeek(int seconds);

    private native int nativeGetDuration();

    private native void nativeSetVolume(int percent);

    private native void nativeSetMute(int value);

    private native void nativeSetSpeed(double speed);

    private native void nativeSetPitch(double pitch);

    private native int nativeGetSampleRate();

    private native void nativeStartStopRecord(boolean bStartRecord);


}
