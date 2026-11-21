// SP compatibility names for the shared JA:R local-view presets. GPL-2.0-only.
#ifndef JAR_CAMERA_H
#define JAR_CAMERA_H
#include "qcommon/jar_camera_presets.h"
namespace jar {
inline int CameraPreset(int selected,bool protectedView){return JAR_CameraPreset(selected,protectedView);}
inline float CameraDamping(int preset,float legacy){return JAR_CameraDamping(preset,legacy);}
inline float CameraMotion(int preset){return JAR_CameraMotion(preset);}
}
#endif
