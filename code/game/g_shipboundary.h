// Native fighter boundary ownership, control and persistence. GPL-2.0-only.
#ifndef G_SHIPBOUNDARY_H
#define G_SHIPBOUNDARY_H
struct gentity_s;struct Vehicle_t;struct usercmd_s;
namespace ojk {class SavedGameHelper;}
bool G_ShipBoundaryClass(const gentity_s&);
bool G_ShipBoundarySet(gentity_s&,float seconds,bool preserveEmpty);
bool G_ShipBoundaryHasVolumes();
void G_ShipBoundaryFrame();
void G_ShipBoundaryGuide(Vehicle_t*);
void G_ShipBoundaryPilot(gentity_s*,usercmd_s*);
void G_ShipBoundaryForget(gentity_s*);
void G_ShipBoundaryReset();
void G_ShipBoundaryReleaseAll();
void G_ShipBoundaryRestore();
void G_ShipBoundaryExport(bool,ojk::SavedGameHelper&);
bool G_ShipBoundaryImport(bool,ojk::SavedGameHelper&);
#endif
