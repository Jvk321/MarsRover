#include "world_art.h"
#include "art_primitives.h"

namespace {
using namespace Art;
Vector2 point(float x,float y,float z=0) {auto p=project(x,y);p.y-=z;return p;}
void box(float x,float y,float w,float d,float height,Color top,Color side,float haze) {
    Vector2 a=point(x,y),b=point(x+w,y),c=point(x+w,y+d),e=point(x,y+d);
    Vector2 at{a.x,a.y-height},bt{b.x,b.y-height},ct{c.x,c.y-height},et{e.x,e.y-height};
    quad(b,c,ct,bt,fog(side,haze));quad(c,e,et,ct,fog(mix(side,Color{34,39,49,255},0.32f),haze));
    quad(at,bt,ct,et,fog(top,haze));
    DrawLineEx(at,bt,1,Fade(fog(VisualConfig::sunRim,haze),0.7f));
    DrawLineEx(bt,ct,0.8f,fog(mix(top,VisualConfig::sunRim,0.5f),haze));
}
int sites(const Grid& grid) {
    if(std::min(grid.width(),grid.height())<12) return 0;
    return grid.width()>=55?3:grid.width()>=30?2:1;
}
Vector2 site(const Grid& grid,int i) {
    if(i==1) return {grid.width()*0.60f,-3.6f};
    if(i==2) return {grid.width()+3.2f,grid.height()*0.65f};
    return {-3.8f,std::max(1.0f,grid.height()*0.22f)};
}
void lamp(Vector2 p,float time,float haze) {
    DrawLineEx(p,{p.x,p.y-24},1.4f,fog({100,113,121,255},haze));
    Vector2 light{p.x,p.y-24};
    DrawCircleGradient(light,12,Fade(ORANGE,0.19f*(1-haze)),Fade(ORANGE,0));
    DrawCircleV(light,1.7f,Fade(Color{255,216,139,255},1-haze*0.6f));
    DrawCircleV({light.x+2,light.y},0.65f,Fade(RAYWHITE,0.7f+0.2f*std::sin(time*2)));
}
}
std::size_t WorldArt::props(const Grid& grid,std::uint32_t seed,Props& out) {
    std::size_t count=0;
    // One compact outpost on small maps; extra stations on large terrain only.
    for(int i=0;i<sites(grid);++i) {
        Vector2 a=site(grid,i);
        auto add=[&](PropKind kind,float x,float y){
            out[count]={kind,a.x+x,a.y+y,hash(i,static_cast<int>(count),seed)};++count;
        };
        add(PropKind::HABITAT,-0.8f,-0.7f);
        if(i==0) add(PropKind::DOME,0.1f,1.9f);
        add(PropKind::SOLAR,1.35f,-0.3f);
        add(PropKind::ANTENNA,-0.8f,1.35f);
        add(PropKind::CRATES,0.85f,1.3f);
        add(PropKind::LAMP,1.7f,1.9f);
    }
    return count;
}
void WorldArt::baseGround(const Grid& grid,Rectangle visible) {
    for(int i=0;i<sites(grid);++i) {
        Vector2 s=site(grid,i),p=project(s.x,s.y);
        if(!CheckCollisionPointRec(p,{visible.x-160,visible.y-100,visible.width+320,visible.height+200})) continue;
        Vector2 a=point(s.x-1.1f,s.y-1.2f),b=point(s.x+2.6f,s.y-1.2f),
                c=point(s.x+2.6f,s.y+3.0f),d=point(s.x-1.1f,s.y+3.0f);
        quad(b,c,{c.x,c.y+28},{b.x,b.y+28},{91,38,29,255});
        quad(c,d,{d.x,d.y+28},{c.x,c.y+28},{64,29,29,255});
        quad(a,b,c,d,{148,64,35,255});
        // A rocky apron joins the outpost to the plateau; no detached floating island.
        // It is drawn below logical map tiles and remains outside editable coordinates.
        Vector2 bridge[4];
        if(i==0) {
            bridge[0]=point(s.x+2.4f,s.y-0.8f);bridge[1]=point(0.1f,s.y-0.5f);
            bridge[2]=point(0.1f,s.y+2.7f);bridge[3]=point(s.x+2.4f,s.y+2.9f);
        } else if(i==1) {
            bridge[0]=point(s.x-0.7f,s.y+2.8f);bridge[1]=point(s.x+2.3f,s.y+2.8f);
            bridge[2]=point(s.x+2.1f,0.1f);bridge[3]=point(s.x-0.5f,0.1f);
        } else {
            bridge[0]=point(grid.width()-0.1f,s.y-0.8f);bridge[1]=point(s.x-0.9f,s.y-1.0f);
            bridge[2]=point(s.x-0.9f,s.y+2.7f);bridge[3]=point(grid.width()-0.1f,s.y+2.5f);
        }
        quad(bridge[2],bridge[3],{bridge[3].x,bridge[3].y+28},{bridge[2].x,bridge[2].y+28},{96,40,30,255});
        quad(bridge[0],bridge[1],bridge[2],bridge[3],{164,69,35,255});
        DrawLineEx(bridge[2],bridge[3],0.8f,{228,123,61,150});
        DrawLineEx(b,c,1,{235,136,69,190});
        for(int k=0;k<4;++k) {
            auto t=lerp(c,d,k/4.0f);DrawLineEx(t,{t.x,t.y+24},0.65f,{45,25,28,150});
        }
    }
}
void WorldArt::propShadow(const Prop& p) {
    float height=p.kind==PropKind::ANTENNA?54:p.kind==PropKind::LAMP?24:24;
    shadow(project(p.x,p.y),p.kind==PropKind::HABITAT?36.0f:p.kind==PropKind::DOME?30.0f:14.0f,height,0.22f);
}
void WorldArt::prop(const Prop& p,bool detail,float haze,float time) {
    Vector2 center=point(p.x,p.y);
    switch(p.kind) {
    case PropKind::HABITAT: {
        box(p.x-0.4f,p.y-0.4f,1.45f,0.90f,24,{218,205,183,255},{147,151,150,255},haze);
        box(p.x-0.25f,p.y-0.22f,1.10f,0.55f,28,mix(Color{234,221,192,255},Color{188,199,196,255},(p.variant%4)*0.07f),{174,160,134,255},haze);
        for(int i=0;i<3;++i) {
            Vector2 a=point(p.x-0.22f+i*0.40f,p.y+0.505f,18),b=point(p.x+0.03f+i*0.40f,p.y+0.505f,18);
            quad(a,b,{b.x,b.y+6},{a.x,a.y+6},fog({25,47,59,255},haze));
            DrawLineEx({a.x,a.y+6},{b.x,b.y+6},0.9f,Fade(ORANGE,0.75f));
        }
        Vector2 door=point(p.x+1.055f,p.y+0.08f,18);
        DrawRectangle(static_cast<int>(door.x-3),static_cast<int>(door.y),6,14,fog({33,47,55,255},haze));
        DrawLineEx(point(p.x-0.3f,p.y+0.52f,4),point(p.x+0.9f,p.y+0.52f,4),2,fog({207,89,34,255},haze));
        if(detail) for(int i=0;i<3;++i) DrawLineEx(point(p.x+i*0.35f,p.y-0.4f,25),point(p.x+i*0.35f,p.y+0.5f,25),0.7f,fog({101,111,118,255},haze));
        break;
    }
    case PropKind::DOME: {
        auto domePoint=[&](float az,float elevation) {
            float r=0.72f*std::cos(elevation);
            return point(p.x+std::cos(az)*r,p.y+std::sin(az)*r,5+std::sin(elevation)*27);
        };
        for(int band=0;band<4;++band) for(int slice=0;slice<12;++slice) {
            float a=slice*2*pi/12,b=(slice+1)*2*pi/12,lo=band*pi/8,hi=(band+1)*pi/8;
            Color glass=mix(Color{36,64,75,255},Color{211,123,61,255},0.24f+0.50f*(1+std::cos(a))/2);
            quad(domePoint(a,lo),domePoint(b,lo),domePoint(b,hi),domePoint(a,hi),fog(glass,haze));
        }
        for(int slice=0;slice<12;++slice) for(int band=0;band<4;++band)
            DrawLineEx(domePoint(slice*2*pi/12,band*pi/8),domePoint(slice*2*pi/12,(band+1)*pi/8),0.8f,fog({200,187,159,255},haze));
        ring({center.x,center.y-5},32,fog({216,194,155,255},haze),1.6f);
        ring({center.x,center.y-19},25,fog({187,178,157,255},haze),0.8f);
        break;
    }
    case PropKind::SOLAR: {
        auto panel=[&](float x,float y){return point(p.x+x,p.y+y,11+y*8);};
        for(int i=0;i<2;++i) {
            float x=i*0.7f;
            Vector2 a=panel(x,0),b=panel(x+0.62f,0),c=panel(x+0.62f,1.0f),d=panel(x,1.0f);
            quad(a,b,c,d,fog({22,52,85,255},haze));
            for(int j=0;j<=4;++j) DrawLineEx(lerp(a,b,j/4.0f),lerp(d,c,j/4.0f),0.5f,fog({104,145,168,255},haze));
            for(int j=0;j<=5;++j) DrawLineEx(lerp(a,d,j/5.0f),lerp(b,c,j/5.0f),0.5f,fog({104,145,168,255},haze));
            DrawLineEx(a,b,1,fog({224,211,179,255},haze));
            DrawLineEx(lerp(a,c,0.5f),point(p.x+x+0.3f,p.y+0.5f),1.6f,fog({104,112,120,255},haze));
        }
        break;
    }
    case PropKind::ANTENNA:
        DrawLineEx(center,{center.x,center.y-55},2,fog({143,149,144,255},haze));
        for(int i=0;i<4;++i) {
            float y=center.y-13-i*10;
            DrawLineEx({center.x-6+i,center.y},{center.x+4-i,y},0.75f,fog({97,101,104,255},haze));
            DrawLineEx({center.x+6-i,center.y},{center.x-4+i,y},0.75f,fog({97,101,104,255},haze));
        }
        DrawEllipse(static_cast<int>(center.x),static_cast<int>(center.y-49),10,4,fog({207,201,175,255},haze));
        DrawLineEx({center.x-11,center.y-48},{center.x+11,center.y-53},1.2f,fog({225,212,177,255},haze));
        DrawCircleV({center.x,center.y-57},1.5f,Fade(ORANGE,1-haze));break;
    case PropKind::CRATES:
        box(p.x,p.y,0.42f,0.38f,8,{177,162,121,255},{97,98,93,255},haze);
        box(p.x-0.32f,p.y+0.28f,0.30f,0.32f,6,{156,140,101,255},{86,89,89,255},haze);break;
    case PropKind::LAMP:lamp(center,time,haze);break;
    }
}
void WorldArt::startPad(Cell start,float time) {
    float pulse=0.75f+0.15f*std::sin(time*2);
    diamond(start,0.04f,Fade(VisualConfig::startGreen,0.055f));
    diamond(start,0.04f,Fade(VisualConfig::startGreen,0.12f*pulse),true,6);
    diamond(start,0.04f,Fade(VisualConfig::startGreen,0.85f*pulse),true,1.1f);
    for(auto p:{point(start.x+0.08f,start.y+0.08f),point(start.x+0.92f,start.y+0.92f)})
        DrawCircleV(p,1.6f,VisualConfig::startGreen);
}
void WorldArt::beacon(Cell goal,float time,bool complete) {
    Vector2 p=point(goal.x+0.5f,goal.y+0.5f);
    float power=complete?1.35f:1.0f,pulse=0.7f+0.3f*(std::sin(time*2.7f)+1)*0.5f;
    Color orange{255,158,35,255};
    diamond(goal,0.04f,Fade(orange,VisualConfig::beaconGlow*0.20f*power));
    diamond(goal,0.04f,Fade(orange,0.19f*power),true,8);
    diamond(goal,0.04f,Fade(VisualConfig::pathCore,0.95f),true,1.4f);
    for(int i=0;i<(complete?4:2);++i) {
        float t=std::fmod(time*VisualConfig::beaconSignalSpeed+i/(complete?4.0f:2.0f),1.0f);
        ring(p,8+t*35,Fade(orange,(1-t)*0.42f*power),1.1f);
    }
    box(goal.x+0.24f,goal.y+0.24f,0.52f,0.52f,8,{217,198,143,255},{65,73,78,255},0);
    for(float side:{-1.0f,1.0f}) {
        DrawCircleV({p.x+side*9,p.y-4},2,{255,212,115,255});
        DrawLineEx({p.x+side*7,p.y-8},{p.x+side*6,p.y-16},1.2f,{184,170,137,255});
    }
    float hover=std::sin(time*1.7f)*2;
    Vector2 b{p.x,p.y-37-hover};
    DrawLineEx({p.x,p.y-8},{b.x,b.y},11,Fade(orange,0.055f*power));
    DrawLineEx({p.x,p.y-8},{b.x,b.y},1.1f,Fade(orange,0.45f*pulse));
    Vector2 top[4]{{b.x-12,b.y-24},{b.x,b.y-30},{b.x+12,b.y-24},{b.x,b.y-18}};
    Vector2 bottom[4];for(int i=0;i<4;++i) bottom[i]={top[i].x,top[i].y+18};
    quad(top[0],top[1],top[2],top[3],Fade(orange,0.07f));
    quad(top[2],top[3],bottom[3],bottom[2],Fade(orange,0.06f));
    for(int i=0;i<4;++i) for(int edge=0;edge<3;++edge) {
        Vector2 a=edge==0?top[i]:bottom[i];
        Vector2 end=edge==0?top[(i+1)%4]:edge==1?bottom[(i+1)%4]:top[i];
        DrawLineEx(a,end,5,Fade(orange,VisualConfig::beaconGlow*0.55f*power));
        DrawLineEx(a,end,1.4f,Fade(Color{255,231,135,255},pulse));
    }
    DrawCircleGradient({p.x,p.y-49-hover},22,Fade(orange,VisualConfig::beaconGlow*0.32f*power),Fade(orange,0));
}
void WorldArt::background(Rectangle v,const Camera2D& camera,const VisualEffects& effects) {
    const auto& storm=effects.sandstorm();
    DrawRectangleGradientV(static_cast<int>(v.x),static_cast<int>(v.y),static_cast<int>(v.width),static_cast<int>(v.height),
                           fog(VisualConfig::skyTop,storm.backgroundHaze(0)*0.6f),fog(VisualConfig::skyBottom,storm.backgroundHaze(0)*0.8f));
    Vector2 planet{v.x+v.width*0.84f-camera.target.x*0.005f,v.y+v.height*0.14f};
    float radius=std::min(v.width*0.15f,v.height*0.19f);
    DrawCircleGradient(planet,radius*1.4f,Fade(Color{249,138,72,255},0.14f),Fade(ORANGE,0));
    DrawCircleGradient(planet,radius,fog({189,99,65,255},storm.backgroundHaze(0)),fog({126,66,59,255},storm.backgroundHaze(0)));
    for(int i=1;i<22;++i) {
        float y=-radius+i*radius*2/22;
        float chord=std::sqrt(std::max(0.0f,radius*radius-y*y));
        DrawLineEx({planet.x-chord,planet.y+y},{planet.x+chord,planet.y+y},2+hash(i,7,42)%4,
                   Fade(Color{225,148,91,255},0.025f+(hash(i,8,42)%4)*0.015f));
    }
    DrawCircleV({planet.x-radius*0.25f,planet.y+radius*0.07f},radius*0.94f,fog({77,48,52,255},storm.backgroundHaze(0)));
    for(int layer=0;layer<3;++layer) {
        float rate=layer==0?VisualConfig::farParallax:VisualConfig::nearParallax*(layer==1?0.60f:1);
        float shift=std::fmod(camera.target.x*rate,240.0f);
        float baseline=v.y+v.height*(0.43f+layer*0.16f)-camera.target.y*rate*0.2f;
        Color color=fog(layer==0?Color{116,65,61,255}:layer==1?Color{88,43,43,255}:Color{59,31,36,255},storm.backgroundHaze(layer==0?0:1));
        DrawRectangle(static_cast<int>(v.x),static_cast<int>(baseline),static_cast<int>(v.width),
                      std::max(0,static_cast<int>(v.y+v.height-baseline)),color);
        for(int i=-2;i<static_cast<int>(v.width/140)+3;++i) {
            auto h=hash(i+20,layer,4721);float x=v.x+i*190.0f-shift,height=40+(h%90)+layer*12.0f;
            Vector2 ridge[]{{x-70,baseline},{x-45,baseline-height*0.45f},{x-30,baseline-height},
                            {x+15,baseline-height-((h>>8)%13)},{x+38,baseline-height*0.72f},{x+90,baseline-height*0.55f},{x+145,baseline}};
            Vector2 bottom{x+45,baseline+2};
            for(int j=0;j<6;++j) triangle(ridge[j],ridge[j+1],bottom,color);
            if(layer>0) {
                triangle(ridge[3],ridge[4],{ridge[4].x-8,baseline+25},mix(color,{33,25,34,255},0.13f));
                for(int j=1;j<5;++j) DrawLineEx(ridge[j],ridge[j+1],0.75f,Fade(fog(VisualConfig::sunRim,storm.backgroundHaze(0)),0.10f));
            }
        }
    }
    float bx=v.x+v.width*0.19f-camera.target.x*0.025f,by=v.y+v.height*0.44f;
    for(int i=0;i<3;++i) {
        DrawRectangle(static_cast<int>(bx+i*14),static_cast<int>(by-9),11,9,{85,64,65,255});
        DrawCircleV({bx+i*14+7,by-4},0.8f,{255,180,100,190});
    }
    DrawLineEx({bx+43,by},{bx+43,by-31},1,{117,85,80,255});
    DrawCircleV({bx+43,by-31},1.2f,{255,151,87,210});
    DrawRectangleGradientV(static_cast<int>(v.x),static_cast<int>(v.y+v.height-100),static_cast<int>(v.width),100,{11,15,24,0},{11,15,24,80});
}
