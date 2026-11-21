// Listener-relative soundscape routing. GPL-2.0-only.
#pragma once

namespace jar { namespace soundscape {
enum class Pan { World, Center, Left, Right };

inline Pan ClipPan(int side){
    return side<0?Pan::Left:side>0?Pan::Right:Pan::Center;
}

inline int RelativeX(Pan pan){
    return pan==Pan::Left?-1:pan==Pan::Right?1:0;
}

inline void Levels(Pan pan,int volume,bool mono,int& left,int& right){
    if(mono){
        left=right=pan==Pan::Center?volume:volume/2;
    }else{
        left=pan==Pan::Right?0:volume;
        right=pan==Pan::Left?0:volume;
    }
}
} }
