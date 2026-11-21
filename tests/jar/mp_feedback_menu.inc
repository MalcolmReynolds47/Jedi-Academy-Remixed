static int feedbackSounds,housingRenders;
static sfxHandle_t FeedbackSound(const char*name){assert(!strcmp(name,"sound/interface/esc.mp3"));return 23;}
static void FeedbackPlay(sfxHandle_t sound,int channel){assert(sound==23&&channel==CHAN_LOCAL_SOUND);++feedbackSounds;}
static qhandle_t HousingModel(const char*name){assert(!strcmp(name,JAR_THRUSTER_MODEL));return 67;}
static void HousingRender(const refEntity_t*e){assert(e->hModel==67&&e->origin[0]==10&&e->origin[2]==30&&e->axis[2][2]>.99f&&e->shaderRGBA[3]==255);++housingRenders;}
static void FeedbackMenuCases(void){
 imports.S_RegisterSound=FeedbackSound;imports.S_StartLocalSound=FeedbackPlay;
 const int start=cg.time;
 MenuStatus("Last thruster removed.");CG_JAR_UndoFeedback();assert(feedbackSounds==1);
 MenuStatus("Previous properties restored.");
 CG_JAR_Draw();assert(undoTitles==1&&!strcmp(undoText,"Last thruster removed."));
 cg.time=start+4500;
 CG_JAR_Draw();assert(undoTitles==1);
 cg.time=start;undoStatusUntil=menuStatusUntil=0;
 imports.R_RegisterModel=HousingModel;imports.R_AddRefEntityToScene=HousingRender;
 centity_t mount={0};mount.currentState.generic1=254;VectorSet(mount.lerpOrigin,10,20,30);VectorSet(mount.currentState.origin2,0,0,1);
 assert(CG_JAR_Entity(&mount)&&housingRenders==1); // off still renders, no extra simulated entity
}
