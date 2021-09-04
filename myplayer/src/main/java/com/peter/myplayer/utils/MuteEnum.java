package com.peter.myplayer.utils;

public enum MuteEnum {
    MUTE_RIGHT("RIGHT", 0), //右声道
    MUTE_LEFT("LEFT", 1),   //左声道
    MUTE_STEREO("STEREO", 2);   //立体声

    private String name;
    private int value;

    MuteEnum(String name, int value) {
        this.name = name;
        this.value = value;
    }

    public int getValue() {
        return value;
    }
}
