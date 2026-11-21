
#include "qcommon/jar_roq_audio.h"

// Decode the exact catalog entry, including localized voices and stereo music.
// This path deliberately bypasses language/appearance filename substitution.
const char *S_JAR_DecodeCatalog(const char *name,sfx_t *sfx) {
    const long size=FS_ReadFile(name,nullptr);
    if(size<=0)return "missing-or-empty-file";
    const auto ext=std::strrchr(name,'.');const bool roq=ext && !Q_stricmp(ext,".roq");
    if(!roq && size>32*1024*1024)return "compressed-file-over-32MiB";
    void *raw=nullptr;
    if(!roq && (FS_ReadFile(name,&raw)!=size || !raw)){if(raw)FS_FreeFile(raw);return "file-read-failed";}
    std::vector<unsigned char> decoded;jar::PcmInfo info;
    const auto data=static_cast<unsigned char *>(raw);const unsigned char *pcm=data;
    const char *reason=nullptr;
    if(roq) {
        fileHandle_t f=0;const long length=FS_FOpenFileRead(name,&f,qtrue);
        if(!f)return "ROQ-open-failed";
        reason=jar::DecodeRoqAudio(length,[&](unsigned char *out,unsigned count){return FS_Read(out,count,f);},decoded);
        FS_FCloseFile(f);
        if(!reason){info.rate=22050;info.channels=1;info.width=2;info.frames=unsigned(decoded.size()/2);pcm=decoded.data();}
    }
    else if(ext && !Q_stricmp(ext,".wav")) reason=jar::ParsePcmWave(data,size,info);
    else if(ext && !Q_stricmp(ext,".mp3")) {
        // Native mono decoder downmixes stereo, including background tracks.
        size_t offset=0;reason=jar::Mp3AudioOffset(data,size,offset);
        void *mpeg=static_cast<unsigned char *>(raw)+offset;const int mpegSize=int(size-offset);
        if(!reason && !MP3_IsValid(name,mpeg,mpegSize,qfalse))reason="native-MP3-decoder-rejected-header";
        if(!reason) {
            const int bytes=MP3_GetUnpackedSize(name,mpeg,mpegSize,qtrue,qfalse);
            if(bytes<=0)reason="native-MP3-size-failed";
            else if(bytes>64*1024*1024)reason="decoded-audio-over-64MiB";
            else {
                decoded.resize(size_t(bytes)+2304+16);
                const int actual=MP3_UnpackRawPCM(name,mpeg,mpegSize,decoded.data(),qfalse);
                if(actual<=0 || actual>bytes+2304 || (actual&1))reason="native-MP3-decode-size-invalid";
                else {info.rate=44100;info.channels=1;info.width=2;info.frames=actual/2;info.offset=0;pcm=decoded.data();}
            }
        }
    } else reason="codec-not-supported-by-native-decoder";
    if(!reason && (dma.speed<8000 || dma.speed>96000))reason="output-rate-unavailable";
    const uint64_t frames=reason?0:uint64_t(info.frames)*uint64_t(dma.speed)/info.rate;
    if(!reason && (!frames || frames>32u*1024u*1024u))reason="resampled-audio-empty-or-over-64MiB";
    if(!reason) {
        sfx->pSoundData=static_cast<short *>(Z_Malloc(size_t(frames)*2,TAG_SND_RAWDATA,qfalse));
        sfx->iSoundLengthInSamples=int(frames);sfx->eSoundCompressionMethod=ct_16;sfx->fVolRange=0;
        for(uint64_t i=0;i<frames;++i){const uint32_t src=uint32_t(i*info.rate/uint64_t(dma.speed));
            const short sample=jar::PcmMonoSample(pcm,info,src<info.frames?src:info.frames-1);
            sfx->pSoundData[i]=sample;sfx->fVolRange=std::max(sfx->fVolRange,float(std::abs(int(sample))>>8));}
        sfx->bInMemory=true;sfx->bDefaultSound=false;Q_strncpyz(sfx->sSoundName,name,sizeof(sfx->sSoundName));
    }
    if(raw)FS_FreeFile(raw);return reason;
}
