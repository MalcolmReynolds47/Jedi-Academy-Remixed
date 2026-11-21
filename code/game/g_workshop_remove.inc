// Keep player/actor occupancy checks ahead of ownership teardown. GPL-2.0-only.
#include "../../shared/game/jar_fall_cleanup.h"

struct WorkshopFallSection {uint32_t entity;uint64_t lifetime;};
std::vector<WorkshopFallSection> workshopFallSections;
float workshopHostFallFloor=MIN_WORLD_COORD;

void updateWorkshopFallSections(){
 vec3_t low,high;
 gi.JAR_WorldBounds(low,high);
 workshopHostFallFloor=JAR_FallFloor(low[2]);
 workshopFallSections.clear();
 for(const auto& entry:state.owned){
  if(entry.entity>=ENTITYNUM_WORLD)continue;
  const auto& entity=g_entities[entry.entity];
  if(jar::bspSection::Portable(entity)&&matches(entry)){
   const auto lifetime=G_EntityLifetime(&entity);
   if(lifetime)workshopFallSections.push_back({entry.entity,lifetime});
  }
 }
}

// Intersect the section's oriented bounds with the falling body's vertical column.
bool workshopSectionColumnBottom(const gentity_t& section,const vec3_t low,const vec3_t high,float& bottom){
 vec3_t corners[8],minimum,maximum;
 jar::bspSection::Corners(section,corners);
 VectorCopy(corners[0],minimum);VectorCopy(corners[0],maximum);
 for(const auto& corner:corners)for(int axis=0;axis<3;++axis){
  if(!std::isfinite(corner[axis]))return false;
  minimum[axis]=std::min(minimum[axis],corner[axis]);
  maximum[axis]=std::max(maximum[axis],corner[axis]);
 }
 if(high[0]<minimum[0]||low[0]>maximum[0]||high[1]<minimum[1]||low[1]>maximum[1])return false;
 for(const auto& corner:corners)if(corner[2]==minimum[2]&&
     corner[0]>=low[0]&&corner[0]<=high[0]&&corner[1]>=low[1]&&corner[1]<=high[1]){
  bottom=minimum[2];return true;
 }

 bool overlaps=false;
 bottom=maximum[2];
 // The lowest clipped point lies on a section face. Four clipping planes
 // can add at most four vertices to each original quad.
 for(int face=0;face<3;++face)for(int side=0;side<2;++side){
  const int base=side<<face,u=1<<((face+1)%3),v=1<<((face+2)%3);
  const int indices[]={base,base|u,base|u|v,base|v};
  vec3_t polygons[2][8];
  for(int i=0;i<4;++i)VectorCopy(corners[indices[i]],polygons[0][i]);
  int count=4;
  for(int plane=0;plane<4&&count;++plane){
   const int axis=plane/2;
   const bool upper=plane%2!=0;
   const float bound=upper?high[axis]:low[axis],direction=upper?-1.f:1.f;
   const auto& from=polygons[plane%2];
   auto& to=polygons[1-plane%2];
   int written=0;
   for(int i=0;i<count;++i){
    const auto& start=from[(i+count-1)%count];const auto& end=from[i];
    const float a=(start[axis]-bound)*direction,b=(end[axis]-bound)*direction;
    if((a<0&&b>0)||(a>0&&b<0)){
     vec3_t delta;VectorSubtract(end,start,delta);
     VectorMA(start,a/(a-b),delta,to[written]);to[written++][axis]=bound;
    }
    if(b>=0){VectorCopy(end,to[written]);++written;}
   }
   count=written;
  }
  for(int i=0;i<count;++i){bottom=std::min(bottom,polygons[0][i][2]);overlaps=true;}
 }
 return overlaps;
}

bool outsideWorkshopMap(const vec3_t origin,const vec3_t mins,const vec3_t maxs){
 if(!JAR_OutsideMap(origin,maxs[2],workshopHostFallFloor))return false;
 if(JAR_OutsideMap(origin,maxs[2],MIN_WORLD_COORD))return true;
 vec3_t low{},high{};
 for(int axis=0;axis<2;++axis){
  low[axis]=origin[axis]+mins[axis];high[axis]=origin[axis]+maxs[axis];
  if(!std::isfinite(low[axis])||!std::isfinite(high[axis])||low[axis]>high[axis])return true;
 }
 for(const auto& region:workshopFallSections){
  const auto& section=g_entities[region.entity];
  if(!section.linked||G_EntityLifetime(&section)!=region.lifetime||
     !jar::bspSection::Portable(section))continue;
  float bottom;
  if(workshopSectionColumnBottom(section,low,high,bottom)&&!JAR_OutsideMap(origin,maxs[2],JAR_FallFloor(bottom)))return false;
 }
 return true;
}

bool removalRoot(const Owned& entry){return !entry.parent||!OwnedId(state,entry.parent);}

bool despawnAllAvailable(){
 for(const auto& entry:state.owned){
  if(matches(entry)&&removalOccupied(g_entities[entry.entity]))return false;
 }
 return true;
}

bool removalAvailable(const Owned& entry,std::unordered_set<uint32_t>* checked=nullptr){
 if(checked&&!checked->insert(entry.id).second)return true;
 if(matches(entry)&&removalOccupied(g_entities[entry.entity]))return false;
 const auto* children=OwnedChildren(state,entry.id);
 if(!children)return true;
 auto pending=*children;
 while(!pending.empty()){
  const auto id=pending.back();
  pending.pop_back();
  const auto* member=OwnedId(state,id);
  if(!member||(checked&&!checked->insert(id).second))continue;
  if(matches(*member)&&removalOccupied(g_entities[member->entity]))return false;
  if(const auto* descendants=OwnedChildren(state,id))
   pending.insert(pending.end(),descendants->begin(),descendants->end());
 }
 return true;
}

bool discardOwned(Owned entry){
 auto& target=g_entities[entry.entity];
 if(matches(entry))G_FreeEntity(&target);
 else G_JAR_WorkshopForget(&target,false);
 const auto* remaining=owned(entry.entity);
 if(remaining&&remaining->id==entry.id){
  message("This entity is protected by its native controller.");
  return false;
 }
 return true;
}

bool remove(uint32_t entity,bool child=false){
 const auto* entry=owned(entity);
 if(!entry)return true;
 if(!child){
  while(const auto* parent=OwnedId(state,entry->parent))
   entry=parent;
 }
 if(!removalAvailable(*entry)){
  message("An object is occupied, controlled or supporting an actor. Move clear before removal.");
  return false;
 }
 return discardOwned(*entry);
}

void keepDetachedChildCreation(const Owned& parent,const Owned& child);
void forgetWorkshopChildren(Owned parent,bool freeing){
 const auto* children=OwnedChildren(state,parent.id);
 if(!children)return;
 // Native frees can remove several entries recursively or reuse a slot.
 const auto childrenToRemove=*children;
 for(auto it=childrenToRemove.rbegin();it!=childrenToRemove.rend();++it){
  auto* child=OwnedId(state,*it);
  if(!child||child->parent!=parent.id)continue;
  auto& target=g_entities[child->entity];
  if(!matches(*child))G_JAR_WorkshopForget(&target,false);
  else if(freeing||removalAvailable(*child))G_FreeEntity(&target);
  else{
   keepDetachedChildCreation(parent,*child);
   SetOwnershipParent(state,*child,0);
  }
 }
}

bool removeOutsideMap(Owned owner,const vec3_t origin,const vec3_t mins,const vec3_t maxs){
 if(!outsideWorkshopMap(origin,mins,maxs))return false;
 if(owner.parent&&OwnedId(state,owner.parent)){
  const auto& entity=g_entities[owner.entity];
  // Loose drops can leave the map while their owning NPC or setup stays inside.
  if(!entity.item||!(entity.flags&FL_DROPPED_ITEM)||entity.s.pos.trType!=TR_GRAVITY)return false;
 }
 if(!remove(owner.entity,true))return false;
 if(gi.JAR_Log)gi.JAR_Log(1,"sandbox","fell_out_of_map","entity=%u id=%u catalog=%u",owner.entity,owner.id,owner.catalog);
 return true;
}
