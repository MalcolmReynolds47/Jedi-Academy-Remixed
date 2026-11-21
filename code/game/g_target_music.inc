// A stack of portable controllers restores the previous music on release.
// Native mission/console music replaces the stack, including identical cues.
// GPL-2.0-only. Included by g_target.cpp and the production-code fixture.
#include "g_target_music.h"
#include "qcommon/jar_music.h"
#include "qcommon/ojk_saved_game_helper.h"
#include <algorithm>
#include <vector>
namespace {
struct MusicOwner {unsigned entity;std::string identity;};
std::vector<MusicOwner> musicOwners;
std::string musicBaseline,musicApplied;
constexpr unsigned musicOwnerLimit=4096;
std::string musicCurrent(){char text[MAX_STRING_CHARS]{};gi.GetConfigstring(CS_MUSIC,text,sizeof(text));return text;}
bool musicMatches(const MusicOwner&owner){if(owner.entity>=ENTITYNUM_WORLD)return false;const auto&e=g_entities[owner.entity];return e.inuse&&G_PortableMusic(e)&&e.targetname&&owner.identity==e.targetname&&e.message&&jar::music::Valid(e.message);}
void musicPublish(const std::string&text){musicApplied=text;gi.SetConfigstring(CS_MUSIC,text.c_str());}
void musicRestoreTop(){
 while(!musicOwners.empty()&&!musicMatches(musicOwners.back()))musicOwners.pop_back();
 if(musicOwners.empty()){const auto previous=musicBaseline;G_MusicReset();gi.SetConfigstring(CS_MUSIC,previous.c_str());}
 else musicPublish(g_entities[musicOwners.back().entity].message);
}
}
bool G_PortableMusic(const gentity_t&e){return jar::music::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_music");}
void G_MusicReset(){musicOwners.clear();musicBaseline.clear();musicApplied.clear();}
void G_SetMusic(const char*text){G_MusicReset();gi.SetConfigstring(CS_MUSIC,text?text:"");}
void G_MusicReconcile(){
 if(musicOwners.empty())return;
 if(musicCurrent()!=musicApplied){G_MusicReset();return;}
 const unsigned top=musicOwners.back().entity;
 musicOwners.erase(std::remove_if(musicOwners.begin(),musicOwners.end(),[](const MusicOwner&o){return !musicMatches(o);}),musicOwners.end());
 if(musicOwners.empty()||musicOwners.back().entity!=top)musicRestoreTop();
}
bool G_MusicActive(const gentity_t&e){G_MusicReconcile();for(const auto&o:musicOwners)if(o.entity==unsigned(e.s.number))return true;return false;}
void G_MusicReleaseAll(){G_MusicReconcile();if(musicOwners.empty())return;const auto original=musicBaseline;G_MusicReset();gi.SetConfigstring(CS_MUSIC,original.c_str());}
void G_MusicForget(gentity_t*e){
 if(!e||musicOwners.empty())return;G_MusicReconcile();if(musicOwners.empty())return;const bool top=musicOwners.back().entity==unsigned(e->s.number);
 musicOwners.erase(std::remove_if(musicOwners.begin(),musicOwners.end(),[&](const MusicOwner&o){return o.entity==unsigned(e->s.number);}),musicOwners.end());if(top)musicRestoreTop();
}
bool G_MusicSetTrack(gentity_t&e,const char*text){
 std::string normalized;if(!e.inuse||!jar::music::Class(e.classname)||!jar::music::Canonical(text,normalized))return false;
 G_MusicReconcile();if(e.message&&gi.bIsFromZone(e.message,TAG_G_ALLOC))gi.Free(e.message);e.message=G_NewString(normalized.c_str());
 if(!musicOwners.empty()&&musicOwners.back().entity==unsigned(e.s.number))musicPublish(normalized);return true;
}
void target_play_music_use(gentity_t*self,gentity_t*,gentity_t*){
 if(!self||!self->inuse)return;const std::string identity=self->targetname?self->targetname:"";G_ActivateBehavior(self,BSET_USE);
 if(!self->inuse||!jar::music::Class(self->classname)||identity!=(self->targetname?self->targetname:""))return;
 if(!G_PortableMusic(*self)){G_SetMusic(self->message);return;}
 if(!self->message||!jar::music::Valid(self->message)||identity.empty()||identity.size()>63)return;
 G_MusicReconcile();if(G_MusicActive(*self)){G_MusicForget(self);return;}
 if(musicOwners.size()==musicOwnerLimit)return;
 if(musicOwners.empty())musicBaseline=musicCurrent();musicOwners.push_back({unsigned(self->s.number),identity});musicPublish(self->message);
}
// Property strings are already native saved fields. The separate chunk stores
// activation order and the exact pre-Sandbox cue, without borrowing entity fields.
void G_MusicExport(bool autosave,ojk::SavedGameHelper&h){
 G_MusicReconcile();const unsigned count=autosave?0:unsigned(musicOwners.size());h.write<uint32_t>(count);
 char baseline[MAX_STRING_CHARS]{},applied[jar::music::textLimit+1]{};Q_strncpyz(baseline,musicBaseline.c_str(),sizeof(baseline));Q_strncpyz(applied,musicApplied.c_str(),sizeof(applied));h.write<char>(baseline,sizeof(baseline));h.write<char>(applied,sizeof(applied));
 if(!autosave)for(const auto&o:musicOwners){h.write<uint32_t>(o.entity);char name[64]{};Q_strncpyz(name,o.identity.c_str(),sizeof(name));h.write<char>(name,sizeof(name));}
}
bool G_MusicImport(bool autosave,ojk::SavedGameHelper&h){
 unsigned count;h.read<uint32_t>(count);if(count>musicOwnerLimit||(autosave&&count))return false;
 char baseline[MAX_STRING_CHARS],applied[jar::music::textLimit+1];h.read<char>(baseline,sizeof(baseline));h.read<char>(applied,sizeof(applied));
 if(!memchr(baseline,0,sizeof(baseline))||!memchr(applied,0,sizeof(applied)))return false;
 if((count&&!applied[0])||(!autosave&&!count&&applied[0])||(applied[0]&&!jar::music::Valid(applied)))return false;
 std::vector<MusicOwner> candidate;
 for(unsigned i=0;i<count;++i){unsigned entity;char name[64];h.read<uint32_t>(entity);h.read<char>(name,sizeof(name));if(!memchr(name,0,sizeof(name))||!name[0])return false;
  MusicOwner owner{entity,name};if(!musicMatches(owner)||std::any_of(candidate.begin(),candidate.end(),[&](const MusicOwner&o){return o.entity==entity;}))return false;candidate.push_back(owner);
 }
 if(count&&strcmp(g_entities[candidate.back().entity].message,applied))return false;
 G_MusicReset();if(!applied[0]||musicCurrent()!=applied)return true;
 // Player-only autosaves omit Sandbox entities, so restore their baseline cue.
 if(autosave||candidate.empty()){gi.SetConfigstring(CS_MUSIC,baseline);return true;}
 musicOwners=std::move(candidate);musicBaseline=baseline;musicApplied=applied;return true;
}

void SP_target_play_music( gentity_t *self )
{
	char *s;
	G_SetOrigin( self, self->s.origin );
	if (!G_SpawnString( "music", "", &s )) {
		G_Error( "target_play_music without a music key at %s", vtos( self->s.origin ) );
	}
	if(self->message&&gi.bIsFromZone(self->message,TAG_G_ALLOC))gi.Free(self->message);
	self->message = G_NewString (s);
	self->e_UseFunc = useF_target_play_music_use;
extern	cvar_t	*com_buildScript;
	//Precache!
	if (com_buildScript->integer) {//copy this puppy over
		char buffer[MAX_QPATH];
		fileHandle_t	hFile;

		Q_strncpyz( buffer, s, sizeof(buffer) );
		COM_DefaultExtension( buffer, sizeof(buffer), ".mp3");

		gi.FS_FOpenFile(buffer, &hFile, FS_READ);
		if (hFile) {
			gi.FS_FCloseFile( hFile );
		}
	}
}
