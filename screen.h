#ifndef SCREEN_H
#define SCREEN_H
#include "commons.h"

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_QUIT
};

int Menu(void);
void LoadingScreen(const char* msg);

#endif