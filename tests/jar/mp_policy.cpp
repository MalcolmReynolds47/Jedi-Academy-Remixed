#include "../../codemp/game/jar_mp_policy.h"
#include <cassert>
#include <initializer_list>
int main(){
 assert(JAR_MP_IsCommand("jar_release")&&JAR_MP_IsCommand("jar_allowTools"));
 assert(JAR_MP_IsCommand("jar_select")&&JAR_MP_IsCommand("jar_distance"));
 assert(!JAR_MP_IsCommand("jar_spawn_extra")&&!JAR_MP_IsCommand("quit")&&!JAR_MP_IsCommand("jar_spawn;quit"));
 unsigned id=0;assert(JAR_MP_ParseId("16783728",&id)&&id==16783728);
 for(const char*s:{"","-1","1;quit","1.0","4294967296"," 2"})assert(!JAR_MP_ParseId(s,&id));
 assert(JAR_MP_CanEdit(0,1,0));assert(!JAR_MP_CanEdit(1,1,0));assert(JAR_MP_CanEdit(1,1,1));assert(!JAR_MP_CanEdit(1,0,1));
 int next=0;assert(JAR_MP_RequestReady(100,&next));assert(!JAR_MP_RequestReady(101,&next));assert(JAR_MP_RequestReady(200,&next));
}
