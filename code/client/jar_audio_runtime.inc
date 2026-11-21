// Separate bounded cache: catalog entries never consume registered sound handles.
namespace {
struct JarAudioEntry {std::string name,reason;bool checked=false;};
struct JarAudioSlot {sfx_t sound={};int entry=-1;uint64_t touched=0;};
std::vector<JarAudioEntry> jarAudioCatalog;
JarAudioSlot jarAudioSlots[128];
uint64_t jarAudioTouch=0;

int jarAudioValidateCursor=0;
sfx_t *jarAudioMusic=nullptr;
int jarAudioMusicPosition=0;
struct JarMovieAudio {sfx_t *sound=nullptr;uint64_t frames=0;bool attempted=false;};
JarMovieAudio jarMovieAudio[16];
int jarVoiceEnd[MAX_GENTITIES]={};
bool JarAudioBusy(const sfx_t *s) {
    if(jarAudioMusic==s)return true;
    for(const auto& movie:jarMovieAudio)if(movie.sound==s)return true;
    for(int i=0;i<MAX_CHANNELS;++i)if(s_channels[i].thesfx==s)return true;
    for(int i=0;i<numLoopSounds;++i)if(loopSounds[i].sfx==s)return true;
    return false;
}
void JarAudioEnsureCatalog() {
    if(!jarAudioCatalog.empty())return;
    std::vector<std::string> names;FS_JAR_ListAudioNames(names);
    for(auto& name:names)name=jar::CanonicalKey(name.c_str());
    std::sort(names.begin(),names.end());names.erase(std::unique(names.begin(),names.end()),names.end());
    for(auto& n:names){JarAudioEntry e;e.name=n;
        if(n.size()>=MAX_QPATH)e.reason="name-exceeds-engine-MAX_QPATH";
        else if(n.find_first_of("\r\n\t")!=std::string::npos)e.reason="control-character-in-name";
        else if(n.size()<4||(n.compare(n.size()-4,4,".wav")&&n.compare(n.size()-4,4,".mp3")&&n.compare(n.size()-4,4,".roq")))e.reason="codec-not-supported-by-native-decoder";
        e.checked=!e.reason.empty();jarAudioCatalog.push_back(std::move(e));}
    Cvar_Get("jar_audioSequence","0",CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);
    Cvar_Get("jar_audioSequenceSeed","",CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);
    JAR_Log(0,"randomizer","audio_catalog","discovered=%u unified=1 lazy_decode=1 cache_slots=128 cache_MiB=128",unsigned(jarAudioCatalog.size()));
}
void JarAudioFreeSlot(JarAudioSlot& slot){SND_FreeSFXMem(&slot.sound);slot.sound={};slot.entry=-1;slot.touched=0;}
JarAudioSlot *JarAudioOldest() {
    JarAudioSlot *out=nullptr;for(auto& slot:jarAudioSlots)if(!JarAudioBusy(&slot.sound)&&(!out||slot.touched<out->touched))out=&slot;return out;
}
sfx_t *JarAudioLoad(unsigned index) {
    auto& entry=jarAudioCatalog[index];if(!entry.reason.empty())return nullptr;
#ifdef USE_OPENAL
    if(s_UseOpenAL){JAR_Log(1,"randomizer","audio_backend_unsupported","backend=OpenAL reason=custom-PCM-cache-requires-DMA");return nullptr;}
#endif
    for(auto& slot:jarAudioSlots)if(slot.entry==int(index)){if(slot.sound.bInMemory&&slot.sound.pSoundData){slot.touched=++jarAudioTouch;return &slot.sound;}JarAudioFreeSlot(slot);}
    auto slot=JarAudioOldest();if(!slot)return nullptr;
    JarAudioFreeSlot(*slot);
    // Reserve a worst-case decode while retaining every live channel/loop/music.
    for(;;){uint64_t bytes=0;for(const auto& s:jarAudioSlots)bytes+=uint64_t(s.sound.iSoundLengthInSamples)*2;
        if(bytes<=64u*1024u*1024u)break;
        auto old=JarAudioOldest();
        // Empty slots sort first, so choose oldest populated idle slot here.
        old=nullptr;for(auto& s:jarAudioSlots)if(s.entry>=0&&!JarAudioBusy(&s.sound)&&(!old||s.touched<old->touched))old=&s;
        if(!old)return nullptr;JarAudioFreeSlot(*old);}
    const char *error=S_JAR_DecodeCatalog(entry.name.c_str(),&slot->sound);entry.checked=true;
    if(error){entry.reason=error;JAR_Log(1,"randomizer","audio_unsupported","file=%s reason=%s",entry.name.c_str(),error);return nullptr;}
    slot->entry=int(index);slot->touched=++jarAudioTouch;return &slot->sound;
}
sfx_t *JarAudioChoose(uint32_t seed,uint32_t& sequence,const char *domain,const char *source,int role) {
    bool fixed;if(!jar::ParseRandomMode(Cvar_VariableString("jar_randomMode"),fixed))return nullptr;
    JarAudioEnsureCatalog();const unsigned count=unsigned(jarAudioCatalog.size());
    uint32_t fixedSequence=jar::FixedDecision(seed,"audio-v1",jar::AudioSourceKey(source).c_str());
    for(unsigned attempt=0;attempt<std::min(count,64u);++attempt){const unsigned index=fixed ?
        jar::CatalogIndex(seed,fixedSequence++,count,"fixed-audio-candidates-v1") : jar::CatalogIndex(seed,sequence++,count,domain);
        if(auto selected=JarAudioLoad(index)){
            if(role!=2 || selected->iLastTimeUsed!=int(seed+1)) {
                JAR_Log(2,"randomizer","sound_choice","seed=%u sequence=%u role=%d source=%s chosen=%s candidates=%u samples=%d mode=%s",seed,sequence,role,source,selected->sSoundName,count,selected->iSoundLengthInSamples,fixed?"fixed":"reroll");
                selected->iLastTimeUsed=int(seed+1);}
            return selected;}
        // Temporary channel/cache pressure must never reroll a fixed mapping.
        // Only a deterministic decoder/format rejection may advance its choice.
        if(fixed && jarAudioCatalog[index].reason.empty()) {
            if(role!=2)JAR_Log(1,"randomizer","audio_fallback","source=%s reason=fixed-destination-temporarily-unavailable chosen=%s",source,jarAudioCatalog[index].name.c_str());
            return nullptr;
        }
    }
    JAR_Log(1,"randomizer","audio_fallback","source=%s reason=no-decodable-idle-cache-entry-within-64-attempts",source);return nullptr;
}
}
static void S_JAR_AudioCatalog_f() {
    JarAudioEnsureCatalog();fileHandle_t f=FS_FOpenFileWrite("jar-audio/catalog.tsv");if(!f)return;
    const char *header="file\tstatus\treason\n";FS_Write(header,strlen(header),f);
    unsigned rejected=0,validated=0;for(const auto& e:jarAudioCatalog){rejected+=!e.reason.empty();validated+=e.checked&&e.reason.empty();
        const std::string line=e.name+"\t"+(e.reason.empty()?(e.checked?"supported-PCM-all-roles":"pending-decode"):"unsupported")+"\t"+e.reason+"\n";FS_Write(line.data(),int(line.size()),f);}
    FS_FCloseFile(f);Com_Printf("JA:R audio catalog: %u files, %u decoded, %u unsupported; jar-audio/catalog.tsv\n",unsigned(jarAudioCatalog.size()),validated,rejected);
    JAR_Log(0,"randomizer","audio_coverage","discovered=%u validated=%u rejected=%u backend=DMA-unified-PCM",unsigned(jarAudioCatalog.size()),validated,rejected);
}
static void S_JAR_AudioValidate_f() {
    JarAudioEnsureCatalog();const int count=std::max(1,std::min(32,atoi(Cmd_Argv(1))));
    for(int i=0;i<count&&jarAudioValidateCursor<int(jarAudioCatalog.size());++i,++jarAudioValidateCursor)JarAudioLoad(jarAudioValidateCursor);
    Com_Printf("JA:R audio decode audit: %d/%u\n",jarAudioValidateCursor,unsigned(jarAudioCatalog.size()));
}
static void S_JAR_FreeAudioCache() {
    jarAudioMusic=nullptr;
    // An in-flight movie falls back to native audio after restart; never rewind it.
    for(auto& movie:jarMovieAudio)movie.sound=nullptr;
    for(auto& slot:jarAudioSlots)JarAudioFreeSlot(slot);
    jarAudioCatalog.clear();jarAudioValidateCursor=0;std::memset(jarVoiceEnd,0,sizeof(jarVoiceEnd));
}
static sfx_t *S_JAR_RandomSound(sfx_t *original,soundChannel_t channel,bool looping) {
    if(!original||!original->sSoundName[0]||original->sSoundName[0]=='*'||!Q_stricmp(original->sSoundName,"sound/null.wav")||!Q_stricmp(original->sSoundName,"sound/null"))return original;
    const bool voice=JarVoiceChannel(channel)||strstr(original->sSoundName,"/chars/")||strstr(original->sSoundName,"/chr_");
    const int option=looping?2:voice?4:jar::AudioRole(original->sSoundName)==jar::SoundRole::Interface?1:0;
    uint32_t seed;if(std::strcmp(Cvar_VariableString(jar::soundCvars[option]),"1")||!jar::ParsePickupSeed(Cvar_VariableString("jar_randomSeed"),seed))return original;
    JarAudioEnsureCatalog();
    if(strcmp(Cvar_VariableString("jar_audioSequenceSeed"),va("%u",seed))){Cvar_Set("jar_audioSequenceSeed",va("%u",seed));Cvar_Set("jar_audioSequence","0");}
    uint32_t sequence=0;
    if(looping)sequence=jar::PickupHashText(2166136261u,original->sSoundName);
    else jar::ParsePickupSeed(Cvar_VariableString("jar_audioSequence"),sequence);
    auto chosen=JarAudioChoose(seed,sequence,looping?"all-audio-loops-v2":"all-audio-events-v2",original->sSoundName,option);
    if(!looping)Cvar_Set("jar_audioSequence",va("%u",sequence&2147483647u));
    if(chosen)return chosen;
    return original;
}
#include "jar_movie_audio.inc"
static bool S_JAR_StartMusic(const char *source,bool preloading) {
    jarAudioMusic=nullptr; // Native fallback/restore must release custom playback.
    uint32_t seed;if(strcmp(Cvar_VariableString("jar_randomMusic"),"1")||!jar::ParsePickupSeed(Cvar_VariableString("jar_randomSeed"),seed))return false;
    // A blank configstring asks for the native map default, not silence.
    // Resolve only a valid native set; missing labels retain native fallback.
    char literal[MAX_QPATH];Q_strncpyz(literal,source?source:"",sizeof(literal));
    COM_DefaultExtension(literal,sizeof(literal),".mp3");
    if(std::strchr(literal,'/') && S_FileExists(literal))source=literal;
    else {
        if(!Music_DynamicDataAvailable(source))return false;
        source=Music_GetFileNameForState(eBGRNDTRACK_EXPLORE);
        if(!source || !*source || !S_FileExists(source))return false;
    }
    uint32_t sequence=jar::PickupHashText(2166136261u,source);
    auto chosen=JarAudioChoose(seed,sequence,"all-audio-music-v2",source,3);if(!chosen)return false;
    S_StopBackgroundTrack();jarAudioMusic=preloading?nullptr:chosen;jarAudioMusicPosition=0;
    Q_strncpyz(sMusic_BackgroundLoop,chosen->sSoundName,sizeof(sMusic_BackgroundLoop));return true;
}
static void S_JAR_UpdateMusic(bool silent) {
    if(s_rawend<s_soundtime)s_rawend=s_soundtime;
    while(s_rawend<s_soundtime+MAX_RAW_SAMPLES){const int count=std::min(MAX_RAW_SAMPLES-(s_rawend-s_soundtime),jarAudioMusic->iSoundLengthInSamples-jarAudioMusicPosition);
        S_RawSamples(count,dma.speed,2,1,reinterpret_cast<byte *>(jarAudioMusic->pSoundData+jarAudioMusicPosition),silent?0:s_musicVolume->value,qtrue);
        jarAudioMusicPosition=(jarAudioMusicPosition+count)%jarAudioMusic->iSoundLengthInSamples;}
}
