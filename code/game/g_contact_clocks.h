// Per-entity cooldowns for portable Hurt and Push controllers. GPL-2.0-only.
#ifndef G_CONTACT_CLOCKS_H
#define G_CONTACT_CLOCKS_H
struct gentity_s;
typedef struct gentity_s gentity_t;
namespace ojk {class SavedGameHelper;}
bool G_ContactClockReady(const gentity_t& trigger,const gentity_t& actor,int interval);
void G_ContactClocksReset();
void G_ContactClocksExport(bool autosave,ojk::SavedGameHelper& save);
bool G_ContactClocksImport(bool autosave,ojk::SavedGameHelper& save);
#endif
