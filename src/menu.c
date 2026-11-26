#include "menu.h"
#include "ui.h"
#include "graph.h"
#include <math.h>

static void DrawAnimatedNetworkBackdrop(float timeSec)
{
    // Tactical grid background
    Color gridCol = {18, 28, 48, 170};
    for (int x = 0; x < SCREEN_WIDTH; x += 60)
        DrawLine(x, 0, x, SCREEN_HEIGHT, gridCol);
    for (int y = 0; y < SCREEN_HEIGHT; y += 60)
        DrawLine(0, y, SCREEN_WIDTH, y, gridCol);

    // Subtle decorative graph edges & nodes in background
    Vector2 offset = {240.0f, 80.0f};
    for (int i = 0; i < EDGE_COUNT; i++)
    {
        Vector2 a = {nodes[edges[i].from].position.x + offset.x, nodes[edges[i].from].position.y + offset.y};
        Vector2 b = {nodes[edges[i].to].position.x + offset.x, nodes[edges[i].to].position.y + offset.y};
        DrawLineEx(a, b, 2.0f, Fade(UI_CYAN, 0.08f));
    }

    for (int i = 0; i < NODE_COUNT; i++)
    {
        Vector2 p = {nodes[i].position.x + offset.x, nodes[i].position.y + offset.y};
        float pulse = 0.5f + 0.5f * sinf(timeSec * 2.0f + (float)i * 0.7f);
        DrawCircleV(p, 16.0f + pulse * 4.0f, Fade(UI_CYAN, 0.05f));
        DrawCircleV(p, 5.0f, Fade(UI_CYAN, 0.20f));
    }
}

MenuAction DrawMainMenu(float timeSec)
{
    DrawAnimatedNetworkBackdrop(timeSec);

    Rectangle card = {260, 100, 680, 520};
    DrawRectangleRounded(card, 0.05f, 10, Fade(UI_PANEL, 0.94f));
    DrawRectangleRoundedLinesEx(card, 0.05f, 10, 2.0f, Fade(UI_CYAN, 0.65f));

    // Top badge
    Rectangle tag = {465, 130, 270, 28};
    DrawRectangleRounded(tag, 0.5f, 6, Fade(UI_CYAN, 0.14f));
    DrawRectangleRoundedLinesEx(tag, 0.5f, 6, 1.0f, Fade(UI_CYAN, 0.6f));
    DrawCenteredText("ALGORITHMS LAB SURVIVAL SIM", 600, 138, 12, UI_CYAN);

    // Title
    DrawCenteredText("DISASTER ESCAPE", 600, 178, 48, UI_WHITE);
    DrawCenteredText("A GRAPH-BASED SURVIVAL GAME", 600, 236, 16, UI_CYAN);

    DrawLineEx((Vector2){340, 274}, (Vector2){860, 274}, 1.5f, UI_BORDER);

    // Short game description
    DrawCenteredText("Navigate the city graph from Starting Point A to Safe Zone H.", 600, 294, 16, UI_WHITE);
    DrawCenteredText("Dynamic disasters strike unpredictably — follow Dijkstra's", 600, 318, 15, UI_GRAY);
    DrawCenteredText("recalculated shortest path and preserve your Life Points.", 600, 340, 15, UI_GRAY);

    // Buttons
    Rectangle startBtn = {430, 390, 340, 52};
    Rectangle howBtn   = {430, 458, 340, 48};
    Rectangle exitBtn  = {430, 522, 340, 48};

    if (DrawFuturisticButton(startBtn, "START GAME", UI_GREEN, true))
        return MENU_ACTION_START_GAME;
    if (DrawFuturisticButton(howBtn, "HOW TO PLAY", UI_CYAN, false))
        return MENU_ACTION_HOW_TO_PLAY;
    if (DrawFuturisticButton(exitBtn, "EXIT", UI_RED, false))
        return MENU_ACTION_EXIT;

    DrawCenteredText("Powered by Raylib 6.2 & Dijkstra's Shortest Path Algorithm", 600, 646, 13, UI_GRAY);

    return MENU_ACTION_NONE;
}

MenuAction DrawHowToPlayScreen(float timeSec)
{
    DrawAnimatedNetworkBackdrop(timeSec);

    Rectangle card = {210, 66, 780, 588};
    DrawRectangleRounded(card, 0.04f, 10, Fade(UI_PANEL, 0.96f));
    DrawRectangleRoundedLinesEx(card, 0.04f, 10, 2.0f, Fade(UI_CYAN, 0.65f));

    DrawCenteredText("HOW TO PLAY", 600, 94, 34, UI_WHITE);
    DrawCenteredText("MISSION BRIEFING & SURVIVAL RULES", 600, 136, 13, UI_CYAN);
    DrawLineEx((Vector2){260, 164}, (Vector2){940, 164}, 1.4f, UI_BORDER);

    const char *rules[8] = {
        "Start your evacuation at Location A (Starting Point).",
        "Reach Safe Zone H to complete the mission.",
        "Use Arrow Keys (or click a glowing adjacent node) to move.",
        "Every road displays a travel cost (edge weight).",
        "Traveling along a road deducts its cost from your Life Points.",
        "Random disasters can block roads, cause floods, or trigger earthquakes.",
        "Follow the glowing green shortest path recalculated by Dijkstra's algorithm.",
        "Press R anytime to restart with a fresh city graph."
    };

    Color badgeColors[8] = {
        UI_GREEN, UI_CYAN, UI_WHITE, UI_ORANGE,
        UI_ORANGE, UI_RED, UI_GREEN, UI_CYAN
    };

    for (int i = 0; i < 8; i++)
    {
        int rowY = 184 + i * 48;
        Rectangle rowBox = {260, (float)rowY, 680, 40};
        DrawRectangleRounded(rowBox, 0.2f, 6, UI_PANEL_DARK);
        DrawRectangleRoundedLinesEx(rowBox, 0.2f, 6, 1.0f, UI_BORDER);

        DrawCircle(286, rowY + 20, 12, Fade(badgeColors[i], 0.22f));
        DrawCircleLinesEx((Vector2){286.0f, (float)(rowY + 20)}, 12.0f, 1.3f, badgeColors[i]);
        DrawCenteredText(TextFormat("%d", i + 1), 286, rowY + 13, 14, badgeColors[i]);

        DrawText(rules[i], 314, rowY + 12, 15, UI_WHITE);
    }

    Rectangle backBtn  = {310, 586, 260, 48};
    Rectangle startBtn = {630, 586, 260, 48};

    if (DrawFuturisticButton(backBtn, "BACK TO MENU", UI_CYAN, false))
        return MENU_ACTION_BACK_TO_MENU;
    if (DrawFuturisticButton(startBtn, "START MISSION", UI_GREEN, true))
        return MENU_ACTION_START_GAME;

    return MENU_ACTION_NONE;
}
