// JA:R live randomizer: one explicit transaction, never a per-frame writer.
// GPL-2.0-only.
#include "qcommon.h"
#include "jar_live_randomizer.h"
#include "jar_sound_randomizer.h"
#include "jar_diagnostics.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace {
bool resumePending = false;
bool Number(const char *text, const jar::LiveSetting& setting, float& value) {
    if (!text || !*text || std::strlen(text)>31) return false;
    char *end = nullptr; value = std::strtof(text,&end);
    return end && !*end && std::isfinite(value) && value >= setting.validMin && value <= setting.validMax;
}
const char *Snapshot(unsigned i, const char *field) { return va("jar_live%u_%s",i,field); }
void Clear(unsigned i) { Cvar_Set(Snapshot(i,"base"),""); Cvar_Set(Snapshot(i,"last"),""); }
bool Owned(unsigned i, const char *current) {
    float number;
    return Number(Cvar_VariableString(Snapshot(i,"base")),jar::liveSettings[i],number) &&
        !std::strcmp(current,Cvar_VariableString(Snapshot(i,"last")));
}
void Restore(unsigned i) {
    const auto& setting = jar::liveSettings[i];
    cvar_t *target = Cvar_FindVar(setting.target);
    if (target && !(target->flags & (CVAR_LATCH | CVAR_INIT)) && !target->latchedString && Owned(i,target->string)) {
        char base[32]; Q_strncpyz(base,Cvar_VariableString(Snapshot(i,"base")),sizeof(base));
        Cvar_Set(setting.target,base);
        JAR_Log(1,"randomizer","cvar_restore","name=%s value=%s",setting.target,base);
    }
    Clear(i); // a later script/user change owns its value; do not overwrite it
}
void Status(const char *prefix) {
    unsigned active = 0;
    for (unsigned i=0; i<jar::liveSettingCount; ++i) active += Owned(i,Cvar_VariableString(jar::liveSettings[i].target));
    char status[192];
    Com_sprintf(status,sizeof(status),"%s: %u active | gravity %s / speed %s / brightness %s / FOV %s / sky %s / lights %s",prefix,active,
        Cvar_VariableString("g_gravity"),Cvar_VariableString("g_speed"),Cvar_VariableString("jar_sceneBrightness"),
        Cvar_VariableString("cg_fov"),Cvar_VariableString("r_fastsky"),Cvar_VariableString("r_dynamiclight"));
    Cvar_Set("jar_liveStatus",status);
    JAR_Log(1,"randomizer","cvar_status","%s",status);
}
}

void JAR_LiveInit() {
    // New cvar: absent in old profiles, therefore migrates to fixed mappings
    // without rewriting the seed, feature toggles or any unrelated preference.
    Cvar_Get("jar_randomMode","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART);
    Cvar_Get("jar_projectileSequenceSeed","",CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);
    Cvar_Get("jar_liveSequence","0",CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);
    Cvar_Get("jar_liveSequenceSeed","",CVAR_ROM|CVAR_SAVEGAME|CVAR_NORESTART);
    for (unsigned i=0; i<jar::liveSettingCount; ++i) {
        const auto& setting = jar::liveSettings[i];
        Cvar_Get(setting.toggle,"0",CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART);
        Cvar_Get(setting.target,setting.initial,0);
        Cvar_Get(Snapshot(i,"base"),"",CVAR_ROM | CVAR_SAVEGAME | CVAR_NORESTART);
        Cvar_Get(Snapshot(i,"last"),"",CVAR_ROM | CVAR_SAVEGAME | CVAR_NORESTART);
    }
    Cvar_Get("jar_liveStatus","No live roll applied. Select features, then Apply live roll.",CVAR_ROM | CVAR_NORESTART);
    Cmd_AddCommand("jar_cvar_apply",JAR_LiveApply);
    Cmd_AddCommand("jar_cvar_restore",JAR_LiveRestore);
    Cmd_AddCommand("jar_cvar_status",[] { Status("Live"); });
}

void JAR_LiveApply() {
    uint32_t seed;
    if (std::strcmp(Cvar_VariableString("sv_running"),"1")) {
        Cvar_Set("jar_liveStatus","Start a mission before applying a live roll."); return;
    }
    if (!jar::ParsePickupSeed(Cvar_VariableString("jar_randomSeed"),seed)) {
        Cvar_Set("jar_liveStatus","Invalid seed; nothing changed."); return;
    }
    for (const auto& setting : jar::liveSettings) {
        const char *toggle = Cvar_VariableString(setting.toggle);
        if (std::strcmp(toggle,"0") && std::strcmp(toggle,"1")) {
            Cvar_Set("jar_liveStatus","Invalid toggle; nothing changed."); return;
        }
    }
    bool fixed;
    if(!jar::ParseRandomMode(Cvar_VariableString("jar_randomMode"),fixed)) {
        Cvar_Set("jar_liveStatus","Invalid randomizer mode; nothing changed.");return;
    }
    uint32_t sequence=0;
    if(!std::strcmp(Cvar_VariableString("jar_liveSequenceSeed"),va("%u",seed)))
        jar::ParsePickupSeed(Cvar_VariableString("jar_liveSequence"),sequence);
    unsigned skipped = 0, applied = 0;
    for (unsigned i=0; i<jar::liveSettingCount; ++i) {
        const auto& setting = jar::liveSettings[i];
        if (!std::strcmp(Cvar_VariableString(setting.toggle),"0")) { Restore(i); continue; }
        cvar_t *target = Cvar_FindVar(setting.target);
        float baseline;
        if (!target || (target->flags & (CVAR_LATCH | CVAR_INIT)) || target->latchedString ||
            ((target->flags & CVAR_ROM) && std::strcmp(setting.target,"g_gravity"))) { ++skipped; continue; }
        const bool owned = Owned(i,target->string);
        const char *baseText = owned ? Cvar_VariableString(Snapshot(i,"base")) : target->string;
        if (!Number(baseText,setting,baseline)) { ++skipped; continue; }
        char base[32]; Q_strncpyz(base,baseText,sizeof(base));
        const uint32_t decision = fixed ? jar::FixedDecision(seed,"live-cvars-v1",setting.target) :
            jar::PickupDecision(seed,Cvar_VariableString("mapname"),sequence,"live-cvars-reroll-v1",setting.target);
        const float unit = float(decision%101)/100.0f;
        float value;
        switch (i) {
            case 0: value = std::max(100.0f,baseline*(0.75f+0.25f*unit)); break;
            case 1: value = std::min(425.0f,baseline*(1.0f+0.25f*unit)); break;
            case 2: value = 0.85f+0.4f*unit; break;
            case 3: value = 80.0f+25.0f*unit; break;
            default: value = float(decision%2); break;
        }
        char result[32];
        if (i == 2) Com_sprintf(result,sizeof(result),"%.3f",value);
        else Com_sprintf(result,sizeof(result),"%d",int(value+0.5f));
        Cvar_Set(Snapshot(i,"base"),base);
        Cvar_Set(Snapshot(i,"last"),result);
        Cvar_Set(setting.target,result);
        ++applied;
        JAR_Log(1,"randomizer","cvar_apply","seed=%u map=%s name=%s baseline=%s value=%s mode=%s sequence=%u",seed,Cvar_VariableString("mapname"),setting.target,base,result,fixed?"fixed":"reroll",sequence);
    }
    if(!fixed && applied) {
        Cvar_Set("jar_liveSequence",va("%u",(sequence+1)&2147483647u));
        Cvar_Set("jar_liveSequenceSeed",va("%u",seed));
    }
    Status(skipped ? "Live (unsupported values skipped)" : "Live");
}

void JAR_LiveRestore() {
    for (unsigned i=0; i<jar::liveSettingCount; ++i) { Restore(i); Cvar_Set(jar::liveSettings[i].toggle,"0"); }
    Status("Restored");
}

void JAR_RandomizerBeforeLoad() {
    // Release the current run before reading a different save. Missing fields in
    // legacy saves must not inherit new features or restore snapshots.
    resumePending = false;
    JAR_LiveRestore();
    for (const char *name : jar::soundCvars) Cvar_Set(name,"0");
    for (const char *name : {"jar_randomObjects","jar_randomProjectiles","jar_worldObjects","jar_worldProjectiles"}) Cvar_Set(name,"0");
    Cvar_Set("jar_worldSeed","1");
    Cvar_Set("jar_randomMode","0");
    // Old full saves rebuild BSP props with the legacy algorithm. New saves
    // overwrite this fallback with the world's serialized applied mode.
    Cvar_Set("jar_worldMode","1");
    Cvar_Set("jar_liveSequence","0");Cvar_Set("jar_liveSequenceSeed","");
    Cvar_Set("jar_projectileSequenceSeed","");
    Cvar_Set("jar_audioSequence","0");Cvar_Set("jar_audioSequenceSeed","");
    Cvar_Set("jar_projectileSequence","0");
    Cvar_Set("jar_npcSafety","1");Cvar_Set("jar_actorContinuity","0");Cvar_Set("jar_actorRecovery","0");Cvar_Set("jar_actorRecoverySaveVersion","0");
}

const char *JAR_LiveConfigValue(const char *name, const char *current) {
    for (unsigned i=0; i<jar::liveSettingCount; ++i)
        if (!std::strcmp(name,jar::liveSettings[i].target) && Owned(i,current)) return Cvar_VariableString(Snapshot(i,"base"));
    return current;
}

void JAR_RandomizerBeforeSave() {
    for (unsigned i=0; i<jar::liveSettingCount; ++i)
        if (!Owned(i,Cvar_VariableString(jar::liveSettings[i].target))) Clear(i);
}

void JAR_RandomizerReadComplete() { resumePending = true; }

void JAR_RandomizerResumeAfterLoad() {
    if (!resumePending) return;
    resumePending = false; // never reuse saved overrides on ordinary map/video init
    for (unsigned i=0; i<jar::liveSettingCount; ++i) {
        const auto& setting = jar::liveSettings[i];
        cvar_t *target = Cvar_FindVar(setting.target);
        float baseNumber, lastNumber;
        char base[32], last[32];
        Q_strncpyz(base,Cvar_VariableString(Snapshot(i,"base")),sizeof(base));
        Q_strncpyz(last,Cvar_VariableString(Snapshot(i,"last")),sizeof(last));
        if (!target || (target->flags & (CVAR_LATCH | CVAR_INIT)) || target->latchedString ||
            ((target->flags & CVAR_ROM) && std::strcmp(setting.target,"g_gravity")) ||
            !Number(base,setting,baseNumber) || !Number(last,setting,lastNumber)) { Clear(i); continue; }
        // Loading a saved visual override must preserve today's preferences.
        if (target->flags & CVAR_ARCHIVE) {
            float preference;
            if (!Number(target->string,setting,preference)) { Clear(i); continue; }
            Cvar_Set(Snapshot(i,"base"),target->string);
        }
        Cvar_Set(setting.target,last);
        JAR_Log(1,"randomizer","cvar_resume","name=%s value=%s",setting.target,last);
    }
    Status("Loaded");
}
