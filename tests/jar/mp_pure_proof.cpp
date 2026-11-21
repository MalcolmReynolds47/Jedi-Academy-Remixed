// Exercise the production proof after native-module loading leaves no DLL flags.
#include "qcommon/qcommon.h"
#include "../../codemp/qcommon/q_shared.c"
#include <cassert>
#include <cstdio>
#include <cstdlib>

struct fileInPack_t {
    const char *name;
    fileInPack_t *next;
};

struct pack_t {
    int checksum;
    int pure_checksum;
    int referenced;
    int hashSize;
    fileInPack_t **hashTable;
};

struct searchpath_t {
    searchpath_t *next;
    pack_t *pack;
};

static searchpath_t *fs_searchpaths;
static int fs_numServerPaks, fs_checksumFeed, fs_fakeChkSum;
static int serverPaks[5];

static void FS_AssertInitialised() { assert(fs_searchpaths); }
static long FS_HashFileName(const char *, int size) { assert(size == 1); return 0; }
qboolean FS_FilenameCompare(const char *a, const char *b) { return Q_stricmp(a, b) ? qtrue : qfalse; }
static bool FS_PakIsPure(const pack_t *pack) {
    if (!fs_numServerPaks) return true;
    for (int i = 0; i < fs_numServerPaks; ++i) {
        if (pack->checksum == serverPaks[i]) return true;
    }
    return false;
}
void QDECL Com_Error(int, const char *, ...) { std::abort(); }
void QDECL Com_Printf(const char *, ...) {}

#include "../../codemp/qcommon/fs_pure_proof.inc"

int main() {
    fileInPack_t cgame{"cgamex86.dll", nullptr};
    fileInPack_t ui{"uix86.dll", nullptr};
    fileInPack_t otherFile{"unused.cfg", &ui};
    fileInPack_t both{"cgamex86.dll", &ui};
    fileInPack_t *cgameHash[]{&cgame}, *uiHash[]{&otherFile}, *bothHash[]{&both}, *emptyHash[]{nullptr};
    pack_t unlisted{123, 900, FS_CGAME_REF | FS_UI_REF | FS_GENERAL_REF, 1, bothHash};
    pack_t firstCgame{1, -500, 0, 1, cgameHash};
    pack_t secondCgame{2, 700, FS_CGAME_REF, 1, cgameHash};
    pack_t retailUI{3, 600, 0, 1, uiHash};
    pack_t baseMap{4, -200, FS_GENERAL_REF, 1, emptyHash};
    pack_t sandboxAssets{5, 300, FS_GENERAL_REF, 1, emptyHash};
    searchpath_t last{nullptr, &baseMap};
    searchpath_t game{&last, &sandboxAssets};
    searchpath_t uiPath{&game, &retailUI};
    searchpath_t shadow{&uiPath, &secondCgame};
    searchpath_t cgamePath{&shadow, &firstCgame};
    searchpath_t unlistedPath{&cgamePath, &unlisted};
    searchpath_t loose{&unlistedPath, nullptr};
    fs_searchpaths = &loose;
    fs_checksumFeed = 42;

    for (int i = 0; i < 5; ++i) serverPaks[i] = i + 1;
    fs_numServerPaks = 5;
    char expected[160];
    Com_sprintf(expected, sizeof(expected), "-500 600 @ 300 -200 %i ", 42 ^ 300 ^ -200 ^ 2);
    assert(!strcmp(FS_ReferencedPakPureChecksums(), expected));
    int selected = 0;
    assert(FS_FileIsInPAK("CGAMEX86.DLL", &selected) == 1 && selected == -500);
    assert(FS_FileIsInPAK("uix86.dll", &selected) == 1 && selected == 600);
    assert(FS_FileIsInPAK("../uix86.dll", &selected) == -1);

    // A vid_restart clears native references. The proof still uses the current
    // filesystem search order and feed, including when modules share a PK3.
    secondCgame.referenced = 0;
    fs_checksumFeed = 21;
    firstCgame.hashTable = bothHash;
    Com_sprintf(expected, sizeof(expected), "-500 -500 @ 300 -200 %i ", 21 ^ 300 ^ -200 ^ 2);
    assert(!strcmp(FS_ReferencedPakPureChecksums(), expected));
    firstCgame.hashTable = cgameHash;
    retailUI.hashTable = emptyHash;
    Com_sprintf(expected, sizeof(expected), "-500 @ 300 -200 %i ", 21 ^ 300 ^ -200 ^ 2);
    assert(!strcmp(FS_ReferencedPakPureChecksums(), expected)); // No invented UI proof.

    fs_fakeChkSum = 81;
    assert(!strncmp(FS_ReferencedPakPureChecksums(), "-500 81 81 @ ", 13));
    fs_fakeChkSum = 0;
    fs_numServerPaks = 0;
    Com_sprintf(expected, sizeof(expected), "900 900 @ 900 300 -200 %i ", 21 ^ 900 ^ 300 ^ -200 ^ 3);
    assert(!strcmp(FS_ReferencedPakPureChecksums(), expected));
    puts("MP pure archive proof: native-module markers, allowlist, precedence, feed, missing marker and non-pure references passed.");
}
