#include "ui.h"
#include "graph.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

const Color UI_BG         = {10, 16, 32, 255};
const Color UI_PANEL      = {19, 29, 48, 255};
const Color UI_PANEL_DARK = {13, 21, 36, 255};
const Color UI_BORDER     = {38, 56, 84, 255};
const Color UI_CYAN       = {0, 220, 255, 255};
const Color UI_GREEN      = {40, 230, 130, 255};
const Color UI_RED        = {255, 59, 79, 255};
const Color UI_ROAD       = {48, 67, 91, 255};
const Color UI_ORANGE     = {255, 170, 55, 255};
const Color UI_WHITE      = {242, 247, 255, 255};
const Color UI_GRAY       = {156, 176, 201, 255};

Rectangle GetMapBounds(void)
{
    return (Rectangle){24.0f, 84.0f, 774.0f, 512.0f};
}

void DrawCenteredText(const char *text, int centerX, int y, int fontSize, Color color)
{
    int w = MeasureText(text, fontSize);
    DrawText(text, centerX - w / 2, y, fontSize, color);
}

bool DrawFuturisticButton(Rectangle rect, const char *label, Color accentColor, bool isPrimary)
{
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    bool pressed = hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool clicked = hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    Color fill = isPrimary
        ? (hovered ? Fade(accentColor, 0.28f) : Fade(accentColor, 0.16f))
        : (hovered ? (Color){28, 44, 72, 255} : UI_PANEL_DARK);

    if (pressed)
        fill = Fade(accentColor, 0.36f);

    Color border = hovered ? accentColor : Fade(accentColor, isPrimary ? 0.85f : 0.45f);

    if (hovered)
    {
        Rectangle glow = {rect.x - 2, rect.y - 2, rect.width + 4, rect.height + 4};
        DrawRectangleRounded(glow, 0.25f, 8, Fade(accentColor, 0.15f));
    }

    DrawRectangleRounded(rect, 0.25f, 8, fill);
    DrawRectangleRoundedLinesEx(rect, 0.25f, 8, hovered ? 2.0f : 1.4f, border);

    int fontSize = (rect.height >= 46.0f) ? 18 : 14;
    int textW = MeasureText(label, fontSize);
    int textX = (int)(rect.x + (rect.width - (float)textW) * 0.5f);
    int textY = (int)(rect.y + (rect.height - (float)fontSize) * 0.5f);

    DrawText(label, textX, textY, fontSize, hovered ? WHITE : UI_WHITE);

    return clicked;
}

static bool IsShortestPathEdge(int u, int v, const int shortestPath[], int shortestPathLength)
{
    for (int i = 0; i < shortestPathLength - 1; i++)
    {
        int a = shortestPath[i];
        int b = shortestPath[i + 1];
        if ((a == u && b == v) || (a == v && b == u))
            return true;
    }
    return false;
}

static void DrawDashedLine(Vector2 start, Vector2 end, float dashLen, float gapLen, float thick, Color color)
{
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f)
        return;

    float ux = dx / len;
    float uy = dy / len;
    float pos = 0.0f;

    while (pos < len)
    {
        float segEnd = fminf(pos + dashLen, len);
        Vector2 p1 = {start.x + ux * pos, start.y + uy * pos};
        Vector2 p2 = {start.x + ux * segEnd, start.y + uy * segEnd};
        DrawLineEx(p1, p2, thick, color);
        pos += dashLen + gapLen;
    }
}

static void DrawMapPanelBackground(Rectangle mapBounds, const DisasterManager *dm, float timeSec)
{
    DrawRectangleRounded(mapBounds, 0.03f, 8, (Color){12, 20, 36, 255});

    BeginScissorMode((int)mapBounds.x, (int)mapBounds.y, (int)mapBounds.width, (int)mapBounds.height);

    // Subtle tactical grid
    Color gridCol = {24, 38, 62, 160};
    for (int x = (int)mapBounds.x + 36; x < (int)(mapBounds.x + mapBounds.width); x += 48)
    {
        DrawLine(x, (int)mapBounds.y, x, (int)(mapBounds.y + mapBounds.height), gridCol);
    }
    for (int y = (int)mapBounds.y + 36; y < (int)(mapBounds.y + mapBounds.height); y += 48)
    {
        DrawLine((int)mapBounds.x, y, (int)(mapBounds.x + mapBounds.width), y, gridCol);
    }

    // Flood visual overlay effect on map
    if (dm->floodEffectTimer > 0.0f)
    {
        float alpha = fminf(1.0f, dm->floodEffectTimer / 1.0f) * 0.14f;
        DrawRectangleRec(mapBounds, Fade((Color){0, 140, 255, 255}, alpha));

        for (int i = 0; i < 6; i++)
        {
            float waveY = mapBounds.y + 55.0f + (float)i * 80.0f + sinf(timeSec * 3.0f + (float)i) * 8.0f;
            DrawLineEx(
                (Vector2){mapBounds.x + 12.0f, waveY},
                (Vector2){mapBounds.x + mapBounds.width - 12.0f, waveY},
                2.0f,
                Fade(UI_CYAN, alpha * 1.4f)
            );
        }
    }

    // Earthquake expanding shockwave rings
    if (dm->quakeWaveTimer > 0.0f)
    {
        float progress = 1.0f - (dm->quakeWaveTimer / 1.25f);
        float radius = 25.0f + progress * 340.0f;
        float alpha = (1.0f - progress) * 0.65f;
        DrawRingLinesEx(dm->quakeOrigin, radius, radius + 3.5f, 0.0f, 360.0f, 48, 3.0f, Fade(UI_ORANGE, alpha));
        if (progress > 0.18f)
        {
            float r2 = (progress - 0.18f) * 320.0f;
            DrawRingLinesEx(dm->quakeOrigin, r2, r2 + 2.0f, 0.0f, 360.0f, 48, 2.0f, Fade(UI_RED, alpha * 0.8f));
        }
    }

    EndScissorMode();

    Color borderCol = UI_BORDER;
    if (dm->phase == DISASTER_PHASE_WARNING)
    {
        float blink = 0.5f + 0.5f * sinf(timeSec * 12.0f);
        borderCol = Fade(UI_ORANGE, 0.55f + 0.45f * blink);
    }
    DrawRectangleRoundedLinesEx(mapBounds, 0.03f, 8, 1.8f, borderCol);
}

static void DrawRoads(const int shortestPath[], int shortestPathLength, float timeSec)
{
    for (int i = 0; i < EDGE_COUNT; i++)
    {
        int u = edges[i].from;
        int v = edges[i].to;
        Vector2 start = nodes[u].position;
        Vector2 end = nodes[v].position;
        bool isShortest = IsShortestPathEdge(u, v, shortestPath, shortestPathLength);

        // Base dark road casing
        DrawLineEx(start, end, 9.0f, (Color){16, 28, 46, 255});

        if (edges[i].blocked)
        {
            DrawLineEx(start, end, 4.0f, Fade(UI_RED, 0.25f));
            DrawDashedLine(start, end, 10.0f, 8.0f, 3.5f, UI_RED);
        }
        else if (edges[i].flooded)
        {
            float pulse = 0.5f + 0.5f * sinf(timeSec * 6.0f);
            DrawLineEx(start, end, 8.0f, Fade(UI_CYAN, 0.22f + 0.14f * pulse));
            DrawLineEx(start, end, 4.0f, isShortest ? UI_GREEN : (Color){0, 175, 255, 255});
        }
        else if (isShortest)
        {
            float pulse = 0.5f + 0.5f * sinf(timeSec * 4.5f);
            DrawLineEx(start, end, 10.0f, Fade(UI_GREEN, 0.22f + 0.10f * pulse));
            DrawLineEx(start, end, 4.5f, UI_GREEN);
        }
        else
        {
            DrawLineEx(start, end, 3.5f, UI_ROAD);
        }
    }

    // Animated directional energy pulses along the shortest path toward H
    for (int i = 0; i < shortestPathLength - 1; i++)
    {
        Vector2 a = nodes[shortestPath[i]].position;
        Vector2 b = nodes[shortestPath[i + 1]].position;
        float t = fmodf(timeSec * 1.1f - (float)i * 0.25f + 10.0f, 1.0f);
        Vector2 dotPos = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
        DrawCircleV(dotPos, 5.5f, Fade(UI_GREEN, 0.45f));
        DrawCircleV(dotPos, 3.0f, WHITE);
    }

    // Draw road weight / blocked badges above lines
    for (int i = 0; i < EDGE_COUNT; i++)
    {
        int u = edges[i].from;
        int v = edges[i].to;
        Vector2 start = nodes[u].position;
        Vector2 end = nodes[v].position;
        bool isShortest = IsShortestPathEdge(u, v, shortestPath, shortestPathLength);

        Vector2 mid = {(start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f};

        if (edges[i].blocked)
        {
            DrawCircleV(mid, 14.0f, (Color){44, 14, 22, 245});
            DrawCircleLinesEx(mid, 14.0f, 1.8f, UI_RED);
            DrawLineEx((Vector2){mid.x - 5, mid.y - 5}, (Vector2){mid.x + 5, mid.y + 5}, 2.4f, UI_RED);
            DrawLineEx((Vector2){mid.x + 5, mid.y - 5}, (Vector2){mid.x - 5, mid.y + 5}, 2.4f, UI_RED);
        }
        else
        {
            const char *costText = edges[i].flooded
                ? TextFormat("%d*", edges[i].weight)
                : TextFormat("%d", edges[i].weight);
            int textW = MeasureText(costText, 13);
            float pillW = (float)textW + 14.0f;
            if (pillW < 26.0f)
                pillW = 26.0f;
            Rectangle pill = {mid.x - pillW * 0.5f, mid.y - 11.0f, pillW, 22.0f};

            Color pillBg = edges[i].flooded ? (Color){12, 42, 68, 245} : UI_PANEL;
            Color pillBorder = edges[i].flooded
                ? UI_CYAN
                : (isShortest ? UI_GREEN : UI_BORDER);
            Color textCol = edges[i].flooded
                ? UI_CYAN
                : (isShortest ? UI_GREEN : UI_WHITE);

            DrawRectangleRounded(pill, 0.5f, 6, pillBg);
            DrawRectangleRoundedLinesEx(pill, 0.5f, 6, 1.3f, pillBorder);
            DrawText(costText, (int)mid.x - textW / 2, (int)mid.y - 6, 13, textCol);
        }
    }
}

static void DrawNodes(const Player *player, float timeSec)
{
    Vector2 mousePos = GetMousePosition();
    float pulse = 0.5f + 0.5f * sinf(timeSec * 4.0f);

    for (int i = 0; i < NODE_COUNT; i++)
    {
        Vector2 p = nodes[i].position;
        Color nodeColor = UI_ORANGE;

        if (i == 0)
            nodeColor = UI_GREEN;
        else if (i == NODE_COUNT - 1)
            nodeColor = UI_CYAN;

        bool isReachable = (player->status == GAME_PLAYING &&
                            i != player->currentNode &&
                            graph[player->currentNode][i] != INT_MAX);
        bool hovered = isReachable && CheckCollisionPointCircle(mousePos, p, 28.0f);

        // Reachable open neighbor indicator ring
        if (isReachable)
        {
            float ringR = hovered ? 28.0f : (24.0f + pulse * 2.0f);
            DrawRingLinesEx(p, ringR, ringR + 1.8f, 0.0f, 360.0f, 32, 1.4f,
                            Fade(hovered ? UI_WHITE : UI_CYAN, hovered ? 0.90f : 0.40f));
        }

        // Safe Zone beacon pulse
        if (i == NODE_COUNT - 1)
        {
            DrawCircleV(p, 29.0f + pulse * 4.0f, Fade(UI_CYAN, 0.14f + 0.06f * pulse));
        }

        DrawCircleV(p, 24.0f, Fade(nodeColor, 0.15f));
        DrawCircleV(p, 17.0f, Fade(nodeColor, 0.30f));
        DrawCircleV(p, 12.0f, nodeColor);
        DrawCircleLinesEx(p, 12.0f, 1.5f, Fade(WHITE, 0.65f));

        const char *idStr = TextFormat("%c", nodes[i].id);
        int idW = MeasureText(idStr, 16);
        DrawText(idStr, (int)p.x - idW / 2, (int)p.y - 8, 16, UI_BG);

        // Node name pill below node
        int nameW = MeasureText(nodes[i].name, 12);
        Rectangle nameTag = {p.x - (float)nameW * 0.5f - 7.0f, p.y + 26.0f, (float)nameW + 14.0f, 18.0f};
        DrawRectangleRounded(nameTag, 0.4f, 6, Fade(UI_BG, 0.86f));
        DrawRectangleRoundedLinesEx(nameTag, 0.4f, 6, 1.0f, Fade(nodeColor, 0.35f));
        DrawText(nodes[i].name, (int)p.x - nameW / 2, (int)p.y + 29, 12, UI_WHITE);
    }
}

static void DrawDisasterBanner(Rectangle mapBounds, const DisasterManager *dm)
{
    if (!dm->alertActive || dm->alertDuration <= 0.0f)
        return;

    float elapsed = dm->alertDuration - dm->alertTimer;
    float alpha = 1.0f;
    if (elapsed < 0.25f)
        alpha = elapsed / 0.25f;
    else if (dm->alertTimer < 0.60f)
        alpha = dm->alertTimer / 0.60f;
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    float bannerW = 540.0f;
    float bannerH = 68.0f;
    Rectangle box = {
        mapBounds.x + (mapBounds.width - bannerW) * 0.5f,
        mapBounds.y + 12.0f,
        bannerW,
        bannerH
    };

    DrawRectangleRounded(box, 0.22f, 8, Fade((Color){14, 22, 38, 255}, 0.94f * alpha));
    DrawRectangleRoundedLinesEx(box, 0.22f, 8, 2.0f, Fade(dm->alertColor, alpha));

    // Left accent strip
    Rectangle strip = {box.x + 8.0f, box.y + 10.0f, 5.0f, box.height - 20.0f};
    DrawRectangleRounded(strip, 0.5f, 4, Fade(dm->alertColor, alpha));

    // Impact badge on right
    int impactW = MeasureText(dm->alertImpact, 12);
    Rectangle impactBox = {box.x + box.width - (float)impactW - 30.0f, box.y + 13.0f, (float)impactW + 18.0f, 22.0f};
    DrawRectangleRounded(impactBox, 0.4f, 6, Fade(dm->alertColor, 0.22f * alpha));
    DrawRectangleRoundedLinesEx(impactBox, 0.4f, 6, 1.2f, Fade(dm->alertColor, alpha));
    DrawText(dm->alertImpact, (int)impactBox.x + 9, (int)impactBox.y + 5, 12, Fade(dm->alertColor, alpha));

    DrawText(dm->alertTitle, (int)box.x + 24, (int)box.y + 13, 18, Fade(dm->alertColor, alpha));
    DrawText(dm->alertMessage, (int)box.x + 24, (int)box.y + 39, 14, Fade(UI_WHITE, alpha));
}

static void DrawHeader(const Player *player, const DisasterManager *dm, double missionTimeSec, UIAction *action)
{
    DrawText("DISASTER ESCAPE", 26, 16, 26, UI_WHITE);
    DrawText("GRAPH-BASED SURVIVAL GAME", 28, 48, 12, UI_CYAN);

    // Status pill
    const char *statusStr = "ACTIVE";
    Color statusCol = UI_GREEN;
    if (player->status == GAME_WON)
    {
        statusStr = "MISSION COMPLETE";
        statusCol = UI_CYAN;
    }
    else if (player->status == GAME_OVER)
    {
        statusStr = "MISSION FAILED";
        statusCol = UI_RED;
    }
    else if (dm->phase == DISASTER_PHASE_WARNING)
    {
        statusStr = "DISASTER INCOMING";
        statusCol = UI_ORANGE;
    }

    Rectangle statusPill = {590, 22, 180, 34};
    DrawRectangleRounded(statusPill, 0.35f, 8, UI_PANEL);
    DrawRectangleRoundedLinesEx(statusPill, 0.35f, 8, 1.3f, Fade(statusCol, 0.8f));
    DrawCircle(608, 39, 5, statusCol);
    DrawText(statusStr, 622, 32, 13, statusCol);

    // Mission timer pill
    int mins = (int)missionTimeSec / 60;
    double secs = missionTimeSec - (double)(mins * 60);
    const char *timeText = TextFormat("TIME  %02d:%04.1f", mins, secs);
    Rectangle timePill = {784, 22, 132, 34};
    DrawRectangleRounded(timePill, 0.35f, 8, UI_PANEL);
    DrawRectangleRoundedLinesEx(timePill, 0.35f, 8, 1.3f, UI_BORDER);
    DrawCenteredText(timeText, (int)(timePill.x + timePill.width * 0.5f), 32, 14, UI_WHITE);

    // Quick Restart button
    Rectangle restartBtn = {930, 22, 112, 34};
    if (DrawFuturisticButton(restartBtn, "RESTART (R)", UI_ORANGE, false))
        action->restartClicked = true;

    // Menu button
    Rectangle menuBtn = {1054, 22, 122, 34};
    if (DrawFuturisticButton(menuBtn, "MENU (M)", UI_CYAN, false))
        action->menuClicked = true;

    DrawLineEx((Vector2){24, 72}, (Vector2){1176, 72}, 1.5f, UI_BORDER);
}

static void DrawSidebar(
    const Player *player,
    const DisasterManager *dm,
    const int shortestPath[],
    int shortestPathLength,
    int shortestCost,
    float timeSec
)
{
    Rectangle panel = {814, 84, 362, 512};
    DrawRectangleRounded(panel, 0.04f, 8, UI_PANEL);
    DrawRectangleRoundedLinesEx(panel, 0.04f, 8, 1.5f, UI_BORDER);

    DrawText("MISSION DASHBOARD", 834, 102, 17, UI_CYAN);
    DrawLineEx((Vector2){834, 129}, (Vector2){1156, 129}, 1.2f, UI_BORDER);

    // Location & Destination cards
    Rectangle locCard = {834, 142, 154, 64};
    Rectangle dstCard = {1002, 142, 154, 64};
    DrawRectangleRounded(locCard, 0.16f, 6, UI_PANEL_DARK);
    DrawRectangleRoundedLinesEx(locCard, 0.16f, 6, 1.0f, UI_BORDER);
    DrawText("CURRENT", 846, 151, 11, UI_GRAY);
    DrawText(TextFormat("%c - %s", nodes[player->currentNode].id, nodes[player->currentNode].name),
             846, 171, 13, UI_GREEN);

    DrawRectangleRounded(dstCard, 0.16f, 6, UI_PANEL_DARK);
    DrawRectangleRoundedLinesEx(dstCard, 0.16f, 6, 1.0f, UI_BORDER);
    DrawText("DESTINATION", 1014, 151, 11, UI_GRAY);
    DrawText("H - Safe Zone", 1014, 171, 13, UI_CYAN);

    // Life Points section
    DrawText("LIFE POINTS", 834, 222, 13, UI_ORANGE);
    Color lpCol = (player->lifePoints > 50) ? UI_GREEN : ((player->lifePoints > 25) ? UI_ORANGE : UI_RED);
    const char *lpStr = TextFormat("%d / 100", player->lifePoints);
    int lpW = MeasureText(lpStr, 22);
    DrawText(lpStr, 1156 - lpW, 217, 22, lpCol);

    Rectangle barBg = {834, 248, 322, 16};
    DrawRectangleRounded(barBg, 0.5f, 6, (Color){22, 32, 52, 255});
    float fillRatio = player->displayLifePoints / 100.0f;
    if (fillRatio < 0.0f) fillRatio = 0.0f;
    if (fillRatio > 1.0f) fillRatio = 1.0f;
    if (fillRatio > 0.01f)
    {
        Rectangle barFill = {836, 250, 318.0f * fillRatio, 12};
        DrawRectangleRounded(barFill, 0.5f, 6, lpCol);
    }
    DrawRectangleRoundedLinesEx(barBg, 0.5f, 6, 1.0f, UI_BORDER);

    // Dijkstra shortest path box
    Rectangle pathBox = {834, 280, 322, 86};
    DrawRectangleRounded(pathBox, 0.12f, 6, UI_PANEL_DARK);
    DrawRectangleRoundedLinesEx(pathBox, 0.12f, 6, 1.2f, Fade(UI_GREEN, 0.45f));

    DrawText("SHORTEST PATH (DIJKSTRA)", 848, 292, 12, UI_GRAY);
    if (shortestCost >= 0)
    {
        const char *costBadge = TextFormat("COST: %d", shortestCost);
        int cbW = MeasureText(costBadge, 13);
        DrawText(costBadge, 1142 - cbW, 291, 13, UI_GREEN);

        char routeBuf[128] = "";
        size_t pos = 0;
        for (int i = 0; i < shortestPathLength && pos + 8 < sizeof(routeBuf); i++)
        {
            pos += (size_t)snprintf(routeBuf + pos, sizeof(routeBuf) - pos, "%c%s",
                                    nodes[shortestPath[i]].id,
                                    (i < shortestPathLength - 1) ? " -> " : "");
        }
        DrawText(routeBuf, 848, 322, 17, UI_WHITE);
    }
    else
    {
        DrawText("UNREACHABLE", 1046, 291, 13, UI_RED);
        DrawText("No valid path to Safe Zone H!", 848, 322, 14, UI_RED);
    }

    // Metrics row (Blocked, Survived, Score)
    int blockedCount = GetBlockedRoadCount();
    Rectangle m1 = {834, 380, 100, 72};
    Rectangle m2 = {945, 380, 100, 72};
    Rectangle m3 = {1056, 380, 100, 72};

    DrawRectangleRounded(m1, 0.16f, 6, UI_PANEL_DARK);
    DrawRectangleRoundedLinesEx(m1, 0.16f, 6, 1.0f, UI_BORDER);
    DrawCenteredText("BLOCKED", 884, 392, 11, UI_GRAY);
    DrawCenteredText(TextFormat("%d / %d", blockedCount, EDGE_COUNT), 884, 415, 20,
                     blockedCount > 0 ? UI_RED : UI_WHITE);

    DrawRectangleRounded(m2, 0.16f, 6, UI_PANEL_DARK);
    DrawRectangleRoundedLinesEx(m2, 0.16f, 6, 1.0f, UI_BORDER);
    DrawCenteredText("SURVIVED", 995, 392, 11, UI_GRAY);
    DrawCenteredText(TextFormat("%d", dm->disastersSurvived), 995, 415, 20, UI_CYAN);

    DrawRectangleRounded(m3, 0.16f, 6, UI_PANEL_DARK);
    DrawRectangleRoundedLinesEx(m3, 0.16f, 6, 1.0f, UI_BORDER);
    DrawCenteredText("SCORE", 1106, 392, 11, UI_GRAY);
    DrawCenteredText(TextFormat("%d", player->survivalScore), 1106, 415, 20, UI_GREEN);

    // Disaster status card
    Rectangle disCard = {834, 468, 322, 110};
    DrawRectangleRounded(disCard, 0.12f, 6, UI_PANEL_DARK);

    if (dm->phase == DISASTER_PHASE_WARNING && player->status == GAME_PLAYING)
    {
        double rem = dm->nextTransitionTime - GetTime();
        if (rem < 0.0) rem = 0.0;
        float blink = 0.5f + 0.5f * sinf(timeSec * 14.0f);
        DrawRectangleRoundedLinesEx(disCard, 0.12f, 6, 1.6f, Fade(UI_ORANGE, 0.5f + 0.5f * blink));
        DrawText("DISASTER STATUS", 848, 480, 12, UI_ORANGE);
        DrawText(TextFormat("WARNING: %s", GetDisasterName(dm->pendingType)), 848, 505, 16, UI_ORANGE);
        DrawText(TextFormat("Impact expected in %.1fs...", rem), 848, 535, 14, UI_WHITE);
    }
    else
    {
        DrawRectangleRoundedLinesEx(disCard, 0.12f, 6, 1.2f, UI_BORDER);
        DrawText("DISASTER STATUS", 848, 480, 12, UI_GRAY);
        if (dm->lastType == DISASTER_NONE)
        {
            DrawText("ALL SYSTEMS NOMINAL", 848, 507, 16, UI_GREEN);
            DrawText("Monitoring seismic & flood sensors", 848, 536, 13, UI_GRAY);
        }
        else
        {
            DrawText(TextFormat("LAST: %s", GetDisasterName(dm->lastType)), 848, 507, 15, UI_CYAN);
            DrawText(TextFormat("Total Events: %d  |  Moves: %d",
                                dm->disastersTriggered, player->movesCount),
                     848, 536, 13, UI_GRAY);
        }
    }
}

static void DrawBottomBar(void)
{
    Rectangle bar = {24, 608, 1152, 98};
    DrawRectangleRounded(bar, 0.06f, 8, UI_PANEL);
    DrawRectangleRoundedLinesEx(bar, 0.06f, 8, 1.4f, UI_BORDER);

    // Controls row
    DrawText("CONTROLS:", 44, 622, 13, UI_CYAN);
    DrawText("ARROW KEYS / CLICK ADJACENT NODE: MOVE", 136, 622, 13, UI_WHITE);
    DrawText("R: RESTART", 516, 622, 13, UI_ORANGE);
    DrawText("M: MENU", 636, 622, 13, UI_WHITE);
    DrawText("ESC: EXIT", 734, 622, 13, UI_GRAY);
    DrawText("REACH H: COMPLETE MISSION", 864, 622, 13, UI_GREEN);

    DrawLineEx((Vector2){44, 650}, (Vector2){1156, 650}, 1.0f, UI_BORDER);

    // Legend row
    DrawText("LEGEND:", 44, 670, 13, UI_CYAN);

    // Shortest Path
    DrawLineEx((Vector2){124, 677}, (Vector2){152, 677}, 4.0f, UI_GREEN);
    DrawText("Shortest Path", 160, 670, 13, UI_WHITE);

    // Blocked Road
    DrawLineEx((Vector2){286, 677}, (Vector2){314, 677}, 3.5f, UI_RED);
    DrawText("X", 296, 669, 14, UI_RED);
    DrawText("Blocked Road", 322, 670, 13, UI_WHITE);

    // Normal Location
    DrawCircle(446, 677, 7.0f, UI_ORANGE);
    DrawText("Normal Location", 460, 670, 13, UI_WHITE);

    // Safe Zone
    DrawCircle(604, 677, 7.0f, UI_CYAN);
    DrawText("Safe Zone (H)", 618, 670, 13, UI_WHITE);

    // Starting Point
    DrawCircle(744, 677, 7.0f, UI_GREEN);
    DrawText("Starting Point (A)", 758, 670, 13, UI_WHITE);

    // Flooded Road
    DrawLineEx((Vector2){916, 677}, (Vector2){944, 677}, 4.0f, (Color){0, 175, 255, 255});
    DrawText("Flooded Road (+Cost)", 952, 670, 13, UI_WHITE);
}

static void DrawEndOverlay(
    const Player *player,
    const DisasterManager *dm,
    double missionTimeSec,
    float overlayAlpha,
    UIAction *action
)
{
    if (player->status == GAME_PLAYING || overlayAlpha <= 0.01f)
        return;

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.72f * overlayAlpha));

    bool won = (player->status == GAME_WON);
    Color accent = won ? UI_GREEN : UI_RED;

    Rectangle modal = {320, 155, 560, 410};
    DrawRectangleRounded(modal, 0.06f, 10, Fade(UI_PANEL, overlayAlpha));
    DrawRectangleRoundedLinesEx(modal, 0.06f, 10, 2.2f, Fade(accent, overlayAlpha));

    // Top glow accent bar
    Rectangle accentBar = {350, 173, 500, 4};
    DrawRectangleRounded(accentBar, 0.5f, 4, Fade(accent, 0.85f * overlayAlpha));

    if (won)
    {
        DrawCenteredText("MISSION COMPLETED!", 600, 194, 34, Fade(UI_GREEN, overlayAlpha));
        DrawCenteredText("You safely reached Safe Zone H!", 600, 240, 17, Fade(UI_WHITE, overlayAlpha));
    }
    else
    {
        DrawCenteredText("GAME OVER", 600, 194, 38, Fade(UI_RED, overlayAlpha));
        const char *reason = (player->gameOverReason[0] != '\0')
            ? player->gameOverReason
            : "You ran out of Life Points!";
        DrawCenteredText(reason, 600, 244, 16, Fade(UI_WHITE, overlayAlpha));
    }

    // Stats card
    Rectangle statsBox = {360, 282, 480, 172};
    DrawRectangleRounded(statsBox, 0.08f, 8, Fade(UI_PANEL_DARK, overlayAlpha));
    DrawRectangleRoundedLinesEx(statsBox, 0.08f, 8, 1.2f, Fade(UI_BORDER, overlayAlpha));

    int mins = (int)missionTimeSec / 60;
    double secs = missionTimeSec - (double)(mins * 60);

    if (won)
    {
        DrawText("Remaining Life Points", 386, 302, 16, Fade(UI_GRAY, overlayAlpha));
        DrawText(TextFormat("%d / 100", player->lifePoints), 710, 302, 18, Fade(UI_GREEN, overlayAlpha));

        DrawText("Total Survival Score", 386, 340, 16, Fade(UI_GRAY, overlayAlpha));
        DrawText(TextFormat("%d", player->survivalScore), 710, 340, 18, Fade(UI_CYAN, overlayAlpha));

        DrawText("Disasters Survived", 386, 378, 16, Fade(UI_GRAY, overlayAlpha));
        DrawText(TextFormat("%d", dm->disastersSurvived), 710, 378, 18, Fade(UI_ORANGE, overlayAlpha));

        DrawText("Mission Time", 386, 416, 16, Fade(UI_GRAY, overlayAlpha));
        DrawText(TextFormat("%02d:%04.1f", mins, secs), 710, 416, 18, Fade(UI_WHITE, overlayAlpha));
    }
    else
    {
        DrawText("Final Survival Score", 386, 302, 16, Fade(UI_GRAY, overlayAlpha));
        DrawText(TextFormat("%d", player->survivalScore), 710, 302, 18, Fade(UI_CYAN, overlayAlpha));

        DrawText("Total Blocked Roads", 386, 340, 16, Fade(UI_GRAY, overlayAlpha));
        DrawText(TextFormat("%d / %d", GetBlockedRoadCount(), EDGE_COUNT), 710, 340, 18, Fade(UI_RED, overlayAlpha));

        DrawText("Disasters Survived", 386, 378, 16, Fade(UI_GRAY, overlayAlpha));
        DrawText(TextFormat("%d", dm->disastersSurvived), 710, 378, 18, Fade(UI_ORANGE, overlayAlpha));

        DrawText("Mission Time", 386, 416, 16, Fade(UI_GRAY, overlayAlpha));
        DrawText(TextFormat("%02d:%04.1f", mins, secs), 710, 416, 18, Fade(UI_WHITE, overlayAlpha));
    }

    if (overlayAlpha > 0.5f)
    {
        Rectangle restartBtn = {360, 480, 225, 50};
        Rectangle menuBtn = {615, 480, 225, 50};

        if (DrawFuturisticButton(restartBtn, "RESTART", accent, true))
            action->restartClicked = true;
        if (DrawFuturisticButton(menuBtn, "MAIN MENU", UI_CYAN, false))
            action->menuClicked = true;
    }
}

UIAction DrawGameUI(
    const Player *player,
    const DisasterManager *dm,
    const int shortestPath[],
    int shortestPathLength,
    int shortestCost,
    double missionTimeSec,
    float overlayAlpha
)
{
    UIAction action = {false, false};
    float timeSec = (float)GetTime();
    Rectangle mapBounds = GetMapBounds();

    Camera2D cam = {0};
    cam.zoom = 1.0f;
    cam.offset = GetDisasterShakeOffset(dm);

    BeginMode2D(cam);

    DrawHeader(player, dm, missionTimeSec, &action);
    DrawMapPanelBackground(mapBounds, dm, timeSec);
    DrawRoads(shortestPath, shortestPathLength, timeSec);
    DrawNodes(player, timeSec);
    DrawPlayer(*player, timeSec, mapBounds);
    DrawDisasterBanner(mapBounds, dm);
    DrawSidebar(player, dm, shortestPath, shortestPathLength, shortestCost, timeSec);
    DrawBottomBar();

    EndMode2D();

    DrawEndOverlay(player, dm, missionTimeSec, overlayAlpha, &action);

    return action;
}
