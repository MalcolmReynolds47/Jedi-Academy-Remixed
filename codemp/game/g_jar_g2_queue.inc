// Keep every pending removal; bounded commands stay within reliable text limits.
// GPL-2.0-only.
#define MAX_G2_KILL_QUEUE 256
int gG2KillIndex[MAX_G2_KILL_QUEUE];
int gG2KillNum;
void G_SendG2KillQueue(void){
 if(!gG2KillNum)return;
 char message[512]="kg2";int used=3,count=gG2KillNum<64?gG2KillNum:64;
 for(int i=0;i<count;++i)used+=snprintf(message+used,sizeof(message)-used," %i",gG2KillIndex[i]);
 trap->SendServerCommand(-1,message);
 gG2KillNum-=count;
 memmove(gG2KillIndex,gG2KillIndex+count,gG2KillNum*sizeof(gG2KillIndex[0]));
}
void G_KillG2Queue(int entity){
 if(entity<0||entity>=ENTITYNUM_WORLD)return;
 if(gG2KillNum==MAX_G2_KILL_QUEUE)G_SendG2KillQueue();
 gG2KillIndex[gG2KillNum++]=entity;
}
