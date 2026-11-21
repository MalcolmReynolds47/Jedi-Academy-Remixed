// Reuse the retail light-cone material, clipped by the native controller trace.
static void CG_PortableSpotlight(centity_t*cent){
 auto*e=cent->gent;jar::spotlight::Settings s;
 if(!e||!G_SpotlightRead(*e,s)||!s.enabled||(e->svFlags&SVF_INACTIVE)||e->radius<=.01f)return;
 vec3_t direction,start,light,color;AngleVectors(cent->lerpAngles,direction,nullptr,nullptr);VectorMA(cent->lerpOrigin,12,direction,start);VectorSubtract(e->s.origin2,start,direction);const float length=VectorNormalize(direction);if(length<=.01f)return;
 VectorCopy(s.rgb,color);const float width=jar::spotlight::Width(s,length);
 FX_AddCylinder(-1,start,direction,2,2,0,width,width,0,length,length,0,.18f,.18f,0,color,color,0,1,cgi_R_RegisterShader("gfx/effects/light_cone"),0);
 if(s.light>0){VectorMA(e->s.origin2,-std::min(5.f,length*.5f),direction,light);cgi_R_AddLightToScene(light,s.light,color[0],color[1],color[2]);}
}
