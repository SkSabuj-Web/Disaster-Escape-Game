# Disaster Escape — A Graph-Based Survival Game

**Disaster Escape** is a 2D graph-based survival game written in **C** using **Raylib 6.2**. Designed for an Algorithms Lab project, it combines real-time tactical decision-making with **Dijkstra's Shortest Path Algorithm** on an **Adjacency Matrix** graph representation while dynamic random disasters reshape the city map.

---

## Project Overview

You start an emergency evacuation at **Location A (Starting Point)** with **50 Life Points** and must reach **Location H (Safe Zone)**. The city is modeled as an undirected, weighted graph with **8 locations** and **12 roads**. Traveling along any road deducts Life Points equal to its weight.

As you navigate the network, unpredictable disasters strike—collapsing roads and bridges, flooding routes with higher travel costs, or shaking the city with earthquakes. Whenever the graph changes or you move, **Dijkstra's Algorithm** dynamically recalculates the optimal evacuation route to the Safe Zone.

---

## Features

- **Real-Time Dijkstra Shortest Path Visualization**: Live shortest-path calculation highlighted in neon green with animated route pulses and cost readout.
- **Adjacency Matrix Graph Engine**: Clean 8-node, 12-edge undirected weighted graph with `INT_MAX` exclusion for blocked edges and automatic weight restoration on reset.
- **Dynamic Random Disaster System**:
  - **Road Collapse**: Closes an open road (marked in emergency red with `X`) while verifying the Safe Zone remains reachable.
  - **Bridge Collapse**: Collapses a bridge crossing and reroutes via Dijkstra without disconnecting the player from Safe Zone `H`.
  - **Earthquake**: Deals `15` Life Points damage with decaying camera screen-shake and expanding seismic shockwave rings.
  - **Heavy Flood**: Deals `10` Life Points damage, displays blue flood overlays, and temporarily increases an open road's cost by `+4` for `10` seconds.
- **Futuristic Survival Dashboard UI**:
  - Main Menu, How To Play briefing screen, Gameplay HUD, Mission Completed screen, and Game Over screen.
  - Animated Life Points bar, survival score counter, mission timer, disaster warning countdowns, and fade-in/out alert banners.
- **Responsive Controls**: Arrow Keys (or `WASD`) and direct mouse click on adjacent reachable locations.
- **Full Reset System**: Pressing `R` (or clicking Restart) cleanly resets player state, blocked roads, original edge weights, disaster timers, and score.

---

## Technologies Used

- **Language**: C (C99)
- **Graphics & Input**: Raylib 6.2
- **Algorithm**: Dijkstra's Shortest Path Algorithm
- **Data Structure**: Adjacency Matrix (`int graph[8][8]`)
- **Compiler**: W64Devkit GCC (MinGW-w64 on Windows)

---

## Graph Representation

The city map consists of **8 vertices (`A`–`H`)** and **12 undirected weighted edges**:

| Node | Location Name   | Coordinates |
| :--- | :-------------- | :---------- |
| `A`  | Starting Point  | `(110, 160)` |
| `B`  | Residential     | `(320, 120)` |
| `C`  | Hospital        | `(180, 330)` |
| `D`  | Market          | `(410, 300)` |
| `E`  | School          | `(120, 530)` |
| `F`  | Police Station  | `(410, 510)` |
| `G`  | Shelter         | `(640, 420)` |
| `H`  | Safe Zone       | `(690, 160)` |

The graph is stored in an `8 x 8` adjacency matrix `graph[NODE_COUNT][NODE_COUNT]`:
- `graph[i][i] = 0`
- `graph[u][v] = graph[v][u] = weight` for open roads
- `graph[u][v] = INT_MAX` when no road exists or when a road is blocked by a disaster

Initial shortest path from `A` to `H`:
`A -> C -> E -> F -> H` (Total Cost: **24**)

---

## Dijkstra Algorithm Explanation

The pathfinder in `src/dijkstra.c` implements classical Dijkstra's algorithm on the adjacency matrix:

1. Initialize `distance[i] = INT_MAX`, `parent[i] = -1`, and `visited[i] = false` for all vertices; set `distance[source] = 0`.
2. Repeat `NODE_COUNT` times:
   - Pick the unvisited vertex `u` with the smallest `distance[u]`.
   - Stop if `u == -1` (remaining vertices unreachable) or `u == destination`.
   - Mark `u` as visited and relax each unvisited neighbor `v` where `graph[u][v] != INT_MAX`.
   - If `distance[u] + graph[u][v] < distance[v]`, update `distance[v]` and `parent[v] = u`.
3. Reconstruct the path backward from `destination` to `source` using `parent[]`, or return `-1` if `distance[destination] == INT_MAX`.

- **Time Complexity**: $O(V^2)$ with an adjacency matrix ($V = 8$).
- **Dynamic Recalculation**: Triggered after every player move, road/bridge collapse, flood weight increase, and flood expiration.

---

## Disaster System Explanation

Managed in `src/disaster.c` using non-blocking `GetTime()` and `GetFrameTime()` timers:

- Events trigger at random intervals of **6–10 seconds**, preceded by a **1.8-second warning state** in the dashboard.
- Before committing any **Road Collapse** or **Bridge Collapse**, the disaster manager temporarily marks the candidate edge as `INT_MAX` and runs `Dijkstra(player.currentNode, H)` to verify Safe Zone `H` remains reachable. If no road can be safely closed, it falls back to Earthquake or Heavy Flood.
- Victory is prioritized if the player reaches Safe Zone `H` on a move that consumes their exact remaining Life Points.

---

## Controls

| Input | Action |
| :--- | :--- |
| `Arrow Keys` / `WASD` | Move to a connected open location in that direction |
| `Left Mouse Click` | Move directly to a highlighted adjacent location |
| `R` | Restart mission (resets graph, disasters, timer, and Life Points) |
| `M` | Return to Main Menu |
| `ESC` | Exit game |

---

## Project Structure

```text
Disaster-Escape/
├── assets/
│   ├── fonts/
│   └── images/
├── include/
│   ├── raylib.h
│   ├── raymath.h
│   └── rlgl.h
├── lib/
│   ├── libraylib.a
│   ├── libraylibdll.a
│   └── raylib.dll
├── src/
│   ├── main.c
│   ├── game.h
│   ├── graph.h
│   ├── graph.c
│   ├── dijkstra.h
│   ├── dijkstra.c
│   ├── player.h
│   ├── player.c
│   ├── disaster.h
│   ├── disaster.c
│   ├── ui.h
│   ├── ui.c
│   ├── menu.h
│   └── menu.c
├── DisasterEscape.exe
└── README.md
```

---

## Build Instructions

Open a terminal in the `Disaster-Escape/` directory with **W64Devkit GCC** in your `PATH` and run:

```bash
gcc src/main.c src/graph.c src/dijkstra.c src/player.c src/disaster.c src/ui.c src/menu.c -o DisasterEscape.exe -Iinclude -Llib -lraylib -lgdi32 -lwinmm
```

---

## How to Run

After compiling, launch the executable from the project root:

```bash
.\DisasterEscape.exe
```

Or double-click `DisasterEscape.exe` in Windows Explorer.

---

## Screenshots

- **Main Menu**: Futuristic tactical survival menu with `START GAME`, `HOW TO PLAY`, and `EXIT`.
- **Gameplay Dashboard**: Live city graph map, neon-green Dijkstra shortest path, blocked road `X` markers, flooded route indicators, animated Life Points bar, and disaster alert banner.
- **Victory & Game Over Overlays**: Mission summary displaying remaining Life Points, survival score, disasters survived, and blocked roads.
