// Staged numeric placement: no requests until the complete pose is applied.
static int contextTransformPage;
static float contextTransform[6];
static char contextTransformDraft[6][24];
static const char*contextTransformLabels[]={"X","Y","Z","Pitch","Yaw","Roll"};
static void ContextTransformReload(void){for(int i=0;i<6;++i)Com_sprintf(contextTransformDraft[i],sizeof(contextTransformDraft[i]),"%.9g",contextTransform[i]);contextEditing=0;contextTransformPage=0;}
static qboolean ContextTransformCommit(void){
 if(contextEditing<100||contextEditing>=106)return qtrue;int i=contextEditing-100;float value;
 if(!JAR_ParseFloat(contextInput,&value)){MenuStatus("Enter a valid coordinate or angle.");return qfalse;}
 Com_sprintf(contextTransformDraft[i],sizeof(contextTransformDraft[i]),"%.9g",value);contextEditing=0;contextTransformPage=1;return qtrue;
}
static void ContextTransformApplyDraft(int first){
 float v[6];if(!ContextTransformCommit())return;memcpy(v,contextTransform,sizeof(v));for(int i=first;i<first+3;++i)if(!JAR_ParseFloat(contextTransformDraft[i],&v[i]))return;
 char action[144];Com_sprintf(action,sizeof(action),"transform_%.9g_%.9g_%.9g_%.9g_%.9g_%.9g",v[0],v[1],v[2],v[3],v[4],v[5]);ContextProperty(action);
}
