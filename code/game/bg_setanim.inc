// PM_SetAnimFinal from OpenJK bg_panimate.cpp (GPL-2.0-only).
// Included by the game and a headless skeletal API regression fixture.
void PM_SetAnimFinal(int *torsoAnim,int *legsAnim,
					 int setAnimParts,int anim,int setAnimFlags,
					 int *torsoAnimTimer,int *legsAnimTimer,
					 gentity_t *gent,int blendTime)		// default blendTime=350
{

// BASIC SETUP AND SAFETY CHECKING
//=================================

	// If It Is A Busted Entity, Don't Do Anything Here.
	//---------------------------------------------------
	if (!gent || !gent->client)
	{
		return;
	}

	// Make Sure This Character Has Such An Anim And A Model
	//-------------------------------------------------------
	if (anim<0 || anim>=MAX_ANIMATIONS || !ValidAnimFileIndex(gent->client->clientInfo.animFileIndex))
	{
		#ifndef FINAL_BUILD
 		if (g_AnimWarning->integer)
		{
			if (anim<0 || anim>=MAX_ANIMATIONS)
			{
				gi.Printf(S_COLOR_RED"PM_SetAnimFinal: Invalid Anim Index (%d)!\n", anim);
			}
			else
			{
				gi.Printf(S_COLOR_RED"PM_SetAnimFinal: Invalid Anim File Index (%d)!\n", gent->client->clientInfo.animFileIndex);
			}
		}
		#endif
		return;
	}


	// Get Global Time Properties
	//----------------------------
	float			timeScaleMod  = PM_GetTimeScaleMod( gent );
	const int		actualTime	  = (cg.time?cg.time:level.time);
	const animation_t*	animations	  = level.knownAnimFileSets[gent->client->clientInfo.animFileIndex].animations;
	const animation_t&	curAnim		  = animations[anim];

	// Make Sure This Character Has Such An Anim And A Model
	//-------------------------------------------------------
	if (animations[anim].numFrames==0)
	{
	#ifndef FINAL_BUILD
		static int	LastAnimWarningNum=0;
		if (LastAnimWarningNum!=anim)
		{
			if ((cg_debugAnim.integer==3)	||												// 3 = do everyone
 				(cg_debugAnim.integer==1 && gent->s.number==0) ||							// 1 = only the player
				(cg_debugAnim.integer==2 && gent->s.number!=0) ||							// 2 = only everyone else
				(cg_debugAnim.integer==4 && gent->s.number!=cg_debugAnimTarget.integer) 	// 4 = specific entnum
				)
			{
				gi.Printf(S_COLOR_RED"PM_SetAnimFinal: Anim %s does not exist in this model (%s)!\n", animTable[anim].name, gent->NPC_type );
			}
		}
		LastAnimWarningNum = anim;
	#endif
		return;
	}

	// If It's Not A Ghoul 2 Model, Just Remember The Anims And Stop, Because Everything Beyond This Is Ghoul2
	//---------------------------------------------------------------------------------------------------------
	if (!gi.G2API_HaveWeGhoul2Models(gent->ghoul2))
	{
		if (setAnimParts&SETANIM_TORSO)
		{
			(*torsoAnim) = anim;
		}
		if (setAnimParts&SETANIM_LEGS)
		{
			(*legsAnim) = anim;
		}
		return;
	}


	// Lower Offensive Skill Slows Down The Saber Start Attack Animations
	//--------------------------------------------------------------------
	PM_SaberStartTransAnim( gent->client->ps.saberAnimLevel, anim, &timeScaleMod, gent );



// SETUP VALUES FOR INCOMMING ANIMATION
//======================================
	const bool	animFootMove  = (PM_WalkingAnim(anim) || PM_RunningAnim(anim) || anim==BOTH_CROUCH1WALK || anim==BOTH_CROUCH1WALKBACK);
	const bool	animHoldless  = (setAnimFlags&SETANIM_FLAG_HOLDLESS)!=0;
	const bool	animHold	  = (setAnimFlags&SETANIM_FLAG_HOLD)!=0;
	const bool	animRestart	  = (setAnimFlags&SETANIM_FLAG_RESTART)!=0;
	const bool	animOverride  = (setAnimFlags&SETANIM_FLAG_OVERRIDE)!=0;
	const bool	animSync	  = (g_synchSplitAnims->integer!=0 && !animRestart);
	float	animCurrent	  = (-1.0f);
	float	animSpeed	  = (50.0f / curAnim.frameLerp * timeScaleMod); // animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
	const float	animFPS		  = (::abs(curAnim.frameLerp));
	const int		animDurMSec	  = (int)(((curAnim.numFrames - 1) * animFPS) / timeScaleMod);
	const int		animHoldMSec  = ((animHoldless && timeScaleMod==1.0f)?((animDurMSec>1)?(animDurMSec-1):(animFPS)):(animDurMSec));
	int		animFlags	  = (curAnim.loopFrames!=-1)?(BONE_ANIM_OVERRIDE_LOOP):(BONE_ANIM_OVERRIDE_FREEZE);
	int		animStart	  = (curAnim.firstFrame);
	int		animEnd		  = (curAnim.firstFrame)+(animations[anim].numFrames);

	// If We Have A Blend Timer, Add The Blend Flag
	//----------------------------------------------
	if (blendTime > 0)
	{
		animFlags |= BONE_ANIM_BLEND;
	}

	// If Animation Is Going Backwards, Swap Last And First Frames
	//-------------------------------------------------------------
	if (animSpeed<0.0f)
	{
//	#ifndef FINAL_BUILD
	#if 0
		if (g_AnimWarning->integer==1)
		{
			if (animFlags&BONE_ANIM_OVERRIDE_LOOP)
			{
				gi.Printf(S_COLOR_YELLOW"PM_SetAnimFinal: WARNING: Anim (%s) looping backwards!\n", animTable[anim].name);
			}
		}
	#endif

		int temp	= animEnd;
		animEnd		= animStart;
		animStart	= temp;
		blendTime	= 0;
	}

	// If The Animation Is Walking Or Running, Attempt To Scale The Playback Speed To Match
	//--------------------------------------------------------------------------------------
	// Mode 1 scales NPCs only. Player prediction must not retime the shared
	// Ghoul2 clock every command. Mode 2 opts into legacy player scaling.
	if (g_noFootSlide->integer
		&& (gent->s.number != 0 || g_noFootSlide->integer >= 2)
		&& animFootMove
		&& !(animSpeed<0.0f)
		//FIXME: either read speed from animation.cfg or only do this for NPCs
		//			for whom we've specifically determined the proper numbers!
		&& gent->client->NPC_class != CLASS_HOWLER
		&& gent->client->NPC_class != CLASS_WAMPA
		&& gent->client->NPC_class != CLASS_GONK
		&& gent->client->NPC_class != CLASS_HOWLER
		&& gent->client->NPC_class != CLASS_MOUSE
		&& gent->client->NPC_class != CLASS_PROBE
		&& gent->client->NPC_class != CLASS_PROTOCOL
		&& gent->client->NPC_class != CLASS_R2D2
		&& gent->client->NPC_class != CLASS_R5D2
		&& gent->client->NPC_class != CLASS_SEEKER)
	{
		bool	Walking = !!PM_WalkingAnim(anim);
		bool	HasDual = (gent->client->ps.saberAnimLevel==SS_DUAL);
		bool	HasStaff = (gent->client->ps.saberAnimLevel==SS_STAFF);
		float	moveSpeedOfAnim  = 150.0f;//g_noFootSlideRunScale->value;

		if (anim==BOTH_CROUCH1WALK || anim==BOTH_CROUCH1WALKBACK)
		{
			moveSpeedOfAnim = 75.0f;
		}
		else
		{
			if (gent->client->NPC_class == CLASS_HAZARD_TROOPER)
			{
				moveSpeedOfAnim = 50.0f;
			}
			else if (gent->client->NPC_class == CLASS_RANCOR)
			{
				moveSpeedOfAnim = 173.0f;
			}
			else
			{
				if (Walking)
				{
					if (HasDual || HasStaff)
					{
						moveSpeedOfAnim = 100.0f;
					}
					else
					{
						moveSpeedOfAnim = 50.0f;// g_noFootSlideWalkScale->value;
					}
				}
				else
				{
					if (HasStaff)
					{
						moveSpeedOfAnim = 250.0f;
					}
					else
					{
						moveSpeedOfAnim = 150.0f;
					}
				}
			}
		}






		// Player commands and prediction share a Ghoul2 instance. The server
		// displacement estimate can be stale between commands; use the current
		// horizontal velocity for player gait, leaving NPC distance scaling intact.
		const float gaitSpeed = gent->s.number == 0
			? std::sqrt(gent->client->ps.velocity[0]*gent->client->ps.velocity[0] + gent->client->ps.velocity[1]*gent->client->ps.velocity[1])
			: gent->resultspeed;
		animSpeed *= (gaitSpeed/moveSpeedOfAnim);
		if (animSpeed<0.01f)
		{
			animSpeed = 0.01f;
		}

		// Make Sure Not To Play Too Fast An Anim
		//----------------------------------------
		float	maxPlaybackSpeed = (1.5f * timeScaleMod);
		if (animSpeed>maxPlaybackSpeed)
		{
			animSpeed = maxPlaybackSpeed;
		}
	}


// GET VALUES FOR EXISTING BODY ANIMATION
//==========================================
	float	bodySpeed	  = 0.0f;
	float	bodyCurrent	  = 0.0f;
	int		bodyStart	  = 0;
	int		bodyEnd		  = 0;
	int		bodyFlags	  = 0;
	int		bodyAnim	  = (*legsAnim);
	int		bodyBone	  = (gent->rootBone);
	bool	bodyTimerOn	  = ((*legsAnimTimer>0) || (*legsAnimTimer)==-1);
	bool	bodyPlay	  = ((setAnimParts&SETANIM_LEGS) && (bodyBone!=-1) && (animOverride || !bodyTimerOn));
	bool	bodyAnimating = !!gi.G2API_GetBoneAnimIndex(&gent->ghoul2[gent->playerModel], bodyBone, actualTime, &bodyCurrent, &bodyStart, &bodyEnd, &bodyFlags, &bodySpeed, NULL);
	bool	bodyOnAnimNow = (bodyAnimating && bodyAnim==anim && bodyStart==animStart && bodyEnd==animEnd);
	bool	bodyMatchTorsFrame = false;


// GET VALUES FOR EXISTING TORSO ANIMATION
//===========================================
	float	torsSpeed	  = 0.0f;
	float	torsCurrent	  = 0.0f;
	int		torsStart	  = 0;
	int		torsEnd		  = 0;
	int		torsFlags	  = 0;
	int		torsAnim	  = (*torsoAnim);
	int		torsBone	  = (gent->lowerLumbarBone);
	bool	torsTimerOn	  = ((*torsoAnimTimer)>0 || (*torsoAnimTimer)==-1);
	bool	torsPlay	  = (gent->client->NPC_class!=CLASS_RANCOR && (setAnimParts&SETANIM_TORSO) && (torsBone!=-1) && (animOverride || !torsTimerOn));
	bool	torsAnimating = !!gi.G2API_GetBoneAnimIndex(&gent->ghoul2[gent->playerModel], torsBone, actualTime, &torsCurrent, &torsStart, &torsEnd, &torsFlags, &torsSpeed, NULL);
	bool	torsOnAnimNow = (torsAnimating && torsAnim==anim && torsStart==animStart && torsEnd==animEnd);
	bool	torsMatchBodyFrame = false;


// APPLY SYNC TO TORSO
//=====================
 	if (animSync && torsPlay && !bodyPlay && bodyOnAnimNow && (!torsOnAnimNow || torsCurrent!=bodyCurrent))
	{
		torsMatchBodyFrame = true;
		animCurrent		=  bodyCurrent;
	}
	// An ongoing locomotion cycle owns its phase. Torso clocks may advance at
	// another rate; copying them each command makes the legs jitter/restart.
 	if (animSync && bodyPlay && !torsPlay && torsOnAnimNow && (!bodyOnAnimNow || (!animFootMove && bodyCurrent!=torsCurrent)))
	{
		bodyMatchTorsFrame = true;
		animCurrent		=  torsCurrent;
	}

	// If Already Doing These Exact Parameters, Then Don't Play
	//----------------------------------------------------------
	if (!animRestart)
	{
		torsPlay &= !(torsOnAnimNow && torsSpeed==animSpeed && !torsMatchBodyFrame);
		bodyPlay &= !(bodyOnAnimNow && bodySpeed==animSpeed && !bodyMatchTorsFrame);
	}

#ifndef FINAL_BUILD
	if ((cg_debugAnim.integer==3)	||												// 3 = do everyone
		(cg_debugAnim.integer==1 && gent->s.number==0) ||							// 1 = only the player
		(cg_debugAnim.integer==2 && gent->s.number!=0) ||							// 2 = only everyone else
		(cg_debugAnim.integer==4 && gent->s.number!=cg_debugAnimTarget.integer) 	// 4 = specific entnum
		)
	{
		if (bodyPlay || torsPlay)
		{
			char*	entName = gent->targetname;
			char*	location;

			// Select Entity Name
			//--------------------
			if (!entName || !entName[0])
			{
				entName = gent->NPC_targetname;
			}
			if (!entName || !entName[0])
			{
				entName = gent->NPC_type;
			}
			if (!entName || !entName[0])
			{
				entName = gent->classname;
			}
			if (!entName || !entName[0])
			{
				entName = "UNKNOWN";
			}

			// Select Play Location
			//----------------------
			if (bodyPlay && torsPlay)
			{
				location = "BOTH ";
			}
			else if (bodyPlay)
			{
				location = "LEGS ";
			}
			else
			{
				location = "TORSO";
			}

			// Print It!
			//-----------
	 		Com_Printf("[%10d] ent[%3d-%18s] %s anim[%3d] - %s\n",
	 			actualTime,
				gent->s.number,
				entName,
				location,
				anim,
				animTable[anim].name );
		}
	}
#endif


// PLAY ON THE TORSO
//========================
	if (torsPlay)
	{
		*torsoAnim = anim;
		float oldAnimCurrent = animCurrent;
		if (animCurrent!=bodyCurrent && torsOnAnimNow && !animRestart && !torsMatchBodyFrame)
		{
			animCurrent = torsCurrent;
		}

		gi.G2API_SetAnimIndex(&gent->ghoul2[gent->playerModel], curAnim.glaIndex);
		gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], torsBone,
			animStart,
			animEnd,
			(torsOnAnimNow && !animRestart)?(animFlags&~BONE_ANIM_BLEND):(animFlags),
			animSpeed,
			actualTime,
			animCurrent,
			blendTime);

		if (gent->motionBone!=-1)
		{
			gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], gent->motionBone,
				animStart,
				animEnd,
				(torsOnAnimNow && !animRestart)?(animFlags&~BONE_ANIM_BLEND):(animFlags),
				animSpeed,
				actualTime,
				animCurrent,
				blendTime);
		}

		animCurrent = oldAnimCurrent;

		// If This Animation Is To Be Locked And Held, Calculate The Duration And Set The Timer
		//--------------------------------------------------------------------------------------
		if (animHold || animHoldless)
		{
			PM_SetTorsoAnimTimer(gent, torsoAnimTimer, animHoldMSec);
		}
	}

// PLAY ON THE WHOLE BODY
//========================
	if (bodyPlay)
	{
		*legsAnim = anim;

		if (bodyOnAnimNow && !animRestart && !bodyMatchTorsFrame)
		{
			animCurrent = bodyCurrent;
		}

		gi.G2API_SetAnimIndex(&gent->ghoul2[gent->playerModel], curAnim.glaIndex);
		gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], bodyBone,
			animStart,
			animEnd,
			(bodyOnAnimNow && !animRestart)?(animFlags&~BONE_ANIM_BLEND):(animFlags),
			animSpeed,
			actualTime,
			animCurrent,
			blendTime);

		// If This Animation Is To Be Locked And Held, Calculate The Duration And Set The Timer
		//--------------------------------------------------------------------------------------
		if (animHold || animHoldless)
		{
			PM_SetLegsAnimTimer(gent, legsAnimTimer, animHoldMSec);
		}
	}





// PRINT SOME DEBUG TEXT OF EXISTING VALUES
//==========================================
	if (false)
	{
		gi.Printf("PLAYANIM: (%3d) Speed(%4.2f) ", anim, animSpeed);
		if (bodyAnimating)
		{
			gi.Printf("BODY: (%4.2f) (%4.2f) ", bodyCurrent,  bodySpeed);
		}
		else
		{
			gi.Printf("                      ");
		}
		if (torsAnimating)
		{
			gi.Printf("TORS: (%4.2f) (%4.2f)\n", torsCurrent,  torsSpeed);
		}
		else
		{
			gi.Printf("\n");
		}
	}
}


