#include "astar.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
void check(bool value) {
    if (!value) { std::cerr << "A* test failed\n"; std::exit(1); }
}
int main() {
    Grid large(33,25);
    AStarSearch incremental(large, {1,1}, {31,23});
    check(incremental.status() == SearchStatus::READY);
    check(incremental.iterations() == 0 && incremental.path().empty());
    check(incremental.open().size() == 1 && !incremental.current());
    check(incremental.step());
    check(incremental.iterations() == 1 && incremental.closed().size() == 1);
    check(incremental.current()->cell == Cell{1,1});
    check(incremental.state({1,1}) == CellState::CURRENT);
    check(incremental.state({1,0}) == CellState::OPEN);
    check(incremental.open().size() == 4 && incremental.path().empty());
    while (incremental.status() == SearchStatus::SEARCHING) {
        const auto before = incremental.iterations();
        check(incremental.path().empty());
        check(incremental.step());
        check(incremental.iterations() == before + 1);
        check(incremental.iterations() <= 33*25);
    }
    check(incremental.status() == SearchStatus::PATH_FOUND);
    check(incremental.path().size() == 53);
    const auto finalIterations = incremental.iterations();
    check(!incremental.step() && incremental.iterations() == finalIterations);
    incremental.reset();
    check(incremental.status() == SearchStatus::READY && incremental.iterations() == 0);
    check(incremental.path().empty() && incremental.closed().empty() && !incremental.current());
    Grid isolated(2,1);
    isolated.setBlocked({1,0});
    AStarSearch invalid(isolated,{0,0},{1,0});
    check(invalid.status() == SearchStatus::INVALID_ENDPOINTS && !invalid.step());
    Grid separated(3,1);
    separated.setBlocked({1,0});
    AStarSearch unreachable(separated,{0,0},{2,0});
    check(unreachable.step() && unreachable.status() == SearchStatus::NO_PATH);
    check(!unreachable.step() && unreachable.iterations() == 1);
    AStarSearch same(large,{4,4},{4,4});
    check(same.path().empty() && same.step());
    check(same.status() == SearchStatus::PATH_FOUND && same.path().size() == 1);
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
