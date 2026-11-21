// Headless wire adapter for scripts/mp_network_smoke.py; no platform or renderer initialization.
#include "../../codemp/client/cl_net_chan.cpp"
#include "../../codemp/game/anims.h"
#include "../../codemp/game/jar_mp_prop.h"
#include "../../codemp/game/jar_mp_searchlight.h"
#include "../../codemp/game/jar_mp_portal.h"
#include "../../codemp/game/jar_mp_section_scenery.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>
#include <string>

clientConnection_t clc{};

static std::string ProbeHex(const byte*data,size_t size){
 static constexpr char hex[]="0123456789abcdef";
 std::string wire;
 for(size_t i=0;i<size;++i){const byte value=data[i];wire+=hex[value>>4];wire+=hex[value&15];}
 return wire;
}

static void ProbePacket(){
 assert(packets.size()==1);
 const auto wire=ProbeHex(packets[0].data(),packets[0].size());
 packets.clear();
 std::cout<<"packet "<<wire<<std::endl;
}

static std::string ProbeValue(const std::string&value){
 return value.empty()?"-":ProbeHex(reinterpret_cast<const byte*>(value.data()),value.size());
}

static std::string ProbeBspSlots(const std::array<std::string,MAX_SUB_BSP>& models){
 std::string slots;
 for(int slot=1;slot<MAX_SUB_BSP;++slot){
  if(models[slot].empty())continue;
  if(!slots.empty())slots+=';';
  slots+=std::to_string(slot)+','+ProbeValue(models[slot]);
 }
 return slots.empty()?"-":slots;
}

static int ProbeHash(const char*text){
 // Match CL_WritePacket's key for the latest reliable server command.
 int sum=0;
 for(int i=0;i<32&&text[i];++i)sum+=text[i]*(119+i);
 return sum^(sum>>10)^(sum>>20);
}

static int NetworkProbe(){
 struct PropTarget {int number;vec3_t center;};
 static std::vector<entityState_t> baselines(MAX_GENTITIES);
 int serverId=0;
 int snapshotTime=0;
 playerState_t latestPlayer{};
 std::string gameDir,music,pakNames,pakSums;
 std::array<std::string,MAX_SUB_BSP> bspModels;
 std::array<std::string,MAX_SOUNDS> sounds;
 std::array<std::string,MAX_FX> effects;
 std::array<std::string,MAX_MODELS> models;
 std::array<std::string,MAX_CLIENTS> playerInfo;
 std::array<std::string,3> constructLight;
 constexpr int constructStyle=CS_LIGHT_STYLES+32*3;
 const auto config=[&](int index,const char*value){
  if(index==CS_MUSIC)music=value;
  if(index>=CS_BSP_MODELS&&index<CS_BSP_MODELS+MAX_SUB_BSP)bspModels[index-CS_BSP_MODELS]=value;
  if(index>=CS_SOUNDS&&index<CS_SOUNDS+MAX_SOUNDS)sounds[index-CS_SOUNDS]=value;
  if(index>=CS_EFFECTS&&index<CS_EFFECTS+MAX_FX)effects[index-CS_EFFECTS]=value;
  if(index>=CS_MODELS&&index<CS_MODELS+MAX_MODELS)models[index-CS_MODELS]=value;
  if(index>=CS_PLAYERS&&index<CS_PLAYERS+MAX_CLIENTS)playerInfo[index-CS_PLAYERS]=value;
  if(index>=constructStyle&&index<constructStyle+3)constructLight[index-constructStyle]=value;
 };
 bool pureServer=false,sectionFixture=false,constructMap=false;
 int selectedEntity=-1;
 unsigned selectedIdentity=0,selectedCatalog=0;
 float selectedMass=0;
 int equippedWeapon=0;
 int forwardCrossings=0,reverseCrossings=0;
 int searchlightNotices=0;
 int sandboxOwned=-1;
 int switchClicks=0, previousSwitchEvent=0;
 std::vector<PropTarget> latestProps;
 netadr_t address{};address.type=NA_IP;
 std::string line;
 while(std::getline(std::cin,line)){
  std::istringstream request(line);std::string action;request>>action;
  if(action=="connect"){
   int protocol,challenge,port;std::string name;
   if(!(request>>protocol>>challenge>>port>>name)||port<1||port>65535||name.size()>24||name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")!=std::string::npos)return 2;
   std::string model="kyle/default",saber="single_1",color="4";
   std::string selection;
   if(request>>selection){
    if(!(request>>saber>>color)||selection.size()>=MAX_QPATH||saber.size()>=MAX_QPATH||color.size()>7||
       selection.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-/")!=std::string::npos||
       saber.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-")!=std::string::npos||
       color.find_first_not_of("#0123456789abcdefABCDEF")!=std::string::npos)return 2;
    model=selection;
   }
   clc.challenge=challenge;qport->integer=port;
   Netchan_Setup(NS_CLIENT,&clc.netchan,&address,port);
   std::string command="connect \"\\protocol\\"+std::to_string(protocol)+"\\qport\\"+std::to_string(port)+"\\challenge\\"+std::to_string(challenge)+"\\name\\"+name+
    "\\rate\\25000\\snaps\\20\\model\\"+model+"\\forcepowers\\7-1-032330000000001333\\color1\\"+color+"\\color2\\4\\handicap\\100"
    "\\cg_predictItems\\1\\saber1\\"+saber+"\\saber2\\none\\char_color_red\\255\\char_color_green\\255\\char_color_blue\\255\"";
   NET_OutOfBandData(NS_CLIENT,&address,reinterpret_cast<byte*>(&command[0]),static_cast<int>(command.size()));ProbePacket();
  }else if(action=="playerinfo"){
   int index;
   if(!(request>>index)||index<0||index>=MAX_CLIENTS)return 2;
   std::cout<<"playerinfo "<<ProbeValue(playerInfo[index])<<std::endl;
  }else if(action=="pureinfo"){
   if(!serverId)return 2;
   std::cout<<"pureinfo "<<pureServer<<' '<<static_cast<uint32_t>(clc.checksumFeed)<<' '
    <<ProbeValue(pakNames)<<' '<<ProbeValue(pakSums)<<std::endl;
  }else if(action=="blockchecksum"){
   std::string hex;request>>hex;
   if(hex.empty()||hex.size()%8||hex.size()>MAX_MSGLEN*2||hex.find_first_not_of("0123456789abcdef")!=std::string::npos)return 2;
   for(size_t i=0;i<hex.size();i+=2)output[i/2]=static_cast<byte>(std::stoi(hex.substr(i,2),nullptr,16));
   std::cout<<"blockchecksum "<<Com_BlockChecksum(output.data(),static_cast<int>(hex.size()/2))<<std::endl;
  }else if(action=="prime"||action=="enter"||action=="aim"||action=="disconnect"||action=="spawn"||action=="undo"||action=="follow"||action=="select"||action=="activate"||action=="pure"||action=="setviewpos"||action=="equip"||action=="physgun"||action=="move"||action=="use"||action=="npcspawn"||action=="cleanup"||action=="space_height"||action=="notarget"){
   byte data[1024]{};msg_t message;MSG_Init(&message,data,sizeof data);
   MSG_WriteLong(&message,serverId);MSG_WriteLong(&message,clc.netchan.incomingSequence);MSG_WriteLong(&message,clc.serverCommandSequence);
   if(action=="enter"||action=="aim"||action=="physgun"||action=="move"||action=="use"){
    if(!serverId)return 2;
    usercmd_t zero{},idle{};idle.weapon=WP_SABER;
    if(action=="aim"){
     int yawOffset;if(!(request>>yawOffset)||yawOffset< -180||yawOffset>180||!snapshotTime)return 2;
     float pitch=35;request>>std::ws;
     if(!request.eof()&&(!(request>>pitch)||!(pitch>=0&&pitch<=89)))return 2;
     idle.serverTime=snapshotTime;
     idle.angles[PITCH]=ANGLE2SHORT(pitch)-latestPlayer.delta_angles[PITCH];
     idle.angles[YAW]=ANGLE2SHORT(latestPlayer.viewangles[YAW]+yawOffset)-latestPlayer.delta_angles[YAW];
    }
    if(action=="physgun"){
     int number,buttons;
     float lift, targetHeight=0;
     if(!snapshotTime||!(request>>number>>buttons>>lift)||buttons<0||buttons>3||!std::isfinite(lift)||std::fabs(lift)>45)return 2;
     request>>std::ws;
     if(!request.eof()&&(number<0||!(request>>targetHeight)||!std::isfinite(targetHeight)||std::fabs(targetHeight)>64))return 2;
     idle.weapon=WP_PHYSGUN;idle.serverTime=snapshotTime;
     if(buttons&1)idle.buttons|=BUTTON_ATTACK;
     if(buttons&2)idle.buttons|=BUTTON_ALT_ATTACK;
     vec3_t angles;VectorCopy(latestPlayer.viewangles,angles);
     if(number>=0){
      const auto target=std::find_if(latestProps.begin(),latestProps.end(),[number](const PropTarget&prop){return prop.number==number;});
      if(target==latestProps.end())return 2;
      vec3_t eye,direction;VectorCopy(latestPlayer.origin,eye);eye[2]+=latestPlayer.viewheight;
      VectorSubtract(target->center,eye,direction);
      direction[2]+=targetHeight;
      vectoangles(direction,angles);
     }
     angles[PITCH]-=lift;
     for(int a=0;a<3;++a)idle.angles[a]=ANGLE2SHORT(angles[a])-latestPlayer.delta_angles[a];
    }
    if(action=="use"){
     vec3_t target,eye,direction,angles;
     if(!snapshotTime||!(request>>target[0]>>target[1]>>target[2])||
        !std::all_of(target,target+3,[](float value){return std::isfinite(value);}))return 2;
     VectorCopy(latestPlayer.origin,eye);eye[2]+=latestPlayer.viewheight;
     VectorSubtract(target,eye,direction);vectoangles(direction,angles);
     idle.weapon=latestPlayer.weapon;idle.serverTime=snapshotTime;idle.buttons=BUTTON_USE;
     for(int a=0;a<3;++a)idle.angles[a]=ANGLE2SHORT(angles[a])-latestPlayer.delta_angles[a];
    }
    if(action=="move"){
     int forward,right,up,maxStep=0;
     if(!snapshotTime||!(request>>forward>>right>>up)||
        forward< -127||forward>127||right< -127||right>127||up< -127||up>127)return 2;
     request>>std::ws;
     if(!request.eof()&&(!(request>>maxStep)||maxStep<1||maxStep>200))return 2;
     idle.weapon=latestPlayer.weapon;idle.serverTime=snapshotTime;
     if(maxStep)idle.serverTime=std::min(idle.serverTime,latestPlayer.commandTime+maxStep);
     idle.forwardmove=forward;idle.rightmove=right;idle.upmove=up;
     for(int a=0;a<3;++a)idle.angles[a]=ANGLE2SHORT(latestPlayer.viewangles[a])-latestPlayer.delta_angles[a];
    }
    MSG_WriteByte(&message,clc_moveNoDelta);MSG_WriteByte(&message,1);
    const auto*latest=clc.serverCommands[clc.serverCommandSequence&(MAX_RELIABLE_COMMANDS-1)];
    MSG_WriteDeltaUsercmdKey(&message,clc.checksumFeed^clc.netchan.incomingSequence^ProbeHash(latest),&zero,&idle);
   }else if(action!="prime"){
    std::string text=action=="undo"?"jar_undo":"disconnect";
    if(action=="cleanup")text="jar_cleanup";
    if(action=="npcspawn")text="npc spawn stormtrooper2";
    if(action=="notarget")text="notarget";
    if(action=="spawn"){
     unsigned id;if(!(request>>id)||!id)return 2;
     text="jar_spawn "+std::to_string(id);
    }
    if(action=="equip"){
     unsigned id;if(!(request>>id)||!id)return 2;
     text="jar_equip "+std::to_string(id);
    }
    if(action=="setviewpos"){
     int x,y,z,yaw;
     if(!(request>>x>>y>>z>>yaw))return 2;
     text="setviewpos "+std::to_string(x)+' '+std::to_string(y)+' '+std::to_string(z)+' '+std::to_string(yaw);
    }
    if(action=="follow"){
     int slot;if(!(request>>slot)||slot<0||slot>=MAX_CLIENTS||slot==clc.clientNum)return 2;
     text="follow "+std::to_string(slot);
    }
    if(action=="select"){
     int number;if(!(request>>number))return 2;
     const auto target=std::find_if(latestProps.begin(),latestProps.end(),[number](const PropTarget&prop){return prop.number==number;});
     if(target==latestProps.end())return 2;
     vec3_t eye,direction;VectorCopy(latestPlayer.origin,eye);eye[2]+=latestPlayer.viewheight;
     VectorSubtract(target->center,eye,direction);if(VectorNormalize(direction)<1)return 2;
     char ray[160];
     Com_sprintf(ray,sizeof ray,"jar_select ray_%.4f_%.4f_%.4f_%.6f_%.6f_%.6f",
      eye[0],eye[1],eye[2],direction[0],direction[1],direction[2]);
     selectedEntity=-1;selectedIdentity=selectedCatalog=0;selectedMass=0;
     text=ray;
    }
    if(action=="activate"){
     unsigned identity;if(!(request>>identity)||!identity||identity!=selectedIdentity)return 2;
     text="jar_property activate "+std::to_string(identity);
    }
    if(action=="space_height"){
     unsigned height;if(!(request>>height)||height<8||height>8192||!selectedIdentity)return 2;
     text="jar_property controller_space_height_"+std::to_string(height)+' '+std::to_string(selectedIdentity);
    }
    if(action=="pure"){
     std::string proof;std::getline(request,proof);
     if(!pureServer||proof.empty()||proof.size()>MAX_INFO_VALUE-4||proof.find_first_not_of("0123456789-@ ")!=std::string::npos)return 2;
     text="cp"+proof;
    }
    const auto command=++clc.reliableSequence;
    Q_strncpyz(clc.reliableCommands[command&(MAX_RELIABLE_COMMANDS-1)],text.c_str(),MAX_STRING_CHARS);
    MSG_WriteByte(&message,clc_clientCommand);MSG_WriteLong(&message,command);MSG_WriteString(&message,text.c_str());
   }
   CL_Netchan_Transmit(&clc.netchan,&message);ProbePacket();
  }else if(action=="read"){
   std::string hex;request>>hex;
   if(hex.size()<8||hex.size()>MAX_PACKETLEN*2||hex.size()%2||hex.find_first_not_of("0123456789abcdef")!=std::string::npos)return 2;
   for(size_t i=0;i<hex.size();i+=2)output[i/2]=static_cast<byte>(std::stoi(hex.substr(i,2),nullptr,16));
   msg_t message;MSG_InitOOB(&message,output.data(),static_cast<int>(output.size()));message.cursize=static_cast<int>(hex.size()/2);
   if(!CL_Netchan_Process(&clc.netchan,&message)){std::cout<<"partial"<<std::endl;continue;}
   MSG_Bitstream(&message);MSG_ReadLong(&message);
   std::string response="message",dropReason;
   for(;;){
    const int command=MSG_ReadByte(&message);
    if(command==svc_EOF)break;
    if(command==svc_nop||command==svc_mapchange)continue;
    if(command==svc_setgame){gameDir=MSG_ReadString(&message);continue;}
    if(command==svc_serverCommand){
     const int sequence=MSG_ReadLong(&message);const char*text=MSG_ReadString(&message);
     if(sequence>clc.serverCommandSequence){
      clc.serverCommandSequence=sequence;Q_strncpyz(clc.serverCommands[sequence&(MAX_RELIABLE_COMMANDS-1)],text,MAX_STRING_CHARS);
      if(!strncmp(text,"jarstatus ",10))std::cerr<<text<<std::endl;
      if(!strcmp(text,"cp \"Forward crossing\""))++forwardCrossings;
      if(!strcmp(text,"cp \"Reverse crossing\""))++reverseCrossings;
      if(!strcmp(text,"cp \"Searchlight detected you\""))++searchlightNotices;
      if(!strncmp(text,"jarstats ",9))sandboxOwned=atoi(text+9);
      if(!strncmp(text,"jarequip ",9))equippedWeapon=atoi(text+9);
      if(!strncmp(text,"disconnect ",11))dropReason=text;
      if(!strncmp(text,"cs ",3)){
       std::istringstream fields(text+3);int index;
       if(fields>>index&&(index==CS_MUSIC||(index>=CS_SOUNDS&&index<CS_SOUNDS+MAX_SOUNDS)
                         ||(index>=CS_EFFECTS&&index<CS_EFFECTS+MAX_FX)
                         ||(index>=CS_MODELS&&index<CS_MODELS+MAX_MODELS)
                         ||(index>=CS_BSP_MODELS&&index<CS_BSP_MODELS+MAX_SUB_BSP)
                         ||(index>=constructStyle&&index<constructStyle+3))){
        const std::string update(text);const auto begin=update.find('"'),end=update.rfind('"');
        if(begin==std::string::npos||begin==end)return 3;
        config(index,update.substr(begin+1,end-begin-1).c_str());
       }
      }
      if(!strncmp(text,"jarselection ",13)){
       std::istringstream fields(text+13);int frame,frameCount,kind,capabilities,frozen,noGravity;
       if(!(fields>>selectedEntity>>frame>>frameCount>>selectedIdentity>>kind>>selectedCatalog>>capabilities>>frozen>>noGravity>>selectedMass))return 3;
      }
     }
     continue;
    }
    if(command==svc_gamestate){
     clc.serverCommandSequence=MSG_ReadLong(&message);
     std::string map;int protocol=0,count=0;music.clear();bspModels.fill({});sounds.fill({});effects.fill({});models.fill({});playerInfo.fill({});
     constructLight.fill({});switchClicks=previousSwitchEvent=0;
     for(;;){
      const int field=MSG_ReadByte(&message);
      if(field==svc_EOF)break;
      if(field==svc_configstring){
       const int index=MSG_ReadShort(&message);const char*value=MSG_ReadBigString(&message);
       if(index<0||index>=MAX_CONFIGSTRINGS)return 3;
       if(index==CS_SYSTEMINFO){
        serverId=atoi(Info_ValueForKey(value,"sv_serverid"));
        pureServer=atoi(Info_ValueForKey(value,"sv_pure"))!=0;
        pakNames=Info_ValueForKey(value,"sv_pakNames");
        pakSums=Info_ValueForKey(value,"sv_paks");
       }
       if(index==CS_SERVERINFO){map=Info_ValueForKey(value,"mapname");protocol=atoi(Info_ValueForKey(value,"protocol"));}
       config(index,value);
      }else if(field==svc_baseline){
       const int index=MSG_ReadBits(&message,GENTITYNUM_BITS);entityState_t empty{};
       if(index<0||index>=ENTITYNUM_WORLD||++count>ENTITYNUM_WORLD)return 3;
       MSG_ReadDeltaEntity(&message,&empty,&baselines[index],index);
      }else return 3;
     }
     clc.clientNum=MSG_ReadLong(&message);clc.checksumFeed=MSG_ReadLong(&message);
     if(MSG_ReadShort(&message)!=0||clc.clientNum<0||clc.clientNum>=MAX_CLIENTS||!serverId||protocol!=PROTOCOL_VERSION)return 3;
     sectionFixture=map=="jar_wire_section";
     constructMap=map=="gm_construct";
     response="gamestate "+map+" "+gameDir+" "+std::to_string(clc.clientNum)+" "+std::to_string(count)+" "+ProbeValue(music)+" "+ProbeBspSlots(bspModels);
     if(constructMap)
      for(const auto& value:constructLight)response+=' '+ProbeValue(value);
     continue;
    }
    if(command==svc_snapshot){
     const int time=MSG_ReadLong(&message),delta=MSG_ReadByte(&message),flags=MSG_ReadByte(&message),length=MSG_ReadByte(&message);
     if(delta||length<0||length>MAX_MAP_AREA_BYTES)return 3;
     byte areas[MAX_MAP_AREA_BYTES]{};MSG_ReadData(&message,areas,length);
     playerState_t player{};MSG_ReadDeltaPlayerstate(&message,nullptr,&player);
     if(player.m_iVehicleNum){playerState_t vehicle{};MSG_ReadDeltaPlayerstate(&message,nullptr,&vehicle,qtrue);}
     int count=0,last=-1;
     std::string lights,props,sections,speakers,torches,unclassified,bobbers,brushes,furniture,lifts,pickups,scenery,dust,searchlights;
     std::string constructMirror,portablePortals,angularMovers,sandboxActors;
     std::vector<PropTarget> propTargets;
     for(;;){
      const int index=MSG_ReadBits(&message,GENTITYNUM_BITS);
      if(index==ENTITYNUM_NONE)break;
      if(index<=last||index>=ENTITYNUM_WORLD)return 3;
      entityState_t current{};MSG_ReadDeltaEntity(&message,&baselines[index],&current,index);++count;last=index;
      const bool active=serverId&&!(flags&SNAPFLAG_NOT_ACTIVE);
      const bool sectionRoot=current.eType==ET_MOVER&&current.generic1==255;
      const bool owned=current.otherEntityNum>=MAX_CLIENTS&&current.otherEntityNum<ENTITYNUM_WORLD;
      const bool speaker=current.eType==ET_SPEAKER&&owned;
      if(constructMap&&active&&current.eType==ET_PORTAL&&VectorCompare(current.pos.trBase,current.origin2)){
       constructMirror=std::to_string(index);
       for(float coordinate:current.pos.trBase)constructMirror+=','+std::to_string(coordinate);
      }
      if(constructMap&&active&&current.eType==ET_SPEAKER&&current.eventParm>0&&current.eventParm<MAX_SOUNDS&&
         sounds[current.eventParm]=="sound/jg/gm_construct/room_switch.wav"){
       if((current.event&~EV_EVENT_BITS)==EV_GENERAL_SOUND&&current.event!=previousSwitchEvent)++switchClicks;
       previousSwitchEvent=current.event;
      }
      if(sectionFixture&&current.eType==ET_NPC){
       if(!sandboxActors.empty())sandboxActors+=';';
       sandboxActors+=std::to_string(index)+','+std::to_string(current.eFlags);
       for(float coordinate:current.pos.trBase)sandboxActors+=','+std::to_string(coordinate);
      }
      const bool torch=sectionFixture&&current.eType==ET_FX&&owned;
      const bool bobber=sectionFixture&&current.eType==ET_MOVER&&!sectionRoot&&owned&&current.pos.trType==TR_SINE;
      const bool breakable=sectionFixture&&current.eType==ET_MOVER&&current.generic1==249&&owned;
      const bool lift=sectionFixture&&current.eType==ET_MOVER&&!sectionRoot&&owned&&!bobber&&!breakable;
      const bool pickup=sectionFixture&&current.eType==ET_ITEM&&owned;
      const bool scenic=sectionFixture&&JAR_MP_SectionScenery(&current);
      const bool dustEvent=sectionFixture&&owned&&current.eType==ET_EVENTS+EV_PLAY_EFFECT_ID&&
        current.eventParm>0&&current.eventParm<MAX_FX&&!Q_stricmp(effects[current.eventParm].c_str(),"chunks/dustFall.efx");
      if(active&&sectionRoot){
       if(current.solid!=SOLID_BMODEL){
        std::cerr<<"Portable BSP root arrived without a solid brush model\n";return 3;
       }
       if(!sections.empty())sections+=';';
       sections+=std::to_string(index)+','+std::to_string(current.modelindex);
       for(float coord:current.pos.trBase)sections+=','+std::to_string(coord);
       sections+=','+std::to_string(current.apos.trBase[YAW]);
      }
      if(active&&speaker){
       if(current.loopSound<=0||current.loopSound>=MAX_SOUNDS||current.loopSound!=current.eventParm||
          current.loopIsSoundset||sounds[current.loopSound].empty()){
        std::cerr<<"Portable speaker arrived without its localized native sound configstring\n";return 3;
       }
       if(!speakers.empty())speakers+=';';
       speakers+=std::to_string(index)+','+std::to_string(current.otherEntityNum)+','+
        std::to_string(current.loopSound)+','+ProbeValue(sounds[current.loopSound]);
       for(float coord:current.pos.trBase)speakers+=','+std::to_string(coord);
      }
      if(active&&torch){
       const int effect=current.modelindex,sound=current.loopSound;
       if(effect<=0||effect>=MAX_FX||effects[effect].empty()||sound<=0||sound>=MAX_SOUNDS||
          sounds[sound].empty()||current.loopIsSoundset||current.modelindex2!=FX_STATE_CONTINUOUS||
          current.speed!=200||current.time!=0){
        std::cerr<<"Portable torch arrived without its native effect, loop or cadence\n";return 3;
       }
       if(!torches.empty())torches+=';';
       torches+=std::to_string(index)+','+std::to_string(current.otherEntityNum)+','+
        std::to_string(effect)+','+ProbeValue(effects[effect])+','+std::to_string(sound)+','+ProbeValue(sounds[sound]);
       for(float coord:current.pos.trBase)torches+=','+std::to_string(coord);
       vec3_t direction;AngleVectors(current.angles,direction,nullptr,nullptr);
       for(float coord:direction)torches+=','+std::to_string(coord);
      }
      if(active&&bobber){
       const int model=current.modelindex2;
       if(current.solid!=SOLID_BMODEL||model<=0||model>=MAX_MODELS||
          models[model].empty()||current.pos.trDuration!=3000){
        std::cerr<<"Portable bobber arrived without its native brush, visual model or cadence\n";return 3;
       }
       if(!bobbers.empty())bobbers+=';';
       bobbers+=std::to_string(index)+','+std::to_string(current.otherEntityNum)+','+
        std::to_string(current.modelindex)+','+std::to_string(model)+','+ProbeValue(models[model])+','+
        std::to_string(current.pos.trTime)+','+std::to_string(current.pos.trDuration);
       for(float coord:current.pos.trBase)bobbers+=','+std::to_string(coord);
       for(float coord:current.pos.trDelta)bobbers+=','+std::to_string(coord);
       bobbers+=','+std::to_string(current.apos.trBase[YAW]);
      }
      if(active&&breakable){
       if(current.solid!=SOLID_BMODEL||current.pos.trType!=TR_STATIONARY||current.apos.trType!=TR_STATIONARY){
        std::cerr<<"Portable breakable arrived without its stationary solid brush model\n";return 3;
       }
       if(!brushes.empty())brushes+=';';
       brushes+=std::to_string(index)+','+std::to_string(current.otherEntityNum)+','+std::to_string(current.modelindex);
       for(float coord:current.pos.trBase)brushes+=','+std::to_string(coord);
       brushes+=','+std::to_string(current.apos.trBase[YAW]);
      }
      if(active&&lift){
       if(current.solid!=SOLID_BMODEL||current.apos.trType!=TR_STATIONARY)return 3;
       if(!lifts.empty())lifts+=';';
       lifts+=std::to_string(index)+','+std::to_string(current.otherEntityNum)+','+std::to_string(current.modelindex)+','+
        std::to_string(current.pos.trType)+','+std::to_string(current.pos.trDuration);
       for(float coord:current.pos.trBase)lifts+=','+std::to_string(coord);
      }
      if(active&&pickup){
       if(!pickups.empty())pickups+=';';
       pickups+=std::to_string(index)+','+std::to_string(current.otherEntityNum)+','+std::to_string(current.modelindex)+','+
        std::to_string(current.eFlags);
       for(float coord:current.pos.trBase)pickups+=','+std::to_string(coord);
      }
      if(active&&scenic){
       const int model=current.modelindex;
       if(current.solid||model<=0||model>=MAX_MODELS||models[model].empty())return 3;
       if(!scenery.empty())scenery+=';';
       scenery+=std::to_string(index)+','+std::to_string(current.otherEntityNum)+','+ProbeValue(models[model]);
       for(float coord:current.pos.trBase)scenery+=','+std::to_string(coord);
       for(float scale:current.origin2)scenery+=','+std::to_string(scale);
       scenery+=','+std::to_string(current.apos.trBase[YAW]);
      }
      if(active&&dustEvent){
       if(!dust.empty())dust+=';';
       dust+=std::to_string(index)+','+std::to_string(current.otherEntityNum);
       for(float coord:current.pos.trBase)dust+=','+std::to_string(coord);
       vec3_t direction;AngleVectors(current.angles,direction,nullptr,nullptr);
       for(float coord:direction)dust+=','+std::to_string(coord);
      }
      if(active&&sectionFixture&&index>=MAX_CLIENTS&&current.eType<ET_EVENTS&&
         !sectionRoot&&!speaker&&!torch&&!bobber&&!breakable&&!lift&&!pickup&&!scenic){
       if(!unclassified.empty())unclassified+=';';
       unclassified+=std::to_string(index)+','+std::to_string(current.eType)+','+
        std::to_string(current.modelindex)+','+std::to_string(current.otherEntityNum);
      }
      if(current.eType==ET_GENERAL&&current.generic1==250){
       if(JAR_MP_PortalSurface(&current)){
        if(!portablePortals.empty())portablePortals+=';';
        portablePortals+=std::to_string(index)+','+std::to_string(current.time)+','+
         std::to_string(current.speed)+','+std::to_string(current.eventParm);
        for(float coordinate:current.pos.trBase)portablePortals+=','+std::to_string(coordinate);
        for(float coordinate:current.origin)portablePortals+=','+std::to_string(coordinate);
       }
       if(JAR_MP_Searchlight(&current)){
        if(!searchlights.empty())searchlights+=';';
        searchlights+=std::to_string(index)+','+std::to_string(current.time)+','+std::to_string(current.eventParm);
        for(float coordinate:current.pos.trBase)searchlights+=','+std::to_string(coordinate);
        for(float coordinate:current.origin)searchlights+=','+std::to_string(coordinate);
       }
       if(JAR_MP_AngularMover(&current)){
        if(!angularMovers.empty())angularMovers+=';';
        angularMovers+=std::to_string(index);
        for(float angle:current.apos.trBase)angularMovers+=','+std::to_string(angle);
       }
       if(!props.empty())props+=',';
       props+=std::to_string(index);
       PropTarget target{};target.number=index;
       for(int i=0;i<3;++i)target.center[i]=current.pos.trBase[i]+(current.origin2[i]+current.angles2[i])*.5f;
       propTargets.push_back(target);
       if(current.modelindex>0&&current.modelindex<MAX_MODELS&&
          models[current.modelindex].rfind("models/jg/gm_construct/p",0)==0){
        vec3_t mins,maxs;
        if(!current.solid||!JAR_MP_PropBounds(&current,mins,maxs))return 3;
        if(!furniture.empty())furniture+=';';
        furniture+=std::to_string(index)+','+ProbeValue(models[current.modelindex]);
        for(float coord:current.pos.trBase)furniture+=','+std::to_string(coord);
        for(float coord:mins)furniture+=','+std::to_string(coord);
        for(float coord:maxs)furniture+=','+std::to_string(coord);
       }
       if(current.constantLight==0x28ffb27f){
        if(!lights.empty())lights+=',';
        lights+=std::to_string(index);
       }
      }
     }
     const bool following=(player.pm_flags&PMF_FOLLOW)!=0;
     if(!(flags&SNAPFLAG_NOT_ACTIVE)&&time>0&&(player.clientNum==clc.clientNum||following)&&player.stats[STAT_HEALTH]>0){
      snapshotTime=time;latestPlayer=player;latestProps=std::move(propTargets);
      response="snapshot "+std::to_string(time)+" "+std::to_string(player.clientNum)+" "+std::to_string(count)+" "+std::to_string(player.stats[STAT_HEALTH])+" "+(lights.empty()?"-":lights)+" "+std::to_string(player.viewangles[PITCH])+" "+std::to_string(following)+" "+std::to_string(player.viewangles[YAW])+" "+ProbeValue(music)+" "+std::to_string(selectedEntity)+" "+std::to_string(selectedIdentity)+" "+std::to_string(selectedCatalog)+" "+(props.empty()?"-":props);
      response+=' '+ProbeBspSlots(bspModels)+' '+(sections.empty()?"-":sections)+' '+(speakers.empty()?"-":speakers);
      response+=' '+(torches.empty()?"-":torches)+' '+(unclassified.empty()?"-":unclassified);
      response+=' ';
      for(float coord:player.origin)response+=std::to_string(coord)+',';
      response+=std::to_string(player.viewheight)+' '+(bobbers.empty()?"-":bobbers)+' '+(brushes.empty()?"-":brushes);
      response+=' '+(furniture.empty()?"-":furniture);
      response+=' '+std::to_string(selectedMass)+' '+std::to_string(equippedWeapon)+' '+std::to_string(player.stats[STAT_JAR_PHYSGUN_HOLD]);
      const auto ladderAnim=[](int anim){
       switch(anim){
        case BOTH_LADDER_UP1:return "up";
        case BOTH_LADDER_DWN1:return "down";
        case BOTH_LADDER_IDLE:return "idle";
        case BOTH_HANG_IDLE:return "hang";
        default:return "-";
       }
      };
      response+=' '+std::to_string(player.velocity[2])+' '+ladderAnim(player.legsAnim)+' '+ladderAnim(player.torsoAnim);
      response+=' '+std::to_string(player.commandTime);
      response+=' '+(lifts.empty()?"-":lifts)+' '+(pickups.empty()?"-":pickups)+' '+(scenery.empty()?"-":scenery);
      response+=' '+(dust.empty()?"-":dust);
      response+=' '+std::to_string(player.gravity)+' '+std::to_string(player.stats[STAT_JAR_ACTOR_FRICTION]);
      response+=' '+std::to_string(forwardCrossings)+' '+std::to_string(reverseCrossings);
      response+=' '+(searchlights.empty()?"-":searchlights)+' '+std::to_string(searchlightNotices);
      if(constructMap){
       for(const auto& value:constructLight)response+=' '+ProbeValue(value);
       response+=' '+std::to_string(switchClicks);
       response+=' '+(constructMirror.empty()?"-":constructMirror);
      }
      if(sectionFixture)response+=' '+(portablePortals.empty()?"-":portablePortals)+' '+(angularMovers.empty()?"-":angularMovers);
      if(sectionFixture)response+=' '+std::to_string(sandboxOwned)+' '+(sandboxActors.empty()?"-":sandboxActors);
     }
     continue;
    }
    return 3;
   }
   if(message.readcount>message.cursize)return 3;
   std::cout<<(dropReason.empty()?response:dropReason)<<std::endl;
  }else return 2;
 }
 return 0;
}
