#include "terrain.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>

bool connected(const Grid& grid,Cell start,Cell goal) {
    if (grid.blocked(start) || grid.blocked(goal)) return false;
    std::vector<unsigned char> visited(static_cast<std::size_t>(grid.width())*grid.height());
    auto index=[&](Cell c){ return c.y*grid.width()+c.x; };
    std::queue<Cell> queue; queue.push(start); visited[index(start)]=1;
    while (!queue.empty()) {
        Cell c=queue.front(); queue.pop();
        if (c==goal) return true;
        for (Cell d : {Cell{0,-1},Cell{0,1},Cell{-1,0},Cell{1,0}}) {
            Cell next{c.x+d.x,c.y+d.y};
            if (!grid.blocked(next) && !visited[index(next)]) {
                visited[index(next)]=1; queue.push(next);
            }
        }
    }
    return false;
}
namespace {
Grid makeGrid(const TerrainConfig& c) {
    if (c.width<2 || c.height<2 || c.width>512 || c.height>512 || c.density<0 || c.density>95)
        throw std::invalid_argument("Width/height: 2..512; density: 0..95");
    return Grid(c.width,c.height);
}
}
Terrain::Terrain(TerrainConfig c) : grid(makeGrid(c)),
    start{c.width>2 ? 1 : 0,c.height>2 ? 1 : 0}, goal{c.width-2,c.height-2},config(c) {
    if (start==goal) goal={c.width-1,c.height-1};
    std::mt19937 rng(c.seed);
    auto random=[&](int n){ return static_cast<int>(rng()%static_cast<unsigned>(n)); };
    const int count=c.width*c.height;
    std::vector<float> scores(count);
    for (float& score:scores) score=random(1000)/1000.0f*0.18f;
    // Overlapping elliptical clusters, crater rims and occasional single boulders.
    for (int i=0;i<std::max(8,count/24);++i) {
        int cx=random(c.width),cy=random(c.height),rx=1+random(5),ry=1+random(4);
        bool crater=random(4)==0;
        float strength=1+random(100)/100.0f;
        for (int y=std::max(0,cy-ry-1);y<=std::min(c.height-1,cy+ry+1);++y)
            for (int x=std::max(0,cx-rx-1);x<=std::min(c.width-1,cx+rx+1);++x) {
                float dx=static_cast<float>(x-cx)/rx,dy=static_cast<float>(y-cy)/ry;
                float r=std::sqrt(dx*dx+dy*dy);
                float shape=crater ? std::max(0.0f,1-std::abs(r-0.8f)*4) : std::max(0.0f,1-r);
                scores[y*c.width+x]+=shape*strength;
            }
    }
    for (int i=0;i<count/100;++i) scores[random(count)]+=1.8f;
    std::vector<int> order(count); std::iota(order.begin(),order.end(),0);
    std::sort(order.begin(),order.end(),[&](int a,int b){ return scores[a]==scores[b] ? a<b : scores[a]>scores[b]; });
    int target=count*c.density/100, placed=0;
    for (int index:order) {
        Cell cell{index%c.width,index/c.width};
        if (placed>=target) break;
        if (cell==start || cell==goal) continue;
        grid.setBlocked(cell); ++placed;
    }
    // Validate, then repair only a narrow connecting corridor if formations cut the route.
    // This is deterministic and bounded even at very high requested density.
    if (!connected(grid,start,goal)) {
        Cell cursor=start;
        while (!(cursor==goal)) {
            if (cursor.x!=goal.x && (cursor.y==goal.y || random(2)==0)) cursor.x+=(goal.x>cursor.x ? 1 : -1);
            else cursor.y+=(goal.y>cursor.y ? 1 : -1);
            grid.setBlocked(cursor,false);
        }
        if (!connected(grid,start,goal)) throw std::logic_error("Terrain corridor repair failed");
    }
}
bool Terrain::edit(Cell cell,EditTool tool) {
    if (!grid.contains(cell)) return false;
    if (tool==EditTool::SET_START || tool==EditTool::SET_GOAL) {
        if (grid.blocked(cell)) return false;
        Cell& endpoint=tool==EditTool::SET_START ? start : goal;
        if (endpoint==cell) return false;
        endpoint=cell; return true;
    }
    if (cell==start || cell==goal) return false;
    bool blocked=tool==EditTool::ADD_ROCK;
    if (grid.blocked(cell)==blocked) return false;
    grid.setBlocked(cell,blocked); return true;
}
int Terrain::obstacleCount() const {
    int count=0;
    for (int y=0;y<grid.height();++y) for (int x=0;x<grid.width();++x) if (grid.blocked({x,y})) ++count;
    return count;
}
bool Terrain::stroke(Cell from,Cell to,EditTool tool) {
    if (!grid.contains(from) || !grid.contains(to)) return false;
    bool changed=false;
    int dx=std::abs(to.x-from.x),dy=-std::abs(to.y-from.y);
    int sx=from.x<to.x ? 1 : -1,sy=from.y<to.y ? 1 : -1,error=dx+dy;
    for (;;) {
        changed=edit(from,tool) || changed;
        if (from==to) break;
        int twice=2*error;
        if (twice>=dy) { error+=dy; from.x+=sx; }
        if (twice<=dx) { error+=dx; from.y+=sy; }
    }
    return changed;
}
