#include "client/client.h"
#include "client/snd_local.h"
#include "qcommon/jar_audio_catalog.h"
#include "test_support.h"
#include <algorithm>
#include <vector>
dma_t dma={};
static int s_soundStarted=1,s_soundMuted=0;
static bool enabled=true;
static unsigned choices=0;
static sfx_t selected={};
static std::vector<short> mixed;
struct JarMovieAudio {sfx_t *sound=nullptr;uint64_t frames=0;bool attempted=false;};
static JarMovieAudio jarMovieAudio[16];
static const char *MovieTestCvar(const char *name){return !std::strcmp(name,"jar_randomSeed")?"73":enabled?"1":"0";}
static void MovieTestLog(int,const char *,const char *,const char *,...){}
static sfx_t *JarAudioChoose(uint32_t,uint32_t&,const char *,const char *,int){++choices;return &selected;}
static void MovieTestRaw(int count,int rate,int width,int channels,byte *data,float,qboolean){
    CHECK(rate==dma.speed&&width==2&&channels==1);
    const auto pcm=reinterpret_cast<short *>(data);mixed.insert(mixed.end(),pcm,pcm+count);
}
#define Cvar_VariableString MovieTestCvar
#define JAR_Log MovieTestLog
#define S_RawSamples MovieTestRaw
#include "../../code/client/jar_movie_audio.inc"
int main(){return RunTests("native cinematic audio helper timing, tails, rewind and restart fallback",[]{
    std::vector<short> pcm(1000,1234);selected.pSoundData=pcm.data();selected.iSoundLengthInSamples=int(pcm.size());
    for(int rate:{11025,22050,44100,48000}) {
        dma.speed=rate;mixed.clear();choices=0;enabled=true;S_JAR_ResetCinematicAudio(0);
        uint64_t input=0;
        for(int count:{1,13,1023,2049,5003}) {
            CHECK(S_JAR_CinematicSamples(0,"video/test.roq",count,1));input+=count;
            CHECK(mixed.size()==input*rate/22050);
        }
        CHECK(choices==1&&jarMovieAudio[0].sound==&selected);
        for(size_t i=0;i<mixed.size();++i)CHECK(mixed[i]==(i<pcm.size()?1234:0));
        S_JAR_RewindCinematicAudio(0);mixed.clear();CHECK(S_JAR_CinematicSamples(0,"video/test.roq",100,1));CHECK(choices==1);
        CHECK(mixed.size()==uint64_t(100)*rate/22050);for(auto n:mixed)CHECK(n==1234);
        enabled=false;CHECK(!S_JAR_CinematicSamples(0,"video/test.roq",100,1));CHECK(jarMovieAudio[0].frames==200);
        enabled=true;CHECK(S_JAR_CinematicSamples(0,"video/test.roq",100,1));CHECK(choices==1&&jarMovieAudio[0].frames==300);
        // Sound restart frees PCM after clearing these pointers; no mid-movie reselection.
        jarMovieAudio[0].sound=nullptr;CHECK(!S_JAR_CinematicSamples(0,"video/test.roq",100,1));CHECK(choices==1);
        S_JAR_ResetCinematicAudio(0);CHECK(S_JAR_CinematicSamples(0,"video/next.roq",1,1));CHECK(choices==2);
    }
    CHECK(!S_JAR_CinematicSamples(-1,"x",1,1));CHECK(!S_JAR_CinematicSamples(16,"x",1,1));
});}
