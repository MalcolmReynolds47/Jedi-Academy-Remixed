// Portable portal dimensions and stock resources shared by SP and MP.
#pragma once
enum {
 JAR_PORTAL_MIN_SIZE=16, JAR_PORTAL_MAX_SIZE=1024,
 JAR_PORTAL_WIDTH=128, JAR_PORTAL_HEIGHT=192, JAR_PORTAL_SURFACE_LIMIT=128,
 JAR_PORTAL_CAMERA_SIZE=256, JAR_PORTAL_CAMERA_FEEDS=4
};
#define JAR_PORTAL_CAMERA_MODEL "models/map_objects/kejim/impcam.md3"
#define JAR_PORTAL_PANEL_SHADER "gfx/jar/portal_panel"
#define JAR_PORTAL_CAMERA_SHADER "gfx/jar/portal_monitor"
typedef struct jarPortablePortalView {
#ifdef __cplusplus
 float origin[3]{},axis[3][3]{},cameraOrigin[3]{},cameraAxis[3][3]{};
 float width=JAR_PORTAL_WIDTH,height=JAR_PORTAL_HEIGHT;
 int mirror=0;
#else
 float origin[3],axis[3][3],cameraOrigin[3],cameraAxis[3][3];
 float width,height;
 int mirror;
#endif
} jarPortablePortalView_t;
