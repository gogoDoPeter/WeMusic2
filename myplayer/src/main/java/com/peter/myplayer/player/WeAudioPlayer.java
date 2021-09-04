package com.peter.myplayer.player;

import android.text.TextUtils;
import android.util.Log;

import com.peter.myplayer.bean.TimeInfoBean;
import com.peter.myplayer.listener.MyOnCompleteListener;
import com.peter.myplayer.listener.MyOnErrorListener;
import com.peter.myplayer.listener.MyOnLoadListener;
import com.peter.myplayer.listener.MyOnPauseResumeListener;
import com.peter.myplayer.listener.MyOnTimeInfoListener;
import com.peter.myplayer.listener.OnPreparedListener;
import com.peter.myplayer.utils.MyLog;

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
    private static TimeInfoBean timeInfoBean;
    private static boolean isPlayNext = false;

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
                native_start();
            }
        }).start();
    }

    public void stop() {//TODO why stop new thread?
        timeInfoBean = null;
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

    private native void native_start();

    private native void native_prepared(String source);

    private native void pauseNative();

    private native void resumeNative();

    private native void nativeStop();

    private native void NativeSeek(int seconds);


}
