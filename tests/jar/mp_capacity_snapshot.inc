#include "../../codemp/client/client.h"
#include <vector>
#include <memory>
clientActive_t cl{};
#include "../../codemp/server/sv_jar_snapshot.inc"
#include "../../codemp/client/cl_jar_snapshot.inc"
#include "../../codemp/client/cl_jar_snapshot_copy.inc"
static void CapacitySnapshotCases(){
 static_assert(MAX_GENTITIES==16384&&MAX_SNAPSHOT_ENTITIES==4096&&MAX_ENTITIES_IN_SNAPSHOT==4096,"Capacity contract");
 static_assert(MAX_PARSE_ENTITIES>=PACKET_BACKUP*MAX_SNAPSHOT_ENTITIES,"Client history must retain full frames");
 std::vector<entityState_t> history(MAX_SNAPSHOT_ENTITIES*2);svs.snapshotEntities=history.data();svs.numSnapshotEntities=int(history.size());
 std::vector<byte> bytes(MAX_MSGLEN);msg_t message;clientSnapshot_t before{},after{};after.num_entities=MAX_SNAPSHOT_ENTITIES;
 for(int i=0;i<MAX_SNAPSHOT_ENTITIES;++i){auto&e=history[i];e.number=12000+i;e.eType=ET_GENERAL;e.modelindex=1800;e.modelindex2=1700;e.eventParm=1900;e.loopSound=1500;e.owner=15000;e.otherEntityNum=12000;e.otherEntityNum2=16000;e.groundEntityNum=14000;e.clientNum=e.number;e.m_iVehicleNum=15999;e.saberEntityNum=15555;e.pos.trType=TR_INTERPOLATE;e.pos.trBase[0]=float(i)*.125f;}
 cl.parseEntitiesNum=MAX_PARSE_ENTITIES-11;clSnapshot_t first{},second{};
 MSG_Init(&message,bytes.data(),int(bytes.size()));SV_EmitPacketEntities(nullptr,&after,&message);assert(!message.overflowed&&message.cursize>65535);
 MSG_BeginReading(&message);CL_ParsePacketEntities(&message,nullptr,&first);assert(first.numEntities==MAX_SNAPSHOT_ENTITIES);
 for(int i=0;i<MAX_SNAPSHOT_ENTITIES;++i){const auto&e=cl.parseEntities[(first.parseEntitiesNum+i)&(MAX_PARSE_ENTITIES-1)];assert(e.number==12000+i&&e.modelindex==1800&&e.modelindex2==1700&&e.eventParm==1900&&e.loopSound==1500&&e.owner==15000&&e.otherEntityNum==12000&&e.otherEntityNum2==16000&&e.groundEntityNum==14000&&e.clientNum==e.number&&e.m_iVehicleNum==15999&&e.saberEntityNum==15555);}
 // Decode removals, an inserted lower ID, changes and unchanged states after wrap.
 before=after;after.first_entity=MAX_SNAPSHOT_ENTITIES;
 for(int i=0;i<MAX_SNAPSHOT_ENTITIES;++i)history[after.first_entity+i]=history[i];
 history[after.first_entity].number=11000;history[after.first_entity+2].pos.trBase[2]=123.5f;
 MSG_Init(&message,bytes.data(),int(bytes.size()));SV_EmitPacketEntities(&before,&after,&message);assert(!message.overflowed);
 MSG_BeginReading(&message);CL_ParsePacketEntities(&message,&first,&second);assert(second.numEntities==MAX_SNAPSHOT_ENTITIES);
 second.valid=qtrue;second.messageNum=4;cl.snap=second;cl.snapshots[4]=second;auto delivered=std::make_unique<snapshot_t>();
 assert(CL_GetSnapshot(4,delivered.get())&&delivered->numEntities==4096&&delivered->entities[0].number==11000&&delivered->entities[2].pos.trBase[2]==123.5f&&delivered->entities[4095].number==16095);
 cl.parseEntitiesNum=second.parseEntitiesNum+MAX_PARSE_ENTITIES;assert(!CL_GetSnapshot(4,delivered.get()));
 // Reserved, repeated or excessive entries cannot index outside the native ring.
 for(int bad=0;bad<3;++bad){entityState_t zero{},entity{};entity.number=bad==0?ENTITYNUM_WORLD:100;
  MSG_Init(&message,bytes.data(),int(bytes.size()));int count=bad==2?MAX_SNAPSHOT_ENTITIES+1:bad==1?2:1;
  for(int i=0;i<count;++i){if(bad==2)entity.number=100+i;MSG_WriteDeltaEntity(&message,&zero,&entity,qtrue);}MSG_WriteBits(&message,ENTITYNUM_NONE,GENTITYNUM_BITS);
  bool rejected=false;MSG_BeginReading(&message);try{CL_ParsePacketEntities(&message,nullptr,&second);}catch(const std::runtime_error&){rejected=true;}assert(rejected);
 }
 // All playerstate tables preserve the wider entity/resource fields too.
 for(int layout=0;layout<3;++layout){playerState_t base{},sent{},received{};sent.clientNum=layout==2?16000:1;sent.groundEntityNum=15000;sent.saberEntityNum=15555;sent.rocketLockIndex=15666;sent.loopSound=1800;sent.eventParms[0]=1900;sent.eventParms[1]=1700;
  if(layout==1){sent.m_iVehicleNum=16000;sent.eFlags=EF_NODRAW;}
  MSG_Init(&message,bytes.data(),int(bytes.size()));MSG_WriteDeltaPlayerstate(&message,&base,&sent,(qboolean)(layout==2));MSG_BeginReading(&message);MSG_ReadDeltaPlayerstate(&message,&base,&received,(qboolean)(layout==2));
  assert(received.clientNum==sent.clientNum&&received.rocketLockIndex==15666&&received.eventParms[0]==1900&&received.eventParms[1]==1700);if(layout!=1)assert(received.groundEntityNum==15000&&received.loopSound==1800);
 }
 svs.snapshotEntities=nullptr;svs.numSnapshotEntities=0;
 puts("MP capacity codec: 4096 high-ID states, native server/client/module paths, resource indices, history wrap, delta removal, invalid limits and all playerstate layouts passed.");
}
