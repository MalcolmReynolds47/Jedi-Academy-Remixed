static int next_amplitude = 0;
static int S_CheckAmplitude(channel_t	*ch, const int s_oldpaintedtime )
{
	// now, is this a cycle - or have we just started a new sample - where we should update the backup table, and write this value
	// into the new table? or should we just take the value FROM the back up table and feed it out.
	assert( ch->startSample != START_SAMPLE_IMMEDIATE );
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
			offset = std::max(0,s_oldpaintedtime - ch->startSample);//s_paintedtime
			if(ch->thesfx->eSoundCompressionMethod==ct_16)current_pos_s += std::min(offset,ch->thesfx->iSoundLengthInSamples);
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
						const int iIndex = (i*100) + ((offset * /*ch->thesfx->width*/2) - ch->iMP3SlidingDecodeWindowPos);
						const short* pwSamples = (short*) (ch->MP3SlidingDecodeBuffer + iIndex);

						sample = *pwSamples;
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
        jarVoiceEnergy[ch->entnum]=jar::VoiceEnergy(float(sample_total),ch->thesfx->fVolRange);

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

#ifdef _MSC_VER
//		OutputDebugString(va("Returning sample %d\n",sample));
#endif

		// store away the value we got into the back up table
		s_entityWavVol_back[ ch->entnum ] = sample;
		return (sample);
	}
	// no, just get last value calculated from backup table
	assert( s_entityWavVol_back[ch->entnum] );
	return (s_entityWavVol_back[ ch->entnum]);
}
