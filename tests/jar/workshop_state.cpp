#include "qcommon/jar_workshop_state.h"
#include "qcommon/jar_savegame.h"
#include "game/weapons.h"
#include "test_support.h"
#include <limits>
#include <memory>
using namespace jar::workshop;
int main(){return RunTests("carried inventory codec, ownership and save boundaries",[]{
 State s;Weapon saber;saber.family=WP_SABER;std::strcpy(saber.saber[0],"single_1");std::strcpy(saber.saber[1],"single_2");
 for(auto&c:saber.colors[0])c=4;for(auto&c:saber.colors[1])c=5;
 CHECK(Add(s,saber)==1);CHECK(Add(s,saber)==1);CHECK(s.count==1);
 Weapon gun;gun.family=WP_BLASTER;CHECK(Add(s,gun)==2);s.active=1;
 CHECK(!FamilyValid(WP_ATST_MAIN)&&!FamilyValid(WP_ATST_SIDE)&&!FamilyValid(WP_STUN_BATON));
 CHECK(Own(s,100,1001));CHECK(!Own(s,100,1002));CHECK(Own(s,101,2003));
 CHECK(Valid(s,1024));auto bytes=Encode(s);State loaded;CHECK(Decode(bytes.data(),bytes.size(),loaded,1024));
 CHECK(loaded.count==2&&loaded.active==1&&loaded.ownedCount==2&&loaded.weapons[0].colors[1][0]==5);
 CHECK(Encode(loaded)==bytes);
 CHECK(OwnedEntity(loaded,100)==OwnedId(loaded,1)&&!OwnedEntity(loaded,99));
 // Older records can be in another order. Keep their wire order and still resolve both keys.
 auto unordered=bytes;constexpr size_t ownershipStart=32+2*160;
 std::rotate(unordered.begin()+ownershipStart,unordered.begin()+ownershipStart+20,unordered.end());
 State unorderedState;CHECK(Decode(unordered.data(),unordered.size(),unorderedState,1024));
 CHECK(unorderedState.owned.front().id==2&&Encode(unorderedState)==unordered);
 CHECK(OwnedEntity(unorderedState,100)==OwnedId(unorderedState,1));
 CHECK(OwnedEntity(unorderedState,101)==OwnedId(unorderedState,2));
 Forget(unorderedState,101);CHECK(!OwnedId(unorderedState,2)&&!OwnedEntity(unorderedState,101));
 CHECK(Own(unorderedState,101,2003)&&OwnedEntity(unorderedState,101)==OwnedId(unorderedState,3));
 auto automatic=Encode(s,true);State entry;CHECK(Decode(automatic.data(),automatic.size(),entry,1024));
 CHECK(entry.ownedCount==0&&entry.count==2&&entry.nextOwned==s.nextOwned);
 // Returning to a saved hub applies the outgoing inventory to the restored
 // destination. Ownership IDs, active joints and their counters stay local.
 {
  State hub, outgoing;
  CHECK(Own(hub,301,1001)&&Own(hub,302,1002));
  jar::constraint::Joint weld;weld.kind=jar::constraint::Weld;weld.first=1;weld.second=2;
  CHECK(AddJoint(hub,weld)==1&&Add(hub,saber)==1);
  Weapon tool;tool.family=WP_TOOLGUN;tool.tool=2;tool.sandbox=1;
  CHECK(Add(outgoing,gun)==1&&Add(outgoing,tool)==2&&Own(outgoing,147,1002));
  outgoing.active=2;
  const auto inTransit=Encode(outgoing,true);
  CHECK(ImportCarriedInventory(hub,inTransit.data(),inTransit.size(),1024));
  CHECK(hub.count==2&&hub.active==2&&hub.nextWeapon==3&&hub.weapons[1].family==WP_TOOLGUN);
  CHECK(OwnedEntity(hub,301)&&OwnedEntity(hub,302)&&!OwnedEntity(hub,147));
  CHECK(hub.nextOwned==3&&hub.nextJoint==2&&hub.jointCount==1&&hub.joints[0].kind==jar::constraint::Weld);
  const auto accepted=Encode(hub);
  const auto full=Encode(outgoing);
  CHECK(!ImportCarriedInventory(hub,full.data(),full.size(),1024)&&Encode(hub)==accepted);
  CHECK(!ImportCarriedInventory(hub,inTransit.data(),inTransit.size()-1,1024)&&Encode(hub)==accepted);
  State fresh;
  CHECK(ImportCarriedInventory(fresh,inTransit.data(),inTransit.size(),1024));
  CHECK(fresh.count==hub.count&&fresh.active==hub.active&&!fresh.ownedCount&&fresh.nextOwned==1);
 }
 // New rope and thruster records round-trip, reject invalid input, and retain v4 compatibility.
 State devices;CHECK(Own(devices,100,1001));CHECK(Own(devices,101,1002));
 jar::constraint::Joint rope;rope.kind=jar::constraint::Rope;rope.first=1;rope.second=2;rope.length=123.5f;
 CHECK(AddJoint(devices,rope)==1);jar::constraint::Joint thruster;thruster.kind=jar::constraint::Thruster;thruster.first=thruster.second=1;thruster.force=5000;thruster.enabled=0;
 CHECK(AddJoint(devices,thruster)==2);auto deviceBytes=Encode(devices);State restoredDevices;
 CHECK(Decode(deviceBytes.data(),deviceBytes.size(),restoredDevices,16384));CHECK(Encode(restoredDevices)==deviceBytes);
 CHECK(restoredDevices.joints[0].length==123.5f&&restoredDevices.joints[1].force==5000&&!restoredDevices.joints[1].enabled);
 // Saved topology is canonical and transactional; legacy wire 7 has no bends.
 devices.joints[0].path.count=2;devices.joints[0].path.bend[0][0]=40;devices.joints[0].path.bend[0][1]=20;
 devices.joints[0].path.bend[1][0]=60;devices.joints[0].path.bend[1][1]=20;
 deviceBytes=Encode(devices);CHECK(deviceBytes.size()==32+2*20+2*184);
 CHECK(Decode(deviceBytes.data(),deviceBytes.size(),restoredDevices,16384));
 CHECK(!memcmp(&restoredDevices.joints[0].path,&devices.joints[0].path,sizeof(jarRopePath_t)));
 auto oldV7=deviceBytes;oldV7.erase(oldV7.begin()+72+184+84,oldV7.end());oldV7.erase(oldV7.begin()+72+84,oldV7.begin()+72+184);oldV7[0]=7;
 CHECK(oldV7.size()==32+2*20+2*84&&Decode(oldV7.data(),oldV7.size(),restoredDevices,16384));CHECK(!restoredDevices.joints[0].path.count);
 CHECK(Decode(deviceBytes.data(),deviceBytes.size(),restoredDevices,16384));const auto savedDevices=Encode(restoredDevices);
 auto badPath=deviceBytes;badPath[72+84]=9;CHECK(!Decode(badPath.data(),badPath.size(),restoredDevices,16384));CHECK(Encode(restoredDevices)==savedDevices);
 badPath=deviceBytes;badPath[72+88+24]=1;CHECK(!Decode(badPath.data(),badPath.size(),restoredDevices,16384));
 badPath=deviceBytes;badPath[72+184+84]=1;CHECK(!Decode(badPath.data(),badPath.size(),restoredDevices,16384));
 for(size_t size=0;size<deviceBytes.size();++size)CHECK(!Decode(deviceBytes.data(),size,restoredDevices,16384));
 devices.joints[0].length=std::numeric_limits<float>::quiet_NaN();CHECK(Encode(devices).empty());
 auto oldV4=bytes;oldV4[0]=4;CHECK(Decode(oldV4.data(),oldV4.size(),restoredDevices,16384));
 // Failed decode must not partly replace existing carried/world ownership data.
 const auto unchanged=Encode(loaded);
 for(size_t n=0;n<bytes.size();++n){CHECK(!Decode(bytes.data(),n,loaded,1024));CHECK(Encode(loaded)==unchanged);}
 auto bad=bytes;bad[0]=0;CHECK(!Decode(bad.data(),bad.size(),loaded,1024));
 bad=bytes;bad[4]=65;CHECK(!Decode(bad.data(),bad.size(),loaded,1024));
 bad=bytes;bad[16]=33;CHECK(!Decode(bad.data(),bad.size(),loaded,1024));
 bad=bytes;bad[32+16+128]=6;CHECK(!Decode(bad.data(),bad.size(),loaded,1024));
 State invalid=s;invalid.weapons[1].id=1;auto invalidBytes=Encode(invalid);CHECK(invalidBytes.empty());
 invalid=s;invalid.active=999;CHECK(Encode(invalid).empty());
 invalid=s;invalid.owned[1].entity=100;CHECK(Encode(invalid).empty());
 CHECK(!Decode(bytes.data(),bytes.size(),loaded,100));
 Forget(s,100);CHECK(s.ownedCount==1&&s.owned[0].entity==101);CHECK(Own(s,100,1002));
 CHECK(s.owned[1].id==3); // Slot reuse gets a new ownership identity.
 CHECK(!OwnedId(s,1)&&OwnedEntity(s,100)==OwnedId(s,3)&&OwnedEntity(s,101)==OwnedId(s,2));
 const auto copiedState=s;
 CHECK(OwnedEntity(copiedState,100)==&copiedState.owned[1]);
 State largeState;for(unsigned entity=1;entity<=ownedLimit;++entity)CHECK(Own(largeState,entity,entity%2?3001:consoleCatalog));
 CHECK(!Own(largeState,ownedLimit+1,consoleCatalog));
 auto large=Encode(largeState);CHECK(large.size()<=wireLimit && Decode(large.data(),large.size(),loaded,JAR_ENTITY_SLOTS-2+1));
 CHECK(loaded.ownedCount==ownedLimit&&loaded.owned.back().catalog==consoleCatalog);
 for(unsigned entity=1;entity<=ownedLimit;++entity)CHECK(OwnedEntity(loaded,entity)==&loaded.owned[entity-1]);
 State full;for(unsigned i=0;i<inventoryLimit;++i){Weapon w=saber;std::snprintf(w.saber[0],nameSize,"custom_%u",i);CHECK(Add(full,w)==i+1);}
 CHECK(!Add(full,gun));CHECK(Encode(full).size()<wireLimit);
 // Worst-case saved world still fits the fixed envelope with every rope bent.
 auto maximum=std::make_unique<State>(full);
 for(unsigned entity=1;entity<=ownedLimit;++entity)CHECK(Own(*maximum,entity,1001));
 for(unsigned i=0;i<jar::constraint::limit;++i){auto j=rope;j.first=i+1;j.second=i+2;j.path=devices.joints[0].path;
  j.path.count=JAR_ROPE_PATH_BENDS;for(int k=0;k<j.path.count;++k)j.path.bend[k][0]=float(k*10);CHECK(AddJoint(*maximum,j));}
 auto maximumBytes=Encode(*maximum);CHECK(maximumBytes.size()==wireLimit);
 CHECK(Decode(maximumBytes.data(),maximumBytes.size(),loaded,16384)&&loaded.jointCount==128&&loaded.ownedCount==ownedLimit);
 CHECK(Encode(loaded)==maximumBytes);
 for(auto& object:maximum->owned)object.physics={maxPropMass,true};
 auto custom=Encode(*maximum);CHECK(custom[0]==9&&custom.size()==maximumBytes.size());
 CHECK(Decode(custom.data(),custom.size(),loaded,16384)&&loaded.owned.back().physics.mass==maxPropMass&&loaded.owned.back().physics.noGravity);
 maximum->owned[0].physics.mass=maxPropMass+1;CHECK(Encode(*maximum).empty());
 CHECK(Decode(maximumBytes.data(),maximumBytes.size(),loaded,16384)&&!loaded.owned[0].physics.mass&&!loaded.owned[0].physics.noGravity);
 auto f=Encode(full);CHECK(Decode(f.data(),f.size(),loaded,1024));CHECK(loaded.count==inventoryLimit);
 CHECK(jar::ReadableSaveVersion(jar::workshopSaveV1,1));CHECK(jar::ReadableSaveVersion(jar::actorRecoverySaveV1,1));
 for(const auto&e:jar::catalog::entries){State expanded;CHECK(CatalogValid(e.id));CHECK(Own(expanded,123,e.id));auto bytes=Encode(expanded);State restored;CHECK(Decode(bytes.data(),bytes.size(),restored,16383));CHECK(restored.owned[0].catalog==e.id);}
 CHECK(jar::ReadableSaveVersion(jar::workshopSaveV7,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV8,1));
 CHECK(jar::WriteSaveVersion(1,false,false)==1);CHECK(jar::WriteSaveVersion(1,true,false)==jar::actorRecoverySaveV1);
 CHECK(jar::WriteSaveVersion(1,false,true)==jar::workshopSaveCurrent);CHECK(jar::WriteSaveVersion(1,true,true)==jar::workshopSaveCurrent);
 CHECK(jar::ReadableSaveVersion(jar::workshopSaveV9,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV10,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV11,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV12,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV13,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV14,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV15,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV16,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV17,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV18,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV19,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV20,1));
 CHECK(jar::ReadableSaveVersion(jar::workshopSaveV31,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV32,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV33,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV34,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV35,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV36,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV37,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV38,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV39,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV40,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV41,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV42,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV43,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV44,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV45,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV46,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV47,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV48,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV49,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV50,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV51,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV52,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV53,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV54,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV55,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV56,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV57,1));CHECK(jar::ReadableSaveVersion(jar::workshopSaveV58,1));CHECK(!jar::ReadableSaveVersion(jar::workshopSaveV58+1,1));
 // Tools must not alias normal weapons or accept arbitrary families.
 State tools;Weapon physics;physics.family=WP_PHYSGUN;physics.tool=1;physics.sandbox=1;
 CHECK(Add(tools,physics)==1);physics.tool=0;CHECK(!Add(tools,physics));physics.family=WP_CONCUSSION;CHECK(Add(tools,physics)==2);
 physics.tool=3;CHECK(!Add(tools,physics));physics.tool=1;physics.family=WP_SABER;CHECK(!Add(tools,physics));
 auto toolBytes=Encode(tools);CHECK(Decode(toolBytes.data(),toolBytes.size(),loaded,1024));CHECK(loaded.weapons[0].tool==1 && loaded.weapons[0].family==WP_PHYSGUN);
 auto oldTools=toolBytes;oldTools.erase(oldTools.begin()+24,oldTools.begin()+32);oldTools[0]=2;oldTools[28]=WP_CONCUSSION;
 CHECK(Decode(oldTools.data(),oldTools.size(),loaded,1024));
 CHECK(loaded.weapons[0].tool==1 && loaded.weapons[0].family==WP_PHYSGUN);
 static_assert(WP_PHYSGUN==29 && WP_TOOLGUN==30,"Tool IDs cannot renumber native weapons");
 // Independent v1 fixture: one E-11, no owned entities, 156-byte record.
 std::vector<uint8_t> legacy(180);auto put=[&](size_t at,uint32_t v){for(int i=0;i<4;++i)legacy[at+i]=uint8_t(v>>(8*i));};
 put(0,0x4a575301);put(4,1);put(8,2);put(12,1);put(20,1);put(24,1);put(28,WP_BLASTER);
 CHECK(Decode(legacy.data(),legacy.size(),loaded,1024));CHECK(loaded.count==1&&!loaded.weapons[0].tool);
 CHECK(jar::ReadableSaveVersion(jar::workshopSaveV2,1));
 State expanded;CHECK(Own(expanded,110,1020));CHECK(Own(expanded,111,3012));auto expandedBytes=Encode(expanded);CHECK(Decode(expandedBytes.data(),expandedBytes.size(),loaded,1024));
 State actors;CHECK(Own(actors,100,3001));CHECK(Own(actors,101,2003,1));CHECK(Own(actors,102,4001,1));
 auto actorBytes=Encode(actors);CHECK(Decode(actorBytes.data(),actorBytes.size(),loaded,1024));
 CHECK(loaded.owned[1].parent==1&&(*OwnedChildren(loaded,1)==std::vector<uint32_t>{2,3}));
 // A save may place the children before their parent, in any wire order.
 std::swap_ranges(actorBytes.begin()+32,actorBytes.begin()+52,actorBytes.begin()+72);
 CHECK(Decode(actorBytes.data(),actorBytes.size(),loaded,1024)&&Encode(loaded)==actorBytes);
 CHECK((*OwnedChildren(loaded,1)==std::vector<uint32_t>{3,2}));
 CHECK(SetOwnershipParent(loaded,*OwnedEntity(loaded,102),0));
 CHECK((*OwnedChildren(loaded,1)==std::vector<uint32_t>{2}));
 CHECK(SetOwnershipParent(loaded,*OwnedEntity(loaded,102),1));
 CHECK((*OwnedChildren(loaded,1)==std::vector<uint32_t>{3,2})&&Encode(loaded)==actorBytes);
 Forget(loaded,102);CHECK(Own(loaded,102,4001));
 CHECK((*OwnedChildren(loaded,1)==std::vector<uint32_t>{2})&&OwnedEntity(loaded,102)->id==4);
 Forget(actors,100);CHECK(Valid(actors,1024));
 CHECK((*OwnedChildren(actors,1)==std::vector<uint32_t>{2,3})); // Legacy orphan records stay valid.
 actors.owned[0].parent=actors.owned[0].id;CHECK(!Valid(actors,1024));
 CHECK(Own(loaded,200,2501));CHECK(Own(loaded,201,2504));CHECK(Valid(loaded,1024));
 CHECK(!CatalogValid(2505));
 for(unsigned id=2101;id<=2103;++id){State melee;CHECK(Own(melee,42,id));auto bytes=Encode(melee);CHECK(Decode(bytes.data(),bytes.size(),loaded,1024)&&loaded.owned[0].catalog==id);}
 CHECK(!CatalogValid(3013)&&!CatalogValid(4003)&&!CatalogValid(2014));
 });}
