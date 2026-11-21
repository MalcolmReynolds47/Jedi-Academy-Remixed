#include "../../code/client/snd_mix.cpp"
#include "test_support.h"
#include <vector>
#include <string>

channel_t s_channels[MAX_CHANNELS];
int s_paintedtime, s_rawend;
int s_soundStarted=1;
qboolean s_soundMuted=qfalse;
dma_t dma;
portable_samplepair_t s_rawsamples[MAX_RAW_SAMPLES];
static cvar_t quiet = {};
cvar_t *s_testsound = &quiet, *s_volume = &quiet, *s_volumeVoice = &quiet;
qboolean MP3Stream_GetSamples(channel_t *, int, int, short *, qboolean)
{
    throw std::runtime_error("Unexpected MP3 decoder call in PCM mixer test");
}

static std::vector<byte> capture;
static std::vector<std::string> arguments;
static size_t position;
static int opens, closes;
int Cmd_Argc() { return int(arguments.size()); }
char *Cmd_Argv(int i) { return const_cast<char *>(arguments.at(i).c_str()); }
qboolean FS_FileExists(const char *) { return qfalse; }
fileHandle_t FS_FOpenFileWrite(const char *path,qboolean) { CHECK(!std::strcmp(path,"jar-audio/test.wav"));++opens;capture.clear();position=0;return 1; }
int FS_Write(const void *data,int bytes,fileHandle_t file) {
    CHECK(file==1 && bytes>=0);if(position+bytes>capture.size())capture.resize(position+bytes);
    std::memcpy(capture.data()+position,data,bytes);position+=bytes;return bytes;
}
void FS_FCloseFile(fileHandle_t file) { CHECK(file==1);++closes; }
int FS_Seek(fileHandle_t file,long offset,int mode) { CHECK(file==1 && offset>=0 && mode==FS_SEEK_SET);position=offset;return 0; }
static unsigned Word(size_t offset) { return capture[offset] | unsigned(capture[offset+1])<<8 | unsigned(capture[offset+2])<<16 | unsigned(capture[offset+3])<<24; }

int main()
{
    return RunTests("512-voice mixing and clipping without signed overflow", [] {
        short samples[] = {32767, -32768};
        sfx_t sound = {}; sound.pSoundData = samples;
        channel_t channel = {}; channel.leftvol = channel.rightvol = 255;
        snd_vol = 256;
        memset(paintbuffer, 0, sizeof(paintbuffer));
        for(int i=0; i<MAX_CHANNELS; ++i) S_PaintChannelFrom16(&channel, &sound, 2, 0, 0);
        CHECK(paintbuffer[0].left == int64_t(32767)*255*MAX_CHANNELS);
        CHECK(paintbuffer[1].left == int64_t(-32768)*255*MAX_CHANNELS);
        CHECK(paintbuffer[0].left > INT_MAX);
        short result[4] = {};
        snd_p = reinterpret_cast<int64_t *>(paintbuffer); snd_out = result; snd_linear_count = 4;
        S_WriteLinearBlastStereo16();
        CHECK(result[0] == 32767 && result[1] == 32767);
        CHECK(result[2] == -32768 && result[3] == -32768);
        dma.speed=22050;byte buffer[8]={};dma.buffer=buffer;
        CHECK(jar::CanCaptureAudio(true,false,false,true,22050));
        CHECK(!jar::CanCaptureAudio(true,false,true,true,22050));
        CHECK(!jar::CanCaptureAudio(false,false,false,true,22050));
        CHECK(!jar::CanCaptureAudio(true,true,false,true,22050));
        arguments={"jar_audio_capture","test","1"};s_soundStarted=0;S_JAR_Capture_f();CHECK(opens==0);
        s_soundStarted=1;s_soundMuted=qtrue;S_JAR_Capture_f();CHECK(opens==0);s_soundMuted=qfalse;
        for(const char *name:{"../test","a/b","bad.wav",""}) { arguments={"jar_audio_capture",name,"1"};S_JAR_Capture_f();CHECK(opens==0); }
        for(const char *duration:{"0","21","1.5","-1","overflow"}) { arguments={"jar_audio_capture","test",duration};S_JAR_Capture_f();CHECK(opens==0); }
        arguments={"jar_audio_capture","test","1"};S_JAR_Capture_f();CHECK(opens==1 && jarCaptureRemaining==22050);
        S_JAR_CapturePaint(2);S_JAR_CaptureStop();CHECK(closes==1 && capture.size()==52);
        CHECK(!std::memcmp(capture.data(),"RIFF",4) && Word(4)==44 && Word(40)==8 && Word(24)==22050);
        CHECK(Word(44)==0x7fff7fff && Word(48)==0x80008000); // exact clipped stereo PCM
        S_JAR_Capture_f();while(jarCaptureFile)S_JAR_CapturePaint(PAINTBUFFER_SIZE);
        CHECK(closes==2 && Word(40)==22050*4 && capture.size()==44+22050*4);
        S_JAR_CaptureStop();CHECK(closes==2);
        short cappedPcm[4]={12000,12000,12000,12000},output[8]={};sfx_t capped={};capped.pSoundData=cappedPcm;capped.iSoundLengthInSamples=4;capped.eSoundCompressionMethod=ct_16;
        std::memset(s_channels,0,sizeof(s_channels));auto& active=s_channels[0];active.thesfx=&capped;active.leftvol=active.rightvol=255;active.jarMaxSamples=2;
        quiet.value=1;dma.channels=2;dma.samplebits=16;dma.samples=8;dma.buffer=reinterpret_cast<byte*>(output);s_paintedtime=0;s_rawend=0;S_PaintChannels(4);
        CHECK(output[0]!=0&&output[2]!=0&&output[4]==0&&output[6]==0);
    });
}
