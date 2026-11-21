#include "client/client.h"
#include "client/snd_local.h"
#include <algorithm>
#include "client/jar_audio_channel_policy.h"
#include "qcommon/jar_face.h"
#include "test_support.h"
int s_soundtime=2000;
static int s_entityWavVol_back[MAX_GENTITIES]={};
static float jarVoiceEnergy[MAX_GENTITIES]={};
static cvar_t threshold={};
static cvar_t *s_lip_threshold_1=&threshold,*s_lip_threshold_2=&threshold,*s_lip_threshold_3=&threshold,*s_lip_threshold_4=&threshold;
#include "../../code/client/jar_audio_amplitude.inc"
int main(){return RunTests("cinematic logical ownership, authored voice start time, exact PCM lip-sampler bounds",[]{
    short pcm[101]={};pcm[100]=32767;sfx_t original={},otherOriginal={},replacement={};replacement.pSoundData=pcm;replacement.iSoundLengthInSamples=100;replacement.eSoundCompressionMethod=ct_16;replacement.fVolRange=100;threshold.value=0.5f;
    channel_t first={},second={};first.thesfx=second.thesfx=&replacement;first.jarLogicalSource=&original;second.jarLogicalSource=&otherOriginal;
    sfx_t *music=&replacement;CHECK(JarAudioMatches(first, &original)&&!JarAudioMatches(second,&original));JarAudioReleaseChannel(first);
    CHECK(!first.thesfx&&second.thesfx==&replacement&&music->pSoundData==pcm);
    second.entchannel=CHAN_VOICE;second.entnum=7;second.jarMaxSamples=1000;second.startSample=START_SAMPLE_IMMEDIATE;int voiceEnds[MAX_GENTITIES]={};
    JarAudioStartChannel(second,1444,voiceEnds);CHECK(second.startSample==1444&&voiceEnds[7]==2444);
    CHECK(S_CheckAmplitude(&second,1444)==-1); // sample[100] sentinel must not contribute
    CHECK(S_CheckAmplitude(&second,1544)==0);CHECK(S_CheckAmplitude(&second,3000)==0);
    replacement.iSoundLengthInSamples=1;pcm[0]=32767;CHECK(S_CheckAmplitude(&second,1444)==4);
});}
