#include "qcommon/jar_audio_catalog.h"
#include "qcommon/jar_roq_audio.h"
#include "test_support.h"
#include <set>
#include <vector>
int main(){return RunTests("complete catalog permutation and bounded mono/stereo PCM decoding",[]{
    for(unsigned count:{1u,2u,13u,97u,10000u,14621u})for(unsigned seed:{0u,73u,123456789u,2147483647u}){
        std::set<unsigned> seen;
        for(unsigned i=0;i<count;++i){auto index=jar::CatalogIndex(seed,i,count,"all-audio-events-v2");CHECK(index<count);CHECK(seen.insert(index).second);CHECK(index==jar::CatalogIndex(seed,i+count,count,"all-audio-events-v2"));}
        CHECK(seen.size()==count);
    }
    // Independently calculated FNV/LE + avalanche golden values freeze the
    // source-key contract across architectures and future library changes.
    CHECK(jar::FixedDecision(0,"audio-v1","sound/player/footsteps/metal_step1")==1301959922u);
    CHECK(jar::FixedDecision(73,"audio-v1","sound/player/footsteps/metal_step1")==477491198u);
    CHECK(jar::FixedDecision(2147483647u,"audio-v1","sound/player/footsteps/metal_step1")==3079704923u);
    std::vector<unsigned char> roq={0x84,0x10,255,255,255,255,30,0,0x21,0x10,4,0,0,0,0x80,0x7f,1,1,129,129};
    std::vector<unsigned char> extracted;
    auto decode=[&](size_t size){size_t cursor=0;return jar::DecodeRoqAudio(size,[&](unsigned char *out,unsigned count){if(count>roq.size()-cursor)return 0;std::memcpy(out,roq.data()+cursor,count);cursor+=count;return int(count);},extracted);};
    CHECK(!decode(roq.size()));CHECK(extracted.size()==4);
    // Stereo predictors straddle zero; signed downmix must match native shorts.
    CHECK(int16_t(jar::AudioLE(extracted.data(),2))==-127);
    CHECK(int16_t(jar::AudioLE(extracted.data()+2,2))==-128);
    for(size_t n=0;n<roq.size();++n)CHECK(decode(n));
    roq[12]=1;CHECK(decode(roq.size()));roq[12]=0;
    roq[8]=0x20;roq[14]=0;roq[15]=0;CHECK(!decode(roq.size()));CHECK(extracted.size()==8);
    roq[8]=0x30;CHECK(std::string(decode(roq.size()))=="ROQ-packet-audio-not-supported");
    roq={0x84,0x10,255,255,255,255,30,0};CHECK(std::string(decode(roq.size()))=="ROQ-has-no-embedded-audio");
    roq.insert(roq.end(),{0x11,0x10,1,0,1,0,0,0});roq.resize(16+65537);
    roq.insert(roq.end(),{0x20,0x10,1,0,0,0,0,0,2});CHECK(!decode(roq.size()));CHECK(extracted.size()==2&&jar::AudioLE(extracted.data(),2)==4);
    CHECK(decode(roq.size()+1)); // Reader cannot satisfy advertised bytes.
    size_t offset=999;
    const unsigned char mpeg[]={255,251,144,100};
    CHECK(!jar::Mp3AudioOffset(mpeg,sizeof(mpeg),offset)&&offset==0);
    std::vector<unsigned char> tagged(4100,0);std::memcpy(tagged.data(),"ID3",3);tagged[3]=3;tagged[8]=31;tagged[9]=118;
    std::memcpy(tagged.data()+4096,mpeg,4);
    CHECK(!jar::Mp3AudioOffset(tagged.data(),tagged.size(),offset)&&offset==4096);
    for(size_t n=3;n<4096;++n)CHECK(jar::Mp3AudioOffset(tagged.data(),n,offset));
    tagged[6]=128;CHECK(jar::Mp3AudioOffset(tagged.data(),tagged.size(),offset));tagged[6]=0;
    tagged[5]=1;CHECK(jar::Mp3AudioOffset(tagged.data(),tagged.size(),offset));tagged[5]=0;
    tagged[3]=2;CHECK(!jar::Mp3AudioOffset(tagged.data(),tagged.size(),offset)&&offset==4096);
    tagged[5]=64;CHECK(!jar::Mp3AudioOffset(tagged.data(),tagged.size(),offset)&&offset==4096);
    tagged[5]=1;CHECK(jar::Mp3AudioOffset(tagged.data(),tagged.size(),offset));
    tagged[3]=4;tagged[5]=16;tagged.resize(4110);std::memcpy(tagged.data()+4096,"3DI",3);std::memcpy(tagged.data()+4099,tagged.data()+3,7);
    CHECK(!jar::Mp3AudioOffset(tagged.data(),tagged.size(),offset)&&offset==4106);
    tagged[4099]=3;CHECK(jar::Mp3AudioOffset(tagged.data(),tagged.size(),offset));
    std::vector<unsigned char> wave(52);
    auto put=[&](unsigned off,unsigned n,unsigned value){for(unsigned i=0;i<n;++i)wave[off+i]=(value>>(8*i))&255;};
    std::memcpy(wave.data(),"RIFF",4);put(4,4,44);std::memcpy(wave.data()+8,"WAVEfmt ",8);put(16,4,16);put(20,2,1);put(22,2,2);put(24,4,44100);put(28,4,176400);put(32,2,4);put(34,2,16);std::memcpy(wave.data()+36,"data",4);put(40,4,8);put(44,2,32767);put(46,2,32767);put(48,2,32768);put(50,2,32768);
    jar::PcmInfo pcm;
    CHECK(!jar::ParsePcmWave(wave.data(),wave.size(),pcm));CHECK(pcm.frames==2&&pcm.channels==2&&pcm.rate==44100);CHECK(jar::PcmMonoSample(wave.data(),pcm,0)==32767&&jar::PcmMonoSample(wave.data(),pcm,1)==-32768);
    for(size_t size=0;size<wave.size();++size)CHECK(jar::ParsePcmWave(wave.data(),size,pcm));
    put(20,2,3);CHECK(std::string(jar::ParsePcmWave(wave.data(),wave.size(),pcm))=="WAVE-codec-not-PCM");put(20,2,1);
    put(34,2,24);CHECK(jar::ParsePcmWave(wave.data(),wave.size(),pcm));put(34,2,8);put(22,2,1);put(32,2,1);wave[44]=0;wave[45]=128;wave[46]=255;
    CHECK(!jar::ParsePcmWave(wave.data(),wave.size(),pcm));CHECK(pcm.frames==8);CHECK(jar::PcmMonoSample(wave.data(),pcm,0)==-32768&&jar::PcmMonoSample(wave.data(),pcm,1)==0&&jar::PcmMonoSample(wave.data(),pcm,2)==32512);
    put(40,4,0xffffffff);CHECK(jar::ParsePcmWave(wave.data(),wave.size(),pcm));
});}
