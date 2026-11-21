// Native multiplayer UI bridge for shared BSP previews.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { JAR_HOST_MAP_MISSING, JAR_HOST_MAP_INVALID, JAR_HOST_MAP_NO_START, JAR_HOST_MAP_PLAYABLE } jarHostMapStatus_t;
qboolean UI_JAR_HostMapNameValid(const char* name);
jarHostMapStatus_t UI_JAR_InspectHostMap(const uiImport_t* fs, const char* name, char* title, int titleSize, qhandle_t* preview);
#ifdef __cplusplus
}
#endif
