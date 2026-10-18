#pragma once
#include <vector>
#include <optional>
struct Cell {
    int x, y;
    bool operator==(const Cell& b) const { return x == b.x && y == b.y; }
};
// Zero-based columns and rows, with row zero at the top.
class Grid {
public:
    Grid(int width, int height);
    int width() const { return width_; }
    int height() const { return height_; }
    bool contains(Cell c) const;
    bool blocked(Cell c) const;
    void setBlocked(Cell c, bool value = true);
private:
    int width_, height_;
    std::vector<unsigned char> obstacles_;
};
enum class SearchStatus { READY, SEARCHING, PATH_FOUND, NO_PATH, INVALID_ENDPOINTS };
enum class CellState { UNVISITED, OPEN, CLOSED, CURRENT };
struct SearchNode {
    Cell cell;
    int g;
    double h, f;
    Cell parent;
};

class AStarSearch {
public:
    AStarSearch(const Grid& grid, Cell start, Cell goal);
    void reset();
    // One iteration: select one OPEN node, close it and expand its neighbors.
    // Terminal calls do nothing and return false.
    bool step();
    SearchStatus status() const { return status_; }
    CellState state(Cell cell) const;
    const std::optional<SearchNode>& current() const { return current_; }
    const std::vector<SearchNode>& open() const { return open_; }
    const std::vector<SearchNode>& closed() const { return closed_; }
    const std::vector<Cell>& path() const { return path_; }
    std::size_t iterations() const { return iterations_; }
private:
    Grid grid_;
    Cell start_, goal_;
    SearchStatus status_ = SearchStatus::READY;
    std::optional<SearchNode> current_;
    std::vector<SearchNode> open_, closed_;
    std::vector<Cell> path_;
    std::size_t iterations_ = 0;
    double heuristic(Cell cell) const;
};

// Synchronous convenience wrapper; Simulation uses AStarSearch::step instead.
// Includes both endpoints; empty means invalid endpoints or no route.
std::vector<Cell> findPath(const Grid& grid, Cell start, Cell goal);
