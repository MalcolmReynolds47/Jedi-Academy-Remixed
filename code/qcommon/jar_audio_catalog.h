// Catalog-wide audio policy and bounded PCM conversion. GPL-2.0-only.
#ifndef JAR_AUDIO_CATALOG_H
#define JAR_AUDIO_CATALOG_H
#include "jar_randomizer.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
namespace jar {
inline std::string AudioSourceKey(const char *source) {
    auto key=CanonicalKey(source);
    // Native registered SFX discard .wav/.mp3 before lookup. Use that same key
    // for literal music paths; ROQ identity keeps its extension.
    if(key.size()>=4 && (key.compare(key.size()-4,4,".wav")==0 || key.compare(key.size()-4,4,".mp3")==0))key.resize(key.size()-4);
    return key;
}
inline uint32_t CatalogIndex(uint32_t seed,uint32_t sequence,uint32_t count,const char *domain) {
    if (!count) return 0;
    const uint32_t offset=PickupDecision(seed,"",0,domain,"offset")%count;
    uint32_t step=1+PickupDecision(seed,"",0,domain,"stride")%count;
    auto gcd=[](uint32_t a,uint32_t b) {while(b){const uint32_t c=a%b;a=b;b=c;}return a;};
    while(gcd(step,count)!=1) step=step==count?1:step+1;
    return uint32_t((offset+uint64_t(sequence%count)*step)%count);
}
// ID3v2.2/2.3/2.4 use a 28-bit synchsafe tag size: https://id3.org/id3v2-00
// and https://id3.org/id3v2.3.0 / https://id3.org/id3v2.4.0-structure.
// Skip only validated metadata; the native decoder still validates MPEG data.
inline const char *Mp3AudioOffset(const unsigned char *data,size_t size,size_t& offset) {
    offset=0;
    while(size-offset>=3 && !std::memcmp(data+offset,"ID3",3)) {
        const auto p=data+offset;const size_t left=size-offset;
        if(left<10)return "truncated-ID3-header";
        if(p[3]<2 || p[3]>4 || p[4]==255 || (p[5] & (p[3]==2?63:p[3]==3?31:15)))return "unsupported-ID3-header";
        uint32_t bytes=0;for(unsigned i=6;i<10;++i){if(p[i]&128)return "invalid-ID3-size";bytes=(bytes<<7)|p[i];}
        const size_t total=10+size_t(bytes)+(p[3]==4 && (p[5]&16)?10:0);
        if(total>left)return "truncated-ID3-tag";
        if(p[3]==4 && (p[5]&16)) {
            const auto footer=p+total-10;
            if(std::memcmp(footer,"3DI",3) || std::memcmp(footer+3,p+3,7))return "invalid-ID3-footer";
        }
        offset+=total;
    }
    return offset==size?"empty-MP3-after-ID3":nullptr;
}
struct PcmInfo {uint32_t rate=0,channels=0,width=0,frames=0,offset=0;};
inline uint32_t AudioLE(const unsigned char *p,unsigned n) {uint32_t v=0;for(unsigned i=0;i<n;++i)v|=uint32_t(p[i])<<(8*i);return v;}
inline const char *ParsePcmWave(const unsigned char *data,size_t size,PcmInfo& out) {
    out={};
    if(size<12 || std::memcmp(data,"RIFF",4) || std::memcmp(data+8,"WAVE",4))return "not-RIFF-WAVE";
    bool format=false,pcm=false,found=false;uint32_t bytes=0,align=0;
    for(size_t pos=12;pos+8<=size;) {
        const uint32_t n=AudioLE(data+pos+4,4);const size_t begin=pos+8;
        if(n>size-begin)return "truncated-WAVE-chunk";
        if(!std::memcmp(data+pos,"fmt ",4)) {
            if(n<16)return "short-WAVE-format";
            format=true;pcm=AudioLE(data+begin,2)==1;out.channels=AudioLE(data+begin+2,2);
            out.rate=AudioLE(data+begin+4,4);align=AudioLE(data+begin+12,2);
            const auto bits=AudioLE(data+begin+14,2);out.width=(bits==8||bits==16)?bits/8:0;
        } else if(!std::memcmp(data+pos,"data",4) && !found) {out.offset=uint32_t(begin);bytes=n;found=true;}
        pos=begin+size_t(n)+(n&1u);
    }
    if(!format||!found)return "missing-WAVE-format-or-data";
    if(!pcm)return "WAVE-codec-not-PCM";
    if(out.channels<1||out.channels>2)return "WAVE-channels-not-mono-or-stereo";
    if(!out.width)return "WAVE-depth-not-8-or-16-bit";
    if(out.rate<8000||out.rate>96000)return "WAVE-rate-outside-8000-96000";
    if(align!=out.channels*out.width || bytes%align)return "WAVE-invalid-block-alignment";
    out.frames=bytes/align;return out.frames?nullptr:"empty-audio";
}
inline short PcmMonoSample(const unsigned char *data,const PcmInfo& in,uint32_t frame) {
    int sum=0;for(uint32_t c=0;c<in.channels;++c){const auto p=data+in.offset+(size_t(frame)*in.channels+c)*in.width;
        sum+=in.width==1?(int(*p)-128)*256:int(int16_t(AudioLE(p,2)));}
    return short(sum/int(in.channels));
}
}
// Enumerates actual mounted files without the legacy 4095-result listing cap.
void FS_JAR_ListAudioNames(std::vector<std::string>& names);
#endif
