// Consume already-held movement while editing the unpaused Sandbox search.
// Key states remain intact so holding W resumes movement after editing ends.
static void JAR_FilterTyping(usercmd_t*cmd,vec3_t angles,const vec3_t before,int catcher,int open,int typing){
 if(catcher!=KEYCATCH_CGAME||!open||!typing)return;
 VectorCopy(before,angles);cmd->forwardmove=cmd->rightmove=cmd->upmove=0;cmd->buttons=0;
}
