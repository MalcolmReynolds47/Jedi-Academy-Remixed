#include "../../shared/qcommon/jar_saber_flash.h"
#include "qcommon/jar_appearance.h"
#include "qcommon/jar_effect_fallback.h"
#include "test_support.h"
#include "qcommon/jar_ui_theme.h"
#include <string>
#include <set>

int main() {
    return RunTests("display bounds, scoped asset selection and sound encoding fallback", [] {
        for(const char*name:{"saber/saber_cut","saber/saber_block.efx","jar/fx/mbii/effects/saber/saber_cut","SABER_CUT"})CHECK(JAR_SaberContactEffect(name));
        CHECK(!JAR_SaberContactEffect("rocket/explosion"));CHECK(!JAR_SaberContactEffect(nullptr));
        for(const auto scale:{1.f,1.5f,2.25f,4.5f,9.f}){
            const float xs=scale*4.f/3;const auto c=jar::ui::MenuCursor(140,210,xs,scale);
            CHECK(std::fabs(c.w*xs-c.h*scale)<.001f); // no widescreen distortion
            CHECK(std::fabs(c.x+c.w*12.f/256-140)<.001f);
            CHECK(std::fabs(c.y+c.h*12.f/256-210)<.001f); // visible tip = click coordinate
            CHECK(c.h*scale>=28 && c.h*scale<=64);
        }
        CHECK(jar::ui::MenuCursor(0,0,0,-1).w==28);
        for (const char *key:{"ships/bomb","ships/ion_blastershot"}) {
            CHECK(jar::MissingEffectFallback(key,false));
            CHECK(!jar::MissingEffectFallback(key,true)); // custom/empty/malformed existing file wins
            CHECK(!jar::MissingEffectFallback(key,false,true)); // editor path remains exact
        }
        CHECK(!jar::MissingEffectFallback("ships/unknown",false));
        CHECK(!jar::MissingEffectFallback(nullptr,false));
        CHECK(jar::ResolutionValid(1920,1080));
        CHECK(jar::ResolutionValid(7680,4320));
        CHECK(jar::ResolutionValid(16384,4096));
        CHECK(!jar::ResolutionValid(16384,16384));
        CHECK(!jar::ResolutionValid(-1,1080));
        CHECK(!jar::ResolutionValid(1920,0));
        for (const char *bad : {"", "-1", "1e3", "1024;quit", " 1024", "99999999999999999999"})
            CHECK(!jar::DecimalInRange(bad,320,16384));
        CHECK(jar::DecimalInRange("2560",320,16384));
        for (const char *path : {"autoexec_sp.cfg","saves/quick.sav","models/players/kyle/model.glm",
                                "scripts/map.txt","maps/t1_fatal.bsp","gfx/2d/dlight.jpg"})
            CHECK(!jar::AssetCategory(path));
        CHECK(!std::strcmp(jar::AssetCategory("gfx/menus/anim/frame01.tga"),"ui"));
        CHECK(!std::strcmp(jar::AssetCategory("gfx/2d/charsgrid_med.tga"),"font"));
        char path[64];
        CHECK(!jar::AssetOverlay(path,sizeof(path),"fonts/test.tga","font","../base"));
        CHECK(!jar::AssetOverlay(path,sizeof(path),"fonts/test.tga","font","stock"));
        CHECK(jar::AssetOverlay(path,sizeof(path),"fonts/test.tga","font","sans"));
        CHECK(!std::strcmp(path,"jar/font/sans/fonts/test.tga"));
        CHECK(!jar::AssetOverlay(path,10,"fonts/test.tga","font","sans"));
        std::set<std::string> files={"sound/weapons/saber/saberon.wav","jar/sound/mbii/sound/weapons/saber/saberon.mp3"};
        const auto exists=[&](const char *name) { return files.count(name)!=0; };
        CHECK(jar::SoundOverlay(path,sizeof(path),"sound/weapons/saber/saberon.wav","mbii",exists));
        CHECK(!std::strcmp(path,"jar/sound/mbii/sound/weapons/saber/saberon.mp3"));
        CHECK(!jar::SoundOverlay(path,sizeof(path),"sound/weapons/saber/missing.wav","mbii",exists));
        CHECK(!jar::SoundOverlay(path,sizeof(path),"sound/weapons/saber/saberon.wav","stock",exists));
        CHECK(!jar::SoundOverlay(path,sizeof(path),"sound/chars/kyle/test.wav","mbii",exists));
    });
}
