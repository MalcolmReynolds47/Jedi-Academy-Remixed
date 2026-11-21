// Native MP lipsync sampling plus a separate continuous speech envelope.
// Sample boundaries are checked before touching PCM or the MP3 sliding window.
#include <cstring>
float S_JAR_VoiceEnergy(int entity){
 if(entity<0||entity>=MAX_GENTITIES||!s_soundStarted||s_soundMuted||s_entityWavVol[entity]<=0)return 0;
#ifdef USE_OPENAL
 if(s_UseOpenAL)return jar::FaceClamp(s_entityWavVol[entity]*.25f,0,1);
#endif
 return jarVoiceEnergy[entity];
}
static int next_amplitude = 0;
static int S_CheckAmplitude(channel_t	*ch, const unsigned int s_oldpaintedtime )
{
	// now, is this a cycle - or have we just started a new sample - where we should update the backup table, and write this value
	// into the new table? or should we just take the value FROM the back up table and feed it out.
	if(!ch||!ch->thesfx||ch->entnum<0||ch->entnum>=MAX_GENTITIES)return 0;
	if(ch->startSample==START_SAMPLE_IMMEDIATE){jarVoiceEnergy[ch->entnum]=0;return 0;}
	const auto*sound=ch->thesfx;
	const long long elapsed=(long long)s_oldpaintedtime-ch->startSample;
	if(sound->iSoundLengthInSamples<=0||elapsed>=sound->iSoundLengthInSamples){jarVoiceEnergy[ch->entnum]=0;return 0;}
	if ( ch->startSample == s_oldpaintedtime || (next_amplitude < s_soundtime) )//(ch->startSample == START_SAMPLE_IMMEDIATE)//!s_entityWavVol_back[ch->entnum]
	{
		int	sample;
		int	sample_total = 0;
		int	count = 0;
		short *current_pos_s;
//		char *current_pos_c;
		int	offset = 0;

		// if we haven't started the sample yet, we must be at the beginning
		current_pos_s = ((short*)ch->thesfx->pSoundData);
//		current_pos_c = ((char*)ch->thesfx->data);

		//if (ch->startSample != START_SAMPLE_IMMEDIATE)
		//{
			// figure out where we are in the sample right now.
			offset = int(std::max(0LL,elapsed));
			if(sound->eSoundCompressionMethod==ct_16){if(!current_pos_s){jarVoiceEnergy[ch->entnum]=0;return 0;}current_pos_s += offset;}
//			current_pos_c += offset;
		//}

		// scan through 10 samples 100( at 11hz or 200 at 22hz) samples apart.
		//
		for (int i=0; i<10; i++)
		{
			//
			// have we run off the end?
			if ((offset + (i*100)) >= ch->thesfx->iSoundLengthInSamples)
			{
				break;
			}
//			if (ch->thesfx->width == 1)
//			{
//				sample = current_pos_c[i*100];
//			}
//			else
			{
				switch (ch->thesfx->eSoundCompressionMethod)
				{
					case ct_16:
					{
						sample = current_pos_s[i*100];
					}
					break;

					case ct_MP3:
					{
                        const long long iIndex=(long long)i*100+(long long)offset*2-ch->iMP3SlidingDecodeWindowPos;
                        const int available=std::max(0,std::min(ch->iMP3SlidingDecodeWritePos,int(sizeof(ch->MP3SlidingDecodeBuffer))));
                        if(iIndex<0||iIndex+sizeof(short)>(unsigned)available)continue;
                        short decoded;std::memcpy(&decoded,ch->MP3SlidingDecodeBuffer+size_t(iIndex),sizeof(decoded));sample=decoded;
					}
					break;

					default:
					{
						assert(0);
						sample = 0;
					}
					break;
				}

//				if (sample < 0)
//					sample = -sample;
				sample = sample>>8;
			}
			// square it for better accuracy
			sample_total += (sample * sample);
			count++;
		}

		// if we are already done with this sample, then its silence
		if (!count)
		{
			jarVoiceEnergy[ch->entnum]=0;
			return(0);
		}
		sample_total /= count;
		jarVoiceEnergy[ch->entnum]=jar::VoiceEnergy(float(sample_total),sound->fVolRange);

		// I hate doing this, but its the simplest way
		if (sample_total < ch->thesfx->fVolRange * s_lip_threshold_1->value)
		{
		// tell the scripts that are relying on this that we are still going, but actually silent right now.
			sample = -1;
		}
		else
		if (sample_total < ch->thesfx->fVolRange * s_lip_threshold_2->value)
		{
			sample = 1;
		}
		else
		if (sample_total < ch->thesfx->fVolRange * s_lip_threshold_3->value)
		{
			sample = 2;
		}
		else
		if (sample_total < ch->thesfx->fVolRange * s_lip_threshold_4->value)
		{
			sample = 3;
		}
		else
		{
			sample = 4;
		}

//		Com_OPrintf("Returning sample %d\n",sample);

		// store away the value we got into the back up table
		s_entityWavVol_back[ ch->entnum ] = sample;
		return (sample);
	}
	// no, just get last value calculated from backup table
	assert( s_entityWavVol_back[ch->entnum] );
	return (s_entityWavVol_back[ ch->entnum]);
}