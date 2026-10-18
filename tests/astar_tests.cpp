#include "astar.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
void check(bool value) {
    if (!value) { std::cerr << "A* test failed\n"; std::exit(1); }
}
int main() {
    for (Cell size : {Cell{24,14}, Cell{3,29}, Cell{31,2}, Cell{1,1}}) {
        Grid grid(size.x, size.y);
        Cell goal{size.x-1,size.y-1};
        auto path = findPath(grid, {0,0}, goal);
        check(path.size() == static_cast<std::size_t>(size.x + size.y - 1));
        check(path.front() == Cell{0,0} && path.back() == goal);
    }
    Grid grid(7,4);
    for (int y = 0; y < grid.height(); ++y) grid.setBlocked({3,y});
    check(findPath(grid, {0,0}, {6,0}).empty());
    grid.setBlocked({3,3}, false);
    auto path = findPath(grid, {0,0}, {6,0});
    check(path.size() == 13);
    for (std::size_t i = 0; i < path.size(); ++i) {
        check(!grid.blocked(path[i]));
        if (i) check(std::abs(path[i].x-path[i-1].x) + std::abs(path[i].y-path[i-1].y) == 1);
    }
    check(findPath(grid, {-1,0}, {6,0}).empty());
    check(findPath(grid, {0,0}, {7,0}).empty());
    check(findPath(grid, {0,0}, {3,0}).empty());
    check(findPath(grid, {2,2}, {2,2}).size() == 1);
    bool rejected = false;
    try { Grid invalid(0,4); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected);
    std::cout << "A* tests passed\n";
}
