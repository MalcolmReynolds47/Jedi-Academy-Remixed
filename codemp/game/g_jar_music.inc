// Portable background music uses the native map cue configstring so the same
// state reaches active clients and clients joining later. GPL-2.0-only.
static int musicSlots[MAX_GENTITIES], musicRank[MAX_GENTITIES], musicCount;
static qboolean musicDirty;
static char musicBaseline[MAX_STRING_CHARS];

static void MusicReset(void) {
    for (int i = 0; i < musicCount; ++i) musicRank[musicSlots[i]] = 0;
    musicCount = 0;
    musicDirty = qfalse;
    musicBaseline[0] = '\0';
}

static qboolean MusicValidate(const char *cue) {
    int first, second;
    if (!JAR_MusicTracks(cue, 1, &first, &second)) return qfalse;
    const int tracks[] = {first, second};
    for (int i = 0; i < 2 && tracks[i] >= 0; ++i) {
        fileHandle_t file = 0;
        const int size = trap->FS_Open(jarMusicTracks[tracks[i]], &file, FS_READ);
        if (file) trap->FS_Close(file);
        if (!file || size <= 0) return qfalse;
    }
    return qtrue;
}

static void MusicFlush(void) {
    // Bulk Undo, disconnect and same-frame activation chains can change the
    // top repeatedly. Send one reliable configstring update for the frame.
    if (!musicDirty) return;
    musicDirty = qfalse;
    if (!musicCount) {
        trap->SetConfigstring(CS_MUSIC, musicBaseline);
        musicBaseline[0] = '\0';
        return;
    }
    char cue[JAR_MUSIC_CUE_SIZE];
    if (JAR_MusicUnpack(objects[musicSlots[musicCount - 1]].controllerText, cue)) {
        // Stock clients read the two paths. The third token also tells a
        // matching cgame to start playback during its initial gamestate.
        if (strchr(cue, ' ')) {
            trap->SetConfigstring(CS_MUSIC, va("%s %s", cue, JAR_MUSIC_CUE_TAG));
        } else {
            trap->SetConfigstring(CS_MUSIC, va("%s %s %s", cue, cue, JAR_MUSIC_CUE_TAG));
        }
    }
}

static void MusicStart(int n) {
    if (musicRank[n]) return;
    if (!musicCount && !musicDirty) trap->GetConfigstring(CS_MUSIC, musicBaseline, sizeof(musicBaseline));
    musicSlots[musicCount++] = n;
    musicRank[n] = musicCount;
    musicDirty = qtrue;
}

static void MusicStop(int n) {
    const int rank = musicRank[n];
    if (!rank) return;
    musicRank[n] = 0;
    for (int i = rank; i < musicCount; ++i) {
        musicSlots[i - 1] = musicSlots[i];
        musicRank[musicSlots[i]] = i;
    }
    --musicCount;
    if (rank > musicCount) musicDirty = qtrue;
}

static void MusicClearSelections(void) {
    for (int i = musicCount - 1; i >= 0; --i) {
        const int n = musicSlots[i];
        musicRank[n] = 0;
        ++objects[n].propertyRevision;
        SelectionChanged(n);
    }
    musicCount = 0;
}

static void MusicReleaseAll(void) {
    if (!musicCount) return;
    MusicClearSelections();
    musicDirty = qtrue;
}

void JAR_MP_SetNativeMusic(const char *cue) {
    MusicClearSelections();
    MusicReset();
    trap->SetConfigstring(CS_MUSIC, cue ? cue : "");
}

static void MusicUse(gentity_t *e, gentity_t *other, gentity_t *activator) {
    const int n = e->s.number;
    if (!sandbox.integer || (e->flags & FL_INACTIVE) || objects[n].copyPending) return;
    if (musicRank[n]) MusicStop(n);
    else MusicStart(n);
    SelectionChanged(n);
}

static qboolean MusicInit(gentity_t *e) {
    if (!MusicValidate(jarMusicDefault)) return qfalse;
    Q_strncpyz(objects[e->s.number].controllerText, jarMusicDefault, 64);
    e->use = MusicUse;
    return qtrue;
}

static void MusicState(int n, jarControllerState_t *state) {
    state->active = musicRank[n] != 0;
    state->inactive = (g_entities[n].flags & FL_INACTIVE) != 0;
    Q_strncpyz(state->text, objects[n].controllerText, sizeof(state->text));
}

static qboolean MusicApply(int n, const jarControllerState_t *state) {
    if (!JAR_ControllerStateValid(JAR_MUSIC_CONTROLLER, state) ||
        !MusicValidate(state->text)) return qfalse;
    const qboolean changed = strcmp(objects[n].controllerText, state->text) != 0;
    Q_strncpyz(objects[n].controllerText, state->text, sizeof(objects[n].controllerText));
    if (state->inactive) g_entities[n].flags |= FL_INACTIVE;
    else g_entities[n].flags &= ~FL_INACTIVE;
    if (!state->active) MusicStop(n);
    else if (!musicRank[n]) MusicStart(n);
    else if (changed && musicRank[n] == musicCount) musicDirty = qtrue;
    return qtrue;
}
