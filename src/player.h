#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include <stdbool.h>

typedef struct
{
    int currentNode;
    int previousNode;
    int lifePoints;
    float displayLifePoints;
    int status;
    int movesCount;
    int totalCostPaid;
    int survivalScore;
    Vector2 visualPos;
    Vector2 animStartPos;
    float moveAnimProgress;
    bool isMovingAnim;
    char gameOverReason[96];
} Player;

#define GAME_PLAYING 0
#define GAME_WON 1
#define GAME_OVER 2
#define MAX_LIFE_POINTS 50

void InitializePlayer(Player *player);
int GetPlayerIntendedMove(const Player *player, Rectangle mapBounds);
bool ExecutePlayerMove(Player *player, int nextNode);
void UpdatePlayerAnimation(Player *player, float dt);
void DrawPlayer(Player player, float timeSec, Rectangle mapBounds);

#endif
