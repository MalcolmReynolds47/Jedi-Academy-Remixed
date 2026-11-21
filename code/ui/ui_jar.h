// JA:R graphics and performance menu. GPL-2.0-only.
#ifndef UI_JAR_H
#define UI_JAR_H
void UI_JAR_LoadMenus();
void UI_JAR_Open(const char *page = nullptr);
void UI_JAR_WorkshopOpen(const char *page);
bool UI_JAR_OverlayActive();
qboolean UI_JAR_OverlayKey(int key,qboolean down);
bool UI_JAR_OverlayTyping();
bool UI_JAR_MapPickerActive();
bool UI_JAR_MapPickerKey(int key,qboolean down);
bool UI_JAR_TextEntryActive();
void UI_JAR_EndTextEntry();
void UI_JAR_OverlayHeld(bool down);
void UI_JAR_ContextHeld(bool down);
void UI_JAR_OverlayClose();
void UI_JAR_Update(int realtime);
bool UI_JAR_DisplayTrialActive();
qboolean UI_JAR_Script(const char *name, const char **args);
#endif
