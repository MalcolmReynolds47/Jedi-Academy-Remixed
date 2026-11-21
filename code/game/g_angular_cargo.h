// Staged riders for the portable angular mover's native push transaction. GPL-2.0-only.
#pragma once

#include "g_local.h"
#include "g_sandbox_charges.h"
#include "jar_physics.h"
#include <vector>

struct AngularCargoMember {
 gentity_t* entity;
 int vehicle; // Hull slot for the vehicle and its seats, or ENTITYNUM_NONE.
 int rackSupport=ENTITYNUM_NONE; // Rack holding this shelf pickup.
 bool linked;
 vec3_t destination,angles,mins,maxs;
 float yaw=0;
 std::shared_ptr<const jar::physics::PropGeometry> geometry;
};

struct AngularCargo {
 std::vector<AngularCargoMember> members;
 std::vector<jar::physics::PoseUpdate> rigid;
 std::vector<SandboxChargeMove> charges;
};

bool G_AngularCargoPlan(gentity_t& deck,const vec3_t destination,const vec3_t angles,AngularCargo& cargo);
bool G_AngularCargoCommit(gentity_t& deck,const AngularCargo& cargo);

struct AngularCargoUnlinked {
 const AngularCargo& cargo;
 SandboxChargesUnlinked charges;
 explicit AngularCargoUnlinked(const AngularCargo& group):cargo(group),charges(group.charges){
  for(const auto& member:cargo.members)if(member.linked)gi.unlinkentity(member.entity);
 }
 ~AngularCargoUnlinked(){
  for(const auto& member:cargo.members)if(member.linked)gi.linkentity(member.entity);
 }
};
