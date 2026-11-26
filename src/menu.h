#ifndef MENU_H
#define MENU_H

#include "game.h"
#include <stdbool.h>

typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_START_GAME,
    MENU_ACTION_HOW_TO_PLAY,
    MENU_ACTION_BACK_TO_MENU,
    MENU_ACTION_EXIT
} MenuAction;

MenuAction DrawMainMenu(float timeSec);
MenuAction DrawHowToPlayScreen(float timeSec);

#endif
