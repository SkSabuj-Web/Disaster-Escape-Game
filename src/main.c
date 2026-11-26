#include "raylib.h"
#include "game.h"
#include "graph.h"
#include "dijkstra.h"
#include "player.h"
#include "disaster.h"
#include "ui.h"
#include "menu.h"

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
    GameScreen screen;
    Player player;
    DisasterManager disaster;
    int shortestPath[NODE_COUNT];
    int shortestPathLength;
    int shortestCost;
    double missionTimeSec;
    float endOverlayAlpha;
    float screenFadeAlpha;
    bool shouldExit;
} GameContext;

static void RecalculateShortestPath(GameContext *ctx)
{
    ctx->shortestCost = Dijkstra(
        ctx->player.currentNode,
        NODE_COUNT - 1,
        ctx->shortestPath,
        &ctx->shortestPathLength
    );

    if (ctx->player.status == GAME_PLAYING && ctx->shortestCost == -1)
    {
        ctx->player.status = GAME_OVER;
        snprintf(ctx->player.gameOverReason, sizeof(ctx->player.gameOverReason),
                 "No valid path remains to Safe Zone H!");
    }
}

static void ResetMission(GameContext *ctx)
{
    InitializeGraph();
    InitializePlayer(&ctx->player);
    InitializeDisasterManager(&ctx->disaster);

    ctx->missionTimeSec = 0.0;
    ctx->endOverlayAlpha = 0.0f;
    RecalculateShortestPath(ctx);
}

static void SwitchScreen(GameContext *ctx, GameScreen nextScreen)
{
    ctx->screen = nextScreen;
    ctx->screenFadeAlpha = 1.0f;
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Disaster Escape - Graph-Based Survival Game");
    SetTargetFPS(60);
    SetRandomSeed((unsigned int)time(NULL));

    GameContext ctx = {0};
    ctx.screen = SCREEN_MENU;
    ctx.screenFadeAlpha = 1.0f;
    ResetMission(&ctx);

    while (!WindowShouldClose() && !ctx.shouldExit)
    {
        float dt = GetFrameTime();
        float timeSec = (float)GetTime();

        if (ctx.screenFadeAlpha > 0.0f)
        {
            ctx.screenFadeAlpha -= dt * 3.5f;
            if (ctx.screenFadeAlpha < 0.0f)
                ctx.screenFadeAlpha = 0.0f;
        }

        if (ctx.screen == SCREEN_MENU)
        {
            if (IsKeyPressed(KEY_ENTER))
            {
                ResetMission(&ctx);
                SwitchScreen(&ctx, SCREEN_GAMEPLAY);
            }
            else if (IsKeyPressed(KEY_H))
            {
                SwitchScreen(&ctx, SCREEN_HOW_TO_PLAY);
            }
        }
        else if (ctx.screen == SCREEN_HOW_TO_PLAY)
        {
            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_M))
            {
                SwitchScreen(&ctx, SCREEN_MENU);
            }
            else if (IsKeyPressed(KEY_ENTER))
            {
                ResetMission(&ctx);
                SwitchScreen(&ctx, SCREEN_GAMEPLAY);
            }
        }
        else if (ctx.screen == SCREEN_GAMEPLAY)
        {
            if (IsKeyPressed(KEY_R))
            {
                ResetMission(&ctx);
            }
            else if (IsKeyPressed(KEY_M))
            {
                SwitchScreen(&ctx, SCREEN_MENU);
            }

            if (ctx.player.status == GAME_PLAYING)
            {
                ctx.missionTimeSec += (double)dt;

                int intendedNode = GetPlayerIntendedMove(&ctx.player, GetMapBounds());
                if (intendedNode != -1)
                {
                    if (TryReactivePathTraceBlock(&ctx.disaster, &ctx.player, intendedNode))
                    {
                        RecalculateShortestPath(&ctx);
                    }
                    else if (ExecutePlayerMove(&ctx.player, intendedNode))
                    {
                        RecalculateShortestPath(&ctx);
                    }
                }

                if (UpdateDisasterManager(&ctx.disaster, &ctx.player))
                {
                    RecalculateShortestPath(&ctx);
                }
            }
            else
            {
                UpdateDisasterManager(&ctx.disaster, &ctx.player);
                ctx.endOverlayAlpha += dt * 2.5f;
                if (ctx.endOverlayAlpha > 1.0f)
                    ctx.endOverlayAlpha = 1.0f;
            }

            UpdatePlayerAnimation(&ctx.player, dt);
        }

        BeginDrawing();
        ClearBackground(UI_BG);

        if (ctx.screen == SCREEN_MENU)
        {
            MenuAction act = DrawMainMenu(timeSec);
            if (act == MENU_ACTION_START_GAME)
            {
                ResetMission(&ctx);
                SwitchScreen(&ctx, SCREEN_GAMEPLAY);
            }
            else if (act == MENU_ACTION_HOW_TO_PLAY)
            {
                SwitchScreen(&ctx, SCREEN_HOW_TO_PLAY);
            }
            else if (act == MENU_ACTION_EXIT)
            {
                ctx.shouldExit = true;
            }
        }
        else if (ctx.screen == SCREEN_HOW_TO_PLAY)
        {
            MenuAction act = DrawHowToPlayScreen(timeSec);
            if (act == MENU_ACTION_BACK_TO_MENU)
            {
                SwitchScreen(&ctx, SCREEN_MENU);
            }
            else if (act == MENU_ACTION_START_GAME)
            {
                ResetMission(&ctx);
                SwitchScreen(&ctx, SCREEN_GAMEPLAY);
            }
        }
        else if (ctx.screen == SCREEN_GAMEPLAY)
        {
            UIAction uiAct = DrawGameUI(
                &ctx.player,
                &ctx.disaster,
                ctx.shortestPath,
                ctx.shortestPathLength,
                ctx.shortestCost,
                ctx.missionTimeSec,
                ctx.endOverlayAlpha
            );

            if (uiAct.restartClicked)
            {
                ResetMission(&ctx);
            }
            else if (uiAct.menuClicked)
            {
                SwitchScreen(&ctx, SCREEN_MENU);
            }
        }

        if (ctx.screenFadeAlpha > 0.0f)
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(UI_BG, ctx.screenFadeAlpha));
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
