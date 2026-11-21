// Original Galak art, following replicated server attack phase. GPL-2.0-only.
// MP does not support SP's entity-relative trace-line primitive, so the beam is
// traced each rendered frame; the original muzzle particles follow its bolt.
static void CG_JAR_GalakEffects(centity_t *cent) {
 static int lastEmission[MAX_GENTITIES];
 unsigned phase;
 int number,bolt,effect;
 mdxaBone_t matrix;
 vec3_t angles,origin,direction,end;
 trace_t trace;
 refEntity_t beam;
 if(cent->currentState.NPC_class!=CLASS_GALAKMECH || !cent->ghoul2 ||
    (cent->currentState.eFlags&EF_DEAD)) return;
 number=cent->currentState.number;
 if(number<0||number>=MAX_GENTITIES) return;
 phase=(unsigned)cent->currentState.surfacesOn & JAR_GALAK_BEAM_MASK;
 if(!phase) { lastEmission[number]=0; return; }
 bolt=trap->G2API_AddBolt(cent->ghoul2,0,"*flashc");
 VectorCopy(cent->lerpAngles,angles); angles[PITCH]=angles[ROLL]=0;
 if(bolt<0 || !trap->G2API_GetBoltMatrix(cent->ghoul2,0,bolt,&matrix,
    angles,cent->lerpOrigin,cg.time,cgs.gameModels,cent->modelScale)) return;
 BG_GiveMeVectorFromMatrix(&matrix,ORIGIN,origin);
 BG_GiveMeVectorFromMatrix(&matrix,NEGATIVE_Y,direction);
 if(phase & JAR_GALAK_FIRING) {
  VectorMA(origin,1024,direction,end);
  CG_Trace(&trace,origin,NULL,NULL,end,number,MASK_SHOT);
  if(!trace.startsolid && !trace.allsolid) {
   memset(&beam,0,sizeof(beam)); beam.reType=RT_LINE;
   VectorCopy(origin,beam.origin); VectorCopy(trace.endpos,beam.oldorigin);
   beam.radius=8; beam.shaderTexCoord[0]=beam.shaderTexCoord[1]=1;
   memset(beam.shaderRGBA,255,sizeof(beam.shaderRGBA));
   beam.customShader=trap->R_RegisterShader("gfx/misc/blueLine");
   trap->R_AddRefEntityToScene(&beam);
  }
 }
 if(lastEmission[number] && cg.time>=lastEmission[number] && cg.time-lastEmission[number]<100) return;
 effect=trap->FX_RegisterEffect(phase&JAR_GALAK_FIRING?
   "galak/trace_beam_follow":"galak/beam_warmup_follow");
 trap->FX_PlayBoltedEffectID(effect,origin,cent->ghoul2,bolt,number,0,0,qtrue);
 if((phase&JAR_GALAK_FIRING) && !trace.startsolid && !trace.allsolid && trace.fraction<1)
  trap->FX_PlayEffectID(trap->FX_RegisterEffect("galak/spark"),trace.endpos,trace.plane.normal,-1,-1,qfalse);
 lastEmission[number]=cg.time;
}
