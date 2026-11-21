// Apply horizontal launch drift using the native hull sweep. GPL-2.0-only.
static Vehicle_t* PM_BeginVehiclePush(Vehicle_t* vehicle){
 if(!vehicle||pm->ps->pm_type!=PM_NORMAL)return nullptr;
 auto& drift=vehicle->jarPushDrift;
 if((pm->ps->eFlags&JAR_VEHICLE_IMMOBILE_FLAGS)||!G_VehiclePushDriftValid(drift)){
  VectorClear(drift);return nullptr;
 }
 if(!VectorLengthSquared(drift))return nullptr;
 VectorSubtract(pm->ps->velocity,drift,pm->ps->velocity);
 return vehicle;
}

static void PM_MoveVehiclePush(Vehicle_t& vehicle){
 vec3_t native;
 VectorCopy(pm->ps->velocity,native);
 VectorCopy(vehicle.jarPushDrift,pm->ps->velocity);
 // The native movement already advanced gravity for this command.
 PM_StepSlideMove(0.f);
 if(pm->moverRemoved)return;
 const float drag=pml.walking?4.f:.25f;
 VectorScale(pm->ps->velocity,std::exp(-drag*pml.frametime),vehicle.jarPushDrift);
 native[2]+=vehicle.jarPushDrift[2];
 vehicle.jarPushDrift[2]=0;
 if(VectorLengthSquared(vehicle.jarPushDrift)<.01f)VectorClear(vehicle.jarPushDrift);
 VectorAdd(native,vehicle.jarPushDrift,pm->ps->velocity);
}
