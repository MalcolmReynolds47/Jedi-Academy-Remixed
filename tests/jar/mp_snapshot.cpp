#include "mp_message_support.inc"
#include "mp_capacity_snapshot.inc"
#include "mp_crowd_demo.inc"
#include "../../codemp/game/jar_mp_teleport_volume.h"
#include "../../codemp/game/bg_jar_physgun_input.h"
#include "../../shared/qcommon/jar_galak_surfaces.h"

static void TeleportPilotSnapshotCases() {
    entityState_t grant{};
    grant.number = 9001;
    grant.eType = ET_TELEPORT_TRIGGER;
    grant.generic1 = JAR_MP_TELEPORT_VOLUME;
    grant.modelindex = 192;
    grant.otherEntityNum = 9000;
    grant.otherEntityNum2 = ENTITYNUM_WORLD - 1;
    grant.time = 7205;
    grant.speed = 33.125f;
    grant.trickedentindex = 1u << 7;
    grant.trickedentindex2 = 1u << 15;
    grant.trickedentindex4 = 1u << 15;
    grant.pos.trType = grant.apos.trType = TR_STATIONARY;
    VectorSet(grant.pos.trBase, -5097.125f, 213.5f, -45.25f);
    VectorSet(grant.apos.trBase, -7.25f, 96.625f, 31.5f);
    VectorSet(grant.origin2, 4600.375f, -3333.625f, 198.125f);
    VectorSet(grant.angles2, 32.5f, -71.75f, 17.25f);
    VectorSet(grant.origin, -65492.375f, 32767.625f, -4097.125f);
    VectorSet(grant.angles, -65502.5f, 32789.375f, -4039.625f);
    VectorSet(grant.pos.trDelta, -117.125f, -61.625f, -35.25f);
    VectorSet(grant.apos.trDelta, 108.625f, 73.875f, 145.0625f);
    VectorSet(grant.boneAngles1, -15.5f, -16.25f, -24.125f);
    VectorSet(grant.boneAngles2, 15.25f, 16.625f, 31.875f);

    entityState_t reassigned = grant;
    reassigned.time += 25;
    reassigned.otherEntityNum2 -= 16;
    reassigned.trickedentindex3 = 1u;
    reassigned.trickedentindex4 = 0;
    reassigned.origin[1] = -5783.5f;
    reassigned.angles[1] = -5760.125f;
    reassigned.pos.trDelta[2] = -78.5f;
    reassigned.apos.trDelta[0] = 126.375f;
    reassigned.boneAngles1[1] = -13.5f;
    reassigned.boneAngles2[2] = 28.125f;

    entityState_t revoked = reassigned;
    revoked.time += 25;
    revoked.otherEntityNum2 = ENTITYNUM_NONE;
    revoked.trickedentindex3 = revoked.trickedentindex4 = 0;
    VectorClear(revoked.origin);
    VectorClear(revoked.angles);
    VectorClear(revoked.pos.trDelta);
    VectorClear(revoked.apos.trDelta);
    VectorClear(revoked.boneAngles1);
    VectorClear(revoked.boneAngles2);

    entityState_t reused{};
    reused.number = grant.number;
    reused.eType = ET_GENERAL;
    reused.generic1 = 250;
    reused.pos.trType = reused.apos.trType = TR_INTERPOLATE;
    VectorSet(reused.pos.trBase, 2800.5f, -45.125f, 96.75f);
    VectorSet(reused.origin2, -14.75f, -28.25f, -4.5f);
    VectorSet(reused.angles2, 19.125f, 36.5f, 39.625f);
    VectorSet(reused.pos.trDelta, -20.125f, 39.5f, -3.125f);
    VectorSet(reused.apos.trDelta, 0, -179.75f, 0);

    byte data[4096];
    msg_t message;
    entityState_t previous{};
    entityState_t stages[] = {grant, reassigned, revoked, reused};
    for (entityState_t& expected : stages) {
        MSG_Init(&message, data, sizeof(data));
        MSG_WriteDeltaEntity(&message, &previous, &expected, qtrue);
        MSG_WriteBits(&message, ENTITYNUM_NONE, GENTITYNUM_BITS);
        assert(!message.overflowed);

        MSG_BeginReading(&message);
        entityState_t decoded{};
        const int number = MSG_ReadBits(&message, GENTITYNUM_BITS);
        MSG_ReadDeltaEntity(&message, &previous, &decoded, number);
        assert(MSG_ReadBits(&message, GENTITYNUM_BITS) == ENTITYNUM_NONE);
        assert(message.readcount <= message.cursize);
        assert(decoded.number == expected.number && decoded.eType == expected.eType &&
               decoded.generic1 == expected.generic1 && decoded.modelindex == expected.modelindex &&
               decoded.otherEntityNum == expected.otherEntityNum && decoded.time == expected.time &&
               decoded.speed == expected.speed);
        assert(decoded.otherEntityNum2 == expected.otherEntityNum2 &&
               decoded.trickedentindex3 == expected.trickedentindex3 &&
               decoded.trickedentindex4 == expected.trickedentindex4);
        assert(decoded.trickedentindex == expected.trickedentindex &&
               decoded.trickedentindex2 == expected.trickedentindex2);
        assert(decoded.pos.trType == expected.pos.trType && decoded.apos.trType == expected.apos.trType);
        assert(VectorCompare(decoded.pos.trBase, expected.pos.trBase) &&
               VectorCompare(decoded.apos.trBase, expected.apos.trBase) &&
               VectorCompare(decoded.origin2, expected.origin2) &&
               VectorCompare(decoded.angles2, expected.angles2));
        assert(VectorCompare(decoded.origin, expected.origin) &&
               VectorCompare(decoded.angles, expected.angles) &&
               VectorCompare(decoded.pos.trDelta, expected.pos.trDelta) &&
               VectorCompare(decoded.apos.trDelta, expected.apos.trDelta) &&
               VectorCompare(decoded.boneAngles1, expected.boneAngles1) &&
               VectorCompare(decoded.boneAngles2, expected.boneAngles2));

        const bool isPortal = expected.eType == ET_TELEPORT_TRIGGER;
        assert(!!JAR_TeleportVolumeValid(&decoded) == isPortal);
        assert(!!JAR_PushVolumeAllowed(&decoded, 7) == isPortal);
        assert(!!JAR_PushVolumeAllowed(&decoded, MAX_CLIENTS - 1) == isPortal);
        assert(!JAR_PushVolumeAllowed(&decoded, 0));
        assert(!!JAR_PushVolumePilotAllowed(&decoded, 0) == (expected.trickedentindex3 != 0));
        assert(!!JAR_PushVolumePilotAllowed(&decoded, MAX_CLIENTS - 1) == (expected.trickedentindex4 != 0));

        playerState_t pilot{}, hull{};
        pilot.clientNum = expected.trickedentindex4 ? MAX_CLIENTS - 1 : 0;
        hull.clientNum = expected.trickedentindex4 ? grant.otherEntityNum2 : reassigned.otherEntityNum2;
        pilot.m_iVehicleNum = hull.clientNum;
        hull.m_iVehicleNum = pilot.clientNum + 1;
        pilot.commandTime = hull.commandTime = expected.time + 20;
        pilot.stats[STAT_HEALTH] = 100;
        hull.stats[STAT_HEALTH] = 300;
        VectorCopy(expected.pos.trBase, hull.origin);
        const vec3_t mins = {-42.25f, -36.125f, -24.5f};
        const vec3_t maxs = {43.75f, 36.375f, 58.125f};
        const usercmd_t cmd{};
        jarVehicleTeleportArrival_t arrival{};
        const bool hasGrant = expected.trickedentindex3 || expected.trickedentindex4;
        assert(!!JAR_TeleportVolumeVehicleCandidate(&decoded, &hull, &pilot, &cmd, mins, maxs, &arrival) == hasGrant);
        if (hasGrant) {
            assert(VectorCompare(arrival.vehicleOrigin, expected.origin) &&
                   VectorCompare(arrival.vehicleMins, expected.pos.trDelta) &&
                   VectorCompare(arrival.vehicleMaxs, expected.apos.trDelta) &&
                   VectorCompare(arrival.pilotOrigin, expected.angles) &&
                   VectorCompare(arrival.pilotMins, expected.boneAngles1) &&
                   VectorCompare(arrival.pilotMaxs, expected.boneAngles2));
        }
        previous = decoded;
    }
}

int main(int argc, char **argv){
    if (argc == 3 && !strcmp(argv[1], "--crowd-demo"))
        return ReadCrowdDemo(argv[2]);
    if (argc == 3 && !strcmp(argv[1], "--rider-demo"))
        return ReadCrowdDemo(argv[2], DemoView::Rider);
    if (argc == 3 && !strcmp(argv[1], "--vehicle-demo"))
        return ReadCrowdDemo(argv[2], DemoView::Vehicle);
    {
        playerState_t base{},holding{},received{};
        holding.stats[STAT_JAR_PHYSGUN_HOLD]=10001;
        byte wire[4096]{};
        msg_t message{};
        MSG_Init(&message,wire,sizeof(wire));
        MSG_WriteDeltaPlayerstate(&message,&base,&holding);
        MSG_BeginReading(&message);MSG_ReadDeltaPlayerstate(&message,&base,&received);
        assert(received.stats[STAT_JAR_PHYSGUN_HOLD]==10001);
        usercmd_t before{},sent{},decoded{};
        sent.buttons=BUTTON_ATTACK|BUTTON_JAR_PHYSGUN_TURN;sent.weapon=WP_PHYSGUN;sent.serverTime=52;
        sent.angles[YAW]=12345;sent.angles[PITCH]=222;
        float remainderYaw=0,remainderPitch=0;
        sent.angles[ROLL]=JAR_PackPhysgunTurn(20.5f,-8.75f,&remainderYaw,&remainderPitch);
        MSG_Init(&message,wire,sizeof(wire));MSG_WriteDeltaUsercmdKey(&message,12,&before,&sent);
        MSG_BeginReading(&message);MSG_ReadDeltaUsercmdKey(&message,12,&before,&decoded);
        assert(decoded.buttons==sent.buttons&&decoded.angles[YAW]==12345&&decoded.angles[PITCH]==222);
        assert(JAR_UnpackPhysgunAxis(decoded.angles[ROLL])==20.5f&&JAR_UnpackPhysgunAxis(decoded.angles[ROLL]>>8)==-8.75f);
        holding.viewangles[ROLL]=15;holding.delta_angles[ROLL]=11;
        JAR_RestorePhysgunCommandView(&holding,&decoded);
        assert(decoded.buttons==BUTTON_ATTACK&&decoded.angles[ROLL]==ANGLE2SHORT(15)-11);
    }
 CapacitySnapshotCases();
 byte data[32768];msg_t message;playerState_t baseline{},sent{},received{};
 for(int weapon=WP_PHYSGUN;weapon<=WP_TOOLGUN;++weapon){
  sent.weapon=weapon;sent.stats[STAT_WEAPONS]=(1<<WP_SABER)|(1<<WP_PHYSGUN)|(1<<WP_TOOLGUN);
  MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaPlayerstate(&message,&baseline,&sent);assert(!message.overflowed);
  MSG_BeginReading(&message);MSG_ReadDeltaPlayerstate(&message,&baseline,&received);assert(received.weapon==weapon&&received.stats[STAT_WEAPONS]==sent.stats[STAT_WEAPONS]);
 }
 entityState_t before{},rope{},out{};rope.number=500;rope.generic1=251;rope.pos.trType=TR_INTERPOLATE;rope.pos.trBase[0]=128;rope.origin2[0]=512;rope.origin2[2]=64;rope.time2=440;
 for(int kind=251;kind<=253;++kind){
  rope.generic1=kind;MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaEntity(&message,&before,&rope,qtrue);assert(!message.overflowed);
  MSG_BeginReading(&message);int number=MSG_ReadBits(&message,GENTITYNUM_BITS);MSG_ReadDeltaEntity(&message,&before,&out,number);
  assert(out.number==500&&out.generic1==kind&&out.time2==440&&out.origin2[0]==512&&out.origin2[2]==64&&out.pos.trType==TR_INTERPOLATE);
 }
 // Routed topology is atomic with its endpoints, including late join, changed
 // corners, unwrapping, unresolved state and slot reuse by a non-rope entity.
 assert(ARRAY_LEN(entityStateFields)<256);
 entityState_t route{};before={};route.number=7000;route.generic1=251;route.jarRopeState=10;route.jarRopeLength=8191.75f;route.otherEntityNum=ENTITYNUM_WORLD;route.otherEntityNum2=MAX_GENTITIES-3;
 for(int i=0;i<8;++i)for(int a=0;a<3;++a)route.jarRopeBends[i][a]=(i+1)*123.125f-a*50000.25f;
 for(int pass=0;pass<4;++pass){MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaEntity(&message,&before,&route,qtrue);assert(!message.overflowed);
  MSG_BeginReading(&message);int n=MSG_ReadBits(&message,GENTITYNUM_BITS);MSG_ReadDeltaEntity(&message,&before,&out,n);
  assert(out.jarRopeState==route.jarRopeState&&out.jarRopeLength==route.jarRopeLength&&out.otherEntityNum==route.otherEntityNum&&!memcmp(out.jarRopeBends,route.jarRopeBends,sizeof(route.jarRopeBends)));
  before=out;if(pass==0)route.jarRopeBends[7][2]+=3.25f;else{memset(route.jarRopeBends,0,sizeof(route.jarRopeBends));route.jarRopeState=pass==1?2:1;}
 }
 route={};route.number=7000;route.generic1=250;MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaEntity(&message,&before,&route,qtrue);MSG_BeginReading(&message);MSG_ReadDeltaEntity(&message,&before,&out,MSG_ReadBits(&message,GENTITYNUM_BITS));assert(!out.jarRopeState&&!out.jarRopeLength&&!out.jarRopeBends[7][2]);before={};
 // All 128 attachments at maximum bend count fit a native message. Use
 // fractional coordinates so the codec cannot take its small-integer shortcut.
 before={};MSG_Init(&message,data,sizeof(data));
 for(int i=0;i<128;++i){route.number=1000+i;route.generic1=251;route.jarRopeState=10;route.jarRopeLength=131071.75f;
  for(int j=0;j<8;++j)for(int a=0;a<3;++a)route.jarRopeBends[j][a]=1234.125f+i*17.25f+j*13.5f+a*2.125f;
  MSG_WriteDeltaEntity(&message,&before,&route,qtrue);assert(!message.overflowed);}
 MSG_BeginReading(&message);for(int i=0;i<128;++i){MSG_ReadDeltaEntity(&message,&before,&out,MSG_ReadBits(&message,GENTITYNUM_BITS));assert(out.number==1000+i&&out.jarRopeState==10&&out.jarRopeLength==131071.75f&&out.jarRopeBends[7][2]==1234.125f+i*17.25f+7*13.5f+2*2.125f);}
 printf("128 fully wrapped rope baselines use %d bytes of a 32768-byte native message.\n",message.cursize);
 // Prediction sensor fields survive late join and delta snapshots, including
 // the high client consent bit and signed once/cooldown timing.
 entityState_t sensor{};before={};sensor.number=8000;sensor.eType=ET_PUSH_TRIGGER;sensor.generic1=247;sensor.modelindex=512;sensor.frame=2068;sensor.speed=1375.25f;sensor.time=-500;sensor.time2=-1;sensor.bolt2=1;sensor.angles2[0]=800;sensor.trickedentindex=sensor.trickedentindex2=65535;sensor.trickedentindex3=32769;sensor.trickedentindex4=65535;sensor.pos.trType=sensor.apos.trType=TR_INTERPOLATE;sensor.pos.trTime=1400;VectorSet(sensor.origin2,400.25f,-200.5f,300.75f);VectorSet(sensor.pos.trBase,20,30,40);VectorSet(sensor.apos.trBase,12.5f,67.75f,90);VectorSet(sensor.pos.trDelta,12.5f,5.25f,-3);VectorSet(sensor.apos.trDelta,0,900,0);
 for(int pass=0;pass<2;++pass){MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaEntity(&message,&before,&sensor,qtrue);MSG_BeginReading(&message);int number=MSG_ReadBits(&message,GENTITYNUM_BITS);MSG_ReadDeltaEntity(&message,&before,&out,number);assert(JAR_PushVolumeValid(&out)&&out.time==sensor.time&&out.time2==sensor.time2&&out.frame==sensor.frame&&out.speed==sensor.speed&&out.trickedentindex3==sensor.trickedentindex3&&out.trickedentindex4==sensor.trickedentindex4&&JAR_PushVolumePilotAllowed(&out,31)==(pass==0)&&JAR_PushVolumePilotAllowed(&out,15)&&out.trickedentindex2==sensor.trickedentindex2&&JAR_PushVolumeAllowed(&out,31)==(pass==0)&&VectorCompare(out.pos.trDelta,sensor.pos.trDelta)&&VectorCompare(out.apos.trBase,sensor.apos.trBase)&&VectorCompare(out.origin2,sensor.origin2));before=out;sensor.time=1600;sensor.time2=60000;sensor.trickedentindex2=sensor.trickedentindex4=0;}
 // Owned teleport grants keep their separate type, high permission bit and
 // exact arrival transform through late-join and revocation delta snapshots.
 entityState_t portal{};before={};portal.number=9000;portal.eType=ET_TELEPORT_TRIGGER;portal.generic1=JAR_MP_TELEPORT_VOLUME;portal.modelindex=256;portal.otherEntityNum=8999;portal.time=2213;portal.speed=32.25f;
 portal.trickedentindex2=1u<<15;VectorSet(portal.origin2,400.125f,-170.625f,2048.75f);VectorSet(portal.angles2,15.25f,-147.125f,0.25f);VectorSet(portal.pos.trBase,64.75f,96.5f,128.25f);VectorSet(portal.apos.trBase,8.75f,61.5f,4.25f);
 for(int pass=0;pass<2;++pass){MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaEntity(&message,&before,&portal,qtrue);assert(!message.overflowed);MSG_BeginReading(&message);MSG_ReadDeltaEntity(&message,&before,&out,MSG_ReadBits(&message,GENTITYNUM_BITS));assert(JAR_TeleportVolumeValid(&out)&&JAR_PushVolumeAllowed(&out,31)==(pass==0)&&!JAR_PushVolumeAllowed(&out,0)&&out.time==portal.time&&out.speed==portal.speed&&out.otherEntityNum==8999&&VectorCompare(out.origin2,portal.origin2)&&VectorCompare(out.angles2,portal.angles2)&&VectorCompare(out.pos.trBase,portal.pos.trBase)&&VectorCompare(out.apos.trBase,portal.apos.trBase));before=out;portal.trickedentindex2=0;portal.time+=25;}
 TeleportPilotSnapshotCases();
 // Galak uses all 32 native surface bits, including sign-bit firing state.
 for(unsigned phase: {0u,JAR_GALAK_CHARGING,JAR_GALAK_FIRING,0u}){
  entityState_t mech{};mech.number=501;mech.eType=ET_NPC;mech.NPC_class=CLASS_GALAKMECH;
  mech.surfacesOn=int(phase|(1u<<14));mech.surfacesOff=1u<<7;
  MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaEntity(&message,&before,&mech,qtrue);
  MSG_BeginReading(&message);int number=MSG_ReadBits(&message,GENTITYNUM_BITS);MSG_ReadDeltaEntity(&message,&before,&out,number);
  assert(out.surfacesOn==mech.surfacesOn&&out.surfacesOff==mech.surfacesOff&&out.NPC_class==CLASS_GALAKMECH&&!message.overflowed);
  before=out;
 }
 before={};
 // Asymmetric rotated prop bounds survive both late join and delta snapshots.
 entityState_t prop{};prop.frame=65535;prop.number=123;prop.modelGhoul2=1;prop.time2=65000;prop.eType=ET_GENERAL;prop.generic1=250;
 prop.bolt2=JAR_MP_MODEL_HULL_TAG;prop.modelindex=52;
 VectorSet(prop.origin2,-480.25f,-517.5f,-250);VectorSet(prop.angles2,620.5f,718.25f,832);
 prop.pos.trType=prop.apos.trType=TR_INTERPOLATE;prop.pos.trTime=1000;prop.apos.trTime=1000;
 VectorSet(prop.pos.trDelta,20,-30,10);VectorSet(prop.apos.trDelta,0,900,0);
 for(int pass=0;pass<2;++pass){
  MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaEntity(&message,&before,&prop,qtrue);assert(!message.overflowed);
  MSG_BeginReading(&message);int number=MSG_ReadBits(&message,GENTITYNUM_BITS);MSG_ReadDeltaEntity(&message,&before,&out,number);
  vec3_t low,high;assert(out.modelGhoul2==1&&JAR_MP_ModelHullState(&out)&&out.time2==prop.time2&&out.frame==prop.frame&&JAR_MP_PropBounds(&out,low,high)&&low[0]==prop.origin2[0]&&high[1]==prop.angles2[1]&&out.pos.trDelta[1]==-30&&out.apos.trDelta[1]==900);
  before=out;prop.time2+=1024;prop.frame=2;prop.origin2[0]=-12.125f;prop.angles2[1]=40.25f;
 }
 vec3_t foot={10,0,0},moved;
 assert(JAR_MP_PropRider(&out,foot,1000,1100,moved));assert(fabsf(moved[0]-2)<.001f&&fabsf(moved[1]-7)<.001f&&fabsf(moved[2]-1)<.001f);
 assert(!JAR_MP_PropRider(&out,foot,1000,1101,moved));
 out.origin2[0]=std::numeric_limits<float>::quiet_NaN();vec3_t low,high;assert(!JAR_MP_PropBounds(&out,low,high));
 // Held state and release use existing native fields, including a fresh late join.
 sent={};sent.clientNum=1;sent.pm_type=PM_FREEZE;sent.commandTime=1200;
 sent.origin[0]=200;sent.origin[2]=96;sent.viewangles[YAW]=45;
 sent.forceHandExtend=HANDEXTEND_CHOKE;sent.forceHandExtendTime=1400;
 sent.legsAnim=sent.torsoAnim=BOTH_CHOKE3;sent.legsTimer=sent.torsoTimer=200;
 sent.stats[STAT_HEALTH]=100;sent.fd.forcePower=100;
 MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaPlayerstate(&message,&baseline,&sent);
 MSG_BeginReading(&message);MSG_ReadDeltaPlayerstate(&message,&baseline,&received);
 assert(received.pm_type==PM_FREEZE&&received.origin[2]==96&&received.viewangles[YAW]==45);
 assert(received.forceHandExtend==HANDEXTEND_CHOKE&&received.torsoAnim==BOTH_CHOKE3&&received.commandTime==1200);
 assert(received.stats[STAT_HEALTH]==100&&received.fd.forcePower==100&&received.fd.forcePowersActive==0);
 playerState_t held=received;sent.pm_type=PM_NORMAL;sent.forceHandExtend=HANDEXTEND_NONE;sent.velocity[0]=600;
 MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaPlayerstate(&message,&held,&sent);
 MSG_BeginReading(&message);MSG_ReadDeltaPlayerstate(&message,&held,&received);
 assert(received.pm_type==PM_NORMAL&&received.forceHandExtend==HANDEXTEND_NONE&&received.velocity[0]==600);
 // The native vehicle-playerstate table differs from ordinary player snapshots.
 sent={};sent.clientNum=100;sent.pm_type=PM_FREEZE;sent.origin[0]=256;sent.origin[2]=192;
 sent.vehOrientation[PITCH]=30;sent.vehOrientation[YAW]=90;sent.vehOrientation[ROLL]=15;sent.stats[STAT_HEALTH]=230;
 MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaPlayerstate(&message,&baseline,&sent,qtrue);
 MSG_BeginReading(&message);MSG_ReadDeltaPlayerstate(&message,&baseline,&received,qtrue);
 assert(received.pm_type==PM_FREEZE&&received.origin[2]==192&&received.vehOrientation[PITCH]==30&&received.vehOrientation[YAW]==90&&received.vehOrientation[ROLL]==15&&received.stats[STAT_HEALTH]==230);
 held=received;sent.pm_type=PM_NORMAL;sent.velocity[0]=450;sent.speed=450;
 MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaPlayerstate(&message,&held,&sent,qtrue);
 MSG_BeginReading(&message);MSG_ReadDeltaPlayerstate(&message,&held,&received,qtrue);
 assert(received.pm_type==PM_NORMAL&&received.velocity[0]==450&&received.speed==450&&received.vehOrientation[YAW]==90);
 // Drift is full precision in ordinary, internal-pilot and vehicle layouts,
 // including a zero reset after release and a fresh late-join baseline.
 assert(ARRAY_LEN(playerStateFields)<256&&ARRAY_LEN(pilotPlayerStateFields)<256&&ARRAY_LEN(vehPlayerStateFields)<256);
 for(int layout=0;layout<3;++layout){
  sent={};sent.clientNum=layout==2?100:1;if(layout==1){sent.m_iVehicleNum=100;sent.eFlags=EF_NODRAW;}
  VectorSet(sent.jarVehicleDrift,137.25f,-62.125f,900.5f);
  MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaPlayerstate(&message,&baseline,&sent,(qboolean)(layout==2));
  MSG_BeginReading(&message);MSG_ReadDeltaPlayerstate(&message,&baseline,&received,(qboolean)(layout==2));assert(VectorCompare(sent.jarVehicleDrift,received.jarVehicleDrift));
  held=received;VectorClear(sent.jarVehicleDrift);
  MSG_Init(&message,data,sizeof(data));MSG_WriteDeltaPlayerstate(&message,&held,&sent,(qboolean)(layout==2));
  MSG_BeginReading(&message);MSG_ReadDeltaPlayerstate(&message,&held,&received,(qboolean)(layout==2));assert(VectorLengthSquared(received.jarVehicleDrift)==0&&!message.overflowed);
 }
 puts("MP snapshots preserve tools, ropes, actor control and vehicle drift in all three playerstate layouts.");
}
