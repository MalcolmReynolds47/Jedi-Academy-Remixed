// Bounded extraction of native ROQ DPCM soundtracks. GPL-2.0-only.
#ifndef JAR_ROQ_AUDIO_H
#define JAR_ROQ_AUDIO_H
#include "jar_audio_catalog.h"
namespace jar {
// read must consume exactly count bytes or return a short read. Video is skipped
// incrementally so a large movie never becomes a large compressed allocation.
template<class Read> const char *DecodeRoqAudio(size_t size,Read read,std::vector<unsigned char>& pcm) {
    pcm.clear();unsigned char head[8],chunk[65536];
    if(size<8 || size>512u*1024u*1024u)return "ROQ-file-size-outside-8B-512MiB";
    if(read(head,8)!=8)return "ROQ-header-read-failed";
    if(AudioLE(head,2)!=0x1084 || AudioLE(head+2,4)!=0xffffffff)return "invalid-ROQ-header";
    size_t remaining=size-8;
    while(remaining) {
        if(remaining<8 || read(head,8)!=8)return "truncated-ROQ-chunk-header";
        remaining-=8;const unsigned id=AudioLE(head,2),bytes=AudioLE(head+2,4),flags=AudioLE(head+6,2);
        if(bytes>remaining)return "truncated-ROQ-chunk";
        if(id==0x1030)return "ROQ-packet-audio-not-supported";
        if(id==0x1020 || id==0x1021) {
            const unsigned channels=id==0x1021?2:1;
            if(bytes>sizeof(chunk) || bytes%channels)return "invalid-ROQ-audio-chunk";
            if(read(chunk,bytes)!=int(bytes))return "ROQ-audio-read-failed";
            if(pcm.size()+size_t(bytes/channels)*2>64u*1024u*1024u)return "decoded-audio-over-64MiB";
            int left=channels==2?flags&0xff00:flags,right=(flags&255)<<8;
            auto delta=[](unsigned char c){const int magnitude=c&127;return c&128?-magnitude*magnitude:magnitude*magnitude;};
            for(unsigned i=0;i<bytes;i+=channels) {
                left=int16_t(left+delta(chunk[i]));int sample=left;
                if(channels==2){right=int16_t(right+delta(chunk[i+1]));sample=(left+right)/2;}
                pcm.push_back(unsigned(sample)&255);pcm.push_back((unsigned(sample)>>8)&255);
            }
        } else {
            for(unsigned skipped=0;skipped<bytes;) {
                const unsigned count=bytes-skipped>sizeof(chunk)?unsigned(sizeof(chunk)):bytes-skipped;
                if(read(chunk,count)!=int(count))return "ROQ-video-read-failed";skipped+=count;
            }
        }
        remaining-=bytes;
    }
    return pcm.empty()?"ROQ-has-no-embedded-audio":nullptr;
}
}
#endif
