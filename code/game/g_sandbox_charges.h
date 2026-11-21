// Native det packs and trip mines mounted on Sandbox props. GPL-2.0-only.
#pragma once

#include <vector>
#include <utility>
#include "qcommon/jar_charge_copy.h"

namespace ojk { class SavedGameHelper; }
namespace jar { namespace physics { struct MountedHull; } }

struct SandboxChargeMove {
 gentity_t* charge;
 const gentity_t* support;
 vec3_t origin,angles,beamStart;
};

struct SandboxChargesUnlinked {
 std::vector<gentity_t*> linked;
 explicit SandboxChargesUnlinked(const std::vector<SandboxChargeMove>& moves){
  for(const auto& move:moves)if(move.charge->linked){linked.push_back(move.charge);gi.unlinkentity(move.charge);}
 }
 ~SandboxChargesUnlinked(){for(auto* charge:linked)gi.linkentity(charge);}
};

// Only active during native prop movement. Stock shot contents remain intact.
constexpr unsigned SANDBOX_MOUNTED_CHARGE_CONTENTS=CONTENTS_UNUSED00200000;
class SandboxMountedChargeCollision {
 std::vector<std::pair<gentity_t*,uint64_t>> marked;
public:
 SandboxMountedChargeCollision();
 ~SandboxMountedChargeCollision();
 SandboxMountedChargeCollision(const SandboxMountedChargeCollision&)=delete;
 SandboxMountedChargeCollision& operator=(const SandboxMountedChargeCollision&)=delete;
};

void G_SandboxChargeAttach(gentity_t& charge,const gentity_t& prop);
const gentity_t* G_SandboxChargeSupport(const gentity_t& charge);
bool G_SandboxChargeProperties(const gentity_t& charge,jar::chargeCopy::Properties& properties);
bool G_SandboxChargeSetProperties(gentity_t& charge,const jar::chargeCopy::Properties& properties);
bool G_SandboxChargesCapture(const gentity_t& prop,std::vector<jar::chargeCopy::State>& charges);
bool G_SandboxChargeCaptureStandalone(const gentity_t& charge,jar::chargeCopy::State& state);
bool G_SandboxChargeBindCopy(gentity_t& charge,const gentity_t& prop,const jar::chargeCopy::State& state);
void G_SandboxChargeRefreshBeam(gentity_t& charge);
void G_SandboxChargeTraceBeam(gentity_t& charge,trace_t& hit);
void G_SandboxChargesFrame();
void G_SandboxChargesForget(gentity_t& entity);
void G_SandboxChargesReset();
void G_SandboxChargesHulls(const gentity_t& prop,std::vector<jar::physics::MountedHull>& hulls);
bool G_SandboxChargesPlan(const gentity_t& prop,const vec3_t origin,std::vector<SandboxChargeMove>& moves);
bool G_SandboxChargesPlanPose(const gentity_t& prop,const vec3_t origin,const vec3_t angles,
 std::vector<SandboxChargeMove>& moves,size_t limit);
float G_SandboxChargesFloorOffset(const std::vector<SandboxChargeMove>& moves,float floor);
void G_SandboxChargesRaise(std::vector<SandboxChargeMove>& moves,float height);
bool G_SandboxChargesClear(const std::vector<SandboxChargeMove>& moves,int& traceBudget);
bool G_SandboxChargesPoseClear(gentity_t& prop,const std::vector<SandboxChargeMove>& moves,
 const float* arc=nullptr,const float* pivot=nullptr);
bool G_SandboxPropTrace(gentity_t& prop,const vec3_t target,trace_t& hit,
 std::vector<SandboxChargeMove>& moves,bool& chargeContact,const struct JarSweepHull* hull=nullptr);
bool G_SandboxChargeFits(const SandboxChargeMove& move,const float* start=nullptr,const float* padding=nullptr);
void G_SandboxChargesPlace(const std::vector<SandboxChargeMove>& moves);
void G_SandboxChargesRefresh(const std::vector<SandboxChargeMove>& moves);
void G_SandboxChargesCommit(const std::vector<SandboxChargeMove>& moves);
void G_SandboxChargesExport(bool autosave,ojk::SavedGameHelper& save);
bool G_SandboxChargesImport(bool autosave,ojk::SavedGameHelper& save);
