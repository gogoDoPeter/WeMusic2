package com.peter.myplayer.listener;

public interface OnCutPcmDataListener {
    void onCutPcmData(byte[] buffer, int size);
    void onPcmRateSample(int sample_rate, int bit, int channels);
}
