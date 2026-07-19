#ifndef SCREEN_H
#define SCREEN_H
#include "common.h"
#include "hud.h"

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_QUIT
};

int Menu(void* list, void*& fbp0);
void LoadingScreen(const char* msg, void* list, void*& fbp0);

#endif