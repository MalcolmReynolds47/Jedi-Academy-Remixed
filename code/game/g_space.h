// Native space-volume lifecycle. GPL-2.0-only.
#ifndef G_SPACE_H
#define G_SPACE_H
struct gentity_s;
bool G_SpaceActive(gentity_s*);
void G_SpaceGravity(gentity_s*,float);
void G_SpaceEffects(gentity_s*);
void G_SpaceForget(gentity_s*);
bool G_SpacePortable(const gentity_s&);
void G_SpaceSet(gentity_s&,bool armed,bool enabled);
void G_SpaceRestore(bool legacyShape=false);
bool G_SpaceHasVolumes();
#endif
