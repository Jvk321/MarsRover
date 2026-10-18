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
AStarSearch::AStarSearch(const Grid& grid, Cell start, Cell goal)
    : grid_(grid), start_(start), goal_(goal) { reset(); }
double AStarSearch::heuristic(Cell c) const {
    return std::hypot(static_cast<double>(c.x) - goal_.x, static_cast<double>(c.y) - goal_.y);
}
void AStarSearch::reset() {
    open_.clear(); closed_.clear(); path_.clear(); current_.reset(); iterations_ = 0;
    status_ = SearchStatus::READY;
    if (grid_.blocked(start_) || grid_.blocked(goal_)) {
        status_ = SearchStatus::INVALID_ENDPOINTS;
        return;
    }
    const double h = heuristic(start_);
    open_.push_back({start_, 0, h, h, start_});
}
bool AStarSearch::step() {
    if (status_ != SearchStatus::READY && status_ != SearchStatus::SEARCHING) return false;
    if (open_.empty()) { status_ = SearchStatus::NO_PATH; return false; }
    std::size_t best = 0;
    for (std::size_t i = 0; i < open_.size(); ++i)
        if (open_[i].f <= open_[best].f) best = i; // Original tie rule.
    const SearchNode node = open_[best];
    open_.erase(open_.begin() + best);
    current_ = node;
    closed_.push_back(node);
    ++iterations_;
    status_ = SearchStatus::SEARCHING;
    if (node.cell == goal_) {
        Cell cursor = goal_;
        path_.push_back(cursor);
        while (!(cursor == start_)) {
            auto found = std::find_if(closed_.begin(), closed_.end(), [cursor](const SearchNode& n) {
                return n.cell == cursor;
            });
            if (found == closed_.end()) throw std::logic_error("Missing parent");
            cursor = found->parent;
            path_.push_back(cursor);
        }
        std::reverse(path_.begin(), path_.end());
        status_ = SearchStatus::PATH_FOUND;
        return true;
    }
    constexpr Cell directions[]{{0,-1}, {0,1}, {-1,0}, {1,0}};
    for (Cell d : directions) {
        Cell next{node.cell.x + d.x, node.cell.y + d.y};
        if (grid_.blocked(next)) continue;
        auto matches = [next](const SearchNode& n) { return n.cell == next; };
        if (std::find_if(closed_.begin(), closed_.end(), matches) != closed_.end()) continue;
        double h = heuristic(next);
        SearchNode neighbor{next, node.g + 1, h, node.g + 1 + h, node.cell};
        auto existing = std::find_if(open_.begin(), open_.end(), matches);
        if (existing == open_.end()) open_.push_back(neighbor);
        else if (neighbor.f < existing->f) *existing = neighbor;
    }
    if (open_.empty()) status_ = SearchStatus::NO_PATH;
    return true;
}
CellState AStarSearch::state(Cell cell) const {
    if (current_ && current_->cell == cell) return CellState::CURRENT;
    auto matches = [cell](const SearchNode& n) { return n.cell == cell; };
    if (std::find_if(open_.begin(), open_.end(), matches) != open_.end()) return CellState::OPEN;
    if (std::find_if(closed_.begin(), closed_.end(), matches) != closed_.end()) return CellState::CLOSED;
    return CellState::UNVISITED;
}
std::vector<Cell> findPath(const Grid& grid, Cell start, Cell goal) {
    AStarSearch search(grid, start, goal);
    while (search.step()) {}
    return search.path();
}
