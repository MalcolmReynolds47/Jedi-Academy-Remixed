#include "../../codemp/qcommon/msg.cpp"
#include "../../codemp/qcommon/q_shared.c"
#include "../../codemp/game/anims.h"
#include "../../codemp/game/jar_mp_prop.h"
#include <cassert>
#include <limits>
#include <stdexcept>
#include <cstdarg>

cvar_t quiet{};
cvar_t *cl_shownet=&quiet;
server_t sv{};
serverStatic_t svs{};
void QDECL Com_Error(int,const char*,...){throw std::runtime_error("Invalid MP snapshot");}
void QDECL Com_Printf(const char*,...){}
void QDECL Com_DPrintf(const char*,...){}
sharedEntity_t*SV_GentityNum(int){return nullptr;}
long FS_FOpenFileRead(const char*,fileHandle_t*f,qboolean){*f=0;return -1;}
void FS_FCloseFile(fileHandle_t){}
int FS_Read(void*,int,fileHandle_t){return 0;}
void*Z_Malloc(int size,memtag_t,qboolean zero,int){return zero?calloc(1,size):malloc(size);}

