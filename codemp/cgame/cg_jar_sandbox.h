#pragma once
void CG_JAR_MenuDown(void);
void CG_JAR_ContextDown(void);
void CG_JAR_MenuUp(void);
void CG_JAR_Undo(void);
void CG_JAR_DistanceIn(void);
void CG_JAR_DistanceOut(void);
void CG_JAR_Draw(void);
qboolean CG_JAR_Key(int key,qboolean down);
qboolean CG_JAR_Mouse(int x,int y);
qboolean CG_JAR_Entity(centity_t *cent);
void CG_JAR_Init(void);
void CG_JAR_Shutdown(void);
void CG_JAR_ClearReflectionProbes(void);
void CG_JAR_ReflectionProbes(void);

void CG_JAR_Settings(void);
void CG_JAR_Equip(void);

void CG_JAR_Selection(void);
void CG_JAR_Controller(void);

void CG_JAR_PropFrame(void);

void CG_JAR_Tool(void);
void CG_JAR_Status(void);
void CG_JAR_Stats(void);

void CG_JAR_UndoFeedback(void);
