// Exercise the actual runtime selector/cache and every audio entry path. Native
// decoding is replaced by bounded PCM fixtures, including permanent rejection.
#include "client/client.h"
#include "client/snd_local.h"
#include "client/jar_audio_channel_policy.h"
#include "qcommon/jar_audio_catalog.h"
#include "qcommon/jar_sound_randomizer.h"
#include "qcommon/jar_diagnostics.h"
#include "test_support.h"
#include <algorithm>
#include <map>
#include <set>
#include <vector>
dma_t dma={};channel_t s_channels[MAX_CHANNELS]={};
int s_rawend=0,s_soundtime=0;
static int s_soundStarted=1,s_soundMuted=0,numLoopSounds=0;
static struct {sfx_t *sfx;} loopSounds[64];
static cvar_t musicVolume={};static cvar_t *s_musicVolume=&musicVolume;
static char sMusic_BackgroundLoop[MAX_QPATH];
static std::vector<std::string> files;
static std::set<std::string> unsupported;
static std::map<std::string,std::string> vars;
static unsigned decoded=0,mixed=0;
static const char *FixtureCvarString(const char *name){return vars[name].c_str();}
static cvar_t *FixtureCvarGet(const char *name,const char *initial,int){vars.emplace(name,initial);return nullptr;}
static void FixtureCvarSet(const char *name,const char *value){vars[name]=value;}
static void FixtureFiles(std::vector<std::string>& out){out=files;}
static void FixtureFree(sfx_t *s){delete[] s->pSoundData;s->pSoundData=nullptr;}
static const char *S_JAR_DecodeCatalog(const char *name,sfx_t *s) {
    ++decoded;if(unsupported.count(name))return "fixture-codec-rejected";
    Q_strncpyz(s->sSoundName,name,sizeof(s->sSoundName));s->iSoundLengthInSamples=1000;
    s->pSoundData=new short[1000]();s->bInMemory=qtrue;return nullptr;
}
static bool S_FileExists(const char *name){return std::find(files.begin(),files.end(),name)!=files.end();}
static bool Music_DynamicDataAvailable(const char *source){return !source||!*source||!strcmp(source,"fixture_set");}
static const char *FixtureMusicFile(int){return "music/fixture.mp3";}
static void FixtureStopMusic(){}
static void FixtureExtension(char *name,int size,const char *ext){if(!strchr(name,'.'))Q_strcat(name,size,ext);}
static void FixtureRaw(int count,int,int,int,byte *,float,qboolean){mixed+=count;s_rawend+=count;}
#define Cvar_VariableString FixtureCvarString
#define Cvar_Get FixtureCvarGet
#define Cvar_Set FixtureCvarSet
#define FS_JAR_ListAudioNames FixtureFiles
#define SND_FreeSFXMem FixtureFree
#define Music_GetFileNameForState FixtureMusicFile
#define eBGRNDTRACK_EXPLORE 0
#define S_StopBackgroundTrack FixtureStopMusic
#define COM_DefaultExtension FixtureExtension
#define S_RawSamples FixtureRaw
#include "../../code/client/jar_audio_runtime.inc"

int main(){return RunTests("fixed audio: real runtime selection, canonical keys, cache pressure, decode failures, all roles and save/reload state",[]{
    dma.speed=22050;vars["jar_randomMode"]="0";vars["jar_randomSeed"]="73";
    for(const char *toggle:jar::soundCvars)vars[toggle]="1";
    for(int i=0;i<200;++i)files.push_back("sound/fixture"+std::to_string(i)+".wav");
    files.push_back("music/fixture.mp3");files.push_back("video/test.roq");
    auto choose=[&](const char *source,int role=0){uint32_t seq=999;auto s=JarAudioChoose(73,seq,"legacy-domain",source,role);CHECK(s&&seq==999);return std::string(s->sSoundName);};
    const auto expected=choose("sound/player/footsteps/metal1");
    for(int i=0;i<100;++i){choose(files[i].c_str(),i%5);CHECK(choose("SOUND\\PLAYER\\FOOTSTEPS\\METAL1.wav",i%5)==expected);}
    CHECK(jar::AudioSourceKey("Sound\\Source.MP3")=="sound/source");
    auto savedVars=vars;vars["jar_randomSeed"]="74";vars["jar_randomMode"]="1";vars=savedVars;
    std::reverse(files.begin(),files.end());S_JAR_FreeAudioCache();
    CHECK(choose("sound/player/footsteps/metal1.mp3")==expected);
    sfx_t original={};Q_strncpyz(original.sSoundName,"sound/player/footsteps/metal1",MAX_QPATH);
    for(auto channel:{CHAN_AUTO,CHAN_VOICE,CHAN_LOCAL}) {
        CHECK(S_JAR_RandomSound(&original,channel,false)!=&original);
        CHECK(std::string(S_JAR_RandomSound(&original,channel,false)->sSoundName)==expected);
    }
    CHECK(std::string(S_JAR_RandomSound(&original,CHAN_AUTO,true)->sSoundName)==expected);
    CHECK(vars["jar_audioSequence"]=="0");
    vars["jar_randomEffects"]="0";CHECK(S_JAR_RandomSound(&original,CHAN_AUTO,false)==&original);vars["jar_randomEffects"]="1";
    sfx_t silence={};Q_strncpyz(silence.sSoundName,"sound/null",MAX_QPATH);CHECK(S_JAR_RandomSound(&silence,CHAN_AUTO,false)==&silence);
    // Temporary cache exhaustion retains its mapping: native fallback, then the
    // original fixed destination once a slot is free; no candidate retry.
    S_JAR_FreeAudioCache();JarAudioEnsureCatalog();
    for(unsigned i=0;i<128;++i)s_channels[i].thesfx=&jarAudioSlots[i].sound;
    const auto before=decoded;CHECK(S_JAR_RandomSound(&original,CHAN_AUTO,false)==&original);CHECK(decoded==before);
    for(auto& c:s_channels)c={};CHECK(choose(original.sSoundName)==expected);
    // A permanent content rejection can advance deterministically, including
    // after a fresh catalog/cache rebuild and different occurrence order.
    unsupported.insert(expected);S_JAR_FreeAudioCache();const auto alternate=choose(original.sSoundName);CHECK(alternate!=expected);
    S_JAR_FreeAudioCache();choose("other");CHECK(choose(original.sSoundName)==alternate);
    unsupported.clear();S_JAR_FreeAudioCache();CHECK(choose(original.sSoundName)==expected);
    auto oldFiles=files;files.erase(std::remove(files.begin(),files.end(),expected),files.end());S_JAR_FreeAudioCache();const auto changed=choose(original.sSoundName);CHECK(changed!=expected);
    std::reverse(files.begin(),files.end());S_JAR_FreeAudioCache();CHECK(choose(original.sSoundName)==changed);files=oldFiles;S_JAR_FreeAudioCache();CHECK(choose(original.sSoundName)==expected);
    // Music literal/default-set paths share the source identity used by SFX.
    const auto music=choose("music/fixture");CHECK(S_JAR_StartMusic("music/fixture.mp3",false));CHECK(std::string(jarAudioMusic->sSoundName)==music);
    CHECK(S_JAR_StartMusic("fixture_set",false));CHECK(std::string(jarAudioMusic->sSoundName)==music);
    CHECK(S_JAR_StartMusic("",false));CHECK(std::string(jarAudioMusic->sSoundName)==music);
    CHECK(!S_JAR_StartMusic("missing_set",false));
    const auto movie=choose("video/test.roq");CHECK(S_JAR_CinematicSamples(0,"video/test.roq",300,1));CHECK(std::string(jarMovieAudio[0].sound->sSoundName)==movie);
    S_JAR_ResetCinematicAudio(0);CHECK(S_JAR_CinematicSamples(0,"VIDEO\\TEST.ROQ",300,1));CHECK(std::string(jarMovieAudio[0].sound->sSoundName)==movie&&mixed==600);
    S_JAR_ResetCinematicAudio(0);
    std::set<std::string> seeds;for(unsigned seed=0;seed<60;++seed){uint32_t seq=0;auto s=JarAudioChoose(seed,seq,"unused",original.sSoundName,0);CHECK(s);seeds.insert(s->sSoundName);}CHECK(seeds.size()>30);
    // The legacy cycle remains selectable and resumes from saved counters.
    vars["jar_randomMode"]="1";vars["jar_audioSequence"]="0";vars["jar_audioSequenceSeed"]="73";
    std::set<std::string> cycle;for(unsigned i=0;i<files.size();++i)cycle.insert(S_JAR_RandomSound(&original,CHAN_AUTO,false)->sSoundName);CHECK(cycle.size()==files.size());
    savedVars=vars;auto next=std::string(S_JAR_RandomSound(&original,CHAN_AUTO,false)->sSoundName);vars=savedVars;S_JAR_FreeAudioCache();CHECK(std::string(S_JAR_RandomSound(&original,CHAN_AUTO,false)->sSoundName)==next);
    vars["jar_randomMode"]="0";CHECK(choose(original.sSoundName)==expected);
    vars["jar_randomMode"]="invalid";CHECK(S_JAR_RandomSound(&original,CHAN_AUTO,false)==&original);
    S_JAR_FreeAudioCache();
});}
