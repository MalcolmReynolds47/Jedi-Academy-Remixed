// Initial opt-in portable-content gate; legacy authored map instances retain
// their existing route. GPL-2.0-only.
#include "qcommon/qfiles.h"
#include "g_subbsp_admission.h"
#include "g_spawn_transaction.h"
#include "../../shared/qcommon/jar_bsp_model.h"
#include "../../shared/qcommon/jar_yavin_sky.h"
#include "qcommon/jar_force_enlightenment.h"
uint32_t Com_BlockChecksum(const void*,int);
// Match G_FindConfigstringIndex's first-empty semantics. Preflight all new
// unique names without touching game state or invoking fatal allocation paths.
static bool BSPSectionConfigstringsFit(int start,int capacity,const std::vector<std::string>&resources){
    if(resources.empty())return true;
    std::vector<std::string> missing;
    for(const auto&resource:resources)if(std::none_of(missing.begin(),missing.end(),[&](const std::string&name){return !Q_stricmp(name.c_str(),resource.c_str());}))missing.push_back(resource);
    int i=1;
    for(;i<capacity&&!missing.empty();++i){
        char name[MAX_STRING_CHARS];gi.GetConfigstring(start+i,name,sizeof(name));if(!name[0])break;
        missing.erase(std::remove_if(missing.begin(),missing.end(),[&](const std::string&item){return !Q_stricmp(item.c_str(),name);}),missing.end());
    }
    return missing.size()<=size_t(capacity-i);
}
static bool G_AdmitPortableSubBSP(const char*model,jar::bspAdmission::Report&report,jar::bspSection::Mode mode=jar::bspSection::Mode::Generic)
{
    using namespace jar::bspAdmission;
    report=Report{};
    char path[MAX_QPATH];int child;
    if(!JAR_BSPModelName(model,path,&child)||strchr(model,':'))return Fail(report,"Invalid portable BSP resource name");
    if(!G_OptionalSpawnActive())return Fail(report,"Portable BSP spawning requires an optional allocation transaction");
    fileHandle_t handle=0;const int size=gi.FS_FOpenFile(path,&handle,FS_READ);
    struct Close {fileHandle_t h;~Close(){if(h)gi.FS_FCloseFile(h);}} close{handle};
    if(!handle||size<int(sizeof(dheader_t))||size_t(size)>maxFileBytes)return Fail(report,"Portable BSP file is missing or outside the size limit");
    std::vector<byte> bytes(size);
    if(gi.FS_Read(bytes.data(),size,handle)!=size)return Fail(report,"Portable BSP read was incomplete");
    if(!Inspect(bytes.data(),bytes.size(),report,mode))return false;
    const unsigned sourceChecksum=LittleLong(Com_BlockChecksum(bytes.data(),size));
    if(mode==jar::bspSection::Mode::Shaft&&sourceChecksum!=jar::shaft::checksum)
        return Fail(report,"Imperial Shaft requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::Rift&&sourceChecksum!=jar::rift::checksum)
        return Fail(report,"Rift Sanctuary requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::Korriban&&sourceChecksum!=jar::korriban::checksum)
        return Fail(report,"Korriban FFA requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::Yavin&&sourceChecksum!=jar::yavinSky::checksum)
        return Fail(report,"Yavin Training Grounds requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::Sentinel&&sourceChecksum!=jar::sentinel::checksum)
        return Fail(report,"Vjun Sentinel requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::Tatooine&&sourceChecksum!=jar::tatooine::checksum)
        return Fail(report,"Tatooine FFA requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::Taspir&&sourceChecksum!=jar::taspir::checksum)
        return Fail(report,"Taspir requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::Factory&&sourceChecksum!=jar::factory::checksum)
        return Fail(report,"Factory requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::Coruscant&&sourceChecksum!=jar::coruscant::checksum)
        return Fail(report,"Coruscant Streets requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::Hilltops&&sourceChecksum!=jar::hilltops::checksum)
        return Fail(report,"Yavin Hilltops requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::DropZone&&sourceChecksum!=jar::dropzone::checksum)
        return Fail(report,"Imperial Drop Zone requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::Wasteland&&sourceChecksum!=jar::wasteland::checksum)
        return Fail(report,"Hoth Wasteland requires the qualified retail BSP");
    if(mode==jar::bspSection::Mode::SiegeKorriban&&sourceChecksum!=jar::siegeKorriban::checksum)
        return Fail(report,"Siege Korriban requires the qualified retail BSP");
    // The last clip handle belongs to the engine's temporary collision box.
    constexpr int inlineCapacity=MAX_SUBMODELS-1;
    int first,count;unsigned checksum;char registered[MAX_QPATH];
    if(gi.BSPModelAt(-1,registered,sizeof(registered),&first,&count,&checksum)!=1||first||count<1||count>inlineCapacity)
        return Fail(report,"Main BSP registry is unavailable");
    int next=count,slots=0;bool cached=false;
    for(;slots<MAX_SUB_BSP;++slots){
        const int found=gi.BSPModelAt(slots,registered,sizeof(registered),&first,&count,&checksum);
        if(!found)break;
        if(found!=1||first!=next||count<1||count>inlineCapacity-next)return Fail(report,"BSP registry is inconsistent");
        // Use the same source checksum as native collision registration and
        // save restoration. Never qualify new bytes while reusing old content.
        if(!Q_stricmp(registered,model)){
            if(cached||count!=report.models||checksum!=sourceChecksum)return Fail(report,"Portable BSP differs from the cached collision resource");
            cached=true;
        }
        next+=count;
    }
    if(!cached&&(slots==MAX_SUB_BSP||report.models>inlineCapacity-next))return Fail(report,"Portable BSP exceeds the remaining brush-model capacity");
    std::vector<std::string> models,sounds,effects;
    for(const auto& name:report.pickupClasses){
        const auto* item=FindItem(name.c_str());
        const auto* enlightenment=jar::enlightenment::Definition(name.c_str());
        const bool forcePickup=name==jar::forceBoon::classname||enlightenment;
        if(forcePickup&&(!item||(mode!=jar::bspSection::Mode::Tatooine&&mode!=jar::bspSection::Mode::Taspir&&
           !jar::bspSection::CapturePractice(mode))||item->giType!=IT_POWERUP||
           item->giTag!=(enlightenment?enlightenment->tag:PW_FORCE_BOON)||
           !item->pickup_sound||gi.FS_ReadFile(item->pickup_sound,nullptr)<=0))
            return Fail(report,"Portable section Force pickup is unavailable: "+name);
        if(!item||!item->world_model||!Resource(item->world_model)||
           !OneOf(Lower(COM_GetExtension(item->world_model)),{"glm","md3"})||
           item->quantity<0||item->quantity>1000||
           (item->giType!=IT_HEALTH&&item->giType!=IT_ARMOR&&item->giType!=IT_AMMO&&item->giType!=IT_WEAPON&&!forcePickup)||
           gi.FS_ReadFile(item->world_model,nullptr)<=0)
            return Fail(report,"Portable section pickup is unavailable: "+name);
        models.push_back(item->world_model);
    }
    for(const auto&resource:report.resources){
        if(gi.FS_ReadFile(resource.c_str(),nullptr)<=0)return Fail(report,"Portable section resource is missing: "+resource);
        const std::string ext=Lower(resource.substr(resource.size()-4));
        if(ext==".md3"||ext==".glm")models.push_back(resource);
        else if(ext==".wav"||ext==".mp3")sounds.push_back(resource.substr(0,resource.size()-4));
        else if(ext==".efx")effects.push_back(resource.substr(8,resource.size()-12)); // effects/ + .efx
    }
    if(!BSPSectionConfigstringsFit(CS_BSP_MODELS,MAX_SUB_BSP,{model})||!BSPSectionConfigstringsFit(CS_MODELS,MAX_MODELS,models)||
       !BSPSectionConfigstringsFit(CS_SOUNDS,MAX_SOUNDS,sounds)||!BSPSectionConfigstringsFit(CS_EFFECTS,MAX_FX,effects))
        return Fail(report,"Portable section exceeds remaining map/model/sound/effect registration capacity");
    return true;
}
