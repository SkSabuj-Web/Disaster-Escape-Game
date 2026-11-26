#include "raylib.h"
#include "graph.h"
#include "player.h"
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

void InitializePlayer(Player *player)
{
    player->currentNode = 0;
    player->previousNode = 0;
    player->lifePoints = 100;
    player->displayLifePoints = 100.0f;
    player->status = GAME_PLAYING;
    player->movesCount = 0;
    player->totalCostPaid = 0;
    player->survivalScore = 0;
    player->visualPos = nodes[0].position;
    player->animStartPos = nodes[0].position;
    player->moveAnimProgress = 1.0f;
    player->isMovingAnim = false;
    player->gameOverReason[0] = '\0';
}

static int SelectDirectionalNeighbor(int current, int direction)
{
    int nextNode = -1;
    float bestScore = -1.0f;

    for (int i = 0; i < NODE_COUNT; i++)
    {
        if (i == current || graph[current][i] == INT_MAX)
            continue;

        float dx = nodes[i].position.x - nodes[current].position.x;
        float dy = nodes[i].position.y - nodes[current].position.y;

        float forward = 0.0f;
        float side = 0.0f;

        if (direction == 0)
        {
            forward = dx;
            side = fabsf(dy);
        }
        else if (direction == 1)
        {
            forward = -dx;
            side = fabsf(dy);
        }
        else if (direction == 2)
        {
            forward = -dy;
            side = fabsf(dx);

            // Allow upward shortcut from Police Station (F) directly onto Safe Zone (H)
            // when F-H is open, falling back to Market (D) if F-H is blocked.
            if (current == 5 && i == 7)
            {
                return i;
            }
        }
        else if (direction == 3)
        {
            forward = dy;
            side = fabsf(dx);
        }

        if (forward <= 0.0f || forward < side)
            continue;

        float score = forward - side * 0.5f;
        if (score > bestScore)
        {
            bestScore = score;
            nextNode = i;
        }
    }

    return nextNode;
}

static int SelectClickedNeighbor(int current, Rectangle mapBounds)
{
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        return -1;

    Vector2 mousePos = GetMousePosition();
    if (!CheckCollisionPointRec(mousePos, mapBounds))
        return -1;

    for (int i = 0; i < NODE_COUNT; i++)
    {
        if (i == current || graph[current][i] == INT_MAX)
            continue;

        if (CheckCollisionPointCircle(mousePos, nodes[i].position, 28.0f))
            return i;
    }

    return -1;
}

static void ExecuteMove(Player *player, int nextNode)
{
    int current = player->currentNode;
    int cost = graph[current][nextNode];

    if (cost == INT_MAX)
        return;

    // Prioritize victory when reaching Safe Zone H if player has enough Life Points
    // (including when the move consumes their exact remaining Life Points).
    if (nextNode == NODE_COUNT - 1)
    {
        if (player->lifePoints >= cost)
        {
            player->lifePoints -= cost;
            player->totalCostPaid += cost;
            player->previousNode = current;
            player->currentNode = nextNode;
            player->animStartPos = player->visualPos;
            player->moveAnimProgress = 0.0f;
            player->isMovingAnim = true;
            player->movesCount++;
            player->survivalScore += 500 + player->lifePoints * 10;
            player->status = GAME_WON;
            return;
        }

        player->lifePoints = 0;
        player->status = GAME_OVER;
        snprintf(player->gameOverReason, sizeof(player->gameOverReason),
                 "Exhausted Life Points before reaching Safe Zone H!");
        return;
    }

    if (cost >= player->lifePoints)
    {
        player->lifePoints = 0;
        player->status = GAME_OVER;
        snprintf(player->gameOverReason, sizeof(player->gameOverReason),
                 "Ran out of Life Points traveling from %c to %c!",
                 nodes[current].id, nodes[nextNode].id);
        return;
    }

    player->lifePoints -= cost;
    player->totalCostPaid += cost;
    player->previousNode = current;
    player->currentNode = nextNode;
    player->animStartPos = player->visualPos;
    player->moveAnimProgress = 0.0f;
    player->isMovingAnim = true;
    player->movesCount++;
    player->survivalScore += 40;
}

bool UpdatePlayer(Player *player, Rectangle mapBounds)
{
    if (player->status != GAME_PLAYING)
        return false;

    int nextNode = SelectClickedNeighbor(player->currentNode, mapBounds);

    if (nextNode == -1)
    {
        int direction = -1;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) direction = 0;
        else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) direction = 1;
        else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) direction = 2;
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) direction = 3;

        if (direction == -1)
            return false;

        nextNode = SelectDirectionalNeighbor(player->currentNode, direction);
    }

    if (nextNode == -1)
        return false;

    int prevNode = player->currentNode;
    ExecuteMove(player, nextNode);
    return (player->currentNode != prevNode);
}

void UpdatePlayerAnimation(Player *player, float dt)
{
    if (player->isMovingAnim)
    {
        player->moveAnimProgress += dt * 5.5f;
        if (player->moveAnimProgress >= 1.0f)
        {
            player->moveAnimProgress = 1.0f;
            player->isMovingAnim = false;
        }

        float t = player->moveAnimProgress;
        float eased = 1.0f - powf(1.0f - t, 3.0f);
        Vector2 target = nodes[player->currentNode].position;

        player->visualPos.x = player->animStartPos.x + (target.x - player->animStartPos.x) * eased;
        player->visualPos.y = player->animStartPos.y + (target.y - player->animStartPos.y) * eased;
    }
    else
    {
        player->visualPos = nodes[player->currentNode].position;
    }

    float targetLP = (float)player->lifePoints;
    float diff = targetLP - player->displayLifePoints;
    if (fabsf(diff) < 0.25f)
    {
        player->displayLifePoints = targetLP;
    }
    else
    {
        player->displayLifePoints += diff * fminf(1.0f, dt * 10.0f);
    }
}

void DrawPlayer(Player player, float timeSec, Rectangle mapBounds)
{
    Vector2 pos = player.visualPos;
    float pulse = 0.5f + 0.5f * sinf(timeSec * 5.0f);

    Color cyan = {0, 220, 255, 255};

    DrawCircleV(pos, 31.0f + pulse * 5.0f, Fade(cyan, 0.14f + pulse * 0.08f));
    DrawRingLinesEx(pos, 22.0f + pulse * 2.0f, 25.0f + pulse * 2.0f, 0.0f, 360.0f, 32, 1.5f, Fade(cyan, 0.75f));
    DrawCircleV(pos, 15.0f, Fade(SKYBLUE, 0.45f));
    DrawCircleV(pos, 10.0f, cyan);
    DrawCircleV(pos, 5.0f, WHITE);

    // Keep the PLAYER badge inside the map panel even on topmost nodes (A, B, H)
    const char *tag = "PLAYER";
    int fontSize = 11;
    int textW = MeasureText(tag, fontSize);
    int badgeW = textW + 12;
    int badgeH = 18;
    int badgeX = (int)pos.x - badgeW / 2;
    int badgeY = (int)pos.y - 46;

    if (badgeY < (int)mapBounds.y + 8)
    {
        badgeY = (int)pos.y - 34;
        if (badgeY < (int)mapBounds.y + 6)
            badgeY = (int)pos.y + 20;
    }

    Rectangle badge = {(float)badgeX, (float)badgeY, (float)badgeW, (float)badgeH};
    DrawRectangleRounded(badge, 0.5f, 6, (Color){10, 24, 44, 235});
    DrawRectangleRoundedLinesEx(badge, 0.5f, 6, 1.2f, cyan);
    DrawText(tag, badgeX + 6, badgeY + 4, fontSize, cyan);
}
