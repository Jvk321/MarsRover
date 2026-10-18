#pragma once
#include "terrain.h"
#include "visual_effects.h"
#include "world_camera.h"
class SceneRenderer {
public:
    void draw(const Terrain& terrain,const Simulation& sim,const WorldCamera& camera,
              const VisualEffects& effects,Rectangle viewport,std::optional<Cell> hovered,bool showCosts=false) const;
    static std::uint32_t tileHash(int x,int y,std::uint32_t seed);
private:
    // Reused rendering scratch space; no ownership of pathfinding or vehicle state.
    mutable std::vector<CellState> states_;
    mutable std::vector<const SearchNode*> nodes_;
    mutable Rover parked_;
    mutable std::optional<Cell> parkedStart_;
};
