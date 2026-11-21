// Context and clipboard settings for portable native shooters. GPL-2.0-only.
#pragma once

namespace jar { namespace shooter {
constexpr int minInterval=100,maxInterval=30000,maxVariation=30000;
constexpr int altFireFlag=1,repeatFlag=2;
struct Settings {
 int interval=500,variation=0;
 bool alternate=false,repeat=false,enabled=true;
};
inline bool Valid(const Settings& value){
 return value.interval>=minInterval&&value.interval<=maxInterval&&value.variation>=0&&value.variation<=maxVariation;
}
inline bool Same(const Settings& a,const Settings& b){
 return a.interval==b.interval&&a.variation==b.variation&&a.alternate==b.alternate&&a.repeat==b.repeat&&a.enabled==b.enabled;
}
} }
