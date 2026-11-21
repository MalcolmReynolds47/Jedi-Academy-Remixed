#include "qcommon/jar_diagnostics.h"
#include "qcommon/jar_sound_randomizer.h"
/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "../server/exe_headers.h"

#include "snd_local.h"

portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
int64_t *snd_p;
int snd_linear_count, snd_vol;
short	*snd_out;

// Optional bounded capture of this game's mixed PCM, never system/other-app
// audio. Native console command: jar_audio_capture <simple-name> <1-20 seconds>.
extern int s_soundStarted;
extern qboolean s_soundMuted;
#ifdef USE_OPENAL
extern int s_UseOpenAL;
#endif
static fileHandle_t jarCaptureFile;
static unsigned jarCaptureFrames, jarCaptureRemaining;
static void CaptureWord(unsigned value) {
    const byte data[4] = {byte(value),byte(value>>8),byte(value>>16),byte(value>>24)};
    FS_Write(data,sizeof(data),jarCaptureFile);
}
void S_JAR_CaptureStop() {
    if (!jarCaptureFile) return;
    FS_Seek(jarCaptureFile,4,FS_SEEK_SET); CaptureWord(36+jarCaptureFrames*4);
    FS_Seek(jarCaptureFile,40,FS_SEEK_SET); CaptureWord(jarCaptureFrames*4);
    FS_FCloseFile(jarCaptureFile); jarCaptureFile=0; jarCaptureRemaining=0;
    JAR_Log(1,"sound","capture_complete","frames=%u rate=%d",jarCaptureFrames,dma.speed);
}
void S_JAR_Capture_f() {
    bool openAL = false;
#ifdef USE_OPENAL
    openAL = s_UseOpenAL != 0;
#endif
    if (!jar::CanCaptureAudio(s_soundStarted != 0,s_soundMuted != 0,openAL,dma.buffer != nullptr,dma.speed)) {
        Com_Printf("Audio capture requires an active, unmuted software mixer.\n"); return;
    }
    uint32_t seconds;
    if (Cmd_Argc()!=3 || !jar::ParsePickupSeed(Cmd_Argv(2),seconds) || seconds<1 || seconds>20 || dma.speed<8000 || dma.speed>96000) {
        Com_Printf("Usage: jar_audio_capture <name> <1-20 seconds> (active software mixer)\n"); return;
    }
    const char *name=Cmd_Argv(1);
    if (!*name || std::strlen(name)>48) return;
    for (const char *p=name; *p; ++p)
        if (!((*p>='a' && *p<='z') || (*p>='A' && *p<='Z') || (*p>='0' && *p<='9') || *p=='_' || *p=='-')) return;
    char path[MAX_QPATH]; Com_sprintf(path,sizeof(path),"jar-audio/%s.wav",name);
    if (FS_FileExists(path)) { Com_Printf("Capture already exists: %s\n",path); return; }
    S_JAR_CaptureStop();
    jarCaptureFile=FS_FOpenFileWrite(path); if (!jarCaptureFile) return;
    jarCaptureFrames=0; jarCaptureRemaining=seconds*dma.speed;
    FS_Write("RIFF",4,jarCaptureFile);CaptureWord(36);FS_Write("WAVEfmt ",8,jarCaptureFile);CaptureWord(16);
    const byte format[4]={1,0,2,0};FS_Write(format,4,jarCaptureFile);CaptureWord(dma.speed);CaptureWord(dma.speed*4);
    const byte alignment[4]={4,0,16,0};FS_Write(alignment,4,jarCaptureFile);FS_Write("data",4,jarCaptureFile);CaptureWord(0);
    JAR_Log(1,"sound","capture_begin","file=%s seconds=%u rate=%d",path,seconds,dma.speed);
}
static void S_JAR_CapturePaint(int frames) {
    if (!jarCaptureFile || frames<=0 || frames>PAINTBUFFER_SIZE) return;
    if (unsigned(frames)>jarCaptureRemaining) frames=jarCaptureRemaining;
    byte data[PAINTBUFFER_SIZE*4];
    for (int i=0; i<frames; ++i) for (int channel=0; channel<2; ++channel) {
        int64_t value=(channel ? paintbuffer[i].right : paintbuffer[i].left)>>8;
        if (value>32767) value=32767; if (value<-32768) value=-32768;
        const unsigned sample=unsigned(value)&65535u; const int offset=i*4+channel*2;
        data[offset]=byte(sample);data[offset+1]=byte(sample>>8);
    }
    FS_Write(data,frames*4,jarCaptureFile);jarCaptureFrames+=frames;jarCaptureRemaining-=frames;
    if (!jarCaptureRemaining) S_JAR_CaptureStop();
}

void S_WriteLinearBlastStereo16 (void)
{
	int		i;
	int64_t val;

	for (i=0 ; i<snd_linear_count ; i+=2)
	{
		val = snd_p[i]>>8;
		if (val > 0x7fff)
			snd_out[i] = 0x7fff;
		else if (val < (short)0x8000)
			snd_out[i] = (short)0x8000;
		else
			snd_out[i] = val;

		val = snd_p[i+1]>>8;
		if (val > 0x7fff)
			snd_out[i+1] = 0x7fff;
		else if (val < (short)0x8000)
			snd_out[i+1] = (short)0x8000;
		else
			snd_out[i+1] = val;
	}
}

void S_TransferStereo16 (unsigned long *pbuf, int endtime)
{
	int		lpos;
	int		ls_paintedtime;

	snd_p = (int64_t *) paintbuffer;
	ls_paintedtime = s_paintedtime;

	while (ls_paintedtime < endtime)
	{
	// handle recirculating buffer issues
		lpos = ls_paintedtime & ((dma.samples>>1)-1);

		snd_out = (short *) pbuf + (lpos<<1);

		snd_linear_count = (dma.samples>>1) - lpos;
		if (ls_paintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - ls_paintedtime;

		snd_linear_count <<= 1;

	// write a linear blast of samples
		S_WriteLinearBlastStereo16 ();

		snd_p += snd_linear_count;
		ls_paintedtime += (snd_linear_count>>1);
	}
}

/*
===================
S_TransferPaintBuffer

===================
*/
void S_TransferPaintBuffer(int endtime)
{
	int 	out_idx;
	int 	count;
	int 	out_mask;
	int64_t *p;
	int 	step;
	int64_t val;
	unsigned long *pbuf;

	pbuf = (unsigned long *)dma.buffer;


	if ( s_testsound->integer ) {
		int		i;
		int		count;

		// write a fixed sine wave
		count = (endtime - s_paintedtime);
		for (i=0 ; i<count ; i++)
			paintbuffer[i].left = paintbuffer[i].right = (int)(sin((s_paintedtime+i)*0.1)*20000*256);
	}


    S_JAR_CapturePaint(endtime-s_paintedtime);
	if (dma.samplebits == 16 && dma.channels == 2)
	{	// optimized case
		S_TransferStereo16 (pbuf, endtime);
	}
	else
	{	// general case
		p = (int64_t *) paintbuffer;
		count = (endtime - s_paintedtime) * dma.channels;
		out_mask = dma.samples - 1;
		out_idx = s_paintedtime * dma.channels & out_mask;
		step = 3 - dma.channels;

		if (dma.samplebits == 16)
		{
			short *out = (short *) pbuf;
			while (count--)
			{
				val = *p >> 8;
				p+= step;
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < (short)0x8000)
					val = (short)0x8000;
				out[out_idx] = (short)val;
				out_idx = (out_idx + 1) & out_mask;
			}
		}
		else if (dma.samplebits == 8)
		{
			unsigned char *out = (unsigned char *) pbuf;
			while (count--)
			{
				val = *p >> 8;
				p+= step;
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < (short)0x8000)
					val = (short)0x8000;
				out[out_idx] = (short)((val>>8) + 128);
				out_idx = (out_idx + 1) & out_mask;
			}
		}
	}
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/
static void S_PaintChannelFrom16( channel_t *ch, const sfx_t *sfx, int count, int sampleOffset, int bufferOffset )
{
	portable_samplepair_t	*pSamplesDest;
	int iData;


	int iLeftVol	= ch->leftvol  * snd_vol;
	int iRightVol	= ch->rightvol * snd_vol;

	pSamplesDest	= &paintbuffer[ bufferOffset ];

	for ( int i=0 ; i<count ; i++ )
	{
		iData = sfx->pSoundData[ sampleOffset++ ];

		pSamplesDest[i].left  += (int64_t(iData) * iLeftVol )>>8;
		pSamplesDest[i].right += (int64_t(iData) * iRightVol)>>8;
	}
}


void S_PaintChannelFromMP3( channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset )
{
	int data;
	int leftvol, rightvol;
	signed short *sfx;
	int	i;
	portable_samplepair_t	*samp;
	static short tempMP3Buffer[PAINTBUFFER_SIZE];

	MP3Stream_GetSamples( ch, sampleOffset, count, tempMP3Buffer, qfalse );	// qfalse = not stereo

	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;
	sfx = tempMP3Buffer;

	samp = &paintbuffer[ bufferOffset ];


	while ( count & 3 ) {
		data = *sfx;
		samp->left += (int64_t(data) * leftvol)>>8;
		samp->right += (int64_t(data) * rightvol)>>8;

		sfx++;
		samp++;
		count--;
	}

	for ( i=0 ; i<count ; i += 4 ) {
		data = sfx[i];
		samp[i].left += (int64_t(data) * leftvol)>>8;
		samp[i].right += (int64_t(data) * rightvol)>>8;

		data = sfx[i+1];
		samp[i+1].left += (int64_t(data) * leftvol)>>8;
		samp[i+1].right += (int64_t(data) * rightvol)>>8;

		data = sfx[i+2];
		samp[i+2].left += (int64_t(data) * leftvol)>>8;
		samp[i+2].right += (int64_t(data) * rightvol)>>8;

		data = sfx[i+3];
		samp[i+3].left += (int64_t(data) * leftvol)>>8;
		samp[i+3].right += (int64_t(data) * rightvol)>>8;
	}
}


// subroutinised to save code dup (called twice)	-ste
//
void ChannelPaint(channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset)
{
	switch (sc->eSoundCompressionMethod)
	{
		case ct_16:

			S_PaintChannelFrom16		(ch, sc, count, sampleOffset, bufferOffset);
			break;

		case ct_MP3:

			S_PaintChannelFromMP3		(ch, sc, count, sampleOffset, bufferOffset);
			break;

		default:

			assert(0);	// debug aid, ignored in release. FIXME: Should we ERR_DROP here for badness-catch?
			break;
	}
}



void S_PaintChannels( int endtime ) {
	int 	i;
	int 	end;
	channel_t *ch;
	sfx_t	*sc;
	int		ltime, count;
	int		sampleOffset;
	int	normal_vol,voice_vol;

	snd_vol = normal_vol = s_volume->value*256.0f;
	voice_vol  = (s_volumeVoice->value*256.0f);

//Com_Printf ("%i to %i\n", s_paintedtime, endtime);
	while ( s_paintedtime < endtime ) {
		// if paintbuffer is smaller than DMA buffer
		// we may need to fill it multiple times
		end = endtime;
		if ( endtime - s_paintedtime > PAINTBUFFER_SIZE ) {
			end = s_paintedtime + PAINTBUFFER_SIZE;
		}

		// clear the paint buffer to either music or zeros
		if ( s_rawend < s_paintedtime ) {
			if ( s_rawend ) {
				//Com_DPrintf ("background sound underrun\n");
			}
			memset(paintbuffer, 0, (end - s_paintedtime) * sizeof(portable_samplepair_t));
		} else {
			// copy from the streaming sound source
			int		s;
			int		stop;

			stop = (end < s_rawend) ? end : s_rawend;

			for ( i = s_paintedtime ; i < stop ; i++ ) {
				s = i&(MAX_RAW_SAMPLES-1);
				paintbuffer[i-s_paintedtime] = s_rawsamples[s];
			}
//		if (i != end)
//			Com_Printf ("partial stream\n");
//		else
//			Com_Printf ("full stream\n");
			for ( ; i < end ; i++ ) {
				paintbuffer[i-s_paintedtime].left =
				paintbuffer[i-s_paintedtime].right = 0;
			}
		}

		// paint in the channels.
		ch = s_channels;
		for ( i = 0; i < MAX_CHANNELS ; i++, ch++ ) {
			if ( !ch->thesfx || (ch->leftvol<0.25 && ch->rightvol<0.25 )) {
				continue;
			}

			if ( ch->entchannel == CHAN_VOICE || ch->entchannel == CHAN_VOICE_ATTEN || ch->entchannel == CHAN_VOICE_GLOBAL )
				snd_vol = voice_vol;
			else
				snd_vol = normal_vol;

			ltime = s_paintedtime;
			sc = ch->thesfx;

			// we might have to make 2 passes if it is
			//	a looping sound effect and the end of
			//	the sameple is hit...
			//
			do
			{
				if (ch->loopSound) {
					sampleOffset = ltime % sc->iSoundLengthInSamples;
				} else {
					sampleOffset = ltime - ch->startSample;
				}

				count = end - ltime;
                if(ch->jarMaxSamples && !ch->loopSound) count=std::min(count,ch->jarMaxSamples-sampleOffset);
				if ( sampleOffset + count > sc->iSoundLengthInSamples ) {
					count = sc->iSoundLengthInSamples - sampleOffset;
				}

				if ( count > 0 ) {
					ChannelPaint(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					ltime += count;
				}
			} while ( ltime < end && ch->loopSound );
		}
/* temprem
		// paint in the looped channels.
		ch = loop_channels;
		for ( i = 0; i < numLoopChannels ; i++, ch++ ) {
			if ( !ch->thesfx || (!ch->leftvol && !ch->rightvol )) {
				continue;
			}

			{

				ltime = s_paintedtime;
				sc = ch->thesfx;

				if (sc->soundData==NULL || sc->soundLength==0) {
					continue;
				}
				// we might have to make two passes if it
				// is a looping sound effect and the end of
				// the sample is hit
				do {
					sampleOffset = (ltime % sc->soundLength);

					count = end - ltime;
                if(ch->jarMaxSamples && !ch->loopSound) count=std::min(count,ch->jarMaxSamples-sampleOffset);
					if ( sampleOffset + count > sc->soundLength ) {
						count = sc->soundLength - sampleOffset;
					}

					if ( count > 0 )
					{
						ChannelPaint(ch, sc, count, sampleOffset, ltime - s_paintedtime);
						ltime += count;
					}

				} while ( ltime < end);
			}
		}
*/
		// transfer out according to DMA format
		S_TransferPaintBuffer( end );
		s_paintedtime = end;
	}
}
