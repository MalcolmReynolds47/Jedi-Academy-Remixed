// Portable music uses the same stock paths as SP. In controller records the
// fixed music/ prefix and .mp3 extension are implicit, so a pair fits JRW18.
// GPL-2.0-only.
#pragma once
#include <string.h>
#include "../../code/qcommon/jar_music_tracks.h"

#define JAR_MUSIC_CUE_TAG "@jar_music"

enum { JAR_MUSIC_CONTROLLER = 34, JAR_MUSIC_CUE_SIZE = 80 };
static const char jarMusicDefault[] = "hoth2/hoth2_explore";

static size_t JAR_MusicStemLength(const char *path) {
    return strlen(path) - 10;
}

static int JAR_MusicFind(const char *text, size_t length, int compact) {
    for (int i = 0; i < (int)(sizeof(jarMusicTracks) / sizeof(jarMusicTracks[0])); ++i) {
        const char *path = jarMusicTracks[i];
        const size_t size = compact ? JAR_MusicStemLength(path) : strlen(path);
        if (size != length) continue;
        if (compact ? !strncmp(text, path + 6, length) : !Q_stricmpn(text, path, length)) return i;
    }
    return -1;
}

static int JAR_MusicTracks(const char *cue, int compact, int *first, int *second) {
    const char *space = strchr(cue, ' ');
    *first = JAR_MusicFind(cue, space ? (size_t)(space - cue) : strlen(cue), compact);
    *second = space ? JAR_MusicFind(space + 1, strlen(space + 1), compact) : -1;
    return *first >= 0 && (!space || *second >= 0);
}

static size_t JAR_MusicWriteStem(char *out, int track) {
    const char *path = jarMusicTracks[track];
    const size_t length = JAR_MusicStemLength(path);
    memcpy(out, path + 6, length);
    out[length] = '\0';
    return length;
}

static int JAR_MusicPack(const char *cue, char out[64]) {
    int first, second;
    if (!JAR_MusicTracks(cue, 0, &first, &second)) return 0;
    size_t length = JAR_MusicStemLength(jarMusicTracks[first]);
    if (length + (second < 0 ? 0 : 1 + JAR_MusicStemLength(jarMusicTracks[second])) >= 64) return 0;
    JAR_MusicWriteStem(out, first);
    if (second >= 0) {
        out[length] = ' ';
        JAR_MusicWriteStem(out + length + 1, second);
    }
    return 1;
}

static int JAR_MusicUnpack(const char *cue, char out[JAR_MUSIC_CUE_SIZE]) {
    int first, second;
    if (!JAR_MusicTracks(cue, 1, &first, &second)) return 0;
    Com_sprintf(out, JAR_MUSIC_CUE_SIZE, "%s%s%s", jarMusicTracks[first],
        second < 0 ? "" : " ", second < 0 ? "" : jarMusicTracks[second]);
    return 1;
}

static void JAR_MusicCycle(const char *cue, int direction, char out[64]) {
    const int count = (int)(sizeof(jarMusicTracks) / sizeof(jarMusicTracks[0]));
    int first, second;
    if (!JAR_MusicTracks(cue, 1, &first, &second)) {
        Q_strncpyz(out, jarMusicDefault, 64);
        return;
    }
    memset(out, 0, 64);
    JAR_MusicWriteStem(out, (first + count + direction) % count);
}
