// Let an aimed portable transport marker take precedence over dismount.
// GPL-2.0-only.
bool G_TryUseVehicleTransportMarker(gentity_t& rider){
 auto* hull=G_TransportVehicleHull(&rider);
 if(!hull||hull==&rider||!G_TransportVehicleReady(*hull))return false;
 const auto& eye=rider.client->renderInfo.eyePoint;
 vec3_t forward,end;
 AngleVectors(rider.client->ps.viewangles,forward,nullptr,nullptr);
 VectorMA(eye,USE_DISTANCE,forward,end);
 trace_t hit{};
 gi.trace(&hit,eye,vec3_origin,vec3_origin,end,rider.s.number,
  MASK_OPAQUE|CONTENTS_SOLID|CONTENTS_TERRAIN|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE,G2_NOCOLLIDE,10);
 if(hit.fraction>=1||hit.entityNum<0||hit.entityNum>=ENTITYNUM_WORLD||hit.entityNum>=globals.num_entities)return false;
 auto& marker=g_entities[hit.entityNum];
 if(!marker.inuse||!ValidUseTarget(&marker))return false;
 jar::push::Settings push;
 jar::teleport::Settings teleport;
 const bool enabled=(G_PushRead(marker,push)&&push.kind==jar::push::Marker&&push.enabled&&push.vehicles)||
  (G_TeleportRead(marker,teleport)&&teleport.kind==jar::teleport::Marker&&teleport.enabled&&teleport.vehicles);
 if(!enabled)return false;
 if(rider.client->ps.useTime<=0){
  rider.client->ps.useTime=250; // Same repeat interval as ordinary PM_Use.
  GEntity_UseFunc(&marker,&rider,&rider);
 }
 return true;
}
