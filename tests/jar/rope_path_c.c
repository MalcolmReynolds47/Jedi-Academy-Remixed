#include "../../shared/qcommon/jar_rope_path.h"
#include "../../shared/qcommon/jar_rope_path_mesh.h"
jarRopePathResult_t JAR_TestCRopePath(jarRopePath_t* path,const float* a,const float* b,
        float length,jarRopePathTrace_t trace,void* data,int budget) {
    return JAR_RopePathSolve(path,a,b,length,trace,data,budget);
}
