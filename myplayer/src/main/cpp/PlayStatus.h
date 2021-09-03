//
// Created by lz6600 on 2021/8/31.
//

#ifndef WEMUSIC_PLAYSTATUS_H
#define WEMUSIC_PLAYSTATUS_H


class PlayStatus {
public:
    bool exit = false;
    bool load = true;
    bool seekStatus =false;

public:
    PlayStatus();

    ~PlayStatus();
};


#endif //WEMUSIC_PLAYSTATUS_H
