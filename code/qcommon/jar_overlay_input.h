// Held Sandbox overlay input policy. GPL-2.0-only.
#ifndef JAR_OVERLAY_INPUT_H
#define JAR_OVERLAY_INPUT_H
#include <cstring>
#include <cstdint>
#include <initializer_list>
namespace jar { namespace overlay {
inline bool MovementCommand(const char* text) {
    if(!text)return false;
    while(*text==' ' || *text=='\t')++text;
    const char* end=text+std::strlen(text);
    while(end>text && (end[-1]==' ' || end[-1]=='\t'))--end;
    for(const char* name:{"+forward","+back","+moveleft","+moveright","+moveup","+movedown","+speed","+strafe","+left","+right","+walk"})
        if(size_t(end-text)==std::strlen(name) && !std::strncmp(text,name,end-text))return true;
    return false;
}
// A compound binding can contain a movement action plus unrelated commands.
// The movement segment passes through; suppress its duplicate UI activation.
inline bool MovementBinding(const char* text){
 if(!text)return false;
 while(*text){const char* end=std::strchr(text,';');if(!end)end=text+std::strlen(text);
  const size_t n=end-text;if(n<64){char part[64];std::memcpy(part,text,n);part[n]=0;if(MovementCommand(part))return true;}
  text=*end?end+1:end;
 }return false;
}
// A button held on entry must be released before it can fire after dismissal.
inline uint32_t FilterButtons(uint32_t raw,bool open,uint32_t movementMask,uint32_t& blocked) {
    blocked &= raw;
    if(open)blocked |= raw & ~movementMask;
    return raw & ~blocked;
}
}}
#endif
