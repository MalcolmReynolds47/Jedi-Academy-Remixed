// Native spawn flags hold the state of a Sandbox Secret marker. GPL-2.0-only.
#pragma once
#include "g_functions.h"
#include "g_workshop.h"

constexpr int JAR_SECRET_REPEAT=1;
constexpr int JAR_SECRET_FOUND=2;
constexpr int JAR_SECRET_FLAGS=JAR_SECRET_REPEAT|JAR_SECRET_FOUND;

inline bool G_SandboxSecret(const gentity_t& entity){
 return entity.e_UseFunc==useF_target_secret_use&&entity.classname&&
  !strcmp(entity.classname,"target_secret")&&G_JAR_WorkshopOwns(&entity);
}
