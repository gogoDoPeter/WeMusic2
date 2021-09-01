package com.peter.wemusic;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.peter.myplayer.listener.OnPreparedListener;
import com.peter.myplayer.player.WeAudioPlayer;
import com.peter.myplayer.utils.MyLog;

public class MainActivity extends AppCompatActivity {
    private static final String TAG="my_tag_"+MainActivity.class.getSimpleName();
    private WeAudioPlayer wePlayer;
    private static final String[] permissions = new String[]{
            Manifest.permission.INTERNET,
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG,"onCreate +");
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        checkPermission();

        wePlayer=new WeAudioPlayer();

        Log.d(TAG,"onCreate setOnPreparedListener");
        wePlayer.setOnPreparedListener(new OnPreparedListener() {
            @Override
            public void onPrepared() {
                MyLog.d("准备好了，可以开始播放声音了");
                wePlayer.start();
            }
        });

        Log.d(TAG,"onCreate -");

    }


    public void begin(View view) {
        Log.d(TAG,"do button +");
        wePlayer.setSource("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");

//        wePlayer.setSource("/mnt/sdcard/1mydream.mp3");

        wePlayer.prepared();
        Log.d(TAG,"do button -");
    }

    private void checkPermission() {
        Log.d(TAG,"checkPermission +");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            for (String permission : permissions) {
                if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(this, permissions, 200);
                    return;
                }
            }
        }
        Log.d(TAG,"checkPermission -");
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