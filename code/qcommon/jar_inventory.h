// Keep old campaign transition strings readable when inventory slots are appended.
#ifndef JAR_INVENTORY_H
#define JAR_INVENTORY_H
#include <cstdlib>
#include <climits>
namespace jar {
inline void ReadInventory(const char*text,int*out,int capacity){
 for(int i=0;i<capacity;++i)out[i]=0;
 if(!text)return;
 for(int i=0;i<capacity;++i){while(*text==' '||*text=='\t')++text;if(!*text)break;
  char*end=nullptr;const long value=std::strtol(text,&end,10);if(end==text)break;
  out[i]=value<0?0:value>INT_MAX?INT_MAX:int(value);text=end;
 }
}
}
#endif
