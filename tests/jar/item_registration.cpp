// Exercise production item registration around the full-save config restore.
#include "game/g_local.h"
#include "test_support.h"
#include <string>

game_import_t gi = {};
gitem_t bg_itemlist[32] = {};
extern const int bg_numItems = 32;
extern char itemRegistered[MAX_ITEMS + 1];
static std::string config;

void NORETURN QDECL G_Error(const char *, ...) { throw std::runtime_error("unexpected G_Error"); }

static void ResetRegistration() {
    std::memset(itemRegistered, '0', bg_numItems);
    itemRegistered[bg_numItems] = 0;
    config.assign(bg_numItems, '0');
}

int main() {
    return RunTests("full-save pickup precache survives later registration and a second save/load", [] {
        gi.SetConfigstring = [](int index, const char *value) { CHECK(index == CS_ITEMS); config = value; };
        gi.GetConfigstring = [](int index, char *buffer, int size) { CHECK(index == CS_ITEMS); Q_strncpyz(buffer, config.c_str(), size); };

        // Initial randomized map: slot 7 was replaced by slot 8.
        ResetRegistration(); RegisterItem(&bg_itemlist[8]);
        const std::string firstSave = config;
        CHECK(firstSave[8] == '1' && firstSave[7] == '0');

        // Full load initializes the unrandomized BSP, then restores CS_ITEMS.
        ResetRegistration(); RegisterItem(&bg_itemlist[7]);
        config = firstSave;
        RestoreRegisteredItems();
        RegisterItem(&bg_itemlist[2]); // later unrelated NPC/weapon registration
        CHECK(config[8] == '1');
        CHECK(config[2] == '1');
        CHECK(config[7] == '0');

        // A subsequent save/reload must still precache the randomized model/icon.
        const std::string secondSave = config;
        ResetRegistration(); RegisterItem(&bg_itemlist[7]);
        config = secondSave;
        RestoreRegisteredItems();
        SaveRegisteredItems();
        CHECK(config == secondSave);
        RegisterItem(&bg_itemlist[3]); CHECK(config[8] == '1' && config[2] == '1');

        // Short/invalid configstrings clear stale bits and stay terminated.
        config = "01x";
        RestoreRegisteredItems(); SaveRegisteredItems();
        CHECK(config.size() == bg_numItems);
        CHECK(config[1] == '1');
        CHECK(config[2] == '0' && config[7] == '0' && config[8] == '0');
        RegisterItem(&bg_itemlist[bg_numItems - 1]);
        CHECK(config.back() == '1' && config.size() == bg_numItems);
    });
}
