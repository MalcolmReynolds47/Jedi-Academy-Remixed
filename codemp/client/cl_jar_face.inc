// MP cgame and engine keep skeletal instances opaque to the renderer.
#include "qcommon/jar_face.h"
static int jarMPFaceEpoch=1;
static void CL_JAR_ResetFaces(){++jarMPFaceEpoch;}
static void CL_JAR_ApplyFace(void*instance,int entity,int time,int preset,float sensitivity,qboolean eligible){
 if(!instance||entity<0||entity>=MAX_GENTITIES)return;
 re->G2API_JAR_ApplyFace(*static_cast<CGhoul2Info_v*>(instance),entity,time,preset,S_JAR_VoiceEnergy(entity),sensitivity,eligible,jarMPFaceEpoch);
}
