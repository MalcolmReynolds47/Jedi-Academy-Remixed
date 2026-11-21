// Called only by ROQ's audible embedded PCM route, never music's raw stream.
void S_JAR_RewindCinematicAudio(int handle) {
    if(handle>=0 && handle<16)jarMovieAudio[handle].frames=0;
}
void S_JAR_ResetCinematicAudio(int handle) {
    if(handle>=0 && handle<16) {
        auto& movie=jarMovieAudio[handle];
        if(movie.sound)JAR_Log(2,"randomizer","movie_audio_end","handle=%d frames=%llu rate=22050",handle,(unsigned long long)movie.frames);
        movie={};
    }
}
bool S_JAR_CinematicSamples(int handle,const char *name,int frames,float volume) {
    if(handle<0 || handle>=16 || frames<=0 || dma.speed<=0)return false;
    auto& movie=jarMovieAudio[handle];
    uint64_t pos=movie.frames*uint64_t(dma.speed)/22050;
    movie.frames+=unsigned(frames);
    const uint64_t end=movie.frames*uint64_t(dma.speed)/22050;
    if(!s_soundStarted || s_soundMuted)return false;
    uint32_t seed;
    if(strcmp(Cvar_VariableString("jar_randomVoices"),"1") || !jar::ParsePickupSeed(Cvar_VariableString("jar_randomSeed"),seed))return false;
    if(!movie.attempted) {
        movie.attempted=true;
        uint32_t sequence=jar::PickupHashText(2166136261u,name);
        movie.sound=JarAudioChoose(seed,sequence,"all-audio-movies-v1",name,4);
    }
    if(!movie.sound)return false;
    // Preserve total native duration across chunk boundaries, including odd chunks.
    // The replacement plays once, then silence; a longer clip ends with the movie.
    short buffer[2048];
    while(pos<end) {
        const int count=int(std::min(uint64_t(2048),end-pos));
        for(int i=0;i<count;++i)buffer[i]=pos+unsigned(i)<unsigned(movie.sound->iSoundLengthInSamples)?movie.sound->pSoundData[pos+i]:0;
        S_RawSamples(count,dma.speed,2,1,reinterpret_cast<byte *>(buffer),volume,qtrue);pos+=count;
    }
    return true;
}
