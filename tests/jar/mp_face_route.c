#include "cgame/cg_local.h"
#include <assert.h>
#include <stdio.h>
cg_t cg;static cgameImport_t imports;cgameImport_t*trap=&imports;
vmCvar_t jar_facePreset,jar_faceSensitivity;
static qboolean eligible;static int calls;
static void Face(void*instance,int entity,int time,int preset,float sensitivity,qboolean active){assert(instance==(void*)1&&entity==23&&time==1000&&preset==3&&sensitivity==1.5f);eligible=active;++calls;}
#include "../../codemp/cgame/cg_jar_face.inc"
int main(void){
 imports.JAR_ApplyFace=Face;cg.time=1000;jar_facePreset.integer=3;jar_faceSensitivity.value=1.5f;
 centity_t cent={0};cent.ghoul2=(void*)1;cent.currentState.number=23;
 CG_JAR_ApplyFace(&cent);assert(eligible);cent.localAnimIndex=1;CG_JAR_ApplyFace(&cent);assert(eligible);
 cent.localAnimIndex=2;CG_JAR_ApplyFace(&cent);assert(!eligible);cent.localAnimIndex=0;
 cent.noFace=qtrue;CG_JAR_ApplyFace(&cent);assert(!eligible);cent.noFace=qfalse;
 cent.isRagging=qtrue;CG_JAR_ApplyFace(&cent);assert(!eligible);cent.isRagging=qfalse;
 cent.currentState.eFlags=EF_DEAD;CG_JAR_ApplyFace(&cent);assert(!eligible);cent.currentState.eFlags=EF_RAG;CG_JAR_ApplyFace(&cent);assert(!eligible);
 assert(calls==7);puts("MP face route: live preset/sensitivity forwarding, both humanoid sets, missing face, nonhumanoid, death and ragdoll eligibility passed.");
}
