#include "graph.h"
#include <limits.h>

Node nodes[NODE_COUNT] = {
    {"Starting Point", 'A', {110, 160}},
    {"Residential",    'B', {320, 120}},
    {"Hospital",       'C', {180, 330}},
    {"Market",         'D', {410, 300}},
    {"School",         'E', {120, 530}},
    {"Police Station", 'F', {410, 510}},
    {"Shelter",        'G', {640, 420}},
    {"Safe Zone",      'H', {690, 160}}
};

Edge edges[EDGE_COUNT] = {
    {0, 1, 10, 10, false, false, false, 0.0},
    {0, 2,  5,  5, false, false, false, 0.0},
    {1, 3,  6,  6, false, false, false, 0.0},
    {2, 3,  8,  8, false, false, false, 0.0},
    {2, 4,  7,  7, false, false, false, 0.0},
    {3, 5,  4,  4, false, true,  false, 0.0},
    {3, 7, 12, 12, false, true,  false, 0.0},
    {4, 5,  3,  3, false, false, false, 0.0},
    {5, 6,  6,  6, false, true,  false, 0.0},
    {6, 7,  5,  5, false, false, false, 0.0},
    {1, 7, 15, 15, false, true,  false, 0.0},
    {5, 7,  9,  9, false, true,  false, 0.0}
};

int graph[NODE_COUNT][NODE_COUNT];

void InitializeGraph(void)
{
    for (int i = 0; i < NODE_COUNT; i++)
    {
        for (int j = 0; j < NODE_COUNT; j++)
        {
            graph[i][j] = (i == j) ? 0 : INT_MAX;
        }
    }

    for (int i = 0; i < EDGE_COUNT; i++)
    {
        int u = edges[i].from;
        int v = edges[i].to;

        edges[i].weight = edges[i].originalWeight;
        edges[i].blocked = false;
        edges[i].flooded = false;
        edges[i].floodEndTime = 0.0;

        graph[u][v] = edges[i].weight;
        graph[v][u] = edges[i].weight;
    }
}

int GetEdgeIndex(int u, int v)
{
    for (int i = 0; i < EDGE_COUNT; i++)
    {
        if ((edges[i].from == u && edges[i].to == v) ||
            (edges[i].from == v && edges[i].to == u))
        {
            return i;
        }
    }
    return -1;
}

void BlockEdge(int edgeIndex)
{
    if (edgeIndex < 0 || edgeIndex >= EDGE_COUNT)
        return;

    int u = edges[edgeIndex].from;
    int v = edges[edgeIndex].to;

    edges[edgeIndex].blocked = true;
    edges[edgeIndex].flooded = false;
    edges[edgeIndex].floodEndTime = 0.0;

    graph[u][v] = INT_MAX;
    graph[v][u] = INT_MAX;
}

void RestoreEdge(int edgeIndex)
{
    if (edgeIndex < 0 || edgeIndex >= EDGE_COUNT)
        return;

    int u = edges[edgeIndex].from;
    int v = edges[edgeIndex].to;

    edges[edgeIndex].blocked = false;
    edges[edgeIndex].flooded = false;
    edges[edgeIndex].floodEndTime = 0.0;
    edges[edgeIndex].weight = edges[edgeIndex].originalWeight;

    graph[u][v] = edges[edgeIndex].weight;
    graph[v][u] = edges[edgeIndex].weight;
}

void ApplyEdgeFloodBoost(int edgeIndex, int extraCost, double durationSeconds)
{
    if (edgeIndex < 0 || edgeIndex >= EDGE_COUNT)
        return;
    if (edges[edgeIndex].blocked)
        return;

    int u = edges[edgeIndex].from;
    int v = edges[edgeIndex].to;

    edges[edgeIndex].weight = edges[edgeIndex].originalWeight + extraCost;
    edges[edgeIndex].flooded = true;
    edges[edgeIndex].floodEndTime = GetTime() + durationSeconds;

    graph[u][v] = edges[edgeIndex].weight;
    graph[v][u] = edges[edgeIndex].weight;
}

bool UpdateTemporaryRoadEffects(double currentTime)
{
    bool graphChanged = false;

    for (int i = 0; i < EDGE_COUNT; i++)
    {
        if (!edges[i].blocked && edges[i].flooded && currentTime >= edges[i].floodEndTime)
        {
            int u = edges[i].from;
            int v = edges[i].to;

            edges[i].flooded = false;
            edges[i].floodEndTime = 0.0;
            edges[i].weight = edges[i].originalWeight;

            graph[u][v] = edges[i].weight;
            graph[v][u] = edges[i].weight;
            graphChanged = true;
        }
    }

    return graphChanged;
}

int GetBlockedRoadCount(void)
{
    int count = 0;
    for (int i = 0; i < EDGE_COUNT; i++)
    {
        if (edges[i].blocked)
            count++;
    }
    return count;
}
