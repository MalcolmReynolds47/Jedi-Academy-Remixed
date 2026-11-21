// Set the two variant strings before parsing the base. Native VF_STRING keeps
// the first value; other vehicle properties and function setup remain native.
static void JAR_ApplyVehicleVariant(vehicleInfo_t *vehicle,const char *name,const char *skin){
 if(skin){char variantName[128],variantSkin[128];Q_strncpyz(variantName,name,sizeof(variantName));Q_strncpyz(variantSkin,skin,sizeof(variantSkin));BG_ParseVehicleParm(vehicle,"name",variantName);BG_ParseVehicleParm(vehicle,"skin",variantSkin);}
}
