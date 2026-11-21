// Read-only diagnostics on a temporary native Ghoul2 copy. GPL-2.0-only.
#include "ghoul2/G2.h"
static qboolean VehicleRigInfo(const char*command){
 if(Q_stricmp(command,"jar_vehicle_rig"))return qfalse;
 char text[32],*end;trap->Argv(1,text,sizeof(text));unsigned long id=strtoul(text,&end,10);
 if(trap->Argc()!=2||!*text||*end||id>UINT_MAX){Tell(NULL,"Server console: jar_vehicle_rig CATALOG_ID");return qtrue;}
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){gentity_t*e=&g_entities[n];
  if(!e->inuse||!e->m_pVehicle||!e->ghoul2||!objects[n].owner||objects[n].catalog<1||objects[n].catalog>JAR_MP_CATALOG_COUNT||jarMPCatalog[objects[n].catalog-1].id!=id)continue;
  void*copy=NULL;trap->G2API_DuplicateGhoul2Instance(e->ghoul2,&copy);if(!copy){Tell(NULL,"Vehicle rig copy unavailable");return qtrue;}
  const char*tags[]={"*driver","*muzzle1","*muzzle2","*muzzle3","*muzzle4","*muzzle5","*muzzle6","*exhaust1","*exhaust2","*exhaust3","*turretview_t","*turretview_b"};
  qboolean valid=trap->G2API_SetBoneAnim(copy,0,"model_root",0,1,BONE_ANIM_OVERRIDE|BONE_ANIM_OVERRIDE_FREEZE,1,level.time,0,0);
  for(int phase=0;phase<2&&valid;++phase){
   for(int turret=0;turret<MAX_VEHICLE_TURRETS;++turret){const turretStats_t*t=&e->m_pVehicle->m_pVehicleInfo->turret[turret];vec3_t yaw={0},pitch={0};
    if(!t->yawBone||!t->pitchBone||t->yawAxis<0||t->yawAxis>2||t->pitchAxis<0||t->pitchAxis>2)continue;
    yaw[t->yawAxis]=phase?90:0;pitch[t->pitchAxis]=phase?30:0;
    valid=trap->G2API_SetBoneAngles(copy,0,t->yawBone,yaw,BONE_ANGLES_POSTMULT,POSITIVE_X,NEGATIVE_Y,NEGATIVE_Z,NULL,0,level.time)&&valid;
    valid=trap->G2API_SetBoneAngles(copy,0,t->pitchBone,pitch,BONE_ANGLES_POSTMULT,POSITIVE_X,NEGATIVE_Y,NEGATIVE_Z,NULL,0,level.time)&&valid;
   }
   for(unsigned i=0;i<ARRAY_LEN(tags)&&valid;++i){int bolt=trap->G2API_AddBolt(copy,0,tags[i]);mdxaBone_t matrix;vec3_t zero={0},scale={1,1,1},point,dir;
    // Match native vehicle weapons and turret cameras: ordinary MP bolt matrices
    // rotate the first two axes for attachments, but these consumers do not.
    if(bolt<0||!trap->G2API_GetBoltMatrix(copy,0,bolt,&matrix,zero,zero,level.time+phase+1,NULL,scale)||!trap->G2API_GetBoltMatrix_NoRecNoRot(copy,0,bolt,&matrix,zero,zero,level.time+phase+1,NULL,scale)){Tell(NULL,va("Rig %u missing %s",(unsigned)id,tags[i]));continue;}
    BG_GiveMeVectorFromMatrix(&matrix,ORIGIN,point);BG_GiveMeVectorFromMatrix(&matrix,NEGATIVE_Y,dir);
    Tell(NULL,va("Rig %u phase %d %s at %.6f %.6f %.6f dir %.6f %.6f %.6f",(unsigned)id,phase,tags[i],point[0],point[1],point[2],dir[0],dir[1],dir[2]));
   }
  }
  trap->G2API_CleanGhoul2Models(&copy);Tell(NULL,valid?"Vehicle rig copy released":"Vehicle rig bone setup failed");return qtrue;
 }
 Tell(NULL,"No spawned vehicle for this catalog ID");return qtrue;
}
