#include "astar.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
Grid::Grid(int width, int height) : width_(width), height_(height) {
    if (width <= 0 || height <= 0) throw std::invalid_argument("Dimensions must be positive");
    obstacles_.resize(static_cast<std::size_t>(width) * height);
}
bool Grid::contains(Cell c) const {
    return c.x >= 0 && c.y >= 0 && c.x < width_ && c.y < height_;
}
bool Grid::blocked(Cell c) const {
    return !contains(c) || obstacles_[static_cast<std::size_t>(c.y) * width_ + c.x] != 0;
}
void Grid::setBlocked(Cell c, bool value) {
    if (!contains(c)) throw std::out_of_range("Cell outside grid");
    obstacles_[static_cast<std::size_t>(c.y) * width_ + c.x] = value;
}
std::vector<Cell> findPath(const Grid& grid, Cell start, Cell goal) {
    if (grid.blocked(start) || grid.blocked(goal)) return {};
    struct Node { Cell cell; int g; double f; Cell parent; };
    auto h = [goal](Cell c) {
        return std::hypot(static_cast<double>(c.x) - goal.x, static_cast<double>(c.y) - goal.y);
    };
    Node current{start, 0, h(start), start};
    std::vector<Node> open, closed{current};
    constexpr Cell directions[]{{0,-1}, {0,1}, {-1,0}, {1,0}};
    while (!(current.cell == goal)) {
        for (Cell d : directions) {
            Cell next{current.cell.x + d.x, current.cell.y + d.y};
            if (grid.blocked(next)) continue;
            auto matches = [next](const Node& n) { return n.cell == next; };
            if (std::find_if(closed.begin(), closed.end(), matches) != closed.end()) continue;
            Node neighbor{next, current.g + 1, current.g + 1 + h(next), current.cell};
            auto existing = std::find_if(open.begin(), open.end(), matches);
            if (existing == open.end()) open.push_back(neighbor);
            else if (neighbor.f < existing->f) *existing = neighbor;
        }
        if (open.empty()) return {};
        std::size_t best = 0;
        for (std::size_t i = 0; i < open.size(); ++i)
            if (open[i].f <= open[best].f) best = i; // Original tie rule.
        current = open[best];
        open.erase(open.begin() + best);
        closed.push_back(current);
    }
    std::vector<Cell> path{current.cell};
    while (!(current.cell == start)) {
        Cell parent = current.parent;
        auto found = std::find_if(closed.begin(), closed.end(), [parent](const Node& n) {
            return n.cell == parent;
        });
        if (found == closed.end()) throw std::logic_error("Missing parent");
        current = *found;
        path.push_back(current.cell);
    }
    std::reverse(path.begin(), path.end());
    return path;
}
