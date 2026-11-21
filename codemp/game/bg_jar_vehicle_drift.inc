// Run native driving/gravity first, then sweep horizontal external momentum.
// Included by shared Pmove and the native step/slide fixture. GPL-2.0-only.
static qboolean PM_JARVehicleDriftBegin(void){
 if(!pm_entSelf||!pm_entSelf->m_pVehicle||pm_entSelf->s.NPC_class!=CLASS_VEHICLE||pm->ps->clientNum<MAX_CLIENTS)return qfalse;
 for(int a=0;a<3;++a)if(!isfinite(pm->ps->jarVehicleDrift[a])){VectorClear(pm->ps->jarVehicleDrift);return qfalse;}
 pm->ps->jarVehicleDrift[2]=0; // vertical motion stays entirely in native physics
 if(VectorLengthSquared(pm->ps->jarVehicleDrift)==0)return qfalse;
 // Do this after Pmove's freeze/intermission returns, before native controllers.
 VectorSubtract(pm->ps->velocity,pm->ps->jarVehicleDrift,pm->ps->velocity);
 return qtrue;
}
static void PM_JARVehicleDriftMove(void){
 vec3_t native;VectorCopy(pm->ps->velocity,native);
 VectorCopy(pm->ps->jarVehicleDrift,pm->ps->velocity);
 // Native movement already applied gravity. Use its step/slide contacts again
 // for this component, without applying gravity a second time.
 PM_StepSlideMove(qfalse);
 // Ground friction damps sideways pushes quickly; airborne fling momentum
 // decays slowly. Exponential damping is independent of command subdivision.
 float drag=pml.walking?4.f:.25f;
 VectorScale(pm->ps->velocity,expf(-drag*pml.frametime),pm->ps->jarVehicleDrift);
 // Stepping/sliding may add a vertical component; retain it in native velocity.
 native[2]+=pm->ps->jarVehicleDrift[2];pm->ps->jarVehicleDrift[2]=0;
 if(VectorLengthSquared(pm->ps->jarVehicleDrift)<.01f)VectorClear(pm->ps->jarVehicleDrift);
 VectorAdd(native,pm->ps->jarVehicleDrift,pm->ps->velocity);
}
