// Include after g_local.h. Native tag ownership is kept separate. GPL-2.0-only.
#ifndef G_REFERENCE_TAGS_H
#define G_REFERENCE_TAGS_H
#include "qcommon/jar_reference.h"
bool G_ReferencePortable(const gentity_t&);
bool G_ReferenceRead(const gentity_t&,jar::reference::Settings&);
bool G_ReferenceSet(gentity_t&,const jar::reference::Settings&);
bool G_ReferenceSpawn(gentity_t&);
bool G_ReferenceCopy(gentity_t&,const jar::reference::Settings&);
void G_ReferenceReset();
void G_ReferenceForget(gentity_t*);
bool G_ReferenceRestore();
reference_tag_t* G_ReferenceFind(const char*,const char*);
#endif
