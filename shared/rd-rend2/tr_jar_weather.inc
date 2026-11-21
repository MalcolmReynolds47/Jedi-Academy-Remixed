// Bounded Sandbox GPU slots, separate from authored weather. GPL-2.0-only.
static void JAR_WeatherApply(weatherSystem_t&ws,const jar::weather::Frame&frame){
 for(int i=0;i<3;++i){const auto&layer=frame.layers[i];auto&slot=ws.sandboxSlots[i];
  if(jar::weather::Same(layer,ws.sandbox.layers[i]))continue;
  slot.active=layer.count>0;slot.particleCount=layer.count;if(!slot.active)continue;
  const auto r=jar::weather::Describe(i,layer);slot.gravity=r.gravity*.001f;slot.fadeDistance=6000;
  slot.velocityOrientationScale=r.oriented?1.f:0.f;slot.size[0]=r.width;slot.size[1]=r.height*(i==jar::weather::Rain?.175f:1.f);
  for(int a=0;a<4;++a)slot.color[a]=r.color[a]*(a<3&&r.additive?r.color[3]:1.f);
  slot.drawImage=R_FindImageFile(r.texture,IMGTYPE_COLORALPHA,IMGFLAG_CLAMPTOEDGE|(tr.hdrLighting?IMGFLAG_SRGB:0));
 }
 ws.sandbox=frame;
}
static void JAR_WeatherWind(const weatherSystem_t&ws,vec3_t out,const float*atPoint=nullptr){
 VectorCopy(ws.constWindDirection,out);for(int i=0;i<ws.activeWindObjects;++i)VectorAdd(out,ws.windSlots[i].currentVelocity,out);
 vec3_t local;jar::weather::LocalWind(ws.sandboxZones,atPoint,local);
 for(int a=0;a<3;++a)out[a]+=(ws.sandbox.wind[a]+local[a])*.001f;
}
