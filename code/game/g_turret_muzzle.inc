// Resolve the attachment selected by the native turret setup. GPL-2.0-only.
static bool G_TurretMuzzleMatrix(gentity_t*self,const vec3_t origin,mdxaBone_t&matrix){
 const bool ns=self->classname&&!strcmp(self->classname,"misc_ns_turret");
 const int model=ns?self->playerModel:0;
 if(model<0||model>=self->ghoul2.size())return false;
 const int bolt=(self->spawnflags&SPF_TURRETG2_TURBO)?
  gi.G2API_AddBolt(&self->ghoul2[model],self->alt_fire?"*muzzle2":"*muzzle1"):
  ns?self->torsoBolt:gi.G2API_AddBolt(&self->ghoul2[model],"*flash03");
 return bolt>=0&&gi.G2API_GetBoltMatrix(self->ghoul2,model,bolt,&matrix,self->currentAngles,origin,level.time,NULL,self->modelScale);
}
