#ifndef JAR_AUDIO_CHANNEL_POLICY_H
#define JAR_AUDIO_CHANNEL_POLICY_H
// Channel ownership and authored voice timing shared by the native mixer paths.
inline bool JarVoiceChannel(soundChannel_t c){return c==CHAN_VOICE||c==CHAN_VOICE_ATTEN||c==CHAN_VOICE_GLOBAL;}
inline bool JarAudioMatches(const channel_t& ch,const sfx_t *source){return ch.thesfx==source||ch.jarLogicalSource==source;}
inline void JarAudioReleaseChannel(channel_t& ch){
    ch.thesfx=nullptr;ch.jarLogicalSource=nullptr;ch.jarMaxSamples=0;
    std::memset(&ch.MP3StreamHeader,0,sizeof(ch.MP3StreamHeader));
    ch.bLooping=ch.bProcessed=ch.bPlaying=ch.bStreaming=false;
}
inline void JarAudioStartChannel(channel_t& ch,int now,int *voiceEnds){
    ch.startSample=now;
    if(ch.jarMaxSamples&&JarVoiceChannel(ch.entchannel)&&ch.entnum>=0&&ch.entnum<MAX_GENTITIES)voiceEnds[ch.entnum]=now+ch.jarMaxSamples;
}
#endif
