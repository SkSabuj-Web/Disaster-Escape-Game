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
    // Pick among the 4 disaster types while avoiding immediate identical repeats when possible
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

    dm->alertDuration = 3.6f;
    dm->shakeDuration = 0.70f;
    dm->shakeMagnitude = 9.0f;
}

static int FindSafeRoadToClose(int playerNode, bool preferBridge)
{
    int candidates[EDGE_COUNT];
    int count = 0;

    for (int pass = 0; pass < 2 && count == 0; pass++)
    {
        bool matchBridge = (pass == 0) ? preferBridge : !preferBridge;
        for (int i = 0; i < EDGE_COUNT; i++)
        {
            if (edges[i].blocked)
                continue;
            if (pass == 0 && edges[i].isBridge != matchBridge)
                continue;
            candidates[count++] = i;
        }
    }

    while (count > 0)
    {
        int idx = GetRandomValue(0, count - 1);
        int edgeIndex = candidates[idx];
        candidates[idx] = candidates[count - 1];
        count--;

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

        if (testCost != -1)
        {
            return edgeIndex;
        }
    }

    return -1;
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

static void SetAlert(DisasterManager *dm, const char *title, const char *msg, const char *impact, Color color)
{
    dm->alertActive = true;
    dm->alertTimer = dm->alertDuration;
    snprintf(dm->alertTitle, sizeof(dm->alertTitle), "%s", title);
    snprintf(dm->alertMessage, sizeof(dm->alertMessage), "%s", msg);
    snprintf(dm->alertImpact, sizeof(dm->alertImpact), "%s", impact);
    dm->alertColor = color;
}

static bool TriggerDisasterEvent(DisasterManager *dm, Player *player)
{
    DisasterType type = dm->pendingType;
    bool graphChanged = false;

    // If a road/bridge collapse has no safe road to block, gracefully switch to Flood or Quake
    if (type == DISASTER_ROAD_COLLAPSE || type == DISASTER_BRIDGE_COLLAPSE)
    {
        bool preferBridge = (type == DISASTER_BRIDGE_COLLAPSE);
        int edgeIdx = FindSafeRoadToClose(player->currentNode, preferBridge);

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
                snprintf(msg, sizeof(msg), "Bridge %c - %c collapsed! Dijkstra rerouting.",
                         nodes[u].id, nodes[v].id);
                SetAlert(dm, "BRIDGE COLLAPSE", msg, "BRIDGE CLOSED", COLOR_EMERGENCY_RED);
            }
            else
            {
                char msg[112];
                snprintf(msg, sizeof(msg), "Road %c - %c blocked by debris! Route updated.",
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
