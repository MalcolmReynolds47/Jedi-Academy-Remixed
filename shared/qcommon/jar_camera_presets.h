// Local-view presets shared by SP and MP; no engine or UI dependencies.
// GPL-2.0-only.
#ifndef JAR_CAMERA_PRESETS_H
#define JAR_CAMERA_PRESETS_H
static int JAR_CameraPreset(int selected,int protectedView){return !protectedView&&selected>=1&&selected<=2?selected:0;}
static float JAR_CameraDamping(int preset,float legacy){return preset==2?1.0f:preset==1?0.7f:legacy;}
static float JAR_CameraMotion(int preset){return preset==2?0.0f:preset==1?0.25f:1.0f;}
#endif
