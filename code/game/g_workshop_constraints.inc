#include "g_door_camera.h"
#include "g_laser_arm.h"
#include "../../shared/qcommon/jar_tool_style.h"
// Toolgun constraints across owned/native/world endpoints.
// GPL-2.0-only. Joint frames remain immutable across freeze, release and saves.
bool removeJoint(unsigned id){
 if(!ForgetJoint(state,id))return false;physicsWorld.ForgetJoint(id);ropeStates.erase(id);
 forgetJointEdit(id);
 if(gi.JAR_Log)gi.JAR_Log(1,"sandbox","constraint_remove","id=%u remaining=%u",id,state.jointCount);return true;
}
void constraintReport(){
 if(!gi.JAR_Log)return;
 gi.JAR_Log(0,"sandbox","constraints","count=%u limit=%u pending=%u",state.jointCount,jar::constraint::limit,jointFirst);
 for(unsigned i=0;i<state.jointCount;++i){const auto&j=state.joints[i];
  gi.JAR_Log(0,"sandbox","constraint","id=%u kind=%s first=%u second=%u a=%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f b=%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",j.id,j.kind==jar::constraint::Weld?"weld":j.kind==jar::constraint::Hinge?"hinge":j.kind==jar::constraint::Rope?"rope":"thruster",j.first,j.second,j.a.position[0],j.a.position[1],j.a.position[2],j.a.rotation[0],j.a.rotation[1],j.a.rotation[2],j.a.rotation[3],j.b.position[0],j.b.position[1],j.b.position[2],j.b.rotation[0],j.b.rotation[1],j.b.rotation[2],j.b.rotation[3]);
 }
}
void constraintLink(gentity_t&e,const vec3_t anchor,const vec3_t axis,unsigned kind){
 const auto key=attachmentKey(e);jar::constraint::Frame frame;
 if(!attachmentFrameAt(e,anchor,axis,frame)){message("Anchor is outside supported bounds.");return;}
 if(jointFirst&&jointPendingMode!=int(kind))jointFirst=0;
 if(!jointFirst){jointFirst=key;jointPendingMode=int(kind);attachmentPending=frame;message(kind==jar::constraint::Hinge?"Hinge pivot set. Its axis is perpendicular to this face. Select a second entity or world surface; right click cancels.":"First weld anchor set. Select a second entity or world surface; right click cancels.");return;}
 const auto first=jointFirst;auto*a=attachmentEntity(first);jointFirst=0;
 if(!a||key==first){message("Choose two different entities, or an entity and the world.");return;}
 vec3_t point,direction;if(!attachmentPoint(*a,attachmentPending,point,direction))return;
 if(DistanceSquared(point,anchor)>8192.f*8192.f){message("Anchors must be within 8192 units.");return;}
 // A fixed endpoint belongs on the clicked surface, even if the world was
 // clicked second. Keep the hinge axis from the first click.
 if(e.s.number==ENTITYNUM_WORLD)VectorCopy(anchor,point);
 jar::constraint::Joint record;record.kind=kind;record.first=first;record.second=key;
 // Both local frames start at the same pivot, keeping the current body poses.
 if(!attachmentFrameAt(*a,point,direction,record.a)||!attachmentFrameAt(e,point,direction,record.b)){message("Joint frame exceeds supported local coordinates.");return;}
 const auto id=AddJoint(state,record);if(!id){message("This pair is already linked, or the 128-link capacity is reached.");return;}
 attachmentRemember(id);attachmentClassifyJoints();
 message(kind==jar::constraint::Hinge?"Hinge created. Release to move around the first face's axis. Frozen entities, path movers and world surfaces anchor it; Undo removes the link.":"Weld created. Release members to move together; frozen entities, path movers and world surfaces anchor the assembly. Undo removes the link.");
}
int toolEntityRoot(int slot){
 if(slot<0||slot>=ENTITYNUM_WORLD)return slot;
 auto& entity=g_entities[slot];
 if(const auto* root=jar::bspSection::SectionLiftRoot(entity))return root->s.number;
 if(const auto* root=G_BSPSectionBreakableRoot(entity))return root->s.number;
 if(const auto* root=G_CrystalCrateRoot(&entity))return root->s.number;
 if(const auto* root=G_DoorCameraRoot(&entity))return root->s.number;
 if(const auto* root=G_LaserArmRoot(&entity))return root->s.number;
 return slot;
}
trace_t workshopTargetTrace(const vec3_t start,const vec3_t direction,EG2_Collision type){
 vec3_t end;
 VectorMA(start,JAR_TOOL_AIM_RANGE,direction,end);
 trace_t trace{};
 gi.trace(&trace,start,nullptr,nullptr,end,0,MASK_SHOT|CONTENTS_ITEM,type,0);
 return trace;
}
int selectControlNode(const vec3_t start,const vec3_t direction,float limit,int fallback,float* hitDistance);
trace_t toolSurfaceTrace(){
 vec3_t start,direction;G_JAR_WorkshopViewRay(start,direction);
 auto trace=workshopTargetTrace(start,direction,G2_PHYSGUN);
 if(!trace.startsolid&&!trace.allsolid){
  float distance=JAR_TOOL_AIM_RANGE*trace.fraction;
  const int node=selectControlNode(start,direction,distance,trace.entityNum,&distance);
  if(node!=trace.entityNum){
   trace.entityNum=node;trace.fraction=distance/JAR_TOOL_AIM_RANGE;
   VectorMA(start,distance,direction,trace.endpos);
   // Logic gizmos are selectable but don't supply an attachable face.
   VectorClear(trace.plane.normal);
  }
 }
 return trace;
}
bool attachmentTool(const char* name,const trace_t* shot=nullptr,bool alternate=false){
 unsigned kind;
 if(!strcmp(name,"weld"))kind=jar::constraint::Weld;
 else if(!strcmp(name,"hinge"))kind=jar::constraint::Hinge;
 else if(!strcmp(name,"rope"))kind=jar::constraint::Rope;
 else if(!strcmp(name,"thruster"))kind=jar::constraint::Thruster;
 else return false;
 if(!sandboxReady())return true;
 if(alternate&&kind!=jar::constraint::Thruster){jointFirst=0;message("First anchor cleared.");return true;}
 const trace_t trace=shot?*shot:toolSurfaceTrace();
 if(trace.startsolid||trace.allsolid||trace.fraction>=1||trace.entityNum<0||trace.entityNum>ENTITYNUM_WORLD||
    VectorLengthSquared(trace.plane.normal)<.5f){
  message("Aim at an eligible surface to place an attachment.");
  return true;
 }
 const int slot=toolEntityRoot(trace.entityNum);
 if(slot!=ENTITYNUM_WORLD&&!editable(g_entities[slot]))return true;
 auto& entity=g_entities[slot];
 if(kind==jar::constraint::Thruster){jointFirst=0;attachmentThruster(entity,alternate,trace.endpos,trace.plane.normal);}
 else if(kind==jar::constraint::Rope)attachmentRope(entity,trace.endpos,trace.plane.normal);
 else constraintLink(entity,trace.endpos,trace.plane.normal,kind);
 return true;
}
void unlinkTool(){
 if(selected>=ENTITYNUM_WORLD||!editable(g_entities[selected]))return;
 const auto key=attachmentKey(g_entities[selected]);jointFirst=0;
 bool removed=false;
 for(unsigned i=0;i<state.jointCount;){
  const auto& joint=state.joints[i];
  if(joint.first!=key&&joint.second!=key){++i;continue;}
  removeJoint(joint.id);
  removed=true;
 }
 attachmentClassifyJoints();message(removed?"All links on this entity removed; poses retained.":"This entity has no links.");
}
