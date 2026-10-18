#pragma once
#include <vector>
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
// Includes both endpoints; empty means invalid endpoints or no route.
std::vector<Cell> findPath(const Grid& grid, Cell start, Cell goal);
