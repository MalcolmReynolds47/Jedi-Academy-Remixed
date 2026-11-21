// Owned portable playback, independent of native entity layout. GPL-2.0-only.
#ifndef G_TARGET_MUSIC_H
#define G_TARGET_MUSIC_H
struct gentity_s;
typedef struct gentity_s gentity_t;
namespace ojk {class SavedGameHelper;}
bool G_PortableMusic(const gentity_t&);
bool G_MusicSetTrack(gentity_t&,const char*);
bool G_MusicActive(const gentity_t&);
void G_MusicReset();
void G_MusicReconcile();
void G_MusicReleaseAll();
void G_MusicForget(gentity_t*);
void G_SetMusic(const char*);
void G_MusicExport(bool,ojk::SavedGameHelper&);
bool G_MusicImport(bool,ojk::SavedGameHelper&);
#endif
