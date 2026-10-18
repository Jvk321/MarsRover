#pragma once
#include "terrain.h"
#include <array>
#include <optional>
#include <string>
class TerrainPanel {
public:
    bool open=false;
    explicit TerrainPanel(TerrainConfig config);
    std::optional<TerrainConfig> draw();
private:
    std::array<std::string,4> fields_;
    int focus_=-1;
    std::string error_;
};
