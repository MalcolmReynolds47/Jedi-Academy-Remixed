// Renderer-owned skeletal instances use the same controller as SP.
#include "qcommon/jar_face_controller.h"
static jar::FaceState jarMPFaceStates[MAX_GENTITIES];
void G2API_JAR_ApplyFace(CGhoul2Info_v&g2,int entity,int time,int preset,float energy,float sensitivity,qboolean eligible,int epoch){
 if(entity<0||entity>=MAX_GENTITIES)return;
 if(!g2.IsValid()||!g2.size())return;
 auto&model=g2[0];
 jar::ApplyFace(model,jarMPFaceStates[entity],entity,time,g2.mItem,epoch,preset,eligible!=qfalse,energy,sensitivity,
  [&](const char*name){
   int index=G2API_GetBoneIndex(&model,name);if(index>=0)return index;
   // Native MP lookup does not create list entries. An identity with zero
   // flags adds an unowned entry only when no existing override was found.
   mdxaBone_t identity={};for(int n=0;n<3;++n)identity.matrix[n][n]=1;
   if(!G2API_SetBoneAnglesMatrix(&model,name,identity,0,nullptr,0,time))return -1;
   return G2API_GetBoneIndex(&model,name);
  },
  [&](int index,const mdxaBone_t&matrix,int flags,int clock){G2API_SetBoneAnglesMatrixIndex(&model,index,matrix,flags,nullptr,0,clock);},
  [&](int index){G2API_StopBoneAnglesIndex(&model,index);});
}
