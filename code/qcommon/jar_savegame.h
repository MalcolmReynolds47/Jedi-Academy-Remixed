// JA:R recovery save contract. GPL-2.0-only.
#ifndef JAR_SAVEGAME_H
#define JAR_SAVEGAME_H
namespace jar {
// Native 0.7 readers reject this distinct header before loading any state.
constexpr int actorRecoverySaveV1=0x4a415201;
constexpr int workshopSaveV1=0x4a415202;
constexpr int workshopSaveV2=0x4a415203;
constexpr int workshopSaveV3=0x4a415204;
// Rigid props store motion in native stationary trajectories; older readers must reject them.
constexpr int workshopSaveV4=0x4a415205;
// 0.18 adds bounded local-frame weld/hinge records; older engines must reject early.
constexpr int workshopSaveV5=0x4a415206;
// 0.20 adds rope/thruster records and suspended native controller restoration.
constexpr int workshopSaveV6=0x4a415207;
// 0.21 adds stable retail catalog identifiers; older readers reject these saves.
constexpr int workshopSaveV7=0x4a415208;
// 0.22 portable movers and owned target networks require these runtime adapters.
constexpr int workshopSaveV8=0x4a415209;
// 0.26 adds native Rope/Thruster endpoints, including players and world anchors.
constexpr int workshopSaveV9=0x4a41520a;
// 0.27 posed Ghoul2 rigid props require the new bounds adapter and catalog.
constexpr int workshopSaveV10=0x4a41520b;
// 0.28 portable maglocks require owned-door destruction and cleanup support.
constexpr int workshopSaveV11=0x4a41520c;
// 0.29 space volumes add serialized touch/use callbacks.
constexpr int workshopSaveV12=0x4a41520d;
// 0.30 portable space volumes retain local dimensions and orientation.
constexpr int workshopSaveV13=0x4a41520e;
// 0.31 bidirectional volumes add native touch/use callbacks and catalog support.
constexpr int workshopSaveV14=0x4a41520f;
// 0.32 persistent interest/location markers rebuild native registrations on load.
constexpr int workshopSaveV15=0x4a415210;
// 0.33 native Weld/Hinge endpoints in workshop wire 7.
constexpr int workshopSaveV16=0x4a415211;
// 0.34 portable music controller ownership/restoration chunk.
constexpr int workshopSaveV17=0x4a415212;
// 0.35 portable parameter controller catalog support; native PARM storage unchanged.
constexpr int workshopSaveV18=0x4a415213;
// 0.36 portable ICARUS runner catalog and lifecycle support.
constexpr int workshopSaveV19=0x4a415214;
// 0.37 ship-boundary callbacks and steering ownership chunk.
constexpr int workshopSaveV20=0x4a415215;
// Cloud use callback; native fields retain all cloud settings.
constexpr int workshopSaveV21=0x4a415216;
// Appends the portable weather controller use callback.
constexpr int workshopSaveV22=0x4a415217;
// Adds saved portable wind/exposure volume geometry and weather kinds.
constexpr int workshopSaveV23=0x4a415218;
// Portable laser-arm assembly, settings and callback branch.
constexpr int workshopSaveV24=0x4a415219;
// Appends portable portal use callback and native surface/camera state.
constexpr int workshopSaveV25=0x4a41521a;
// Portable searchlight settings, secondary output and saved detection state.
constexpr int workshopSaveV26=0x4a41521b;
constexpr int workshopSaveV27=0x4a41521c;
constexpr int workshopSaveV28=0x4a41521d;
constexpr int workshopSaveV29=0x4a41521e;
constexpr int workshopSaveV30=0x4a41521f;
constexpr int workshopSaveV31=0x4a415220;
// Persistent combat-point indices require controller-aware restore.
constexpr int workshopSaveV32=0x4a415221;
// Portable sky ownership stack and appended use callback.
constexpr int workshopSaveV33=0x4a415222;
// Native color variants and expanded vehicle definition registry.
constexpr int workshopSaveV34=0x4a415223;
// Keep header selection and the game-module writer on the same contract.
constexpr int workshopSaveV35=0x4a415224; // Portable shield callbacks and inventory slot.
constexpr int workshopSaveV36=0x4a415225; // Reusable datapad inventory and catalog route.
constexpr int workshopSaveV37=0x4a415226; // Portable rail callbacks and saved motion.
constexpr int workshopSaveV38=0x4a415227; // Portable reflection-probe callback and catalog.
constexpr int workshopSaveV39=0x4a415228; // Native map-section ownership chunk.
constexpr int workshopSaveV40=0x4a415229; // Opt-in map-section name isolation.
constexpr int workshopSaveV41=0x4a41522a; // BSP source identities and restored inline-model remapping.
constexpr int workshopSaveV42=0x4a41522b; // World-axis angular trajectories and rigid secondary-model offsets.
constexpr int workshopSaveV43=0x4a41522c; // Portable stock BSP section controller and local destinations.
constexpr int workshopSaveV44=0x4a41522d; // Persistent rope scenery paths in workshop wire 8.
constexpr int workshopSaveV45=0x4a41522e; // Per-actor portable Hurt cooldowns.
constexpr int workshopSaveV46=0x4a41522f; // Portable push settings and contact cooldowns.
constexpr int workshopSaveV47=0x4a415230; // Portable teleporter settings and arrival guards.
constexpr int workshopSaveV48=0x4a415231; // Prop and projectile teleporter settings and arrival guards.
constexpr int workshopSaveV49=0x4a415232; // Vehicle teleporter settings.
constexpr int workshopSaveV50=0x4a415233; // Vehicle push settings and launch drift.
constexpr int workshopSaveV51=0x4a415234; // Linked prop teleporter setting.
constexpr int workshopSaveV52=0x4a415235; // Rolling and loose explosive teleporter arrivals.
constexpr int workshopSaveV53=0x4a415236; // Prop-mounted det pack and trip mine records.
constexpr int workshopSaveV54=0x4a415237; // Per-prop physics settings in workshop wire 9.
constexpr int workshopSaveV55=0x4a415238; // Persistent creation Undo order and assembly groups.
constexpr int workshopSaveV56=0x4a415239; // Native NPC navigation links.
constexpr int workshopSaveV57=0x4a41523a; // NPC patrol progress.
constexpr int workshopSaveV58=0x4a41523b; // Sandbox Explosion Trail angles are Euler poses.
constexpr int workshopSaveCurrent=workshopSaveV58;
inline bool IsWorkshopSaveVersion(int version){return version>=workshopSaveV1&&version<=workshopSaveCurrent;}
inline bool ReadableSaveVersion(int version,int nativeVersion){return version==nativeVersion||version==actorRecoverySaveV1||IsWorkshopSaveVersion(version);}
inline int WriteSaveVersion(int nativeVersion,bool recoveryState,bool workshopState=false){return workshopState?workshopSaveCurrent:recoveryState?actorRecoverySaveV1:nativeVersion;}
}
#endif
