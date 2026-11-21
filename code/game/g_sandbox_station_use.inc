// Use the existing per-difficulty native grant and feedback. GPL-2.0-only.
static void sandboxStationUse(gentity_t* source,gentity_t* other,gentity_t* user,
 void (*dispense)(gentity_t*,gentity_t*,gentity_t*)){
 jar::sandboxStation::Settings settings;
 if(!G_SandboxStationRead(*source,settings)||!settings.enabled||
    user!=&g_entities[0]||!user->inuse||!user->client||user->health<=0||
    source->setTime>=level.time)return;
 const auto life=G_EntityLifetime(source);
 if(settings.unlimited)source->count=std::max(settings.capacity,MAX_AMMO_GIVE+1);
 dispense(source,other,user);
 if(!life||life!=G_EntityLifetime(source))return;
 if(settings.unlimited)source->count=settings.charge;
 source->attackDebounceTime=source->s.loopSound?level.time+jar::sandboxStation::useSoundDuration:0;
 if(source->count<settings.charge)source->painDebounceTime=0;
 G_SandboxStationSchedule(*source);
}
