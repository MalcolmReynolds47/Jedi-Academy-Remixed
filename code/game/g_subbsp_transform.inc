// Transform canonical spawn keys before native callbacks read them. GPL-2.0-only.
#include <cmath>
#include <string>
#include <vector>
#include "qcommon/jar_angular_trajectory.h"

static void G_SubBSPAngles(const vec3_t local, const vec3_t parentAxis[3], vec3_t out)
{
    vec3_t localAxis[3], world[3]{};
    AnglesToAxis(local,localAxis);
    for(int i=0;i<3;++i) for(int j=0;j<3;++j) VectorMA(world[i],localAxis[i][j],parentAxis[j],world[i]);
    const double horizontal=sqrt(double(world[0][0])*world[0][0]+double(world[0][1])*world[0][1]);
    const double degrees=180.0/M_PI;
    out[0]=float(-atan2(world[0][2],horizontal)*degrees);
    if(horizontal>0.000001)
    { out[1]=float(atan2(world[0][1],world[0][0])*degrees); out[2]=float(atan2(world[1][2],world[2][2])*degrees); }
    else
    { out[1]=float(atan2(-world[1][0],world[1][1])*degrees); out[2]=0; }
}

static std::string G_SubBSPVectorText(const vec3_t value)
{
    for(int i=0;i<3;++i) if(!std::isfinite(value[i])) G_Error("JA:R: invalid sub-BSP transform");
    char text[128]; Com_sprintf(text,sizeof(text),"%.9g %.9g %.9g",value[0],value[1],value[2]); return text;
}

static void G_AdjustSubBSPSpawnVars(const vec3_t origin, const vec3_t angles)
{
    G_SubBSPVectorText(origin); G_SubBSPVectorText(angles);
    vec3_t axis[3], localOrigin{}, localAngles{}, modelAngles{};
    bool hasModelAngles=false;
    const char *classname="";
    for(int i=0;i<numSpawnVars;++i) if(!Q_stricmp(spawnVars[i][0],"classname")) classname=spawnVars[i][1];
    const bool angular=!Q_stricmp(classname,"func_rotating") || !Q_stricmp(classname,"func_pendulum");
    const bool directionAngles=!Q_stricmp(classname,"func_door") || !Q_stricmp(classname,"func_button") ||
        !Q_stricmp(classname,"misc_portal_camera") || !Q_stricmp(classname,"shooter_plasma");
    AnglesToAxis(angles,axis);
    std::vector<std::pair<std::string,std::string>> fields;
    for(int i=0;i<numSpawnVars;++i)
    {
        const char *key=spawnVars[i][0],*value=spawnVars[i][1];
        if(angular && !Q_stricmp(key,"_jar_localAngles")) continue;
        vec3_t *vector=nullptr;
        if(!Q_stricmp(key,"origin")) vector=&localOrigin;
        else if(!Q_stricmp(key,"angles")) vector=&localAngles;
        else if(!Q_stricmp(key,"modelAngles")) { vector=&modelAngles; hasModelAngles=true; }
        else if(!Q_stricmp(key,"angle"))
        {
            float yaw; if(sscanf(value,"%f",&yaw)!=1||!std::isfinite(yaw)) G_Error("JA:R: invalid sub-BSP angle");
            VectorClear(localAngles);
            if(directionAngles && yaw==-1) localAngles[0]=-90; else if(directionAngles && yaw==-2) localAngles[0]=90; else localAngles[1]=yaw;
            continue;
        }
        if(vector)
        {
            if(sscanf(value,"%f %f %f",&(*vector)[0],&(*vector)[1],&(*vector)[2])!=3) G_Error("JA:R: invalid sub-BSP vector");
            G_SubBSPVectorText(*vector); continue;
        }
        fields.emplace_back(key,value);
    }
    vec3_t position, orientation;
    VectorCopy(origin,position);
    for(int i=0;i<3;++i) VectorMA(position,localOrigin[i],axis[i],position);
    G_SubBSPAngles(localAngles,axis,orientation);
    fields.emplace_back("origin",G_SubBSPVectorText(position));
    fields.emplace_back("angles",G_SubBSPVectorText(orientation));
    if(angular) fields.emplace_back("_jar_localAngles",G_SubBSPVectorText(localAngles));
    if(hasModelAngles)
    {
        const bool travelAngles=!Q_stricmp(classname,"func_door") || !Q_stricmp(classname,"func_button") || !Q_stricmp(classname,"func_plat");
        if(angular)
        {
            // Keep the authored rest appearance as a rigid body-local offset.
            vec3_t model;VectorAdd(localAngles,modelAngles,model);
            jar::angular::Relative(model,localAngles,orientation);
        }
        else if(travelAngles || !Q_stricmp(classname,"func_bobbing"))
        {
            // The native client adds modelAngles to the brush's Euler angles.
            // Preserve that stock local appearance, then compose the section
            // once and encode the resulting relative offset for the client.
            vec3_t brush, model;
            if(travelAngles) VectorCopy(angles,brush);
            else G_SubBSPAngles(localAngles,axis,brush);
            if(!travelAngles) VectorAdd(modelAngles,localAngles,modelAngles);
            G_SubBSPAngles(modelAngles,axis,model);
            VectorSubtract(model,brush,orientation);
        }
        else G_SubBSPAngles(modelAngles,axis,orientation);
        fields.emplace_back("modelAngles",G_SubBSPVectorText(orientation));
    }
    size_t chars=0;
    for(const auto &field:fields) chars+=field.first.size()+field.second.size()+2;
    if(fields.size()>MAX_SPAWN_VARS || chars>MAX_SPAWN_VARS_CHARS) G_Error("JA:R: sub-BSP transformed spawn keys exceed capacity");
    // Publish only after the complete transformed record fits; no partially
    // overwritten spawn strings on a rejected transform or capacity failure.
    numSpawnVars=numSpawnVarChars=0;
    for(const auto &field:fields)
    { spawnVars[numSpawnVars][0]=G_AddSpawnVarToken(field.first.c_str()); spawnVars[numSpawnVars][1]=G_AddSpawnVarToken(field.second.c_str()); ++numSpawnVars; }
}
