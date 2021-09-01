package com.peter.myplayer.player;

import android.text.TextUtils;
import android.util.Log;

import com.peter.myplayer.listener.OnPreparedListener;
import com.peter.myplayer.utils.MyLog;

public class WeAudioPlayer {
    private static final String TAG="my_tag_"+WeAudioPlayer.class.getSimpleName();

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

    public WeAudioPlayer(){
        MyLog.d("WeAudioPlayer constructor in MyLog");
        Log.d(TAG,"WeAudioPlayer constructor in Log");
    }

    /**
     * 设置数据源
     * @param source
     */
    public void setSource(String source) {
        this.source = source;
    }

    /**
     * 设置准备接口回调
     * @param onPreparedListener
     */
    public void setOnPreparedListener(OnPreparedListener onPreparedListener){
        this.onPreparedListener = onPreparedListener;
    }

    //TODO 为什么这里要开子线程做？
    public void prepared(){
        if(TextUtils.isEmpty(source)){
            MyLog.d("data source not be empty");
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                native_prepared(source);
            }
        }).start();
    }

    //TODO 为什么这里要开子线程做？
    public void start(){
        if(TextUtils.isEmpty(source)){
            MyLog.d("data source not be empty");
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG,"native start");
                native_start();
            }
        }).start();
    }

    /**
     * c++回调java的方法
     */
    public void onCallPrepared()
    {
        if(onPreparedListener != null)
        {
            onPreparedListener.onPrepared();
        }
    }

    public native void native_start();

    public native void native_prepared(String source);

}
