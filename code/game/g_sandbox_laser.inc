// Authoritative finite-range beam for owned SP emitters. GPL-2.0-only.
static bool sandboxLaserVector(const vec3_t v){
 for(int i=0;i<3;++i)if(!std::isfinite(v[i])||std::fabs(v[i])>131072)return false;
 return true;
}

static void sandboxLaserThink(gentity_t& source){
 using namespace jar::sandboxLaser;
 Settings settings;
 source.alt_fire=qfalse;
 source.nextthink=0;
 if(!G_SandboxLaserRead(source,settings)||!settings.firing)return;
 source.nextthink=level.time+interval;
 if(!settings.enabled||!sandboxLaserVector(source.currentOrigin)||!sandboxLaserVector(source.currentAngles))return;

 vec3_t start,direction,end;
 VectorCopy(source.currentOrigin,start);
 start[2]+=12;
 if(source.target&&*source.target){
  const auto* target=G_SandboxLaserAim(source);
  if(!target)return;
  for(int i=0;i<3;++i)direction[i]=target->currentOrigin[i]+.5f*(target->mins[i]+target->maxs[i])-start[i];
  if(!sandboxLaserVector(direction)||VectorNormalize(direction)<.001f)return;
 }else AngleVectors(source.currentAngles,direction,nullptr,nullptr);
 VectorMA(start,range,direction,end);
 if(!sandboxLaserVector(start)||!sandboxLaserVector(end))return;

 trace_t hit{};
 gi.trace(&hit,start,nullptr,nullptr,end,source.s.number,MASK_SHOT,(EG2_Collision)0,0);
 if(hit.startsolid||hit.allsolid||!(hit.fraction>=0&&hit.fraction<=1)||!sandboxLaserVector(hit.endpos))return;
 VectorCopy(start,source.pos2);
 VectorCopy(hit.endpos,source.s.origin2);
 source.alt_fire=qtrue;
 if(settings.damage&&hit.fraction<1&&hit.entityNum>=0&&hit.entityNum<ENTITYNUM_WORLD&&hit.entityNum!=source.s.number){
  auto& victim=g_entities[hit.entityNum];
  // Damage can destroy the source via native effects. Publish first and
  // leave all later cleanup to the usual entity lifetime path.
  if(victim.inuse&&victim.takedamage)
   G_Damage(&victim,&source,&source,direction,hit.endpos,settings.damage,DAMAGE_NO_KNOCKBACK,MOD_ENERGY);
 }
}
