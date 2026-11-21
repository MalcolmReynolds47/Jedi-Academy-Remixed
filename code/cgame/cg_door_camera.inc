// Match the telescoping stock mount's length to interpolated head motion.
static void CG_DoorCameraScale(const centity_t*cent,refEntity_t&ent){
 if(!G_DoorCameraShaft(cent->gent)||!cent->interpolate||!cent->nextState||cent->currentState.modelindex!=cent->nextState->modelindex)return;
 const float a=cent->currentState.modelScale[2],b=cent->nextState->modelScale[2];
 if(std::isfinite(a)&&std::isfinite(b)&&a>0&&b>0)ent.modelScale[2]=a+(b-a)*std::max(0.f,std::min(1.f,cg.frameInterpolation));
}
