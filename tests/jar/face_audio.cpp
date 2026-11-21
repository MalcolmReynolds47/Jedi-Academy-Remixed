#include "client/snd_local.h"
#include "qcommon/jar_face.h"
#include "test_support.h"
#include <algorithm>
#include <vector>

int s_soundtime=0;
int s_entityWavVol_back[MAX_GENTITIES]={};
static float jarVoiceEnergy[MAX_GENTITIES]={};
static cvar_t t1={},t2={},t3={},t4={};
cvar_t *s_lip_threshold_1=&t1,*s_lip_threshold_2=&t2,*s_lip_threshold_3=&t3,*s_lip_threshold_4=&t4;
#include "../../code/client/jar_audio_amplitude.inc"

int main() {
    return RunTests("production voice sampler: continuous energy and unchanged legacy states",[] {
        t1.value=0.3f;t2.value=4;t3.value=6;t4.value=8;
        std::vector<short> pcm(1200);
        sfx_t sound={};sound.pSoundData=pcm.data();sound.iSoundLengthInSamples=int(pcm.size());sound.fVolRange=128;sound.eSoundCompressionMethod=ct_16;
        channel_t channel={};channel.thesfx=&sound;channel.entnum=23;channel.startSample=0;
        float before=0;
        for(int amplitude : {0,2048,4096,8192,16384}) {
            std::fill(pcm.begin(),pcm.end(),short(amplitude));
            const int legacy=S_CheckAmplitude(&channel,0);
            if(!amplitude) {CHECK(legacy==-1);CHECK(jarVoiceEnergy[23]==0);}
            else {CHECK(legacy>=1&&legacy<=4);CHECK(jarVoiceEnergy[23]>before);}
            before=jarVoiceEnergy[23];
            CHECK(pcm.front()==amplitude&&pcm.back()==amplitude);
        }
        // Finished short clips cannot retain energy or read beyond the PCM.
        ++s_soundtime;CHECK(S_CheckAmplitude(&channel,int(pcm.size()))==0);CHECK(jarVoiceEnergy[23]==0);
    });
}
