#pragma once

#include "g_npc_definition.inc"

extern stringID_table_t FPTable[];

static bool JarNpcHasSingleValue(const char *key)
{
    static const char *fields[] = {
        "playerModel", "headmodel", "torsomodel", "legsmodel", "customSkin",
        "class", "playerTeam", "enemyTeam", "weapon", "width", "height",
        "surfOff", "surfOn", "headYawRangeLeft", "headYawRangeRight",
        "headPitchRangeUp", "headPitchRangeDown", "torsoYawRangeLeft",
        "torsoYawRangeRight", "torsoPitchRangeUp", "torsoPitchRangeDown",
        "scale", "scaleX", "scaleY", "scaleZ", "aggression", "aim", "earshot",
        "evasion", "hfov", "intelligence", "move", "reactions", "shootDistance",
        "vfov", "vigilance", "visrange", "rank", "health", "fullName",
        "dismemberProbHead", "dismemberProbArms", "dismemberProbHands",
        "dismemberProbWaist", "dismemberProbLegs", "crouchheight", "movetype",
        "yawSpeed", "walkSpeed", "runSpeed", "acceleration", "sex", "behavior",
        "snd", "sndcombat", "sndextra", "sndjedi", "altFire", "forcePowerMax",
        "forceRegenRate", "forceRegenAmount", "saber", "saber2", "saberStyle"
    };
    for (const char *field : fields)
        if (!Q_stricmp(key, field))
            return true;

    static const char *bladeFields[] = {
        "saberColor", "saber2Color", "saberLength", "saber2Length",
        "saberRadius", "saber2Radius"
    };
    for (const char *field : bladeFields) {
        const size_t length = strlen(field);
        if (!Q_stricmpn(key, field, length) &&
            (!key[length] || (key[length] >= '2' && key[length] <= '8' && !key[length + 1])))
            return true;
    }
    return GetIDForString(FPTable, key) >= 0;
}

static bool JarNpcNamedColor(const char *value)
{
    static const char *palettes[] = {
        "random", "random1", "jedi_hf", "jedi_hm", "jedi_kdm",
        "jedi_rm", "jedi_tf", "jedi_zf"
    };
    for (const char *palette : palettes)
        if (!Q_stricmp(value, palette))
            return true;
    return false;
}

// Used by the precache passes to skip fields they do not register.
static void JarNpcSkipField(const char *key, const char *&cursor)
{
    if (!Q_stricmp(key, "customRGBA")) {
        const char *value = JarNpcReadValue(cursor);
        if (value && !JarNpcNamedColor(value)) {
            for (int channel = 1; channel < 4 && JarNpcReadValue(cursor); ++channel) {
            }
        }
    } else if (JarNpcHasSingleValue(key)) {
        JarNpcReadValue(cursor);
    } else {
        JarNpcSkipLine(cursor);
    }
}

