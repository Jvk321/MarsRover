#include "terrain_art.h"
#include "art_primitives.h"

namespace {
using namespace Art;
int neighbors(const Grid& grid,Cell c) {
    int count=0;
    for(Cell d:{Cell{1,0},Cell{0,1},Cell{-1,0},Cell{0,-1}})
        if(grid.contains({c.x+d.x,c.y+d.y}) && grid.blocked({c.x+d.x,c.y+d.y})) ++count;
    return count;
}
float heightAt(float x,float y,std::uint32_t seed) {
    float grain=(hash(static_cast<int>(x*2),static_cast<int>(y*2),seed^0x7829u)%1000)/1000.0f;
    return VisualConfig::cliffHeight+noise(x,y,seed^0xf12ab36u,3)*VisualConfig::cliffVariation*0.55f+grain*13;
}
Vector2 rimPoint(float x,float y,std::uint32_t seed) {
    Vector2 p=project(x,y);auto h=hash(static_cast<int>(x*2),static_cast<int>(y*2),seed^0xac712u);
    return {p.x+(static_cast<int>(h%101)-50)*0.075f,p.y-heightAt(x,y,seed)+(static_cast<int>((h>>8)%101)-50)*0.025f};
}
void strata(Vector2 a,Vector2 b,Vector2 lowA,Vector2 lowB,Color color,std::uint32_t h,bool detail) {
    for(int band=0;band<4;++band) {
        float t=band/4.0f,u=(band+1)/4.0f;
        Color shade=mix(color,VisualConfig::rockShade,0.15f+band*0.10f);
        quad(lerp(a,lowA,t),lerp(b,lowB,t),lerp(b,lowB,u),lerp(a,lowA,u),shade);
        if(detail && band) {
            Vector2 start=lerp(a,lowA,t),end=lerp(b,lowB,t);
            DrawLineEx(start,end,0.65f,Fade(Color{42,24,27,255},0.35f));
        }
    }
    if(detail) for(int i=0;i<3;++i) {
        float t=(i+0.25f+(h%7)*0.03f)/3;
        Vector2 p=lerp(a,b,t),q=lerp(lowA,lowB,t+0.05f);
        DrawLineEx(p,lerp(p,q,0.86f),0.8f,Fade(Color{50,23,25,255},0.55f));
        DrawLineEx({p.x+1.1f,p.y+2},{q.x+1,q.y-3},0.5f,Fade(VisualConfig::sunRim,0.19f));
    }
}
void boulder(Vector2 p,float radius,float height,std::uint32_t h,bool detail,float haze) {
    constexpr int n=7;
    Vector2 base[n],top[n];
    for(int i=0;i<n;++i) {
        auto v=hash(i,3,h);float a=i*2*pi/n;
        float r=radius*(0.81f+(v%100)*0.002f);
        base[i]={p.x+std::cos(a)*r,p.y+std::sin(a)*r*0.48f};
        top[i]={p.x+std::cos(a)*r*(0.55f+(v%9)*0.025f),
                p.y+std::sin(a)*r*0.3f-height*(0.78f+((v>>8)%20)*0.01f)};
    }
    for(int i=0;i<n;++i) {
        int j=(i+1)%n;
        Color side=mix(VisualConfig::rockShade,Color{180,74,35,255},0.20f+0.50f*(1+std::cos(i*2*pi/n))/2);
        quad(base[i],base[j],top[j],top[i],fog(side,haze));
        if(detail) {
            Vector2 split=lerp(base[i],top[j],0.5f);
            triangle(top[i],top[j],split,fog(mix(side,VisualConfig::rockTop,0.20f),haze));
            DrawLineEx(lerp(top[i],base[i],0.33f),lerp(top[j],base[j],0.42f),0.6f,Fade(Color{53,23,25,255},0.43f));
        }
    }
    Vector2 peak{p.x+radius*0.08f,p.y-height*1.03f};
    for(int i=0;i<n;++i) {
        Color cap=mix(VisualConfig::rockTop,VisualConfig::sunRim,0.08f+(hash(i,0,h)%25)*0.01f);
        triangle(top[i],top[(i+1)%n],peak,fog(cap,haze));
        if(detail && i%2==0) DrawLineEx(top[i],peak,0.65f,Fade(fog(VisualConfig::sunRim,haze),0.30f));
    }
    if(detail) DrawLineEx(top[4],top[5],1.0f,Fade(fog(VisualConfig::sunRim,haze),VisualConfig::rimAlpha));
}
}
bool TerrainArt::cliff(const Grid& grid,Cell c) {
    return grid.contains(c) && grid.blocked(c) && neighbors(grid,c)>=2;
}
float TerrainArt::cliffHeight(int x,int y,std::uint32_t seed) {
    return heightAt(static_cast<float>(x),static_cast<float>(y),seed);
}
void TerrainArt::ground(Cell c,std::uint32_t seed,bool detail,float haze) {
    auto h=hash(c.x,c.y,seed);
    float broad=noise(static_cast<float>(c.x),static_cast<float>(c.y),seed);
    float tone=(broad-0.5f)*0.48f+(static_cast<int>(h%7)-3)*0.006f;
    Color color=tone<0?mix(VisualConfig::regolith,Color{105,44,32,255},-tone):mix(VisualConfig::regolith,Color{241,139,66,255},tone);
    diamond(c,0,fog(color,haze));
    if(!detail || haze>0.55f) return;
    for(int i=0;i<VisualConfig::terrainDetailCount;++i) {
        h=h*1664525u+1013904223u;float x=c.x+0.08f+(h%840)/1000.0f;
        h=h*1664525u+1013904223u;float y=c.y+0.08f+(h%840)/1000.0f;
        Vector2 p=project(x,y);
        if(i==0) {
            DrawEllipse(static_cast<int>(p.x),static_cast<int>(p.y),8,3,Color{76,33,28,20});
        } else if(i==1 && h%3==0) {
            DrawLineEx(p,{p.x+5,p.y+2},0.65f,{69,30,25,125});
            DrawLineEx({p.x+5,p.y+2},{p.x+8,p.y},0.65f,{69,30,25,115});
            DrawLineEx({p.x+5,p.y+2},{p.x+4,p.y+4},0.55f,{69,30,25,100});
        } else if(i==2 && h%2==0) {
            for(int k=0;k<3;++k) DrawLineEx({p.x-5-k,p.y+k*1.7f},{p.x+5-k,p.y+2+k*1.7f},0.55f,Color{244,162,82,27});
        } else {
            float r=0.9f+(h%4)*0.23f;
            DrawEllipse(static_cast<int>(p.x-1),static_cast<int>(p.y+1),r*1.7f,r*0.6f,{55,27,25,65});
            triangle({p.x-r,p.y},{p.x,p.y-r*0.8f},{p.x+r,p.y},Color{116,47,29,170});
            DrawLineEx({p.x,p.y-r*0.8f},{p.x+r,p.y},0.55f,Color{242,151,76,165});
        }
    }
}
void TerrainArt::gridLines(const Grid& grid,Rectangle visible,bool active,float zoom) {
    Color color=Fade(active?Color{144,194,221,255}:Color{235,161,104,255},active?VisualConfig::activeGridAlpha:VisualConfig::gridAlpha);
    float thickness=active?std::max(0.7f,0.45f/zoom):0.45f;
    auto line=[&](Vector2 a,Vector2 b) {
        Rectangle bounds{std::min(a.x,b.x),std::min(a.y,b.y),std::abs(a.x-b.x)+1,std::abs(a.y-b.y)+1};
        if(CheckCollisionRecs(bounds,visible)) DrawLineEx(a,b,thickness,color);
    };
    for(int x=0;x<=grid.width();++x) line(project(static_cast<float>(x),0),project(static_cast<float>(x),static_cast<float>(grid.height())));
    for(int y=0;y<=grid.height();++y) line(project(0,static_cast<float>(y)),project(static_cast<float>(grid.width()),static_cast<float>(y)));
}
void TerrainArt::shadow(const Grid& grid,Cell c,std::uint32_t seed,float haze) {
    Art::shadow(project(c.x+0.5f,c.y+0.5f),cliff(grid,c)?27.0f:20.0f,
                cliff(grid,c)?cliffHeight(c.x,c.y,seed):VisualConfig::rockHeight+(hash(c.x,c.y,seed)%18),
                VisualConfig::shadowAlpha*(1-haze));
}
void TerrainArt::obstacle(const Grid& grid,Cell c,std::uint32_t seed,bool detail,float haze) {
    auto h=hash(c.x,c.y,seed);auto p=project(c.x+0.5f,c.y+0.5f);
    if(!cliff(grid,c)) {
        switch(h%4) {
        case 0: boulder(p,22,VisualConfig::rockHeight+(h%18),h,detail,haze);break;
        case 1:
            boulder({p.x-9,p.y-3},14,28,h,detail,haze);
            boulder({p.x+10,p.y+4},14,20,h+1,detail,haze);break;
        case 2: boulder(p,25,VisualConfig::rockHeight*1.5f,h,detail,haze);break;
        default:
            DrawEllipse(static_cast<int>(p.x),static_cast<int>(p.y),26,12,fog({100,38,27,255},haze));
            DrawEllipse(static_cast<int>(p.x),static_cast<int>(p.y-3),24,11,fog({224,115,54,255},haze));
            DrawEllipse(static_cast<int>(p.x),static_cast<int>(p.y-2),18,8,fog({66,31,30,255},haze));
            boulder({p.x+18,p.y+4},9,14,h,detail,haze);break;
        }
        return;
    }
    const Cell adjacent[]{{c.x,c.y-1},{c.x+1,c.y},{c.x,c.y+1},{c.x-1,c.y}};
    const Vector2 offsets[]{{0,0},{0.5f,0},{1,0},{1,0.5f},{1,1},{0.5f,1},{0,1},{0,0.5f}};
    Vector2 base[8],top[8];
    for(int i=0;i<8;i+=detail?1:2) {
        float x=c.x+offsets[i].x,y=c.y+offsets[i].y;
        base[i]=project(x,y);top[i]=rimPoint(x,y,seed);
    }
    for(int i=0;i<4;++i) if(!cliff(grid,adjacent[i])) {
        Color face=i==1?Color{174,68,34,255}:i==2?Color{108,42,30,255}:Color{137,49,29,255};
        if(detail) for(int half=0;half<2;++half) {
            int a=i*2+half,b=(a+1)%8;
            strata(top[a],top[b],base[a],base[b],fog(mix(face,VisualConfig::rockShade,half*0.12f),haze),h,detail);
        } else strata(top[i*2],top[(i*2+2)%8],base[i*2],base[(i*2+2)%8],fog(face,haze),h,false);
    }
    Color cap=fog(mix(VisualConfig::rockTop,Color{238,143,68,255},noise(c.x,c.y,seed,5)*0.30f),haze);
    Vector2 center=rimPoint(c.x+0.5f,c.y+0.5f,seed);center.y-=3;
    int step=detail?1:2;
    for(int i=0;i<8;i+=step) {
        float variation=(hash(i,4,h)%100)/1000.0f;
        triangle(top[i],top[(i+step)%8],center,mix(cap,fog(VisualConfig::rockShade,haze),variation));
    }
    if(detail) for(int i=0;i<4;++i) if(!cliff(grid,adjacent[i])) for(int half=0;half<2;half+=step) {
        int a=i*2+half,b=(a+step)%8;
        DrawLineEx(top[a],top[b],1.0f,Fade(fog(VisualConfig::sunRim,haze),VisualConfig::rimAlpha));
    }
    if(detail) {
        for(int i=0;i<3;++i) {
            auto v=hash(i,8,h);Vector2 t{center.x-12+(v%25),center.y-4+((v>>6)%9)};
            DrawLineEx(t,{t.x+6,t.y+2},0.7f,Fade(Color{78,34,28,255},0.42f));
        }
        if(h%2==0) boulder({center.x+5,center.y+1},7,7,h,false,haze);
    }
}
void TerrainArt::plateau(const Grid& grid,std::uint32_t seed,Rectangle visible,bool detail) {
    const float depth=VisualConfig::plateauDepth*std::min(1.0f,std::min(grid.width(),grid.height())/12.0f);
    auto edge=[&](Cell a,Cell b,bool sunlit) {
        Vector2 p=project(static_cast<float>(a.x),static_cast<float>(a.y)),q=project(static_cast<float>(b.x),static_cast<float>(b.y));
        if(p.x<visible.x-80 || p.x>visible.x+visible.width+80 || p.y<visible.y-VisualConfig::plateauDepth-35 || p.y>visible.y+visible.height) return;
        Vector2 lowP{p.x-3,p.y+depth+noise(a.x,a.y,seed,3)*depth*0.27f};
        Vector2 lowQ{q.x-3,q.y+depth+noise(b.x,b.y,seed,3)*depth*0.27f};
        strata(p,q,lowP,lowQ,sunlit?Color{145,53,31,255}:Color{96,35,29,255},hash(a.x,a.y,seed),detail);
        auto h=hash(a.x,a.y,seed);
        if(h%7==0) {
            Vector2 base=lerp(lowP,lowQ,0.55f);base.y+=8;
            boulder(base,std::min(depth*0.22f,10.0f+(h%9)),depth*0.65f,h,detail,0);
        }
        if(detail) DrawLineEx(p,q,1.1f,Fade(VisualConfig::sunRim,0.45f));
    };
    for(int y=0;y<grid.height();++y) edge({grid.width(),y},{grid.width(),y+1},true);
    for(int x=grid.width()-1;x>=0;--x) edge({x+1,grid.height()},{x,grid.height()},false);
}
