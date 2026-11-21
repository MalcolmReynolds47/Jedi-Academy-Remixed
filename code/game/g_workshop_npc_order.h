// Native single-player state and dispatch for Sandbox NPC orders. GPL-2.0-only.
#pragma once

#include "g_local.h"
#include "g_subbsp_ownership.h"
#include "g_reference_tags.h"
#include "g_waypoints.h"
#include "qcommon/jar_context.h"
#include "qcommon/jar_npc_order.h"

// gNPC_t saves the mode and follow distance in its native NPC chunk.
constexpr int workshopNPCOrderShift=16;
constexpr int workshopNPCOrderMask=7<<workshopNPCOrderShift;

inline jar::npcOrder::Settings G_WorkshopReadNPCOrder(const gNPC_t& npc){
 const auto mode=static_cast<jar::npcOrder::Mode>((npc.eFlags&workshopNPCOrderMask)>>workshopNPCOrderShift);
 return {mode,npc.followDist};
}

inline void G_WorkshopWriteNPCOrder(gNPC_t& npc,const jar::npcOrder::Settings& order){
 npc.eFlags=(npc.eFlags&~workshopNPCOrderMask)|(int(order.mode)<<workshopNPCOrderShift);
 npc.followDist=order.distance;
}

inline jar::npcOrder::Destination G_WorkshopReadNPCGoal(const gNPC_t& npc){
 return {npc.workshopGoalOwner,npc.workshopGoalSlot};
}

inline void G_WorkshopWriteNPCGoal(gNPC_t& npc,const jar::npcOrder::Destination& goal){
 npc.workshopGoalOwner=goal.owner;
 npc.workshopGoalSlot=goal.slot;
}

inline jar::npcOrder::Destination G_WorkshopReadNPCPatrol(const gNPC_t& npc){
 return {npc.workshopPatrolOwner,npc.workshopPatrolSlot};
}

inline void G_WorkshopWriteNPCPatrol(gNPC_t& npc,jar::npcOrder::Destination point){
 npc.workshopPatrolOwner=point.owner;
 npc.workshopPatrolSlot=point.slot;
}

inline gentity_t* G_WorkshopResolveNPCGoal(jar::npcOrder::Destination link){
 if(!link.owner||!jar::npcOrder::ValidDestination(link,ENTITYNUM_WORLD))return nullptr;
 auto& goal=g_entities[link.slot];
 const bool navigationGoal=G_ReferencePortable(goal)&&!strcmp(goal.classname,"waypoint_navgoal");
 if((!navigationGoal&&!G_WaypointPortable(goal))||!goal.targetname||
    !std::isfinite(goal.radius)||goal.radius<0||goal.radius>1024||goal.radius!=std::floor(goal.radius))return nullptr;
 // The stored ownership ID and slot must both match, even after slot reuse.
 char identity[32];
 Com_sprintf(identity,sizeof(identity),"jar_sb_%u",link.owner);
 return strcmp(goal.targetname,identity)?nullptr:&goal;
}

inline jar::npcOrder::Destination G_WorkshopNextNPCPatrol(const gentity_t& point){
 constexpr char prefix[]="jar_sb_";
 uint32_t owner;
 if(!point.target||strncmp(point.target,prefix,sizeof(prefix)-1)||
    !jar::context::Token(point.target+sizeof(prefix)-1,owner))return {};
 auto* next=G_Find(nullptr,FOFS(targetname),point.target);
 if(!next||next==&point||!G_WaypointPortable(*next))return {};
 const jar::npcOrder::Destination destination{owner,unsigned(next->s.number)};
 return G_WorkshopResolveNPCGoal(destination)?destination:jar::npcOrder::Destination{};
}

inline bool G_WorkshopNPCOrderCanFollow(const gentity_t& actor,const gentity_t& player){
 return player.inuse&&player.client&&player.health>0&&
  actor.client->playerTeam==TEAM_PLAYER&&player.client->playerTeam==TEAM_PLAYER;
}

inline bool G_WorkshopNPCOrderHasNativeControl(const gentity_t& actor){
 const auto& ps=actor.client->ps;
 const auto& npc=*actor.NPC;
 const auto behavior=npc.tempBehavior!=BS_DEFAULT?npc.tempBehavior:
  npc.behaviorState!=BS_DEFAULT?npc.behaviorState:npc.defaultBehavior;
 constexpr int scriptedFlags=SCF_PILOT|SCF_FORCED_MARCH|SCF_FIRE_WEAPON|SCF_FIRE_WEAPON_NO_ANIM|
  SCF_LEAN_LEFT|SCF_LEAN_RIGHT|SCF_SAFE_REMOVE;
 return behavior==BS_CINEMATIC||actor.m_pVehicle||actor.s.m_iVehicleNum||ps.weapon==WP_EMPLACED_GUN||
  (actor.svFlags&(SVF_LOCKEDENEMY|SVF_ICARUS_FREEZE))||
  (npc.scriptFlags&scriptedFlags)||actor.next_roff_time>level.time;
}

bool G_WorkshopNPCOrderScripted(gentity_t& actor);

inline void G_WorkshopStopNPCOrderAttack(gentity_t& actor){
 auto& ps=actor.client->ps;
 actor.NPC->attackHoldTime=0;
 actor.client->fireDelay=0;
 if(ps.weaponstate==WEAPON_CHARGING||ps.weaponstate==WEAPON_CHARGING_ALT)
  ps.weaponstate=WEAPON_READY;
 ps.weaponChargeTime=0;
 ps.eFlags&=~(EF_FIRING|EF_ALT_FIRING);
 actor.s.eFlags&=~(EF_FIRING|EF_ALT_FIRING);
}

template<class Wait,class Follow,class Go>
bool G_WorkshopRunNPCOrder(gentity_t& actor,gentity_t& player,Wait wait,Follow follow,Go go){
 if(!(actor.NPC->eFlags&workshopNPCOrderMask))return false;
 const auto order=G_WorkshopReadNPCOrder(*actor.NPC);
 if(!jar::npcOrder::Valid(order)||G_WorkshopNPCOrderHasNativeControl(actor)||
    G_WorkshopNPCOrderScripted(actor))return false;
 if(jar::npcOrder::UsesDestination(order.mode)){
  G_WorkshopStopNPCOrderAttack(actor);
  const bool patrol=order.mode==jar::npcOrder::Patrol;
  const auto cursor=G_WorkshopReadNPCPatrol(*actor.NPC);
  const auto target=patrol&&cursor.owner?cursor:G_WorkshopReadNPCGoal(*actor.NPC);
  auto* goal=G_WorkshopResolveNPCGoal(target);
  if(!goal||(goal->svFlags&SVF_INACTIVE)||(patrol&&!G_WaypointPortable(*goal))){
   wait();
  }else if(go(*goal)&&patrol){
   const auto next=G_WorkshopNextNPCPatrol(*goal);
   if(next.owner)G_WorkshopWriteNPCPatrol(*actor.NPC,next);
  }
  return true;
 }
 if(order.mode==jar::npcOrder::Wait||!G_WorkshopNPCOrderCanFollow(actor,player)){
  G_WorkshopStopNPCOrderAttack(actor);
  wait();
 }else{
  // A Sandbox order must not replace a leader assigned by a native script.
  auto* client=actor.client;
  auto* originalLeader=client->leader;
  const auto lifetime=G_EntityLifetime(&actor);
  client->leader=&player;
  follow();
  if(actor.inuse&&G_EntityLifetime(&actor)==lifetime&&actor.client==client&&client->leader==&player)
   client->leader=originalLeader;
 }
 return true;
}
