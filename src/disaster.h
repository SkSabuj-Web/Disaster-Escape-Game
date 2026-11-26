#ifndef DISASTER_H
#define DISASTER_H

#include "raylib.h"
#include "player.h"
#include <stdbool.h>

typedef enum {
    DISASTER_NONE = 0,
    DISASTER_ROAD_COLLAPSE,
    DISASTER_EARTHQUAKE,
    DISASTER_HEAVY_FLOOD,
    DISASTER_BRIDGE_COLLAPSE
} DisasterType;

typedef enum {
    DISASTER_PHASE_CALM = 0,
    DISASTER_PHASE_WARNING
} DisasterPhase;

typedef struct {
    DisasterPhase phase;
    DisasterType pendingType;
    DisasterType lastType;
    double nextTransitionTime;
    double warningDuration;
    double lastIntervalSeconds;

    int disastersTriggered;
    int disastersSurvived;

    // Alert banner state
    bool alertActive;
    float alertTimer;
    float alertDuration;
    char alertTitle[48];
    char alertMessage[112];
    char alertImpact[48];
    Color alertColor;

    // Visual effect timers
    float shakeTimer;
    float shakeDuration;
    float shakeMagnitude;
    float quakeWaveTimer;
    float floodEffectTimer;
    Vector2 quakeOrigin;
} DisasterManager;

void InitializeDisasterManager(DisasterManager *dm);
bool UpdateDisasterManager(DisasterManager *dm, Player *player);
Vector2 GetDisasterShakeOffset(const DisasterManager *dm);
const char *GetDisasterName(DisasterType type);

#endif
