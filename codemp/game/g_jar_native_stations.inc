// Native station recharge and use callbacks, unchanged. GPL-2.0-only.
void check_recharge(gentity_t *ent)
{
	if (ent->fly_sound_debounce_time < level.time ||
		!ent->activator ||
		!ent->activator->client ||
		!(ent->activator->client->pers.cmd.buttons & BUTTON_USE))
	{
		if (ent->activator)
		{
			G_Sound(ent, CHAN_AUTO, ent->genericValue7);
		}
		ent->s.loopSound = 0;
		ent->s.loopIsSoundset = qfalse;
		ent->activator = NULL;
		ent->fly_sound_debounce_time = 0;
	}

	if (!ent->activator)
	{ //don't recharge during use
		if (ent->genericValue8 < level.time)
		{
			if (ent->count < ent->genericValue4)
			{
				ent->count++;
			}
			ent->genericValue8 = JAR_NumberDeadline(level.time, ent->genericValue5);
		}
	}
	ent->s.health = ent->count; //the "health bar" is gonna be how full we are
	ent->nextthink = level.time;
}

void shield_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int dif,add;
	int stop = 1;

	if (!activator || !activator->client)
	{
		return;
	}

	if ( level.gametype == GT_SIEGE
		&& other
		&& other->client
		&& other->client->siegeClass )
	{
		if ( !bgSiegeClasses[other->client->siegeClass].maxarmor )
		{//can't use it!
			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/interface/shieldcon_empty"));
			return;
		}
	}

	if (self->setTime < level.time)
	{
		int	maxArmor;
		if (!self->s.loopSound)
		{
			self->s.loopSound = G_SoundIndex("sound/interface/shieldcon_run");
			self->s.loopIsSoundset = qfalse;
		}
		self->setTime = level.time + 100;

		if ( level.gametype == GT_SIEGE
			&& other
			&& other->client
			&& other->client->siegeClass != -1 )
		{
			maxArmor = bgSiegeClasses[other->client->siegeClass].maxarmor;
		}
		else
		{
			maxArmor = activator->client->ps.stats[STAT_MAX_HEALTH];
		}
		dif = maxArmor - activator->client->ps.stats[STAT_ARMOR];

		if (dif > 0)					// Already at full armor?
		{
			if (dif >MAX_AMMO_GIVE)
			{
				add = MAX_AMMO_GIVE;
			}
			else
			{
				add = dif;
			}

			if (self->count<add)
			{
				add = self->count;
			}

		    if (!self->genericValue12)
			{
				self->count -= add;
			}
			if (self->count <= 0)
			{
				self->setTime = 0;
			}
			stop = 0;

			self->fly_sound_debounce_time = level.time + 500;
			self->activator = activator;

			activator->client->ps.stats[STAT_ARMOR] += add;
		}
	}

	if (stop || self->count <= 0)
	{
		if (self->s.loopSound && self->setTime < level.time)
		{
			if (self->count <= 0)
			{
				G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/interface/shieldcon_empty"));
			}
			else
			{
				G_Sound(self, CHAN_AUTO, self->genericValue7);
			}
		}
		self->s.loopSound = 0;
		self->s.loopIsSoundset = qfalse;
		if (self->setTime < level.time)
		{
			self->setTime = JAR_NumberDeadline(level.time, self->genericValue5+100.0);
		}
	}
}

void ammo_generic_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int /*dif,*/ add;
	//int ammoType;
	int stop = 1;

	if (!activator || !activator->client)
	{
		return;
	}

	if (self->setTime < level.time)
	{
		qboolean gaveSome = qfalse;
		/*
		while (i < 3)
		{
			if (!self->s.loopSound)
			{
				self->s.loopSound = G_SoundIndex("sound/interface/ammocon_run");
				self->s.loopIsSoundset = qfalse;
			}
			self->setTime = level.time + 100;

			//dif = activator->client->ps.stats[STAT_MAX_HEALTH] - activator->client->ps.stats[STAT_ARMOR];
			switch (i)
			{ //don't give rockets I guess
			case 0:
				ammoType = AMMO_BLASTER;
				break;
			case 1:
				ammoType = AMMO_POWERCELL;
				break;
			case 2:
				ammoType = AMMO_METAL_BOLTS;
				break;
			default:
				ammoType = -1;
				break;
			}

			if (ammoType != -1)
			{
				dif = ammoData[ammoType].max - activator->client->ps.ammo[ammoType];
			}
			else
			{
				dif = 0;
			}

			if (dif > 0)
			{ //only give if not full
				if (dif > MAX_AMMO_GIVE)
				{
					add = MAX_AMMO_GIVE;
				}
				else
				{
					add = dif;
				}

				if (self->count<add)
				{
					add = self->count;
				}

				self->count -= add;
				if (self->count <= 0)
				{
					self->setTime = 0;
					break;
				}
				stop = 0;

				self->fly_sound_debounce_time = level.time + 500;
				self->activator = activator;

				activator->client->ps.ammo[ammoType] += add;
			}

			i++;
		}
		*/
		int i = AMMO_BLASTER;
		if (!self->s.loopSound)
		{
			self->s.loopSound = G_SoundIndex("sound/interface/ammocon_run");
			self->s.loopIsSoundset = qfalse;
		}
		//self->setTime = level.time + 100;
		self->fly_sound_debounce_time = level.time + 500;
		self->activator = activator;
		while (i < AMMO_MAX)
		{
			add = ammoData[i].max*0.05;
			if (add < 1)
			{
				add = 1;
			}
			if ( ( (activator->client->ps.eFlags & EF_DOUBLE_AMMO) && (activator->client->ps.ammo[i] < ammoData[i].max*2)) ||
				( activator->client->ps.ammo[i] < ammoData[i].max ) )
			{
				gaveSome = qtrue;
				if ( level.gametype == GT_SIEGE  && i == AMMO_ROCKETS && activator->client->ps.ammo[i] >= 10 )
				{ //this stuff is already a freaking mess, so..
					gaveSome = qfalse;
				}
				activator->client->ps.ammo[i] += add;
				if ( level.gametype == GT_SIEGE  && i == AMMO_ROCKETS && activator->client->ps.ammo[i] >= 10 )
				{	// fixme - this should SERIOUSLY be externed.
					activator->client->ps.ammo[i] = 10;
				}
				else if ( activator->client->ps.eFlags & EF_DOUBLE_AMMO )
				{
					if (activator->client->ps.ammo[i] >= ammoData[i].max * 2)
					{	// yuck.
						activator->client->ps.ammo[i] = ammoData[i].max * 2;
					}
					else
					{
						stop = 0;
					}
				}
				else
				{
					if (activator->client->ps.ammo[i] >= ammoData[i].max)
					{
						activator->client->ps.ammo[i] = ammoData[i].max;
					}
					else
					{
						stop = 0;
					}
				}
			}
			i++;
			if (!self->genericValue12 && gaveSome)
			{
				int sub = (add*0.2);
				if (sub < 1)
				{
					sub = 1;
				}
				self->count -= sub;
				if (self->count <= 0)
				{
					self->count = 0;
					stop = 1;
					break;
				}
			}
		}
	}

	if (stop || self->count <= 0)
	{
		if (self->s.loopSound && self->setTime < level.time)
		{
			if (self->count <= 0)
			{
				G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/interface/ammocon_empty"));
			}
			else
			{
				G_Sound(self, CHAN_AUTO, self->genericValue7);
			}
		}
		self->s.loopSound = 0;
		self->s.loopIsSoundset = qfalse;
		if (self->setTime < level.time)
		{
			self->setTime = JAR_NumberDeadline(level.time, self->genericValue5+100.0);
		}
	}
}

void ammo_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int			add = 0.0f;//,highest;
//	int			difBlaster,difPowerCell,difMetalBolts;
	int			stop = 1;

	if (!activator || !activator->client)
	{
		return;
	}

	if (self->setTime < level.time)
	{
		if (!self->s.loopSound)
		{
			self->s.loopSound = G_SoundIndex("sound/player/pickupshield.wav");
		}

		self->setTime = level.time + 100;

		if (self->count)	// Has it got any power left?
		{
			int i = AMMO_BLASTER;
			while (i < AMMO_MAX)
			{
				add = ammoData[i].max*0.1;
				if (add < 1)
				{
					add = 1;
				}
				if (activator->client->ps.ammo[i] < ammoData[i].max)
				{
					activator->client->ps.ammo[i] += add;
					if (activator->client->ps.ammo[i] > ammoData[i].max)
					{
						activator->client->ps.ammo[i] = ammoData[i].max;
					}
				}
				i++;
			}
			if (!self->genericValue12)
			{
				self->count -= add;
			}
			stop = 0;

			self->fly_sound_debounce_time = level.time + 500;
			self->activator = activator;

			/*
			if (self->count > MAX_AMMO_GIVE)
			{
				add = MAX_AMMO_GIVE;
			}
			else if (self->count<0)
			{
				add = 0;
			}
			else
			{
				add = self->count;
			}

			activator->client->ps.ammo[AMMO_BLASTER] += add;
			activator->client->ps.ammo[AMMO_POWERCELL] += add;
			activator->client->ps.ammo[AMMO_METAL_BOLTS] += add;

			self->count -= add;
			stop = 0;

			self->fly_sound_debounce_time = level.time + 500;
			self->activator = activator;

			difBlaster = activator->client->ps.ammo[AMMO_BLASTER] - ammoData[AMMO_BLASTER].max;
			difPowerCell = activator->client->ps.ammo[AMMO_POWERCELL] - ammoData[AMMO_POWERCELL].max;
			difMetalBolts = activator->client->ps.ammo[AMMO_METAL_BOLTS] - ammoData[AMMO_METAL_BOLTS].max;

			// Find the highest one
			highest = difBlaster;
			if (difPowerCell>difBlaster)
			{
				highest = difPowerCell;
			}

			if (difMetalBolts > highest)
			{
				highest = difMetalBolts;
			}
			*/
		}
	}

	if (stop)
	{
		self->s.loopSound = 0;
		self->s.loopIsSoundset = qfalse;
	}
}

void health_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int dif,add;
	int stop = 1;

	if (!activator || !activator->client)
	{
		return;
	}

	if (self->setTime < level.time)
	{
		if (!self->s.loopSound)
		{
			self->s.loopSound = G_SoundIndex("sound/player/pickuphealth.wav");
		}
		self->setTime = level.time + 100;

		dif = activator->client->ps.stats[STAT_MAX_HEALTH] - activator->health;

		if (dif > 0)					// Already at full armor?
		{
			if (dif >/*MAX_AMMO_GIVE*/5)
			{
				add = 5;//MAX_AMMO_GIVE;
			}
			else
			{
				add = dif;
			}

			if (self->count<add)
			{
				add = self->count;
			}

			//self->count -= add;
			stop = 0;

			self->fly_sound_debounce_time = level.time + 500;
			self->activator = activator;

			activator->health += add;
		}
	}

	if (stop)
	{
		self->s.loopSound = 0;
		self->s.loopIsSoundset = qfalse;
	}
}

