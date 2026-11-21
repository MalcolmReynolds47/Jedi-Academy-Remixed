// Snow rendered only inside the retail Hoth Wasteland weather volumes.
static void CG_WastelandSnow(int model,const vec3_t origin,const vec3_t angles,const vec3_t (&axes)[3]){
    using namespace jar::wasteland;
    const float scale=gi.cvar("r_weatherScale","1",CVAR_ARCHIVE)->value;
    if(!std::isfinite(scale)||scale<=0||!cgs.media.wastelandSnowShader)return;
    vec3_t offset,eye;
    VectorSubtract(cg.refdef.vieworg,origin,offset);
    for(int axis=0;axis<3;++axis)eye[axis]=DotProduct(offset,axes[axis]);
    constexpr int tile=120,reach=780;
    const int layers=scale>=2?2:1;
    const int clip=cgi_CM_InlineModel(model);
    const auto point=[&](float x,float y,float z,vec3_t out){
        VectorCopy(origin,out);
        VectorMA(out,x,axes[0],out);VectorMA(out,y,axes[1],out);VectorMA(out,z,axes[2],out);
    };
    for(const auto& volume:weatherVolumes){
        if(eye[2]<volume.mins[2]-reach||eye[2]>volume.maxs[2]+reach)continue;
        const int firstX=std::max(0,int(std::floor((eye[0]-reach-volume.mins[0])/tile)));
        const int firstY=std::max(0,int(std::floor((eye[1]-reach-volume.mins[1])/tile)));
        const int lastX=int(std::floor((std::min(eye[0]+reach,volume.maxs[0])-volume.mins[0])/tile));
        const int lastY=int(std::floor((std::min(eye[1]+reach,volume.maxs[1])-volume.mins[1])/tile));
        for(int x=firstX;x<=lastX;++x)for(int y=firstY;y<=lastY;++y)for(int layer=0;layer<layers;++layer){
            const uint32_t seed=(uint32_t(x+3)*73856093u)^(uint32_t(y+7)*19349663u)^
                (uint32_t(volume.model+layer*11)*83492791u);
            const float t=std::max(0,cg.time)*.001f;
            const float flakeX=volume.mins[0]+tile*x+20+(seed&63u)+12*std::sin(t+float(seed&255u));
            const float flakeY=volume.mins[1]+tile*y+20+((seed>>6)&63u)+8*std::cos(t+float((seed>>8)&255u));
            if(flakeX>=volume.maxs[0]||flakeY>=volume.maxs[1])continue;
            const float height=volume.maxs[2]-volume.mins[2];
            const float phase=std::fmod((seed&2047u)+t*(34+float((seed>>16)&31u)),height);
            const float flakeZ=volume.maxs[2]-phase;
            vec3_t pos,sky,diff;
            point(flakeX,flakeY,flakeZ,pos);
            VectorSubtract(pos,cg.refdef.vieworg,diff);
            const float distance=VectorLengthSquared(diff);
            if(distance>reach*reach||DotProduct(diff,cg.refdef.viewaxis[0])<=0)continue;
            point(flakeX,flakeY,volume.maxs[2]+128,sky);
            trace_t roof{};
            cgi_CM_TransformedBoxTrace(&roof,pos,sky,vec3_origin,vec3_origin,clip,CONTENTS_SOLID,origin,angles);
            if(roof.startsolid||(roof.fraction<1&&!(roof.surfaceFlags&SURF_SKY)))continue;
            constexpr float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
            const float size=1.5f+float((seed>>10)&3u)*.25f;
            const byte alpha=byte(210.f*std::min(scale,1.f)*
                 std::min(1.f,std::min(phase,height-phase)*.1f)*
                 std::min(1.f,4.f*(1.f-distance/(reach*reach))));
            polyVert_t quad[4]{};
            for(int i=0;i<4;++i){
                VectorCopy(pos,quad[i].xyz);
                VectorMA(quad[i].xyz,(2*uv[i][0]-1)*size,cg.refdef.viewaxis[1],quad[i].xyz);
                VectorMA(quad[i].xyz,(2*uv[i][1]-1)*size,cg.refdef.viewaxis[2],quad[i].xyz);
                quad[i].st[0]=uv[i][0];quad[i].st[1]=uv[i][1];
                quad[i].modulate[0]=quad[i].modulate[1]=quad[i].modulate[2]=255;
                quad[i].modulate[3]=alpha;
            }
            cgi_R_AddPolyToScene(cgs.media.wastelandSnowShader,4,quad);
        }
    }
}
