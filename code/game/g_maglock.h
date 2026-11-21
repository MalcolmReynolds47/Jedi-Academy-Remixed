// Native magnetic-lock ownership and lifetime helpers. GPL-2.0-only.
#ifndef JAR_MAGLOCK_H
#define JAR_MAGLOCK_H
struct gentity_s;
bool G_MaglockAttach(gentity_s*,gentity_s*);
void G_MaglockDetach(gentity_s*);
void G_MaglockForget(gentity_s*);
#endif
