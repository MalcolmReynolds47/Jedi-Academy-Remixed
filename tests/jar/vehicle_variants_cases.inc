static void VehicleCases(int argc,char**argv){
 int i,index;const char*skin=NULL;char text[1024],name[64];
 numVehicles=1;numVehicleWeapons=1;VehicleParms[0]=0;VehWeaponParms[0]=0;
 CHECK(MAX_VEHICLES==64);
 CHECK(!JAR_VehicleVariant(NULL)&&!JAR_VehicleVariant("swoop_orange"));
 CHECK(JAR_VehicleVariant("SWOOP_RED")&&!strcmp(JAR_VehicleVariant("SWOOP_RED")->skin,"default"));
 CHECK(!JAR_ResolveVehicleBlock("swoop { name swoop model unsupported }","swoop_blue",&skin)&&!skin);
 CHECK(!JAR_ResolveVehicleBlock("unclosed {","swoop_red",&skin));
 // A malformed explicit color definition never falls through to a stock choice.
 strcpy(text,"swoop_blue broken");CHECK(JAR_ResolveVehicleBlock(text,"swoop_blue",&skin)==text+10&&!skin);
 strcpy(VehicleParms,"swoop_blue\n{\nname swoop_blue\ntype VH_SPEEDER\nmodel mod_swoop\nskin special\narmor 731\n}\n");
 index=BG_VehicleGetIndex("SWOOP_BLUE");CHECK(index==1&&numVehicles==2&&!strcmp(g_vehicleInfo[index].skin,"special")&&!strcmp(g_vehicleInfo[index].model,"mod_swoop")&&g_vehicleInfo[index].armor==731);
 CHECK(BG_VehicleGetIndex("swoop_blue")==index&&numVehicles==2);
 CHECK(BG_VehicleGetIndex("swoop_purple")==VEHICLE_NONE&&numVehicles==2);
 if(argc==2){
  FILE*f=fopen(argv[1],"rb");size_t size;CHECK(f);size=fread(VehicleParms,1,sizeof(VehicleParms)-1,f);CHECK(!ferror(f));fclose(f);VehicleParms[size]=0;
  CHECK(JAR_StockSwoopHash(JAR_VehicleBlockHash(JAR_VehicleBlock(VehicleParms,"swoop"))));
  numVehicles=1;memset(g_vehicleInfo,0,sizeof(g_vehicleInfo));
  index=BG_VehicleGetIndex("swoop");CHECK(index==1);vehicleInfo_t base=g_vehicleInfo[index];
#ifdef JAR_MP
  CHECK(!strcmp(base.skin,"default"));
#endif
  for(i=0;i<7;++i){const jarVehicleVariant_t*v=&jarVehicleVariants[i];index=BG_VehicleGetIndex(v->name);
   CHECK(index==i+2&&!Q_stricmp(g_vehicleInfo[index].name,v->name)&&!strcmp(g_vehicleInfo[index].skin,v->skin));
   CHECK(!strcmp(g_vehicleInfo[index].model,"swoop")&&g_vehicleInfo[index].type==VH_SPEEDER&&g_vehicleInfo[index].armor==base.armor&&g_vehicleInfo[index].mass==base.mass&&g_vehicleInfo[index].speedMax==base.speedMax);
   CHECK(BG_VehicleGetIndex(v->name)==index);printf("Native variant: %s skin=%s index=%d armor=%d\n",v->name,g_vehicleInfo[index].skin,index,g_vehicleInfo[index].armor);
  }
  CHECK(numVehicles==9);CHECK(!strcmp(g_vehicleInfo[1].name,"Swoop")&&!strcmp(g_vehicleInfo[1].skin,base.skin));
 }
 // Fill all definition slots through the real parser, past the original 16.
 for(i=numVehicles;i<MAX_VEHICLES;++i){Com_sprintf(name,sizeof(name),"fixture_%d",i);Com_sprintf(text,sizeof(text),"%s\n{\nname %s\ntype VH_SPEEDER\nmodel swoop\nskin default\n}\n",name,name);Q_strncpyz(VehicleParms,text,sizeof(VehicleParms));CHECK(BG_VehicleGetIndex(name)==i);}
 CHECK(numVehicles==64);strcpy(VehicleParms,"overflow\n{\nname overflow\ntype VH_SPEEDER\nmodel swoop\n}\n");CHECK(BG_VehicleGetIndex("overflow")==VEHICLE_NONE&&numVehicles==64);
 CHECK(BG_VehicleGetIndex("fixture_63")==63);
}
