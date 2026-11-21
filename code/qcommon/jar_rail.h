// Portable rail traffic settings. Native map rails keep their original registry.
#pragma once
#include <cstring>
#include <cmath>
namespace jar { namespace rail {
enum Kind {None,Track,Lane,Mover};
inline Kind Class(const char*n){return !n?None:!std::strcmp(n,"rail_track")?Track:!std::strcmp(n,"rail_lane")?Lane:!std::strcmp(n,"rail_mover")?Mover:None;}
struct Settings {Kind kind=None;float length=512,width=256,cell=64,speed=128,delay=0;int count=1;bool enabled=true,center=false;};
inline bool Valid(const Settings&s){
 if(s.kind<Track||s.kind>Mover||!std::isfinite(s.delay)||s.delay<0||s.delay>30)return false;
 if(s.kind!=Mover&&(!std::isfinite(s.width)||s.width<64||s.width>2048))return false;
 return s.kind!=Track||(std::isfinite(s.length)&&s.length>=128&&s.length<=8192&&std::isfinite(s.cell)&&s.cell>=32&&s.cell<=256&&s.cell<=s.width&&std::floor(s.cell)==s.cell&&std::isfinite(s.speed)&&s.speed>=16&&s.speed<=2000&&s.count>=1&&s.count<=8);
}
inline bool Link(Kind from,Kind to){return (from==Lane&&to==Track)||(from==Mover&&(to==Track||to==Lane));}
} }
