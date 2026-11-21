// Native cue publication, ownership, copy policy and persistent ordering.
static char musicConfig[MAX_STRING_CHARS];
static const char *unavailableTrack;
static int musicWrites;

static void MusicTestConfig(int index, char *out, int size) {
    if (index == CS_MUSIC) Q_strncpyz(out, musicConfig, size);
    else Config(index, out, size);
}

static void MusicTestSetConfig(int index, const char *value) {
    assert(index == CS_MUSIC);
    ++musicWrites;
    Q_strncpyz(musicConfig, value, sizeof(musicConfig));
}

static int MusicTestOpen(const char *path, fileHandle_t *file, fsMode_t mode) {
    if (unavailableTrack && !strcmp(path, unavailableTrack)) {
        *file = 0;
        return -1;
    }
    return RigidOpen(path, file, mode);
}

static void MusicCases(void) {
    static const char baseline[] = "mp/ffa3";
    static const char pair[] = "music/artus_topside/impbased_action.mp3 music/bespin_streets/bespina_action.mp3";
    static const char single[] = "music/hoth2/hoth2_explore.mp3 music/hoth2/hoth2_explore.mp3 " JAR_MUSIC_CUE_TAG;
    const char *pairConfig = "music/artus_topside/impbased_action.mp3 music/bespin_streets/bespina_action.mp3 " JAR_MUSIC_CUE_TAG;
    char compact[64], decodedCue[JAR_MUSIC_CUE_SIZE];
    assert(strlen(pair) > 63 && JAR_MusicPack(pair, compact) && JAR_MusicUnpack(compact, decodedCue));
    assert(!strcmp(decodedCue, pair));
    assert(!JAR_MusicPack("music/../bad.mp3", compact));
    assert(!JAR_MusicPack("music/badsmall.mp3  music/goodsmall.mp3", compact));

    RigidFixture();
    imports.GetConfigstring = MusicTestConfig;
    imports.SetConfigstring = MusicTestSetConfig;
    imports.FS_Open = MusicTestOpen;
    JAR_MP_SetNativeMusic(baseline);
    gentity_t *p = &g_entities[0];
    gentity_t *a = ControllerAt("target_play_music", 0, 100);
    gentity_t *b = ControllerAt("target_play_music", 0, 300);
    gentity_t *disable = ControllerAt("target_deactivate", 0, 500);
    assert(a && b && disable && !strcmp(musicConfig, baseline));
    int first = a->s.number, second = b->s.number;
    assert(ControllerKind(first) == JAR_MUSIC_CONTROLLER);

    ControllerShot(p, second, qfalse); // Activate in reverse entity-slot order.
    MusicFlush();
    assert(!strcmp(musicConfig, single));
    ControllerShot(p, first, qfalse);
    MusicFlush();
    assert(musicRank[first] == 2 && musicRank[second] == 1);
    assert(JAR_MusicPack(pair, compact));
    unavailableTrack = "music/bespin_streets/bespina_action.mp3";
    MediaText(a, compact);
    MusicFlush();
    assert(!strcmp(musicConfig, single));
    unavailableTrack = NULL;
    MediaText(a, compact);
    MusicFlush();
    assert(!strcmp(musicConfig, pairConfig) && ContextUndoReady(p, first));
    Command(0, "jar_property", "property_undo");
    MusicFlush();
    assert(!strcmp(musicConfig, single) && musicRank[first] == 2);
    MediaText(a, compact);
    MusicFlush();

    CopyAssembly(p, first);
    assert(copies[0].count == 1 && !copies[0].node[0].controller.active);
    vec3_t offset = {600, 0, 80};
    assert(PlaceCopy(p, &copies[0], offset, NULL, NULL));
    const int pasted = players[0].selected;
    assert(!musicRank[pasted] && !strcmp(objects[pasted].controllerText, compact) && !strcmp(musicConfig, pairConfig));
    G_FreeEntity(&g_entities[pasted]);

    static jarWorld_t saved, decoded;
    static byte bytes[WORLD_BYTES];
    assert(WorldCapture(&saved));
    const int size = WorldEncode(&saved, bytes);
    assert(size && WorldDecode(bytes, size, &decoded));
    assert(decoded.copy.node[0].controllerRemaining == 2 && decoded.copy.node[1].controllerRemaining == 1);
    decoded.copy.node[0].controllerRemaining = 1;
    assert(!WorldValid(&decoded) && WorldDecode(bytes, size, &decoded));
    const int writes = musicWrites;
    assert(WorldClear() && musicWrites == writes);
    MusicFlush();
    assert(musicWrites == writes + 1 && !strcmp(musicConfig, baseline));
    unavailableTrack = "music/bespin_streets/bespina_action.mp3";
    assert(!PlaceCopy(NULL, &decoded.copy, vec3_origin, decoded.velocity, decoded.angular));
    assert(!musicCount && !strcmp(musicConfig, baseline));
    unavailableTrack = NULL;
    assert(PlaceCopy(NULL, &decoded.copy, vec3_origin, decoded.velocity, decoded.angular));
    MusicFlush();
    assert(musicCount == 2 && !strcmp(musicConfig, pairConfig));
    MusicStop(musicSlots[1]);
    MusicFlush();
    assert(musicCount == 1 && !strcmp(musicConfig, single));
    JAR_MP_SetNativeMusic("authored replacement");
    assert(!musicCount && !strcmp(musicConfig, "authored replacement"));
    assert(WorldClear() && !strcmp(musicConfig, "authored replacement"));

    a = ControllerAt("target_play_music", 0, 100);
    disable = ControllerAt("target_deactivate", 0, 300);
    assert(a && disable);
    SwitchLink(p, disable, a, 0);
    ControllerShot(p, a->s.number, qfalse);
    assert(musicCount == 1);
    ControllerShot(p, disable->s.number, qfalse);
    MusicFlush();
    assert(!musicCount && !strcmp(musicConfig, "authored replacement"));
    ControllerShot(p, a->s.number, qfalse);
    assert(!musicCount);
    assert(WorldClear());

    JAR_MP_SetNativeMusic("");
    a = ControllerAt("target_play_music", 0, 100);
    assert(a);
    ControllerShot(p, a->s.number, qfalse);
    MusicFlush();
    assert(!strcmp(musicConfig, single));
    assert(WorldClear());
    MusicFlush();
    assert(!*musicConfig);

    imports.GetConfigstring = Config;
    imports.SetConfigstring = NULL;
    imports.FS_Open = RigidOpen;
    puts("MP portable music: retail cue pairs, validation, links, edit/Undo, silent duplication, save order and native restoration passed.");
}
