// Production music callbacks and persistence against native service boundaries.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "memory_save.h"
#include <deque>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;static std::string current,spawnMusic;static bool destroyOnUse=false;static unsigned writes=0;
cvar_t buildScript{};cvar_t*com_buildScript=&buildScript;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);}
char*vtos(const vec3_t){return const_cast<char*>("test");}
void G_Error(const char*,...){throw std::runtime_error("native spawn error");}
qboolean G_SpawnString(const char*k,const char*,char**out){CHECK(!strcmp(k,"music"));*out=const_cast<char*>(spawnMusic.c_str());return spawnMusic.empty()?qfalse:qtrue;}
qboolean G_ActivateBehavior(gentity_t*e,int){if(destroyOnUse)e->inuse=qfalse;return qfalse;}
#include "game/g_target_music.inc"
static gentity_t&Make(unsigned n,const char*track,bool portable=true){auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.classname=G_NewString("target_play_music");e.model=portable?G_NewString("@jar_music"):nullptr;e.targetname=G_NewString(va("owner_%u",n));spawnMusic=track;SP_target_play_music(&e);return e;}
static void Use(gentity_t&e){target_play_music_use(&e,nullptr,nullptr);}
int main(){return RunTests("portable music, stock properties, mission ownership, stacked cleanup and save/load",[]{
 gi.SetConfigstring=[](int n,const char*s){CHECK(n==CS_MUSIC);current=s;++writes;};gi.GetConfigstring=[](int n,char*out,int capacity){CHECK(n==CS_MUSIC);Q_strncpyz(out,current.c_str(),capacity);};gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};
 using namespace jar::music;CHECK(trackCount>100);for(const auto*track:tracks)CHECK(Valid(track));
 std::string normalized;CHECK(Canonical("MUSIC/HOTH2/HOTH2_EXPLORE.MP3",normalized)&&normalized==defaultTrack);
 const std::string pair=std::string(tracks[0])+" "+tracks[1];CHECK(Valid(pair));
 for(const char*bad:{"","hoth2","music/../save.mp3","music/missing.mp3","music/death_music.mp3;quit","music/death_music.mp3\n","music/death_music.mp3  music/endcredits.mp3"})CHECK(!Valid(bad));
 CHECK(!Valid(std::string(defaultTrack)+std::string("\0bad",4)));CHECK(!strcmp(Cycle(tracks[0],-1),tracks[trackCount-1]));
 auto&a=Make(10,defaultTrack);auto&b=Make(11,tracks[0]);auto&authored=Make(12,"native_dynamic_group",false);
 CHECK(a.e_UseFunc==useF_target_play_music_use&&G_PortableMusic(a)&&!G_PortableMusic(authored));
 G_SetMusic("mission_intro mission_loop");Use(a);CHECK(current==defaultTrack&&G_MusicActive(a));Use(b);CHECK(current==tracks[0]&&G_MusicActive(a)&&G_MusicActive(b));
 Use(a);CHECK(current==tracks[0]&&!G_MusicActive(a));Use(b);CHECK(current=="mission_intro mission_loop"&&!G_MusicActive(b));
 Use(a);Use(b);CHECK(G_MusicSetTrack(a,tracks[1])&&current==tracks[0]);G_MusicForget(&b);b.inuse=qfalse;CHECK(current==tracks[1]);
 CHECK(G_MusicSetTrack(a,pair.c_str())&&current==pair);CHECK(!G_MusicSetTrack(a,"music/missing.mp3")&&current==pair);
 G_MusicForget(&a);a.inuse=qfalse;CHECK(current=="mission_intro mission_loop");a.inuse=qtrue;b.inuse=qtrue;
 Use(a);Use(authored);CHECK(current=="native_dynamic_group"&&!G_MusicActive(a));G_MusicForget(&a);CHECK(current=="native_dynamic_group");
 // A mission cue identical to the active portable cue still owns playback.
 Use(a);G_SetMusic(a.message);CHECK(!G_MusicActive(a));G_MusicForget(&a);CHECK(current==a.message);
 G_SetMusic("original");Use(a);current="external_configstring";G_MusicReleaseAll();CHECK(current=="external_configstring");
 G_SetMusic("original");Use(a);Use(b);G_MusicReleaseAll();CHECK(current=="original"&&!G_MusicActive(a)&&!G_MusicActive(b));
 // Full saves retain order, identity and the exact original dynamic music cue.
 Use(a);Use(b);MemorySave full;ojk::SavedGameHelper h(&full);G_MusicExport(false,h);G_MusicReset();full.reset_buffer_offset();CHECK(G_MusicImport(false,h));h.ensure_all_data_read();CHECK(G_MusicActive(a)&&G_MusicActive(b));
 G_MusicForget(&b);CHECK(current==a.message);G_MusicForget(&a);CHECK(current=="original");
 // Wrong slot identity and duplicate owner entries cannot commit a registry.
 current=b.message;b.targetname=G_NewString("reused");full.reset_buffer_offset();CHECK(!G_MusicImport(false,h));b.targetname=G_NewString("owner_11");
 auto*raw=static_cast<unsigned char*>(const_cast<void*>(full.get_buffer_data()));const size_t header=4+MAX_STRING_CHARS+textLimit+1;memcpy(raw+header+68,raw+header,68);full.reset_buffer_offset();CHECK(!G_MusicImport(false,h));
 // Autosaves omit controller entities and restore the pre-controller track.
 G_SetMusic("autosave_map_music");Use(a);MemorySave automatic;ojk::SavedGameHelper autoHelper(&automatic);G_MusicExport(true,autoHelper);CHECK(current==a.message);G_MusicReset();a.inuse=b.inuse=qfalse;automatic.reset_buffer_offset();CHECK(G_MusicImport(true,autoHelper));autoHelper.ensure_all_data_read();CHECK(current=="autosave_map_music");
 // A load containing a later native cue does not rewind that cue.
 current="later_mission";automatic.reset_buffer_offset();CHECK(G_MusicImport(true,autoHelper)&&current=="later_mission");
 a.inuse=qtrue;destroyOnUse=true;Use(a);CHECK(current=="later_mission"&&!G_MusicActive(a));destroyOnUse=false;
 a.inuse=qtrue;Use(a);a.targetname=G_NewString("another_lifetime");G_MusicReconcile();CHECK(current=="later_mission"&&!G_MusicActive(a));
 // Empty, malformed and excessive-count chunks reject before changing music.
 MemorySave invalid;ojk::SavedGameHelper bad(&invalid);bad.write<uint32_t>(4097);invalid.reset_buffer_offset();CHECK(!G_MusicImport(false,bad)&&current=="later_mission");
 CHECK(writes>15);
 });}
