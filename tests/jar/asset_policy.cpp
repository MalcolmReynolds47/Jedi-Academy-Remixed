#include "sys/jar_asset_policy.h"
#include "test_support.h"
int main(){return RunTests("JA:R-only base package isolation",[]{using jar::assets::SkipBasePak;
 CHECK(SkipBasePak("JediAcademyRemixedMP","","base","japro-assets.pk3"));CHECK(SkipBasePak("JEDIACADEMYREMIXEDMP",nullptr,"BASE","JAPRO-WIN-X86.PK3"));
 CHECK(!SkipBasePak("japro","","base","japro-assets.pk3"));CHECK(!SkipBasePak("base","","base","japro-assets.pk3"));CHECK(!SkipBasePak("JediAcademyRemixedMP","another_mod","base","japro-assets.pk3"));
 CHECK(!SkipBasePak("JediAcademyRemixedMP","","JediAcademyRemixedMP","japro-assets.pk3"));
 for(const char*n:{"assets0.pk3","assets3.pk3","JKA_Flatgrass.pk3","zMap_Menus.pk3","my_model_pack.pk3"})CHECK(!SkipBasePak("JediAcademyRemixedMP","","base",n));
 });}
