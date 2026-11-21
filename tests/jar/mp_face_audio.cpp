#define NOMINMAX
#define USE_OPENAL
#include "client/snd_local.h"
#include "qcommon/jar_face.h"
#include <cassert>
#include <cstdio>
#include <vector>
int s_soundtime=1;
int s_entityWavVol[MAX_GENTITIES]={},s_entityWavVol_back[MAX_GENTITIES]={};
static float jarVoiceEnergy[MAX_GENTITIES]={};
qboolean s_soundStarted=qtrue,s_soundMuted=qfalse;
static cvar_t thresholds[4];
cvar_t*s_lip_threshold_1=&thresholds[0],*s_lip_threshold_2=&thresholds[1],*s_lip_threshold_3=&thresholds[2],*s_lip_threshold_4=&thresholds[3];
#ifdef USE_OPENAL
int s_UseOpenAL=0;
#endif
#include "../../codemp/client/snd_jar_voice.inc"
int main(){
 thresholds[0].value=.3f;thresholds[1].value=4;thresholds[2].value=6;thresholds[3].value=8;
 std::vector<short> pcm(1200);sfx_t sound{};sound.pSoundData=pcm.data();sound.iSoundLengthInSamples=(int)pcm.size();sound.fVolRange=128;sound.eSoundCompressionMethod=ct_16;
 channel_t channel{};channel.thesfx=&sound;channel.entnum=23;
 const int amplitudes[]={0,2048,4096,8192,16384},levels[]={-1,1,1,4,4};float previous=0;
 for(int n=0;n<5;++n){std::fill(pcm.begin(),pcm.end(),short(amplitudes[n]));int level=S_CheckAmplitude(&channel,0);assert(level==levels[n]);s_entityWavVol[23]=level;float energy=S_JAR_VoiceEnergy(23);assert(n?energy>previous:energy==0);previous=energy;assert(pcm.front()==amplitudes[n]&&pcm.back()==amplitudes[n]);}
 next_amplitude=100;assert(S_CheckAmplitude(&channel,10)==4&&S_JAR_VoiceEnergy(23)==previous);next_amplitude=0;
 s_soundMuted=qtrue;assert(!S_JAR_VoiceEnergy(23));s_soundMuted=qfalse;s_soundStarted=qfalse;assert(!S_JAR_VoiceEnergy(23));s_soundStarted=qtrue;
 s_entityWavVol[23]=-1;assert(!S_JAR_VoiceEnergy(23));s_entityWavVol[23]=4;
 assert(!S_JAR_VoiceEnergy(-1)&&!S_JAR_VoiceEnergy(MAX_GENTITIES));
 // End-of-clip and one-sample clips must not read one element past PCM.
 assert(!S_CheckAmplitude(&channel,1200)&&!jarVoiceEnergy[23]);sound.iSoundLengthInSamples=1;assert(S_CheckAmplitude(&channel,0)==4);assert(!S_CheckAmplitude(&channel,1));
 sound.pSoundData=nullptr;assert(!S_CheckAmplitude(&channel,0));sound.pSoundData=pcm.data();channel.startSample=START_SAMPLE_IMMEDIATE;assert(!S_CheckAmplitude(&channel,0));channel.startSample=0;
 channel.entnum=MAX_GENTITIES;assert(!S_CheckAmplitude(&channel,0));channel.entnum=23;assert(!S_CheckAmplitude(nullptr,0));
 // Exercise actual decoded-window sampling, including truncated/negative/
 // stale windows. Only bytes published by the decoder may be consumed.
 sound.eSoundCompressionMethod=ct_MP3;sound.iSoundLengthInSamples=1200;channel.iMP3SlidingDecodeWritePos=600;
 short value=8192;for(int n=0;n<600;n+=2)std::memcpy(channel.MP3SlidingDecodeBuffer+n,&value,2);
 assert(S_CheckAmplitude(&channel,0)==4&&jarVoiceEnergy[23]>.5f);
 channel.iMP3SlidingDecodeWindowPos=10000;assert(!S_CheckAmplitude(&channel,0)&&!jarVoiceEnergy[23]);channel.iMP3SlidingDecodeWindowPos=0;
 channel.iMP3SlidingDecodeWritePos=1;assert(!S_CheckAmplitude(&channel,0));channel.iMP3SlidingDecodeWritePos=-1;assert(!S_CheckAmplitude(&channel,0));
#ifdef USE_OPENAL
 s_UseOpenAL=1;for(int level=-1;level<=4;++level){s_entityWavVol[23]=level;assert(S_JAR_VoiceEnergy(23)==(level>0?level*.25f:0));}
#endif
 puts("MP voice envelope: native legacy levels, continuous PCM/MP3 energy, sample/window bounds, silence/mute/stopped guards and cached cadence passed.");
}
