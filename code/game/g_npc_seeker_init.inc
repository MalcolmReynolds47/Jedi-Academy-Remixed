// Seeker movement and ammo are class behavior, including the neutral retail
// remote definition. Keep the existing friendly Seeker initialization intact.
static bool JAR_NpcInitSeeker(gentity_t*ent){
 if(!ent||!ent->client||!ent->NPC||ent->client->NPC_class!=CLASS_SEEKER)return false;
 ent->NPC->defaultBehavior=BS_DEFAULT;
 ent->client->ps.gravity=0;
 ent->svFlags|=SVF_CUSTOM_GRAVITY;
 ent->client->moveType=MT_FLYSWIM;
 ent->count=30;
 return true;
}
