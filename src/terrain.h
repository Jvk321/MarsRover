#pragma once
#include "astar.h"
#include <cstdint>

struct TerrainConfig { int width=20, height=20, density=25; std::uint32_t seed=42; };
enum class EditTool { SET_START, SET_GOAL, ADD_ROCK, ERASER };
class Terrain {
public:
    explicit Terrain(TerrainConfig config);
    Grid grid;
    Cell start, goal;
    TerrainConfig config;
    bool edit(Cell cell, EditTool tool);
    bool stroke(Cell from, Cell to, EditTool tool);
    int obstacleCount() const;
};
bool connected(const Grid& grid, Cell start, Cell goal);
