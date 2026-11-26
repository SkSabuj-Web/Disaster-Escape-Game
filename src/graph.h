#ifndef GRAPH_H
#define GRAPH_H

#include "raylib.h"
#include <stdbool.h>

#define NODE_COUNT 8
#define EDGE_COUNT 12

typedef struct {
    char name[25];
    char id;
    Vector2 position;
} Node;

typedef struct {
    int from;
    int to;
    int weight;
    int originalWeight;
    bool blocked;
    bool isBridge;
    bool flooded;
    double floodEndTime;
} Edge;

extern Node nodes[NODE_COUNT];
extern Edge edges[EDGE_COUNT];
extern int graph[NODE_COUNT][NODE_COUNT];

void InitializeGraph(void);
int GetEdgeIndex(int u, int v);
void BlockEdge(int edgeIndex);
void RestoreEdge(int edgeIndex);
void ApplyEdgeFloodBoost(int edgeIndex, int extraCost, double durationSeconds);
bool UpdateTemporaryRoadEffects(double currentTime);
int GetBlockedRoadCount(void);

#endif
