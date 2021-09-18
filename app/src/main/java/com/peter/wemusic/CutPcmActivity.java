package com.peter.wemusic;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;

import com.peter.myplayer.bean.TimeInfoBean;
import com.peter.myplayer.listener.OnTimeInfoListener;
import com.peter.myplayer.listener.OnCutPcmDataListener;
import com.peter.myplayer.listener.OnPreparedListener;
import com.peter.myplayer.player.WeAudioPlayer;

public class CutPcmActivity extends AppCompatActivity {
    private static final String TAG="my_tag_"+CutPcmActivity.class.getSimpleName();
    private WeAudioPlayer audioPlayer;
    private static final String[] permissions = new String[]{
            Manifest.permission.INTERNET,
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
    };

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        audioPlayer = new WeAudioPlayer();
        setContentView(R.layout.activity_cutaudio);

        checkPermission();

        audioPlayer.setOnPreparedListener(new OnPreparedListener() {
            @Override
            public void onPrepared() {
                audioPlayer.cutAudioPlayer(20, 40, true);
            }
        });

        audioPlayer.setOnTimeInfoListener(new OnTimeInfoListener() {
            @Override
            public void onTimeInfo(TimeInfoBean timeInfoBean) {
                Log.d(TAG,"play time:"+timeInfoBean.toString());
            }
        });
        audioPlayer.setOnCutPcmDataListener(new OnCutPcmDataListener() {
            @Override
            public void onCutPcmData(byte[] buffer, int size) {
                Log.d(TAG,"bufferSize="+size);
            }

            @Override
            public void onPcmRateSample(int sample_rate, int bit, int channels) {
                Log.d(TAG,"sample_rate="+sample_rate+", bit="+bit+", channels="+channels);
            }
        });
    }

    public void cutAudio(View view) {
        Log.d(TAG, "do button +");
//        wePlayer.setSource("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
        audioPlayer.setSource("/sdcard/Music/周杰伦-回到过去.mp3");
//        wePlayer.setSource("/sdcard/extra_high.Linear48St.ape");
//        wePlayer.setSource("/mnt/sdcard/1mydream.mp3");

        audioPlayer.prepared();
        Log.d(TAG, "do button -");
    }

    private void checkPermission() {
        Log.d(TAG, "checkPermission +");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            for (String permission : permissions) {
                if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(this, permissions, 200);
                    return;
                }
            }
        }
        Log.d(TAG, "checkPermission -");
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && requestCode == 200) {
            for (int i = 0; i < permissions.length; i++) {
                if (grantResults[i] != PackageManager.PERMISSION_GRANTED) {
                    Intent intent = new Intent();
                    intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                    Uri uri = Uri.fromParts("package", getPackageName(), null);
                    intent.setData(uri);
                    startActivityForResult(intent, 200);
                    return;
                }
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 200 && resultCode == RESULT_OK) {
            checkPermission();
        }
    }
}
