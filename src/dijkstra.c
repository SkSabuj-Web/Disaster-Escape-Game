#include "dijkstra.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

static int FindMinimum(const int distance[], const bool visited[])
{
    int minimum = INT_MAX;
    int minVertex = -1;

    for (int i = 0; i < NODE_COUNT; i++)
    {
        if (!visited[i] && distance[i] < minimum)
        {
            minimum = distance[i];
            minVertex = i;
        }
    }

    return minVertex;
}

int Dijkstra(int source, int destination, int path[], int *pathLength)
{
    if (pathLength == NULL)
        return -1;

    *pathLength = 0;

    if (source < 0 || source >= NODE_COUNT || destination < 0 || destination >= NODE_COUNT)
        return -1;

    int distance[NODE_COUNT];
    int parent[NODE_COUNT];
    bool visited[NODE_COUNT] = {false};

    for (int i = 0; i < NODE_COUNT; i++)
    {
        distance[i] = INT_MAX;
        parent[i] = -1;
    }

    distance[source] = 0;

    for (int count = 0; count < NODE_COUNT; count++)
    {
        int u = FindMinimum(distance, visited);

        if (u == -1)
            break;

        visited[u] = true;

        if (u == destination)
            break;

        for (int v = 0; v < NODE_COUNT; v++)
        {
            if (!visited[v] &&
                graph[u][v] != INT_MAX &&
                graph[u][v] > 0 &&
                distance[u] != INT_MAX &&
                distance[u] <= INT_MAX - graph[u][v])
            {
                int newDistance = distance[u] + graph[u][v];

                if (newDistance < distance[v])
                {
                    distance[v] = newDistance;
                    parent[v] = u;
                }
            }
        }
    }

    if (distance[destination] == INT_MAX)
        return -1;

    if (path != NULL)
    {
        int current = destination;
        while (current != -1 && *pathLength < NODE_COUNT)
        {
            path[(*pathLength)++] = current;
            current = parent[current];
        }

        for (int i = 0; i < *pathLength / 2; i++)
        {
            int temp = path[i];
            path[i] = path[*pathLength - 1 - i];
            path[*pathLength - 1 - i] = temp;
        }
    }

    return distance[destination];
}
