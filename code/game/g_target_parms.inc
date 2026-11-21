// Native callback and save enum retained for authored and portable controllers.
void Q3_SetParm(int entID,int parmNum,const char*parmValue);
void target_change_parm_use(gentity_t*self,gentity_t*,gentity_t*activator){
 if(!self||!self->inuse||!self->parms||!activator||!activator->inuse||
    activator->s.number<0||activator->s.number>=ENTITYNUM_WORLD)return;
 // An activator may be this controller. Read a stable copy of all source slots.
 const parms_t source=*self->parms;
 for(int i=0;i<MAX_PARMS;++i)if(source.parm[i][0]&&memchr(source.parm[i],0,MAX_PARM_STRING_LENGTH))Q3_SetParm(activator->s.number,i,source.parm[i]);
}
void SP_target_change_parm(gentity_t*self){
 G_SetOrigin(self,self->s.origin);
 // Empty controllers are harmless and can receive parameters from the editor.
 self->e_UseFunc=useF_target_change_parm_use;
}
