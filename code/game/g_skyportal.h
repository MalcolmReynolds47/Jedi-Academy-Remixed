// Portable sky lifecycle and save contract. GPL-2.0-only.
#ifndef G_JAR_SKYPORTAL_H
#define G_JAR_SKYPORTAL_H
#include "qcommon/jar_skyportal.h"
namespace ojk {class SavedGameHelper;}
struct gentity_s;typedef struct gentity_s gentity_t;
bool G_SkyPortalPortable(const gentity_t&);
bool G_SkyPortalRead(const gentity_t&,jar::skyPortal::Settings&);
bool G_SkyPortalSet(gentity_t&,const jar::skyPortal::Settings&,int order=-1);
int G_SkyPortalOrder(const gentity_t&);
bool G_SkyPortalSpawn(gentity_t&);
bool G_SkyPortalMoved(gentity_t&);
void G_SkyPortalFrame();
void G_SkyPortalForget(gentity_t*);
void G_SkyPortalReset();
void G_SkyPortalReleaseAll();
void G_SetSkyPortal(const char*);
void G_SkyPortalExport(bool,ojk::SavedGameHelper&);
bool G_SkyPortalImport(bool,ojk::SavedGameHelper&);
bool G_SkyPortalRestore();
#endif
