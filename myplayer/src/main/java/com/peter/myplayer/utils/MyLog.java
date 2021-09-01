package com.peter.myplayer.utils;

import android.util.Log;

public class MyLog {
    private static final String TAG = "my_tag";
    public static void d(String msg)
    {
        Log.d(TAG, msg);
    }
}
