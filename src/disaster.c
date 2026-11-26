#include "disaster.h"
#include "graph.h"
#include "dijkstra.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const Color COLOR_EMERGENCY_RED = {255, 59, 79, 255};
static const Color COLOR_WARNING_AMBER = {255, 170, 55, 255};
static const Color COLOR_FLOOD_BLUE    = {0, 190, 255, 255};

const char *GetDisasterName(DisasterType type)
{
    switch (type)
    {
        case DISASTER_ROAD_COLLAPSE:   return "ROAD COLLAPSE";
        case DISASTER_EARTHQUAKE:      return "EARTHQUAKE";
        case DISASTER_HEAVY_FLOOD:     return "HEAVY FLOOD";
        case DISASTER_BRIDGE_COLLAPSE: return "BRIDGE COLLAPSE";
        default:                       return "NOMINAL";
    }
}

static DisasterType PickRandomDisasterType(DisasterType lastType)
{
    DisasterType candidates[4] = {
        DISASTER_ROAD_COLLAPSE,
        DISASTER_EARTHQUAKE,
        DISASTER_HEAVY_FLOOD,
        DISASTER_BRIDGE_COLLAPSE
    };

    DisasterType chosen = candidates[GetRandomValue(0, 3)];
    if (chosen == lastType)
    {
        chosen = candidates[(GetRandomValue(0, 2) + (int)lastType) % 4];
    }
    return chosen;
}

static double PickRandomCalmWait(double warningDuration)
{
    // Total interval between events: 6.0 to 10.0 seconds (calm + warning)
    double totalInterval = (double)GetRandomValue(60, 100) / 10.0;
    double calm = totalInterval - warningDuration;
    return (calm < 2.5) ? 2.5 : calm;
}

void InitializeDisasterManager(DisasterManager *dm)
{
    memset(dm, 0, sizeof(DisasterManager));

    dm->phase = DISASTER_PHASE_CALM;
    dm->warningDuration = 1.8;
    dm->pendingType = PickRandomDisasterType(DISASTER_NONE);
    dm->lastType = DISASTER_NONE;
    dm->lastIntervalSeconds = (double)GetRandomValue(55, 80) / 10.0;
    dm->nextTransitionTime = GetTime() + (dm->lastIntervalSeconds - dm->warningDuration);
    dm->nextReactiveBlockTime = GetTime();

    dm->alertDuration = 3.6f;
    dm->shakeDuration = 0.70f;
    dm->shakeMagnitude = 9.0f;
}

static void SetAlert(DisasterManager *dm, const char *title, const char *msg, const char *impact, Color color)
{
    dm->alertActive = true;
    dm->alertTimer = dm->alertDuration;
    snprintf(dm->alertTitle, sizeof(dm->alertTitle), "%s", title);
    snprintf(dm->alertMessage, sizeof(dm->alertMessage), "%s", msg);
    snprintf(dm->alertImpact, sizeof(dm->alertImpact), "%s", impact);
    dm->alertColor = color;
}

static int TestEdgeBlockSafety(int edgeIndex, int playerNode)
{
    int u = edges[edgeIndex].from;
    int v = edges[edgeIndex].to;
    int savedWeight = graph[u][v];

    graph[u][v] = INT_MAX;
    graph[v][u] = INT_MAX;

    int testPath[NODE_COUNT];
    int testLen = 0;
    int testCost = Dijkstra(playerNode, NODE_COUNT - 1, testPath, &testLen);

    graph[u][v] = savedWeight;
    graph[v][u] = savedWeight;

    return testCost;
}

static void AppendUniqueCandidate(int candidates[], int *count, int edgeIndex, const bool added[])
{
    if (edgeIndex < 0 || edgeIndex >= EDGE_COUNT || added[edgeIndex] || edges[edgeIndex].blocked)
        return;
    candidates[(*count)++] = edgeIndex;
}

static int FindSafeRoadToClose(int playerNode, int playerLifePoints, bool preferBridge)
{
    // Trace the player's current Dijkstra shortest path so disasters target their active route
    int activePath[NODE_COUNT];
    int activeLen = 0;
    Dijkstra(playerNode, NODE_COUNT - 1, activePath, &activeLen);

    int ordered[EDGE_COUNT];
    bool added[EDGE_COUNT] = {false};
    int count = 0;

    // Tier 1: Edges along the player's current shortest path from their current node
    for (int i = 0; i < activeLen - 1; i++)
    {
        int e = GetEdgeIndex(activePath[i], activePath[i + 1]);
        if (e >= 0 && !added[e] && !edges[e].blocked)
        {
            AppendUniqueCandidate(ordered, &count, e, added);
            added[e] = true;
        }
    }

    // Tier 2: Other open roads directly connected to the player's current node
    for (int i = 0; i < EDGE_COUNT; i++)
    {
        if (!added[i] && !edges[i].blocked &&
            (edges[i].from == playerNode || edges[i].to == playerNode))
        {
            AppendUniqueCandidate(ordered, &count, i, added);
            added[i] = true;
        }
    }

    // Tier 3: Remaining open roads on the map
    for (int i = 0; i < EDGE_COUNT; i++)
    {
        if (!added[i] && !edges[i].blocked)
        {
            AppendUniqueCandidate(ordered, &count, i, added);
            added[i] = true;
        }
    }

    // Evaluate candidates while always guaranteeing at least 1 safe path to H remains.
    // Prefer candidates that match preferBridge and keep remaining shortest cost <= playerLifePoints.
    int fallbackAnyReachable = -1;

    for (int bridgePass = 0; bridgePass < 2; bridgePass++)
    {
        bool wantBridge = (bridgePass == 0) ? preferBridge : !preferBridge;

        for (int i = 0; i < count; i++)
        {
            int edgeIndex = ordered[i];
            if (bridgePass == 0 && edges[edgeIndex].isBridge != wantBridge)
                continue;

            int testCost = TestEdgeBlockSafety(edgeIndex, playerNode);
            if (testCost == -1)
                continue;

            if (testCost <= playerLifePoints)
                return edgeIndex;

            if (fallbackAnyReachable == -1)
                fallbackAnyReachable = edgeIndex;
        }
    }

    return fallbackAnyReachable;
}

bool TryReactivePathTraceBlock(DisasterManager *dm, Player *player, int targetNode)
{
    if (player->status != GAME_PLAYING)
        return false;

    double now = GetTime();
    if (now < dm->nextReactiveBlockTime || dm->reactiveBlocksCount >= 3)
        return false;

    int edgeIdx = GetEdgeIndex(player->currentNode, targetNode);
    if (edgeIdx < 0 || edges[edgeIdx].blocked)
        return false;

    // Only intercept the attempted road if at least 1 safe & survivable path to H remains
    int testCost = TestEdgeBlockSafety(edgeIdx, player->currentNode);
    if (testCost == -1 || testCost > player->lifePoints)
        return false;

    int u = edges[edgeIdx].from;
    int v = edges[edgeIdx].to;
    BlockEdge(edgeIdx);

    dm->reactiveBlocksCount++;
    dm->disastersTriggered++;
    dm->disastersSurvived++;
    dm->lastType = DISASTER_ROAD_COLLAPSE;
    dm->nextReactiveBlockTime = now + 2.0;
    player->survivalScore += 50;

    char msg[112];
    snprintf(msg, sizeof(msg), "Path traced! Road %c - %c collapsed ahead. Reroute!",
             nodes[u].id, nodes[v].id);
    SetAlert(dm, "PATH TRACED: BLOCKED", msg, "REROUTE REQUIRED", COLOR_EMERGENCY_RED);

    return true;
}

static int FindOpenRoadForFlood(void)
{
    int candidates[EDGE_COUNT];
    int count = 0;

    for (int i = 0; i < EDGE_COUNT; i++)
    {
        if (!edges[i].blocked && !edges[i].flooded)
        {
            candidates[count++] = i;
        }
    }

    if (count == 0)
        return -1;

    return candidates[GetRandomValue(0, count - 1)];
}

static bool TriggerDisasterEvent(DisasterManager *dm, Player *player)
{
    DisasterType type = dm->pendingType;
    bool graphChanged = false;

    // If a road/bridge collapse has no safe road to block, gracefully switch to Flood or Quake
    if (type == DISASTER_ROAD_COLLAPSE || type == DISASTER_BRIDGE_COLLAPSE)
    {
        bool preferBridge = (type == DISASTER_BRIDGE_COLLAPSE);
        int edgeIdx = FindSafeRoadToClose(player->currentNode, player->lifePoints, preferBridge);

        if (edgeIdx == -1)
        {
            type = (GetRandomValue(0, 1) == 0) ? DISASTER_HEAVY_FLOOD : DISASTER_EARTHQUAKE;
        }
        else
        {
            int u = edges[edgeIdx].from;
            int v = edges[edgeIdx].to;
            BlockEdge(edgeIdx);
            graphChanged = true;

            if (type == DISASTER_BRIDGE_COLLAPSE)
            {
                char msg[112];
                snprintf(msg, sizeof(msg), "Bridge %c - %c on your route collapsed! Rerouting.",
                         nodes[u].id, nodes[v].id);
                SetAlert(dm, "BRIDGE COLLAPSE", msg, "BRIDGE CLOSED", COLOR_EMERGENCY_RED);
            }
            else
            {
                char msg[112];
                snprintf(msg, sizeof(msg), "Road %c - %c on your path collapsed! Safe path updated.",
                         nodes[u].id, nodes[v].id);
                SetAlert(dm, "ROAD COLLAPSE", msg, "ROAD BLOCKED", COLOR_EMERGENCY_RED);
            }
        }
    }

    if (type == DISASTER_EARTHQUAKE)
    {
        int damage = 15;
        player->lifePoints -= damage;
        if (player->lifePoints <= 0)
        {
            player->lifePoints = 0;
            player->status = GAME_OVER;
            snprintf(player->gameOverReason, sizeof(player->gameOverReason),
                     "Eliminated by a severe Earthquake!");
        }

        dm->shakeTimer = dm->shakeDuration;
        dm->quakeWaveTimer = 1.25f;
        dm->quakeOrigin = player->visualPos;

        SetAlert(dm, "EARTHQUAKE", "Major seismic tremor struck the sector!",
                 "-15 LIFE POINTS", COLOR_WARNING_AMBER);
    }
    else if (type == DISASTER_HEAVY_FLOOD)
    {
        int damage = 10;
        player->lifePoints -= damage;
        if (player->lifePoints <= 0)
        {
            player->lifePoints = 0;
            player->status = GAME_OVER;
            snprintf(player->gameOverReason, sizeof(player->gameOverReason),
                     "Swept away by Heavy Flood waters!");
        }

        dm->floodEffectTimer = 3.5f;

        int floodEdge = FindOpenRoadForFlood();
        if (floodEdge != -1 && player->status == GAME_PLAYING)
        {
            ApplyEdgeFloodBoost(floodEdge, 4, 10.0);
            graphChanged = true;

            int u = edges[floodEdge].from;
            int v = edges[floodEdge].to;
            char msg[112];
            snprintf(msg, sizeof(msg), "Flash flood! Road %c - %c cost +4 for 10s.",
                     nodes[u].id, nodes[v].id);
            SetAlert(dm, "HEAVY FLOOD", msg, "-10 LP | +4 COST", COLOR_FLOOD_BLUE);
        }
        else
        {
            SetAlert(dm, "HEAVY FLOOD", "Flash floodwaters surged across the city!",
                     "-10 LIFE POINTS", COLOR_FLOOD_BLUE);
        }
    }

    dm->disastersTriggered++;
    dm->lastType = type;

    if (player->status == GAME_PLAYING)
    {
        dm->disastersSurvived++;
        player->survivalScore += 75;
    }

    return graphChanged;
}

bool UpdateDisasterManager(DisasterManager *dm, Player *player)
{
    float dt = GetFrameTime();
    double now = GetTime();
    bool graphChanged = false;

    if (dm->alertActive)
    {
        dm->alertTimer -= dt;
        if (dm->alertTimer <= 0.0f)
        {
            dm->alertTimer = 0.0f;
            dm->alertActive = false;
        }
    }

    if (dm->shakeTimer > 0.0f)
    {
        dm->shakeTimer -= dt;
        if (dm->shakeTimer < 0.0f)
            dm->shakeTimer = 0.0f;
    }

    if (dm->quakeWaveTimer > 0.0f)
    {
        dm->quakeWaveTimer -= dt;
        if (dm->quakeWaveTimer < 0.0f)
            dm->quakeWaveTimer = 0.0f;
    }

    if (dm->floodEffectTimer > 0.0f)
    {
        dm->floodEffectTimer -= dt;
        if (dm->floodEffectTimer < 0.0f)
            dm->floodEffectTimer = 0.0f;
    }

    if (player->status != GAME_PLAYING)
        return false;

    if (UpdateTemporaryRoadEffects(now))
    {
        graphChanged = true;
    }

    if (dm->phase == DISASTER_PHASE_CALM && now >= dm->nextTransitionTime)
    {
        dm->pendingType = PickRandomDisasterType(dm->lastType);
        dm->phase = DISASTER_PHASE_WARNING;
        dm->nextTransitionTime = now + dm->warningDuration;
    }
    else if (dm->phase == DISASTER_PHASE_WARNING && now >= dm->nextTransitionTime)
    {
        if (TriggerDisasterEvent(dm, player))
        {
            graphChanged = true;
        }

        dm->phase = DISASTER_PHASE_CALM;
        double calmWait = PickRandomCalmWait(dm->warningDuration);
        dm->lastIntervalSeconds = calmWait + dm->warningDuration;
        dm->nextTransitionTime = now + calmWait;
    }

    return graphChanged;
}

Vector2 GetDisasterShakeOffset(const DisasterManager *dm)
{
    if (dm->shakeTimer <= 0.0f || dm->shakeDuration <= 0.0f)
        return (Vector2){0.0f, 0.0f};

    float ratio = dm->shakeTimer / dm->shakeDuration;
    float mag = dm->shakeMagnitude * ratio;
    float t = (float)GetTime() * 55.0f;

    return (Vector2){
        sinf(t) * mag,
        cosf(t * 1.3f) * mag * 0.8f
    };
}
