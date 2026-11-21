#include "client/cl_jar_typing.inc"
static void SearchMenuCases(void){
 args[1]="81";args[2]="";keyHeld=1;catcher=0;CG_JAR_Init();CG_JAR_MenuDown();Click(78,85);assert(tab==1);
 Click(60,115);assert(searchEditing);const char*query="STORM";for(const char*p=query;*p;++p)CG_JAR_Key(*p|K_CHAR_FLAG,qtrue);
 Cards();assert(cardCount>0&&total>0);for(int i=0;i<cardCount;++i)assert(JAR_MenuMatches(JAR_MPCatalogName(&jarMPCatalog[cards[i]]),"storm"));
 // Key-up and its queued -jar_menu command must both preserve a text entry.
 CG_JAR_Key(81,qfalse);CG_JAR_MenuUp();keyHeld=0;CG_JAR_Draw();assert(menu&&searchEditing&&catcher==KEYCATCH_CGAME);
 CG_JAR_Key('q'|K_CHAR_FLAG,qtrue);assert(!strcmp(searchText,"STORMq"));CG_JAR_Key(A_BACKSPACE,qtrue);assert(!strcmp(searchText,"STORM"));
 CG_JAR_Key(A_ENTER,qtrue);CG_JAR_Draw();assert(menu&&!searchEditing&&catcher==KEYCATCH_CGAME);CG_JAR_Key(A_ESCAPE,qtrue);assert(!menu);keyHeld=1;
 CG_JAR_MenuDown();Click(248,115);assert(!searchText[0]);Click(60,115);
 for(int i=0;i<110;++i)CG_JAR_Key('x'|K_CHAR_FLAG,qtrue);assert(strlen(searchText)==95);Cards();assert(!cardCount&&!total&&page==0);
 CG_JAR_Key(A_ESCAPE,qtrue);assert(!menu&&!searchEditing);
 CG_JAR_MenuDown();Click(248,115);Click(28,85);Cards();CG_JAR_Key(A_MWHEELDOWN,qtrue);assert(page==1);Click(78,85);assert(page==0);Click(28,85);assert(page==1);
 // Card padding and empty search results cannot fire an adjacent card.
 int sent=requests;Click(102,140);assert(requests==sent);Click(60,115);CG_JAR_Key('~'|K_CHAR_FLAG,qtrue);Click(30,140);assert(!cardCount&&requests==sent);
 Click(174,85);assert(tab==3);
 Q_strncpyz(searchText,"map model",sizeof(searchText));SearchChanged();assert(cardCount==2);
 Click(30,140);assert(!strcmp(request,"jar_spawn 88708799"));
 Click(110,140);assert(!strcmp(request,"jar_spawn 99343814"));
 Q_strncpyz(searchText,"misc_weapon_shooter",sizeof(searchText));SearchChanged();assert(cardCount==1);
 assert(!strcmp(JAR_MPCatalogName(&jarMPCatalog[cards[0]]),"E-11 shooter"));
 Click(30,140);assert(!strcmp(request,"jar_spawn 98645765"));
 Q_strncpyz(searchText,"item_medpac",sizeof(searchText));SearchChanged();
 int smallBacta=0;
 for(int i=0;i<cardCount;++i){
  const jarMPEntry_t* entry=&jarMPCatalog[cards[i]];
  assert(entry->id!=68822705u);
  if(entry->id==73222696u)++smallBacta;
 }
 assert(smallBacta==1);
 CG_JAR_Key(A_ESCAPE,qtrue);
 usercmd_t cmd={0};vec3_t angles={10,20,30},before={1,2,3};cmd.forwardmove=127;cmd.rightmove=-127;cmd.upmove=127;cmd.buttons=BUTTON_ATTACK|BUTTON_USE;
 JAR_FilterTyping(&cmd,angles,before,KEYCATCH_CGAME,1,1);assert(!cmd.forwardmove&&!cmd.rightmove&&!cmd.upmove&&!cmd.buttons&&VectorCompare(angles,before));
 cmd.forwardmove=127;cmd.buttons=BUTTON_ATTACK;JAR_FilterTyping(&cmd,angles,before,KEYCATCH_CGAME,1,0);assert(cmd.forwardmove==127&&cmd.buttons==BUTTON_ATTACK);
 JAR_FilterTyping(&cmd,angles,before,0,1,1);assert(cmd.forwardmove==127);
 char label[64];JAR_MenuLabel("models/players/stormtrooper/model.glm",label,sizeof(label));assert(!strcmp(label,"stormtrooper"));
 puts("Shared Sandbox search: case-insensitive catalog filtering, 95-char bound, empty pages, key-up/typing lifecycle, per-tab pages and held-movement suppression passed.");
}
