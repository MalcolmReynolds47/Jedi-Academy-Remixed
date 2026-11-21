// Native archive lookup and client proof. Included by files.cpp.
// GPL-2.0-only.

int FS_FileIsInPAK(const char *filename, int *checksum) {
    FS_AssertInitialised();
    if (!filename) {
        Com_Error(ERR_FATAL, "FS_FileIsInPAK: NULL filename");
    }

    if (filename[0] == '/' || filename[0] == '\\') ++filename;
    if (strstr(filename, "..") || strstr(filename, "::")) return -1;

    for (const searchpath_t *search = fs_searchpaths; search; search = search->next) {
        pack_t *pack = search->pack;
        if (!pack) continue;
        const long hash = FS_HashFileName(filename, pack->hashSize);
        if (!pack->hashTable[hash] || !FS_PakIsPure(pack)) continue;

        for (const fileInPack_t *file = pack->hashTable[hash]; file; file = file->next) {
            if (FS_FilenameCompare(file->name, filename)) continue;
            if (checksum) *checksum = pack->pure_checksum;
            return 1;
        }
    }
    return -1;
}

const char *FS_ReferencedPakPureChecksums() {
    static char proof[BIG_INFO_STRING];
    proof[0] = '\0';
    int checksum = fs_checksumFeed;
    int count = 0;

    for (int flag = FS_CGAME_REF; flag; flag >>= 1) {
        if (flag == FS_GENERAL_REF) Q_strcat(proof, sizeof(proof), "@ ");

        if (fs_numServerPaks && flag != FS_GENERAL_REF) {
            // JA:R loads loose native modules. The server's stock proof still
            // identifies the archives containing these legacy module entries.
            // Resolve the same allowed entries without opening or loading them.
            const char *marker = flag == FS_CGAME_REF ? "cgamex86.dll" : "uix86.dll";
            int archiveChecksum;
            if (FS_FileIsInPAK(marker, &archiveChecksum) == 1) {
                Q_strcat(proof, sizeof(proof), va("%i ", archiveChecksum));
            }
        } else {
            for (const searchpath_t *search = fs_searchpaths; search; search = search->next) {
                pack_t *pack = search->pack;
                if (!pack || !(pack->referenced & flag)) continue;
                // The directory scan can mark every mod archive as referenced.
                // Only allowlisted archives can actually supply pure content.
                if (fs_numServerPaks && !FS_PakIsPure(pack)) continue;
                Q_strcat(proof, sizeof(proof), va("%i ", pack->pure_checksum));
                if (flag != FS_GENERAL_REF) break;
                checksum ^= pack->pure_checksum;
                ++count;
            }
        }
        if (fs_fakeChkSum) {
            Q_strcat(proof, sizeof(proof), va("%i ", fs_fakeChkSum));
        }
    }

    Q_strcat(proof, sizeof(proof), va("%i ", checksum ^ count));
    return proof;
}
