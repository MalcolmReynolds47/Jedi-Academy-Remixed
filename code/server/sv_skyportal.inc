// Reuse native area/cluster visibility and snapshot deduplication. GPL-2.0-only.
static void SV_AddPortableSkyEntities(const char*config,clientSnapshot_t*frame,snapshotEntityNumbers_t*numbers){
 jar::skyPortal::View sky;
 if(jar::skyPortal::Decode(config,sky,ENTITYNUM_WORLD)&&sky.entities)
  SV_AddEntitiesVisibleFromPoint(sky.origin,frame,numbers,qtrue);
}
