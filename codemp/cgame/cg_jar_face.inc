// Accent only a living humanoid face; inactive routes clear owned accents.
static void CG_JAR_ApplyFace(centity_t*cent){
 trap->JAR_ApplyFace(cent->ghoul2,cent->currentState.number,cg.time,jar_facePreset.integer,jar_faceSensitivity.value,
  !cent->noFace&&cent->localAnimIndex<=1&&!cent->isRagging&&!(cent->currentState.eFlags&(EF_DEAD|EF_RAG)));
}
