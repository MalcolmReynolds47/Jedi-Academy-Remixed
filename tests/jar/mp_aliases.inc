// Map-class labels execute native actor definitions; stable alias IDs survive
// command dispatch, clipboard copies, world encoding and cleanup.
static void AliasCases(void){
 static jarWorld_t captured,decoded;static byte bytes[WORLD_BYTES];
 assert(JAR_MP_ALIAS_COUNT==82);
 for(unsigned i=0;i<JAR_MP_ALIAS_COUNT;++i){
  WorldFixture();const jarMPAlias_t*alias=&jarMPAliases[i];int index=-1;
  const jarMPEntry_t*entry=Entry(alias->id,&index);assert(entry&&entry->tab==3&&(entry->kind==2||entry->kind==3));
  assert(strcmp(entry->name,alias->name)&&!strcmp(JAR_MPCatalogName(entry),alias->name));
  char key[32];snprintf(key,sizeof(key),"%u",alias->id);Command(0,"jar_spawn",key);
  int n=Last(0);assert(n>=MAX_CLIENTS&&objects[n].catalog==index+1);
  assert(!strcmp(actorSpawnName,entry->name)&&actorSpawnVehicle==(entry->kind==3));
  gentity_t*e=&g_entities[n];assert(e->NPC&&e->client);CopyReady(e);
  assert(!!e->m_pVehicle==(entry->kind==3));
  CopyAssembly(&g_entities[0],n);assert(copies[0].count==1&&copies[0].node[0].id==alias->id);
  PasteAssembly(&g_entities[0]);int pasted=players[0].selected;assert(pasted!=n&&pasted>=MAX_CLIENTS);
  CopyReady(&g_entities[pasted]);assert(objects[pasted].catalog==index+1);
  assert(!strcmp(actorSpawnName,entry->name)&&actorSpawnVehicle==(entry->kind==3));
  assert(WorldCapture(&captured));int size=WorldEncode(&captured,bytes);assert(size>0&&WorldDecode(bytes,size,&decoded));
  assert(decoded.copy.count==2&&decoded.copy.node[0].id==alias->id&&decoded.copy.node[1].id==alias->id);
  Undo(&g_entities[0],qfalse);assert(e->inuse&&!g_entities[pasted].inuse);
  Undo(&g_entities[0],qfalse);assert(!LiveCount());
 }
 puts("MP map aliases: all 82 labels/IDs, native NPC/vehicle dispatch, copies, save codec and Undo passed.");
}
