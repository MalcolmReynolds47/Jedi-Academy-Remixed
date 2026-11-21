// Read through the native UI imports so map discovery follows the game's filesystem.
#include "ui_local.h"
#include "ui_jar_map_bsp.h"
#include "ui/jar_map_bsp.h"

namespace {
class HostBsp {
public:
    HostBsp(const uiImport_t* fs,const char* name): fs(fs) {
        std::snprintf(path,sizeof(path),"maps/%s.bsp",name);
        size=fs->FS_Open(path,&file,FS_READ);
        valid=file && size>=int(sizeof(header)) && ReadBytes(&header,sizeof(header)) &&
            jar::mapPicker::ValidHeader(header,size);
    }
    ~HostBsp() { if (file) fs->FS_Close(file); }
    HostBsp(const HostBsp&)=delete;
    HostBsp& operator=(const HostBsp&)=delete;
    bool Exists() const { return file!=0; }
    explicit operator bool() const { return valid; }
    int Offset(int index) const { return header.lumps[index].fileofs; }

    template<class T>
    bool Read(int index,int limit,std::vector<T>& result) {
        const auto& lump=header.lumps[index];
        if (!valid || lump.filelen>limit || size_t(lump.filelen)%sizeof(T)) return false;
        result.resize(lump.filelen/sizeof(T));
        return result.empty() || (Seek(lump.fileofs) && ReadBytes(result.data(),lump.filelen));
    }

private:
    bool ReadBytes(void* dest,int length) {
        if (fs->FS_Read(dest,length,file)!=length) return false;
        offset+=length;
        return true;
    }
    bool Seek(int target) {
        // The UI import table has no seek. Native PK3 seeking also inflates
        // the skipped bytes, so advance through a bounded scratch buffer.
        if (target<offset) {
            fs->FS_Close(file);
            if (fs->FS_Open(path,&file,FS_READ)!=size || !file) { valid=false; return false; }
            offset=0;
        }
        char scratch[32768];
        while (target>offset)
            if (!ReadBytes(scratch,std::min(target-offset,int(sizeof(scratch))))) return false;
        return true;
    }

    const uiImport_t* fs;
    dheader_t header{};
    char path[MAX_QPATH]{};
    fileHandle_t file=0;
    int size=-1, offset=0;
    bool valid=false;
};

qhandle_t Levelshot(const uiImport_t* fs,const char* name) {
    char path[MAX_QPATH];
    for (const char* extension:{".jpg",".tga",".png"}) {
        if (std::snprintf(path,sizeof(path),"levelshots/%s%s",name,extension)>=int(sizeof(path))) continue;
        fileHandle_t file=0;
        const int bytes=fs->FS_Open(path,&file,FS_READ);
        if (file) fs->FS_Close(file);
        if (bytes>0 && file) {
            std::snprintf(path,sizeof(path),"levelshots/%s",name);
            if (const auto shader=fs->R_RegisterShaderNoMip(path)) return shader;
        }
    }
    return 0;
}

qhandle_t GeneratedShot(const uiImport_t* fs,const char* name,const jar::mapPicker::Point& focus) {
    if (!fs->FS_Write) return 0;
    HostBsp bsp(fs,name);
    jar::mapPicker::Image image;
    if (!bsp || !jar::mapPicker::RasterizeBsp(bsp,focus,image)) return 0;
    char path[MAX_QPATH];
    jar::mapPicker::ThumbnailPath(image,path);
    fileHandle_t file=0;
    const int size=fs->FS_Open(path,&file,FS_READ);
    if (file) fs->FS_Close(file);
    if (size!=int(image.size())) {
        fs->FS_Open(path,&file,FS_WRITE);
        if (!file) return 0;
        const int written=fs->FS_Write(image.data(),int(image.size()),file);
        fs->FS_Close(file);
        if (written!=int(image.size())) return 0;
    }
    return fs->R_RegisterShaderNoMip(path);
}
}

qboolean UI_JAR_HostMapNameValid(const char* name) {
    return jar::mapPicker::SafeName(name) ? qtrue : qfalse;
}

jarHostMapStatus_t UI_JAR_InspectHostMap(const uiImport_t* fs,const char* name,char* title,int titleSize,qhandle_t* preview) {
    if (title) title[0]=0;
    if (preview) *preview=0;
    if (!fs->FS_Read || !UI_JAR_HostMapNameValid(name)) return JAR_HOST_MAP_INVALID;
    jar::mapPicker::MapInfo info;
    {
        HostBsp bsp(fs,name);
        if (!bsp.Exists()) return JAR_HOST_MAP_MISSING;
        std::vector<char> entities;
        if (!bsp || !bsp.Read(LUMP_ENTITIES,4*1024*1024,entities) || entities.empty())
            return JAR_HOST_MAP_INVALID;
        info=jar::mapPicker::ReadEntities({entities.data(),entities.size()},jar::mapPicker::StartMode::MultiplayerFFA);
    }
    if (title) Q_strncpyz(title,info.title.c_str(),titleSize);
    if (preview) {
        *preview=Levelshot(fs,name);
        if (!*preview) *preview=GeneratedShot(fs,name,info.start);
    }
    return info.playable ? JAR_HOST_MAP_PLAYABLE : JAR_HOST_MAP_NO_START;
}
