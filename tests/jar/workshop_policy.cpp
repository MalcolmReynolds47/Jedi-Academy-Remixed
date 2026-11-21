#include "game/jar_workshop_policy.h"
#include "test_support.h"
using namespace jar::workshop;
int main(){return RunTests("native Workshop idle, transition and protected-control guards",[]{
 playerState_t ps{};ps.weapon=WP_BLASTER;ps.weaponstate=WEAPON_READY;
 auto control=[&]{return NativeControlAllowed(ps,100,1000,false,false,false,true,false);};
 CHECK(control() && NativeEquipAllowed(ps,control(),false,false));
 ps.weaponstate=WEAPON_IDLE;CHECK(NativeEquipAllowed(ps,control(),false,false));
 for(int state:{WEAPON_RAISING,WEAPON_DROPPING,WEAPON_FIRING,WEAPON_CHARGING,WEAPON_CHARGING_ALT}){
  ps.weaponstate=state;CHECK(!NativeEquipAllowed(ps,control(),false,false));}
 ps.weapon=WP_NONE;ps.weaponstate=WEAPON_RAISING;CHECK(NativeEquipAllowed(ps,control(),false,false));
 ps.weaponTime=1;CHECK(!NativeEquipAllowed(ps,control(),false,false));ps.weaponTime=0;
 CHECK(!NativeEquipAllowed(ps,false,false,false));ps.weapon=WP_BLASTER;
 ps.weaponstate=WEAPON_IDLE;ps.weaponTime=1;CHECK(!NativeEquipAllowed(ps,control(),false,false));
 ps.weaponTime=0;ps.saberInFlight=qtrue;CHECK(!NativeEquipAllowed(ps,control(),false,false));ps.saberInFlight=qfalse;
 CHECK(!NativeEquipAllowed(ps,control(),true,false));CHECK(!NativeEquipAllowed(ps,control(),false,true));
 ps.saberLockTime=1001;CHECK(!control());ps.saberLockTime=1000;CHECK(control());
 for(int flag:{EF_IN_ATST,EF_LOCKED_TO_WEAPON,EF_HELD_BY_RANCOR,EF_HELD_BY_WAMPA,EF_HELD_BY_SAND_CREATURE,EF_FORCE_GRIPPED,EF_FORCE_DRAINED}){
  ps.eFlags=flag;CHECK(!control());CHECK(!NativeEquipAllowed(ps,control(),false,false));}ps.eFlags=0;
 ps.viewEntity=1;CHECK(!control());ps.viewEntity=0;
 CHECK(!NativeControlAllowed(ps,0,1000,false,false,false,true,false));
 CHECK(!NativeControlAllowed(ps,100,1000,true,false,false,true,false));
 CHECK(!NativeControlAllowed(ps,100,1000,false,true,false,true,false));
 CHECK(!NativeControlAllowed(ps,100,1000,false,false,true,true,false));
 CHECK(!NativeControlAllowed(ps,100,1000,false,false,false,false,false));
 CHECK(!NativeControlAllowed(ps,100,1000,false,false,false,true,true));
 });}
