#ifndef UI_H
#define UI_H

#include "raylib.h"
#include "player.h"
#include "disaster.h"
#include "game.h"
#include <stdbool.h>

typedef struct {
    bool restartClicked;
    bool menuClicked;
} UIAction;

// Theme colors
extern const Color UI_BG;
extern const Color UI_PANEL;
extern const Color UI_PANEL_DARK;
extern const Color UI_BORDER;
extern const Color UI_CYAN;
extern const Color UI_GREEN;
extern const Color UI_RED;
extern const Color UI_ROAD;
extern const Color UI_ORANGE;
extern const Color UI_WHITE;
extern const Color UI_GRAY;

Rectangle GetMapBounds(void);
void DrawCenteredText(const char *text, int centerX, int y, int fontSize, Color color);
bool DrawFuturisticButton(Rectangle rect, const char *label, Color accentColor, bool isPrimary);

UIAction DrawGameUI(
    const Player *player,
    const DisasterManager *dm,
    const int shortestPath[],
    int shortestPathLength,
    int shortestCost,
    double missionTimeSec,
    float overlayAlpha
);

#endif
