// Native ET_BEAM draw submission with deterministic renderer imports.
static refEntity_t laserDraw;static int laserDraws;
void CG_SetGhoul2Info(refEntity_t*e,centity_t*c){}
#include "cgame/cg_jar_native_beam.inc"
static void LaserRender(const refEntity_t*e){laserDraw=*e;++laserDraws;}
static void LaserClientCases(void){
 centity_t beam={0};beam.currentState.eType=ET_BEAM;VectorSet(beam.currentState.pos.trBase,100,200,300);VectorSet(beam.currentState.origin2,400,500,600);VectorSet(beam.lerpOrigin,-1,-2,-3);
 imports.R_AddRefEntityToScene=LaserRender;CG_Beam(&beam);assert(laserDraws==1&&laserDraw.reType==RT_BEAM&&laserDraw.renderfx==RF_NOSHADOW&&VectorCompare(laserDraw.origin,beam.currentState.pos.trBase)&&VectorCompare(laserDraw.oldorigin,beam.currentState.origin2));
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]="0";args[3]="1";args[4]="99";args[5]="1";args[6]="0";args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 args[1]="99";args[2]="13";args[3]=args[4]=args[5]=args[6]=args[7]=args[8]=args[9]=args[11]="0";args[10]="-";args[12]="1";args[13]="1";CG_JAR_Controller();assert(contextControllerKind==13&&contextController.active&&contextController.damage==1);
 contextPage=1;CG_JAR_Draw();Click(450,155);assert(contextEditing==206);ContextType("37");CG_JAR_MenuUp();assert(menu&&contextEditing);CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_damage_37 99"));Click(450,182);assert(!strcmp(request,"jar_property controller_playing 99"));Click(450,238);assert(!strcmp(request,"jar_property controller_enabled 99"));
 Click(450,155);ContextType("2147483648");int count=requests;CG_JAR_Key(A_ENTER,qtrue);assert(requests==count&&contextEditing);args[13]="32768";CG_JAR_Controller();assert(contextController.damage==1);args[13]="0";CG_JAR_Controller();assert(contextController.damage==0&&contextEditing);args[12]="2";CG_JAR_Controller();assert(contextController.active==1);
 CG_JAR_Key(A_ESCAPE,qtrue);args[12]=args[13]="0";puts("MP laser client: native RT_BEAM endpoints and shadow flags, typed damage bounds, pinned input, Firing/Enabled commands and invalid echoes passed.");
}
