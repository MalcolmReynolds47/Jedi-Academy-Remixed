// SP counterpart to Raven's vehicle turret controller. GPL-2.0-only.
// Uses existing vehicle status/save fields and the native projectile emitter.
#include <algorithm>
#include <cmath>
extern gentity_t *G_FireVehicleProjectile(gentity_t*,vec3_t,vec3_t,vehWeaponInfo_t*);

static bool VehicleTurretMuzzle(Vehicle_t *vehicle,gentity_t *parent,int muzzle)
{
 if(muzzle<0||muzzle>=MAX_VEHICLE_MUZZLES||vehicle->m_iMuzzleTag[muzzle]<0||
    parent->playerModel<0||parent->playerModel>=parent->ghoul2.size())return false;
 mdxaBone_t matrix;
 if(!gi.G2API_GetBoltMatrix(parent->ghoul2,parent->playerModel,vehicle->m_iMuzzleTag[muzzle],&matrix,
    vehicle->m_vOrientation,parent->currentOrigin,level.time,nullptr,parent->modelScale))return false;
 Muzzle &out=vehicle->m_Muzzles[muzzle];
 gi.G2API_GiveMeVectorFromMatrix(matrix,ORIGIN,out.m_vMuzzlePos);
 gi.G2API_GiveMeVectorFromMatrix(matrix,NEGATIVE_Y,out.m_vMuzzleDir);
 for(int a=0;a<3;++a)if(!std::isfinite(out.m_vMuzzlePos[a])||!std::isfinite(out.m_vMuzzleDir[a]))return false;
 return VectorNormalize(out.m_vMuzzleDir)>0;
}

static gentity_t *VehicleTurretPilot(Vehicle_t *vehicle,gentity_t *parent)
{
 gentity_t *pilot=vehicle->m_pPilot;
 return pilot&&pilot->inuse&&pilot->client&&pilot->health>0?pilot:parent;
}

static bool VehicleTurretEnemy(Vehicle_t *vehicle,gentity_t *parent,gentity_t *target)
{
 if(!target||!target->inuse||target==parent||target->health<=0||!target->takedamage||
    (target->flags&FL_NOTARGET)||target==vehicle->m_pPilot||target==vehicle->m_pDroidUnit||target->owner==parent)return false;
 gentity_t *shooter=VehicleTurretPilot(vehicle,parent),*opponent=target;
 if(target->m_pVehicle)opponent=VehicleTurretPilot(target->m_pVehicle,target);
 if(OnSameTeam(shooter,opponent))return false;
 // Explicit native enemies can be breakables/turrets. Otherwise respect SP's
 // enemy factions: an empty neutral vehicle must not hunt the player or props.
 if(target==parent->enemy||target==shooter->enemy)return true;
 if(!shooter->client||!opponent->client)return false;
 const auto team=shooter->client->playerTeam,other=opponent->client->playerTeam;
 if(team==TEAM_NEUTRAL||other==TEAM_NEUTRAL)return false;
 return (shooter->client->enemyTeam!=TEAM_FREE&&shooter->client->enemyTeam==other)||
        (opponent->client->enemyTeam!=TEAM_FREE&&opponent->client->enemyTeam==team);
}

static void VehicleTurretLocalAngles(const Vehicle_t *vehicle,const vec3_t direction,vec3_t angles)
{
 vec3_t forward,right,up,local;
 AngleVectors(vehicle->m_vOrientation,forward,right,up);
 VectorSet(local,DotProduct(direction,forward),-DotProduct(direction,right),DotProduct(direction,up));
 vectoangles(local,angles);
 angles[PITCH]=AngleNormalize180(angles[PITCH]);angles[YAW]=AngleNormalize180(angles[YAW]);
}

static bool VehicleTurretClamp(const turretStats_t &turret,vec3_t angles)
{
 bool inside=true;
 const float low[2]={turret.pitchClampUp,turret.yawClampRight},high[2]={turret.pitchClampDown,turret.yawClampLeft};
 for(int a=0;a<2;++a){
  if(low[a]&&angles[a]<low[a]){angles[a]=low[a];inside=false;}
  if(high[a]&&angles[a]>high[a]){angles[a]=high[a];inside=false;}
 }
 return inside;
}

static bool VehicleTurretTarget(Vehicle_t *vehicle,gentity_t *parent,const turretStats_t &turret,
                               const vehWeaponInfo_t &weapon,int muzzle,gentity_t *enemy,vec3_t point)
{
 if(!VehicleTurretEnemy(vehicle,parent,enemy))return false;
 for(int a=0;a<3;++a)point[a]=enemy->currentOrigin[a]+(enemy->mins[a]+enemy->maxs[a])*.5f;
 vec3_t direction,angles;
 VectorSubtract(point,vehicle->m_Muzzles[muzzle].m_vMuzzlePos,direction);
 const float distance=VectorLength(direction);
 if(!std::isfinite(distance)||distance>turret.fAIRange||distance<1||!gi.inPVS(vehicle->m_Muzzles[muzzle].m_vMuzzlePos,point))return false;
 if(turret.bAILead&&weapon.fSpeed>0){
  const float *velocity=enemy->client?enemy->client->ps.velocity:enemy->s.pos.trDelta;
  VectorMA(point,std::min(distance/weapon.fSpeed,2.0f),velocity,point);
  VectorSubtract(point,vehicle->m_Muzzles[muzzle].m_vMuzzlePos,direction);
 }
 for(int a=0;a<3;++a)if(!std::isfinite(direction[a])||!std::isfinite(point[a]))return false;
 VehicleTurretLocalAngles(vehicle,direction,angles);
 if(!VehicleTurretClamp(turret,angles))return false;
 trace_t trace;
 gi.trace(&trace,vehicle->m_Muzzles[muzzle].m_vMuzzlePos,nullptr,nullptr,point,parent->s.number,MASK_SHOT,(EG2_Collision)0,0);
 return !trace.startsolid&&!trace.allsolid&&(trace.fraction==1.0f||trace.entityNum==enemy->s.number);
}

static bool VehicleTurretAim(Vehicle_t *vehicle,gentity_t *parent,const turretStats_t &turret,int muzzle,const vec3_t direction)
{
 vec3_t current,desired,yaw={0},pitch={0};
 VehicleTurretLocalAngles(vehicle,vehicle->m_Muzzles[muzzle].m_vMuzzleDir,current);
 VehicleTurretLocalAngles(vehicle,direction,desired);
 VehicleTurretClamp(turret,desired);
 const float step=turret.fTurnSpeed*std::max(.01f,std::min(vehicle->m_fTimeModifier,12.0f));
 if(!std::isfinite(step)||step<=0)return false;
 for(int a=0;a<2;++a){const float delta=AngleSubtract(desired[a],current[a]);current[a]=AngleNormalize180(current[a]+std::max(-step,std::min(step,delta)));}
 yaw[turret.yawAxis]=current[YAW];pitch[turret.pitchAxis]=current[PITCH];
 const qboolean yawOK=gi.G2API_SetBoneAngles(&parent->ghoul2[parent->playerModel],turret.yawBone,yaw,
  BONE_ANGLES_POSTMULT,POSITIVE_X,NEGATIVE_Y,NEGATIVE_Z,nullptr,0,level.time);
 const qboolean pitchOK=gi.G2API_SetBoneAngles(&parent->ghoul2[parent->playerModel],turret.pitchBone,pitch,
  BONE_ANGLES_POSTMULT,POSITIVE_X,NEGATIVE_Y,NEGATIVE_Z,nullptr,0,level.time);
 if(!yawOK||!pitchOK||!VehicleTurretMuzzle(vehicle,parent,muzzle))return false;
 return true;
}

static void G_VehicleTurretThink(Vehicle_t *vehicle,gentity_t *parent,int number)
{
 if(!vehicle||!parent||!parent->inuse||!parent->client||parent->health<=0||vehicle->m_iDieTime||
    vehicle->m_iRemovedSurfaces||!vehicle->m_pVehicleInfo||number<0||number>=MAX_VEHICLE_TURRETS)return;
 const turretStats_t &turret=vehicle->m_pVehicleInfo->turret[number];
 if(turret.iAmmoMax<=0||turret.iWeapon<=VEH_WEAPON_BASE||turret.iWeapon>=numVehicleWeapons||
    turret.iWeapon>=MAX_VEH_WEAPONS||!turret.yawBone||!turret.pitchBone||
    turret.yawAxis<0||turret.yawAxis>2||turret.pitchAxis<0||turret.pitchAxis>2)return;
 vehWeaponInfo_t &weapon=g_vehWeaponInfo[turret.iWeapon];
 if(!weapon.bIsProjectile||weapon.iAmmoPerShot<=0||!std::isfinite(turret.fAIRange)||turret.fAIRange<0)return;
 vehTurretStatus_t &status=vehicle->turretStatus[number];
 int valid[2],count=0;
 for(int i=0;i<MAX_VEHICLE_TURRET_MUZZLES;++i){const int n=turret.iMuzzle[i]-1;
  if(n>=0&&n<MAX_VEHICLE_MUZZLES&&vehicle->m_iMuzzleTag[n]>=0&&(!count||valid[0]!=n))valid[count++]=n;
 }
 if(!count)return;
 int muzzle=status.nextMuzzle;
 if(muzzle!=valid[0]&&(count<2||muzzle!=valid[1]))muzzle=status.nextMuzzle=valid[0];
 if(!VehicleTurretMuzzle(vehicle,parent,muzzle))return;
 vec3_t direction,point;gentity_t *enemy=nullptr;
 {
  if(!turret.bAI){status.enemyEntNum=ENTITYNUM_NONE;return;}
  if(status.enemyEntNum>=0&&status.enemyEntNum<ENTITYNUM_WORLD){
   enemy=&g_entities[status.enemyEntNum];
   if(!VehicleTurretTarget(vehicle,parent,turret,weapon,muzzle,enemy,point))enemy=nullptr;
  }
  if(level.time>=status.enemyHoldTime){
   gentity_t *candidates[MAX_GENTITIES];const int num=G_RadiusList(vehicle->m_Muzzles[muzzle].m_vMuzzlePos,turret.fAIRange,parent,qtrue,candidates);
   float best=turret.fAIRange*turret.fAIRange;gentity_t *picked=nullptr;vec3_t candidate;
   for(int i=0;i<num;++i)if(VehicleTurretTarget(vehicle,parent,turret,weapon,muzzle,candidates[i],candidate)){
    const float distance=DistanceSquared(vehicle->m_Muzzles[muzzle].m_vMuzzlePos,candidate);
    if(distance<best){best=distance;picked=candidates[i];VectorCopy(candidate,point);}
   }
   enemy=picked;status.enemyHoldTime=level.time+(enemy?3000:500);
  }
  status.enemyEntNum=enemy?enemy->s.number:ENTITYNUM_NONE;
  if(!enemy)return;
  VectorSubtract(point,vehicle->m_Muzzles[muzzle].m_vMuzzlePos,direction);VectorNormalize(direction);
 }
 if(!VehicleTurretAim(vehicle,parent,turret,muzzle,direction))return;
 if(DotProduct(direction,vehicle->m_Muzzles[muzzle].m_vMuzzleDir)<.999f)return;
 if(status.ammo<weapon.iAmmoPerShot||vehicle->m_Muzzles[muzzle].m_iMuzzleWait>level.time)return;
 Muzzle &barrel=vehicle->m_Muzzles[muzzle];
 gentity_t *projectile=G_FireVehicleProjectile(parent,barrel.m_vMuzzlePos,barrel.m_vMuzzleDir,&weapon);
 if(!projectile)return;
 if(weapon.iMuzzleFX)G_PlayEffect(weapon.iMuzzleFX,barrel.m_vMuzzlePos,barrel.m_vMuzzleDir);
 status.ammo-=weapon.iAmmoPerShot;parent->client->ps.ammo[MAX_VEHICLE_WEAPONS+number]=status.ammo;
 status.nextMuzzle=count==2?(muzzle==valid[0]?valid[1]:valid[0]):valid[0];
 for(int i=0;i<count;++i)vehicle->m_Muzzles[valid[i]].m_iMuzzleWait=level.time+std::max(1,std::min(turret.iDelay,60000));
}
