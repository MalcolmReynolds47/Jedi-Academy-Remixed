// Native whole-map setup with nested parser/BSP-state isolation. GPL-2.0-only.
#include <climits>
#include "g_bsp_section.h"
#include "g_subbsp_ownership.h"
#include "g_siege_item.h"
#include "g_subbsp_admission.inc"
static int subBSPActiveModel=-1;
void G_ResetSubBSPSpawnContext()
{
    // ERR_DROP may bypass C++ scopes. New map spawns cannot inherit that state.
    G_SubBSPOwnershipReset();
    subBSPActiveModel=-1; gi.SetActiveSubBSP(-1);
    level.mTargetAdjust=nullptr; level.mBSPInstanceDepth=0; level.mRotationAdjust=0;
    VectorClear(level.mOriginAdjust);
}
static void G_SpawnSubBSP(gentity_t *ent,jar::bspAdmission::Report* completed)
{
    char model[MAX_QPATH],*name;
    int portable=0,isolated=0,expectedMembers=-1;
    jar::bspAdmission::Report report;
    G_SpawnInt("_jar_portable","0",&portable);
    if(!portable){
        if(level.mBSPInstanceDepth>=MAX_SUB_BSP) G_Error("JA:R: sub-BSP nesting depth exceeded");
        if(level.mNumBSPInstances==INT_MAX) G_Error("JA:R: sub-BSP instance identifiers exhausted");
    }
    G_SpawnString("bspmodel","",&name);
    if(!name[0] || strlen(name)+2>sizeof(model))
    { gi.Printf("JA:R: misc_bsp needs a valid bspmodel name\n"); G_FreeEntity(ent); return; }
    Com_sprintf(model,sizeof(model),"#%s",name);
    G_SpawnInt("_jar_namespace","0",&isolated);
    if(portable){
        bool valid=portable==1;
        if(!valid)report.reason="Invalid portable mode";
        if(valid&&(level.mBSPInstanceDepth>=MAX_SUB_BSP||level.mNumBSPInstances==INT_MAX)){
            report.reason="Portable BSP instance capacity reached";valid=false;
        }
        for(int i=0;i<3&&valid;++i)if(!std::isfinite(ent->s.origin[i])||!std::isfinite(ent->s.angles[i])||
            std::fabs(ent->s.origin[i])>65536||std::fabs(ent->s.angles[i])>65536){report.reason="Invalid portable BSP transform";valid=false;}
        const auto*profile=ent->model&&!strcmp(ent->model,model)?jar::bspSection::Resource(model):nullptr;
        if(!valid||!G_AdmitPortableSubBSP(model,report,profile?profile->mode:jar::bspSection::Mode::Generic)){
            gi.Printf("JA:R: portable BSP %s refused (entity %d): %s\n",name,report.rejectedEntity,
                report.reason.c_str());
            G_FreeEntity(ent);return;
        }
        isolated=1;if(profile)expectedMembers=report.entities-1+report.ownedHelpers;
    }
    // G_ParseField already accepts angle and angles. Preserve all three axes.
    ent->s.eFlags|=EF_PERMANENT;
    G_SpawnInt("spacing","0",&ent->s.time2);
    G_SpawnInt("flatten","0",&ent->s.time);
    gi.SetBrushModel(ent,model); G_BSPIndex(model);
    if(isolated!=0&&isolated!=1)G_Error("JA:R: invalid sub-BSP namespace mode");
    if(!G_SubBSPBegin(ent,isolated!=0))G_Error("JA:R: invalid or repeated sub-BSP ownership root");
    G_SubBSPCapture capture(ent);
    const int instance=++level.mNumBSPInstances;
    struct InstanceScope
    {
        vec3_t origin; float rotation; char *target; int depth,active; char prefix[MAX_QPATH];
        InstanceScope(gentity_t *entity,int id):rotation(level.mRotationAdjust),target(level.mTargetAdjust),depth(level.mBSPInstanceDepth),active(subBSPActiveModel)
        {
            VectorCopy(level.mOriginAdjust,origin);
            Com_sprintf(prefix,sizeof(prefix),"%d-",id);
            level.mTargetAdjust=prefix; level.mRotationAdjust=entity->s.angles[1];
            VectorCopy(entity->s.origin,level.mOriginAdjust); level.mBSPInstanceDepth=depth+1;
            subBSPActiveModel=entity->s.modelindex;
        }
        ~InstanceScope()
        {
            gi.SetActiveSubBSP(active); subBSPActiveModel=active;
            level.mTargetAdjust=target; level.mRotationAdjust=rotation; level.mBSPInstanceDepth=depth;
            VectorCopy(origin,level.mOriginAdjust);
        }
    } scope(ent,instance);
    level.hasBspInstances=qtrue;
    VectorCopy(ent->s.origin,ent->s.pos.trBase); VectorCopy(ent->s.origin,ent->currentOrigin);
    VectorCopy(ent->s.angles,ent->s.apos.trBase); VectorCopy(ent->s.angles,ent->currentAngles);
    ent->s.eType=ET_MOVER; gi.linkentity(ent);
    const char *entities=gi.SetActiveSubBSP(ent->s.modelindex);
    if(entities) G_SubBSPSpawnEntitiesFromString(entities,ent->s.origin,ent->s.angles);
    if(G_SiegeKorribanRoom(ent)&&!G_SiegeKorribanFinishMedia(*ent)){
        gi.Printf("JA:R: Siege Korriban media setup failed; removed the incomplete section.\n");
        G_FreeEntity(ent);return;
    }
    if(expectedMembers>=0){
        std::vector<gentity_t*> members;
        if(!G_SubBSPMembers(ent,members)||int(members.size())!=expectedMembers){
            gi.Printf("JA:R: section %s did not create every admitted child; removed the incomplete section.\n",name);
            G_FreeEntity(ent);return;
        }
    }
    if(completed)*completed=std::move(report);
}
void SP_misc_bsp(gentity_t*ent){G_SpawnSubBSP(ent,nullptr);}
