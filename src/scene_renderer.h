#pragma once
#include "terrain.h"
#include "visual_effects.h"
#include "world_camera.h"
#include "rover_renderer.h"
class SceneRenderer {
public:
    void draw(const Terrain& terrain,const Simulation& sim,const WorldCamera& camera,
              const VisualEffects& effects,Rectangle viewport,std::optional<Cell> hovered,bool showCosts=false,bool withMinimap=true) const;
    static std::uint32_t tileHash(int x,int y,std::uint32_t seed);
    static Rectangle minimapBounds(Rectangle viewport);
    void shutdown() {roverArt_.shutdown();}
    void drawMinimap(const Terrain& terrain,const Simulation& sim,const WorldCamera& camera,Rectangle viewport,Rectangle panel) const;
private:
    enum class DrawKind { OBSTACLE, PROP, BEACON, ROVER };
    struct DrawItem { float depth;DrawKind kind;Cell cell;std::size_t propIndex; };
    // Reused rendering scratch space; no ownership of pathfinding or vehicle state.
    mutable std::vector<CellState> states_;
    mutable std::vector<const SearchNode*> nodes_;
    mutable std::vector<DrawItem> drawItems_;
    mutable Rover parked_;
    mutable RoverRenderer roverArt_;
    mutable std::optional<Cell> parkedStart_;
    void minimap(const Terrain& terrain,const Simulation& sim,const WorldCamera& camera,Rectangle viewport) const;
};
