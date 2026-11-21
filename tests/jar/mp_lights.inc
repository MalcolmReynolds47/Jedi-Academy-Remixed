// Portable light bodies exercise production commands, snapshots and world state.
static void LightCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;
 gentity_t*p=&g_entities[0],*a=ControllerAt("light",0,100),*b=ControllerAt("misc_dlight",0,300);
 gentity_t*disable=ControllerAt("target_deactivate",0,500),*enable=ControllerAt("target_activate",0,700);
 assert(a&&b&&enable&&disable);int n=a->s.number;
 assert(ControllerKind(n)==33&&ControllerKind(b->s.number)==33&&a->s.eType==ET_GENERAL&&a->s.constantLight==(int)0x28ffb27fu);
 ControllerSelect(a);Command(0,"jar_property","controller_radius_512");assert((unsigned)a->s.constantLight==0x80ffb27fu&&ContextUndoReady(p,n));
 Command(0,"jar_property","controller_radius_513");assert((unsigned)a->s.constantLight==0x80ffb27fu);
 Command(0,"jar_property","property_undo");assert((unsigned)a->s.constantLight==0x28ffb27fu);
 Command(0,"jar_property","controller_radius_1020");assert((unsigned)a->s.constantLight==0xffffb27fu);
 SwitchLink(p,disable,a,0);SwitchLink(p,enable,a,0);
 ControllerShot(p,disable->s.number,qfalse);assert(a->flags&FL_INACTIVE);assert(!a->s.constantLight);
 ControllerShot(p,enable->s.number,qfalse);assert(!(a->flags&FL_INACTIVE));assert((unsigned)a->s.constantLight==0xffffb27fu);
 ControllerShot(p,n,qfalse);assert(!a->s.constantLight&&b->s.constantLight);
 static jarWorld_t world,decoded;static byte data[WORLD_BYTES];assert(WorldCapture(&world));int len=WorldEncode(&world,data);assert(len&&WorldDecode(data,len,&decoded));
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 int found=0;for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==33){
  jarControllerState_t s;ControllerState(i,&s);++found;
  assert(s.speed==(s.active?160:1020)&&!!g_entities[i].s.constantLight==s.active);
 }
 assert(found==2&&WorldClear());imports.SendServerCommand=Send;
 puts("MP portable lights: stock class IDs, exact light packing, edit/undo, links, persistence and cleanup passed.");
}
