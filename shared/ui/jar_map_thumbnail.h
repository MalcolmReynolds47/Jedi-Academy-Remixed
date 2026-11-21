// Generates an isometric thumbnail from native BSP surfaces. GPL-2.0-only.
#pragma once

#include "qcommon/qfiles.h"
#include "qcommon/jar_spawn_difficulty.h"
#include "game/surfaceflags.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace jar { namespace mapPicker {
inline bool SafeName(const std::string& name) {
    if (name.empty() || name.size()+10 > MAX_QPATH || name.front()=='/' || name.back()=='/')
        return false;
    for (size_t i=0;i<name.size();++i) {
        const char c=name[i];
        if ((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='-')
            continue;
        if (c!='/' || name[i-1]=='/') return false;
    }
    return true;
}

using Point=std::array<float,3>;

inline bool FinitePoint(const Point& point) {
    return std::all_of(point.begin(),point.end(),[](float value){
        return std::isfinite(value) && std::abs(value)<=131072;
    });
}

struct MapInfo {
    std::string title;
    Point start{};
    bool playable=false;
};

enum class StartMode { SinglePlayer, MultiplayerFFA };

inline MapInfo ReadEntities(const std::string& text, StartMode mode=StartMode::SinglePlayer,
                            int skill=0, bool buildScript=false) {
    const bool singlePlayer=mode==StartMode::SinglePlayer;
    size_t pos=0;
    bool malformed=false;
    const auto token=[&](std::string& value) {
        while (pos<text.size() && text[pos]) {
            if (static_cast<unsigned char>(text[pos])<=32) ++pos;
            else if (text.compare(pos,2,"//")==0) {
                pos=text.find('\n',pos+2);
                if (pos==text.npos) pos=text.size();
            } else if (text.compare(pos,2,"/*")==0) {
                pos=text.find("*/",pos+2);
                if (pos==text.npos) { malformed=true; return false; }
                pos+=2;
            } else break;
        }
        if (pos==text.size() || !text[pos]) return false;
        if (text[pos]=='"') {
            const auto begin=++pos;
            while (pos<text.size() && text[pos] && text[pos]!='"') ++pos;
            if (pos==text.size() || !text[pos]) { malformed=true; return false; }
            value.assign(text,begin,pos-begin);
            ++pos;
        } else if (!singlePlayer && (text[pos]=='{' || text[pos]=='}')) {
            // Retain the multiplayer browser's compact brace syntax.
            value.assign(1,text[pos++]);
        } else {
            const auto begin=pos;
            while (pos<text.size() && static_cast<unsigned char>(text[pos])>32) ++pos;
            value.assign(text,begin,pos-begin);
        }
        return true;
    };
    MapInfo result;
    bool world=false;
    std::string field, contents;
    while (token(field)) {
        if (field!="{") return {};
        std::string classname,origin,title,gametype;
        bool named=false;
        bool notFree=false, noHumans=false, restricted=false;
        bool notSingle=false, hasNotSingle=false;
        unsigned spawnFlags=0;
        size_t fields=0, bytes=0;
        while (true) {
            if (!token(field)) return {};
            if (field=="}") break;
            if (field=="{" || !token(contents) || (!contents.empty() && contents.front()=='}')) return {};
            ++fields;
            bytes+=field.size()+contents.size()+2;
            if (singlePlayer && (fields>64 || bytes>2048)) return {};
            if (!Q_stricmp(field.c_str(),"classname")) classname=contents;
            else if (!Q_stricmp(field.c_str(),"origin")) origin=contents;
            else if (!Q_stricmp(field.c_str(),"targetname")) named=true;
            else if (!Q_stricmp(field.c_str(),"message")) title=contents;
            else if (!Q_stricmp(field.c_str(),"notsingle") && !hasNotSingle) {
                notSingle=std::strtol(contents.c_str(),nullptr,10)!=0;
                hasNotSingle=true;
            }
            else if (!Q_stricmp(field.c_str(),"spawnflags"))
                spawnFlags=static_cast<unsigned>(std::strtol(contents.c_str(),nullptr,10));
            else if (!Q_stricmp(field.c_str(),"notfree")) notFree=std::atoi(contents.c_str())!=0;
            else if (!Q_stricmp(field.c_str(),"nohumans")) noHumans=std::atoi(contents.c_str())!=0;
            else if (!Q_stricmp(field.c_str(),"gametype")) { gametype=contents; restricted=true; }
        }
        if (!world) {
            if (classname!="worldspawn") return {};
            world=true;
            if (!title.empty() && title.size()<=60 &&
                std::all_of(title.begin(),title.end(),[](char c){return c>=32 && c<127;}))
                result.title=title;
        }
        if (result.playable || (classname!="info_player_start" && classname!="info_player_deathmatch"))
            continue;
        if (singlePlayer ? (named || notSingle || (!buildScript && !SpawnAtDifficulty(spawnFlags,skill))) :
            (notFree || noHumans || (restricted && gametype.find("ffa")==std::string::npos))) continue;
        if (!origin.empty()) {
            if (origin.size()>80) continue;
            Point start{};
            if (std::sscanf(origin.c_str(),"%f %f %f",&start[0],&start[1],&start[2])!=3 ||
                !FinitePoint(start)) continue;
            result.start=start;
        }
        result.playable=true;
    }
    return malformed ? MapInfo{} : result;
}

constexpr int thumbWidth=144;
constexpr int thumbHeight=96;
using Image=std::array<uint8_t,18+3*thumbWidth*thumbHeight>;

// In the preview coordinate system, z grows toward the viewer.
inline Point Project(const Point& point,const Point& focus,float scale) {
    const float x=point[0]-focus[0], y=point[1]-focus[1], z=point[2]-focus[2];
    return {thumbWidth*.5f+(x-y)*.707107f*scale,
            thumbHeight*.56f-((x+y)*.353553f+z*.866025f)*scale,
            -(x+y)*.612372f+z*.5f};
}

inline bool Rasterize(const dmodel_t& world,const std::vector<dshader_t>& shaders,
                      const std::vector<drawVert_t>& vertices,const std::vector<int>& indices,
                      const std::vector<dsurface_t>& surfaces,const Point& focus,Image& image) {
    const auto range=[](int first,int count,size_t size) {
        return first>=0 && count>=0 && size_t(first)<=size && size_t(count)<=size-size_t(first);
    };
    if (!range(world.firstSurface,world.numSurfaces,surfaces.size())) return false;
    const Point low={world.mins[0],world.mins[1],world.mins[2]};
    const Point high={world.maxs[0],world.maxs[1],world.maxs[2]};
    if (!FinitePoint(low) || !FinitePoint(high) || !FinitePoint(focus)) return false;
    const float width=std::max(384.0f,std::min(2000.0f,std::max(high[0]-low[0],high[1]-low[1])));
    const float scale=thumbWidth/width;
    std::vector<Point> projected;
    projected.reserve(vertices.size());
    for (const auto& v:vertices) {
        const Point point={v.xyz[0],v.xyz[1],v.xyz[2]};
        projected.push_back(FinitePoint(point) ? Project(point,focus,scale) :
                            Point{0,0,std::numeric_limits<float>::quiet_NaN()});
    }

    image={};
    image[2]=2;
    image[12]=thumbWidth&255; image[13]=thumbWidth>>8;
    image[14]=thumbHeight&255; image[15]=thumbHeight>>8;
    image[16]=24; image[17]=32;
    std::array<float,thumbWidth*thumbHeight> depth;
    depth.fill(-std::numeric_limits<float>::infinity());
    for (int y=0;y<thumbHeight;++y) for (int x=0;x<thumbWidth;++x) {
        const int at=18+3*(y*thumbWidth+x);
        image[at]=uint8_t(46-y/6);
        image[at+1]=uint8_t(37-y/7);
        image[at+2]=uint8_t(27-y/8);
    }
    int remainingPixels=4'000'000;
    bool painted=false;
    const auto edge=[](const Point& a,const Point& b,float x,float y) {
        return (b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0]);
    };
    const auto triangle=[&](int a,int b,int c,unsigned tint) {
        const auto& p=projected[a]; const auto& q=projected[b]; const auto& r=projected[c];
        if (!std::isfinite(p[2]) || !std::isfinite(q[2]) || !std::isfinite(r[2])) return;
        // Leave a cutaway above the spawn so roofs do not obscure interior layouts.
        if (std::min({vertices[a].xyz[2],vertices[b].xyz[2],vertices[c].xyz[2]})>focus[2]+300) return;
        const float area=edge(p,q,r[0],r[1]);
        if (std::abs(area)<.025f) return;
        const int x0=std::max(0,int(std::floor(std::min({p[0],q[0],r[0]}))));
        const int x1=std::min(thumbWidth-1,int(std::ceil(std::max({p[0],q[0],r[0]}))));
        const int y0=std::max(0,int(std::floor(std::min({p[1],q[1],r[1]}))));
        const int y1=std::min(thumbHeight-1,int(std::ceil(std::max({p[1],q[1],r[1]}))));
        if (x1<x0 || y1<y0 || remainingPixels<=0) return;
        remainingPixels-=(x1-x0+1)*(y1-y0+1);
        const auto& va=vertices[a].xyz; const auto& vb=vertices[b].xyz; const auto& vc=vertices[c].xyz;
        const float ux=vb[0]-va[0], uy=vb[1]-va[1], uz=vb[2]-va[2];
        const float vx=vc[0]-va[0], vy=vc[1]-va[1], vz=vc[2]-va[2];
        const float nx=uy*vz-uz*vy, ny=uz*vx-ux*vz, nz=ux*vy-uy*vx;
        const float length=std::sqrt(nx*nx+ny*ny+nz*nz);
        const float light=length>0 ? std::abs((-nx*.36f+ny*.25f+nz*.9f)/length) : 0;
        const int brightness=57+int(light*74);
        for (int y=y0;y<=y1;++y) for (int x=x0;x<=x1;++x) {
            const float u=edge(q,r,x+.5f,y+.5f)/area;
            const float v=edge(r,p,x+.5f,y+.5f)/area;
            const float w=1-u-v;
            if (u<-.001f || v<-.001f || w<-.001f) continue;
            const float z=u*p[2]+v*q[2]+w*r[2];
            const int pixel=y*thumbWidth+x;
            if (z<=depth[pixel]) continue;
            depth[pixel]=z;
            const int outline=std::min({u,v,w})<.025f ? -9 : 0;
            const int at=18+3*pixel;
            image[at]=uint8_t(brightness+14+int(tint&31)+outline);
            image[at+1]=uint8_t(brightness+9+int((tint>>5)&31)+outline);
            image[at+2]=uint8_t(brightness+int((tint>>10)&31)+outline);
            painted=true;
        }
    };
    int remainingTriangles=160000;
    for (int i=0;i<world.numSurfaces && remainingPixels>0 && remainingTriangles>0;++i) {
        const auto& s=surfaces[world.firstSurface+i];
        if (s.shaderNum<0 || size_t(s.shaderNum)>=shaders.size() ||
            !range(s.firstVert,s.numVerts,vertices.size())) continue;
        const auto& shader=shaders[s.shaderNum];
        if (shader.surfaceFlags&(SURF_SKY|SURF_NODRAW)) continue;
        uint32_t tint=2166136261u;
        for (char ch:shader.shader) {
            if (!ch) break;
            tint=(tint^uint8_t(ch))*16777619u;
        }
        if (s.surfaceType==MST_PATCH) {
            if (s.patchWidth<2 || s.patchHeight<2 || s.patchWidth>64 || s.patchHeight>64 ||
                s.patchWidth*s.patchHeight>s.numVerts) continue;
            for (int y=0;y+1<s.patchHeight && remainingTriangles>0;++y)
                for (int x=0;x+1<s.patchWidth && remainingTriangles>0;++x) {
                    const int a=s.firstVert+y*s.patchWidth+x, b=a+s.patchWidth;
                    triangle(a,a+1,b,tint);
                    triangle(a+1,b+1,b,tint);
                    remainingTriangles-=2;
                }
        } else if ((s.surfaceType==MST_PLANAR || s.surfaceType==MST_TRIANGLE_SOUP) &&
                   range(s.firstIndex,s.numIndexes,indices.size())) {
            for (int k=0;k+2<s.numIndexes && remainingTriangles>0 && remainingPixels>0;k+=3) {
                const int a=indices[s.firstIndex+k], b=indices[s.firstIndex+k+1], c=indices[s.firstIndex+k+2];
                --remainingTriangles;
                if (a>=0 && b>=0 && c>=0 && a<s.numVerts && b<s.numVerts && c<s.numVerts)
                    triangle(s.firstVert+a,s.firstVert+b,s.firstVert+c,tint);
            }
        }
    }
    return painted;
}
}} // namespace jar::mapPicker
