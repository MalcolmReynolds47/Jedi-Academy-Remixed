// Charge clearance for Workshop poses and constraints. GPL-2.0-only.
#pragma once

bool mountedChargeSweepClear(gentity_t& entity,const vec3_t angles,
 const std::vector<SandboxChargeMove>& charges,bool sweep,const float* pivot=nullptr){
 if(!sweep||charges.empty())return G_SandboxChargesPoseClear(entity,charges);

 const auto& before=entity.currentAngles;
 const auto spin=jar::physics::AngularVelocity({before[0],before[1],before[2]},
  {angles[0],angles[1],angles[2]},1);
 vec3_t axis={spin.x,spin.y,spin.z},arc;
 const float sag=1-std::cos(.5f*VectorNormalize(axis));
 for(int i=0;i<3;++i)arc[i]=sag*std::sqrt(std::max(0.f,1-axis[i]*axis[i]));
 return G_SandboxChargesPoseClear(entity,charges,arc,pivot);
}

bool mountedChargePoseClear(gentity_t& entity,const vec3_t origin,const vec3_t angles,
 std::vector<SandboxChargeMove>& charges,bool sweep){
 return G_SandboxChargesPlanPose(entity,origin,angles,charges,ENTITYNUM_WORLD)&&
  mountedChargeSweepClear(entity,angles,charges,sweep);
}

void placeEditedCharges(const std::vector<SandboxChargeMove>& charges){
 if(charges.empty())return;
 G_SandboxChargesPlace(charges);
 G_SandboxChargesRefresh(charges);
}
