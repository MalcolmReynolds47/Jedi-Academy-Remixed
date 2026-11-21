// Native controller services shared with the entity editor. GPL-2.0-only.
#ifndef G_TARGET_SCRIPTRUNNER_H
#define G_TARGET_SCRIPTRUNNER_H
#include "qcommon/jar_scriptrunner.h"
struct gentity_s;
bool G_ScriptRunnerRead(const gentity_s&,jar::scriptRunner::Settings&);
bool G_ScriptRunnerSet(gentity_s&,const jar::scriptRunner::Settings&);
bool G_ScriptRunnerAvailable(const char*);
void G_ScriptRunnerForget(gentity_s*);
#endif
