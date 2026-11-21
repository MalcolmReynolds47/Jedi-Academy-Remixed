// Original selection bar for distinct carried choices and native tools.
// Included in cg_weapons.cpp. GPL-2.0-only.
static bool CG_JAR_DrawWeaponBar(){
 JarWeaponBarEntry entries[jar::workshop::inventoryLimit];int selected=0;
 const int count=G_JAR_WorkshopWeaponBar(entries,jar::workshop::inventoryLimit,selected);if(!count)return false;
 const int visible=std::min(count,7),left=std::min(3,visible/2);
 for(int slot=0;slot<visible;++slot){const int index=(selected-left+slot+count)%count;const bool active=index==selected;
  const float size=active?62.0f:44.0f,x=320+(slot-left)*70.0f-size*.5f,y=active?389.0f:402.0f;
  vec4_t background={.025f,.04f,.065f,.9f};CG_FillRect(x-4,y-4,size+8,size+8,background);
  const float*color=active?colorWhite:colorLtGrey;cgi_R_SetColor(color);
  CG_DrawPic(x,y,size,size,cgi_R_RegisterShaderNoMip(entries[index].icon));
 }
 vec4_t label={.65f,.95f,1,1};const char*name=entries[selected].name;
 const float scale=.8f;const int width=cgi_R_Font_StrLenPixels(name,cgs.media.qhFontSmall,scale);
 cgi_R_Font_DrawString((640-width)*.5f,459,name,label,cgs.media.qhFontSmall,-1,scale);
 cgi_R_SetColor(nullptr);return true;
}
