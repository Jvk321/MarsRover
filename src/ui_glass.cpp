#include "ui_glass.h"
#include "rlgl.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>

namespace {
struct Tween {std::uint32_t key=0;float value=0;};
struct Resources {
    RenderTexture2D world{};Shader glass{};Font font{};
    bool initialized=false,customFont=false,inWorld=false;
    int resolution=-1,rectangle=-1,rounding=-1,level=-1,alpha=-1,accent=-1,active=-1;
    int width=0,height=0;float dt=1.0f/60,opacity=1;
    Ui::Quality quality=Ui::Quality::HIGH;Ui::Pass pass=Ui::Pass::BOTH;
    std::array<Tween,512> tweens{};
} state;
constexpr const char* glassFragment=R"glsl(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec2 resolution;
uniform vec4 rectangle;
uniform float rounding;
uniform float level;
uniform float alpha;
uniform vec3 accent;
uniform float emphasis;
uniform float blurRadius;
uniform float refractionPixels;
out vec4 finalColor;
float hash(vec2 p) {return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
void main() {
    vec2 p=vec2(fragTexCoord.x,1.0-fragTexCoord.y)*resolution;
    vec2 halfSize=rectangle.zw*0.5;
    vec2 q=abs(p-rectangle.xy-halfSize)-halfSize+rounding;
    float d=length(max(q,0.0))+min(max(q.x,q.y),0.0)-rounding;
    float mask=1.0-smoothstep(-0.7,0.7,d);
    if(mask<0.001) discard;
    vec2 local=(p-rectangle.xy)/rectangle.zw;
    vec2 bend=(local-0.5)*refractionPixels*smoothstep(-9.0,-0.2,d);
    vec2 uv=fragTexCoord+vec2(bend.x,-bend.y)/resolution;
    vec3 bg=texture(texture0,uv).rgb;
    if(level>0.5) {
        vec2 px=vec2(blurRadius)/resolution;
        bg=bg*0.20;
        bg+=(texture(texture0,uv+vec2(px.x,0)).rgb+texture(texture0,uv-vec2(px.x,0)).rgb)*0.12;
        bg+=(texture(texture0,uv+vec2(0,px.y)).rgb+texture(texture0,uv-vec2(0,px.y)).rgb)*0.12;
        bg+=(texture(texture0,uv+px).rgb+texture(texture0,uv-px).rgb+texture(texture0,uv+vec2(px.x,-px.y)).rgb+texture(texture0,uv+vec2(-px.x,px.y)).rgb)*0.08;
    }
    bg=mix(vec3(dot(bg,vec3(0.2126,0.7152,0.0722))),bg,0.55);
    vec3 glass=mix(bg,vec3(0.035,0.070,0.112),0.78);
    glass+=vec3(0.030,0.045,0.060)*(1.0-local.y)*0.70;
    float edge=1.0-smoothstep(0.2,1.35,abs(d));
    float upper=clamp(1.0-local.y*0.8-local.x*0.32,0.0,1.0);
    glass+=edge*mix(vec3(0.035,0.050,0.065),vec3(0.24,0.31,0.35),upper);
    glass+=accent*emphasis*(0.016+0.08*edge);
    glass+=(hash(floor(p))-0.5)*0.008;
    finalColor=vec4(glass,mask*alpha);
})glsl";
float roundness(Rectangle r,float radius) {return std::min(1.0f,2*radius/std::max(1.0f,std::min(r.width,r.height)));}
}
Ui::Layout Ui::layout(float width,float height) {
    Layout l;l.scale=std::clamp(std::sqrt(height/1080.0f),0.84f,1.25f);
    const float s=l.scale,pad=24*s,gap=12*s,dockWidth=std::min(410*s,width*0.30f);
    l.canvas={0,0,width,height};l.dock={width-pad-dockWidth,pad,dockWidth,height-2*pad};
    l.world={0,0,l.dock.x-gap,height};
    float y=pad;
    auto section=[&](float h) {Rectangle r{l.dock.x,y,dockWidth,h*s};y+=r.height+gap;return r;};
    l.status=section(108);l.navigation=section(146);l.search=section(128);l.mission=section(112);
    l.tabs={l.dock.x,y,dockWidth,42*s};y+=54*s;
    l.settings={l.dock.x,y,dockWidth,std::clamp(height-pad-y,190*s,384*s)};
    float barWidth=std::min(l.world.width-2*pad,980*s);
    l.bar={l.world.width/2-barWidth/2,height-pad-98*s,barWidth,98*s};
    l.minimap={pad,l.bar.y-230*s-20*s,234*s,230*s};
    l.brand={pad,pad,std::min(410*s,l.world.width*0.54f),78*s};
    l.topActions={l.world.width-pad-288*s,pad,288*s,54*s};
    l.editor={pad,l.brand.y+l.brand.height+14*s,std::min(630*s,l.world.width-2*pad),62*s};
    return l;
}
void Ui::init() {
    if(state.initialized || !IsWindowReady()) return;
    state.glass=LoadShaderFromMemory(nullptr,glassFragment);
    state.resolution=GetShaderLocation(state.glass,"resolution");state.rectangle=GetShaderLocation(state.glass,"rectangle");
    state.rounding=GetShaderLocation(state.glass,"rounding");state.level=GetShaderLocation(state.glass,"level");
    state.alpha=GetShaderLocation(state.glass,"alpha");state.accent=GetShaderLocation(state.glass,"accent");state.active=GetShaderLocation(state.glass,"emphasis");
    SetShaderValue(state.glass,GetShaderLocation(state.glass,"blurRadius"),&Theme::blurRadius,SHADER_UNIFORM_FLOAT);
    SetShaderValue(state.glass,GetShaderLocation(state.glass,"refractionPixels"),&Theme::refraction,SHADER_UNIFORM_FLOAT);
    for(const auto& path:{std::filesystem::path(GetApplicationDirectory())/"assets/ui/Inter-Regular.ttf",
                          std::filesystem::path("assets/ui/Inter-Regular.ttf"),std::filesystem::path("C:/Windows/Fonts/segoeui.ttf")}) {
        if(FileExists(path.string().c_str())) {
            state.font=LoadFontEx(path.string().c_str(),80,nullptr,0);
            if(IsFontValid(state.font) && state.font.texture.id!=GetFontDefault().texture.id) {state.customFont=true;SetTextureFilter(state.font.texture,TEXTURE_FILTER_BILINEAR);break;}
        }
    }
    if(!state.customFont) state.font=GetFontDefault();
    state.initialized=true;
}
void Ui::shutdown() {
    if(!state.initialized) return;
    if(state.world.id) UnloadRenderTexture(state.world);
    if(state.glass.id && state.glass.id!=rlGetShaderIdDefault()) UnloadShader(state.glass);
    if(state.customFont) UnloadFont(state.font);
    state=Resources{};
}
bool Ui::glassReady() {return state.initialized && state.glass.id && state.glass.id!=rlGetShaderIdDefault();}
void Ui::beginFrame(float dt,int width,int height) {
    init();state.dt=std::clamp(dt,0.0f,0.1f);state.pass=Pass::BOTH;
    if(width!=state.width || height!=state.height) {
        if(state.world.id) UnloadRenderTexture(state.world);
        state.world=LoadRenderTexture(width,height);SetTextureFilter(state.world.texture,TEXTURE_FILTER_BILINEAR);
        state.width=width;state.height=height;
    }
}
void Ui::beginWorld() {state.inWorld=true;BeginTextureMode(state.world);ClearBackground({8,15,25,255});}
void Ui::endWorld() {EndTextureMode();state.inWorld=false;}
void Ui::drawWorld() {
    DrawTexturePro(state.world.texture,{0,0,float(state.width),-float(state.height)},
                   {0,0,float(state.width),float(state.height)},{0,0},0,WHITE);
}
void Ui::pass(Pass value) {state.pass=value;}
bool Ui::drawing() {return state.pass!=Pass::INPUT;}
bool Ui::input() {return state.pass!=Pass::DRAW;}
float Ui::delta() {return state.dt;}
void Ui::opacity(float value) {state.opacity=std::clamp(value,0.0f,1.0f);}
float Ui::opacity() {return state.opacity;}
void Ui::quality(Quality value) {state.quality=value;}
Ui::Quality Ui::quality() {return state.quality;}
float Ui::measure(const char* value,float size) {return MeasureTextEx(state.initialized?state.font:GetFontDefault(),value,size,size*0.015f).x;}
void Ui::text(const char* value,Vector2 p,float size,Color color) {
    color.a=static_cast<unsigned char>(color.a*state.opacity);
    if(drawing()) DrawTextEx(state.initialized?state.font:GetFontDefault(),value,p,size,size*0.015f,color);
}
void Ui::centered(const char* value,Rectangle r,float size,Color color) {text(value,{r.x+(r.width-measure(value,size))/2,r.y+(r.height-size)/2},size,color);}
void Ui::fitText(const char* value,Rectangle r,float size,Color color) {
    while(size>12 && measure(value,size)>r.width) size-=0.5f;
    text(value,{r.x,r.y+(r.height-size)/2},size,color);
}
std::uint32_t Ui::id(const char* label,Rectangle r) {
    std::uint32_t h=2166136261u;for(;*label;++label) h=(h^static_cast<unsigned char>(*label))*16777619u;
    return (h^static_cast<unsigned>(r.x*3+r.y*7))|1u;
}
float Ui::animate(std::uint32_t key,float target,float initial) {
    auto index=key%state.tweens.size();
    for(std::size_t i=0;i<state.tweens.size();++i) {
        auto& t=state.tweens[(index+i)%state.tweens.size()];
        if(!t.key) {t.key=key;t.value=initial;}
        if(t.key==key) {if(drawing()) t.value+=(target-t.value)*(1-std::exp(-state.dt*14));return t.value;}
    }
    return target;
}
void Ui::panel(Rectangle r,float radius,Color accent,float active,float opacity) {
    if(!drawing() || r.width<=0 || r.height<=0) return;
    opacity*=state.opacity;
    for(int i=3;i>0;--i) DrawRectangleRounded({r.x-i*2.0f,r.y+i*2.0f,r.width+i*4.0f,r.height+i*2.0f},roundness(r,radius+2*i),10,Fade(Color{2,7,15,255},0.065f*opacity));
    if(state.initialized && state.world.id && !state.inWorld && state.glass.id!=rlGetShaderIdDefault()) {
        const float resolution[]{float(state.width),float(state.height)},rect[]{r.x,r.y,r.width,r.height};
        const float level=quality()==Quality::HIGH?1.0f:0.0f,accentValue[]{accent.r/255.0f,accent.g/255.0f,accent.b/255.0f};
        SetShaderValue(state.glass,state.resolution,resolution,SHADER_UNIFORM_VEC2);
        SetShaderValue(state.glass,state.rectangle,rect,SHADER_UNIFORM_VEC4);SetShaderValue(state.glass,state.rounding,&radius,SHADER_UNIFORM_FLOAT);
        SetShaderValue(state.glass,state.level,&level,SHADER_UNIFORM_FLOAT);SetShaderValue(state.glass,state.alpha,&opacity,SHADER_UNIFORM_FLOAT);
        SetShaderValue(state.glass,state.accent,accentValue,SHADER_UNIFORM_VEC3);SetShaderValue(state.glass,state.active,&active,SHADER_UNIFORM_FLOAT);
        BeginShaderMode(state.glass);
        DrawTexturePro(state.world.texture,{r.x,float(state.height)-r.y-r.height,r.width,-r.height},r,{0,0},0,WHITE);
        EndShaderMode();
    } else {
        DrawRectangleRounded(r,roundness(r,radius),10,Fade(Color{19,36,53,255},0.91f*opacity));
        DrawRectangleRoundedLines(r,roundness(r,radius),10,Fade(accent,0.14f*opacity));
        DrawLineEx({r.x+radius,r.y+1},{r.x+r.width-radius,r.y+1},1,Fade(WHITE,0.13f*opacity));
    }
}
void Ui::icon(Icon kind,Vector2 c,float size,Color color) {
    if(!drawing() || kind==Icon::NONE) return;
    color.a=static_cast<unsigned char>(color.a*state.opacity);
    float r=size/2,w=std::max(1.3f,size*0.075f);
    auto line=[&](float x,float y,float a,float b) {DrawLineEx({c.x+x*r,c.y+y*r},{c.x+a*r,c.y+b*r},w,color);};
    switch(kind) {
    case Icon::PLAY: DrawTriangle({c.x-r*0.55f,c.y-r*0.75f},{c.x-r*0.55f,c.y+r*0.75f},{c.x+r*0.75f,c.y},color);break;
    case Icon::PAUSE: line(-0.4f,-0.75f,-0.4f,0.75f);line(0.4f,-0.75f,0.4f,0.75f);break;
    case Icon::STEP: line(-0.65f,-0.75f,0.25f,0);line(0.25f,0,-0.65f,0.75f);line(0.7f,-0.75f,0.7f,0.75f);break;
    case Icon::RESET: DrawRingLines(c,r*0.75f,r*0.75f,35,315,24,color);line(0.8f,-0.7f,0.05f,-0.7f);line(0.8f,-0.7f,0.8f,0.1f);break;
    case Icon::CAMERA: DrawRectangleRoundedLines({c.x-r,c.y-r*0.65f,size,r*1.3f},0.25f,4,color);DrawCircleLinesV(c,r*0.35f,color);line(-0.4f,-0.65f,-0.15f,-1);line(-0.15f,-1,0.4f,-1);break;
    case Icon::MAP: line(-0.9f,-0.7f,-0.9f,0.75f);line(-0.9f,0.75f,-0.3f,0.5f);line(-0.3f,0.5f,0.35f,0.8f);line(0.35f,0.8f,0.9f,0.6f);line(0.9f,0.6f,0.9f,-0.8f);line(0.9f,-0.8f,0.35f,-0.55f);line(0.35f,-0.55f,-0.3f,-0.85f);line(-0.3f,-0.85f,-0.9f,-0.7f);line(-0.3f,-0.85f,-0.3f,0.5f);line(0.35f,-0.55f,0.35f,0.8f);break;
    case Icon::STORM: for(int i=-1;i<=1;++i) {line(-0.9f,i*0.48f,0.65f,i*0.48f);DrawCircleLinesV({c.x+r*0.65f,c.y+r*(i*0.48f-0.15f)},r*0.15f,color);}break;
    case Icon::AUDIO: line(-0.85f,-0.25f,-0.3f,-0.25f);line(-0.3f,-0.25f,0.2f,-0.8f);line(0.2f,-0.8f,0.2f,0.8f);line(0.2f,0.8f,-0.3f,0.25f);line(-0.3f,0.25f,-0.85f,0.25f);line(-0.85f,0.25f,-0.85f,-0.25f);DrawRingLines({c.x,c.y},r*0.65f,r*0.65f,-45,45,12,color);break;
    case Icon::EDIT: line(-0.7f,0.8f,0.65f,-0.65f);line(-0.4f,0.9f,0.9f,-0.4f);line(0.65f,-0.65f,0.9f,-0.4f);line(-0.7f,0.8f,-0.4f,0.9f);break;
    case Icon::ROCK: line(-0.9f,0.65f,-0.55f,-0.45f);line(-0.55f,-0.45f,0.25f,-0.8f);line(0.25f,-0.8f,0.85f,-0.2f);line(0.85f,-0.2f,0.8f,0.65f);line(0.8f,0.65f,-0.9f,0.65f);line(-0.55f,-0.45f,0.1f,0.2f);break;
    case Icon::HELP: DrawCircleLinesV(c,r*0.85f,color);line(-0.3f,-0.4f,0,-0.6f);line(0,-0.6f,0.35f,-0.3f);line(0.35f,-0.3f,0,0.1f);DrawCircleV({c.x,c.y+r*0.5f},w*0.7f,color);break;
    case Icon::CHEVRON: line(-0.5f,-0.2f,0,0.3f);line(0,0.3f,0.5f,-0.2f);break;
    default:break;
    }
}
